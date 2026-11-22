#==- fiber_context_lldb.py - LLDB Fiber Support -----------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Registers a summary for fiber handles and the fiber-bt command for inspecting
# a parked fiber's call stack.
#
#===------------------------------------------------------------------------===#

import importlib.util
from pathlib import Path
import sys

import lldb

_spec = importlib.util.spec_from_file_location(
    __name__ + "._fiber_debug", Path(__file__).with_name("fiber_context_debug.py")
)
_core = importlib.util.module_from_spec(_spec)
sys.modules[_spec.name] = _core
_spec.loader.exec_module(_core)

class FiberAccess:
    def __init__(self):
        pass

    @staticmethod
    def field(value, name):
        field = value.GetNonSyntheticValue().GetChildMemberWithName(name, lldb.eNoDynamicValues)
        if not field.IsValid() or field.GetError().Fail():
            raise ValueError(f"cannot read {value.GetTypeName()}.{name}")

        return field

    @staticmethod
    def unsigned(value):
        error = lldb.SBError()
        number = value.GetValueAsUnsigned(error)
        if error.Fail():
            raise ValueError(error.GetCString())

        return number

    @staticmethod
    def address(value):
        location = value.GetLoadAddress()
        if location == lldb.LLDB_INVALID_ADDRESS:
            raise ValueError(f"no address available for {value.GetTypeName()}")

        return location

    def deref(self, value):
        pointee = value.GetType().GetCanonicalType().GetPointeeType()
        if not pointee.IsTypeComplete() or not pointee.GetNumberOfFields():
            target = value.GetTarget()
            type_name = pointee.GetUnqualifiedType().GetName()
            for module in target.modules:
                for candidate in module.FindTypes(type_name):
                    if candidate.IsTypeComplete() and candidate.GetNumberOfFields():
                        addr = lldb.SBAddress(self.unsigned(value), target)
                        return target.CreateValueFromAddress("pointee", addr, candidate)

            raise ValueError(f"missing debug type for {pointee.GetName()}")

        obj = value.Dereference()
        if not obj.IsValid() or obj.GetError().Fail():
            raise ValueError(f"cannot dereference {value.GetTypeName()}")

        return obj

    @staticmethod
    def element(value, index):
        element = value.GetNonSyntheticValue().GetChildAtIndex(index)
        if not element.IsValid() or element.GetError().Fail():
            raise ValueError(f"cannot read {value.GetName()}[{index}]")

        return element

    @staticmethod
    def has_field(value, name):
        record = value.GetType().GetCanonicalType()
        return any(record.GetFieldAtIndex(i).GetName() == name for i in range(record.GetNumberOfFields()))


def _inspect(handle):
    access = FiberAccess()
    handle = handle.GetNonSyntheticValue()
    if not handle.IsValid() or handle.GetError().Fail():
        raise ValueError("fiber handle is unavailable in this frame")

    while handle.GetType().IsReferenceType() or handle.GetType().IsPointerType():
        if handle.GetType().IsPointerType() and access.unsigned(handle) == 0:
            raise ValueError("null pointer to fiber_context")

        handle = access.deref(handle)

    if not _core.fiber_type.fullmatch(handle.GetType().GetCanonicalType().GetUnqualifiedType().GetName() or ""):
        raise ValueError(f"expected stdx::fiber_context, got {handle.GetTypeName()}")

    return _core.inspect_fiber(handle, access)


def fiber_summary(value, internal_dict):
    try:
        return _core.summary(_inspect(value))
    except ValueError as error:
        return f"unavailable ({error})"


def fiber_backtrace(debugger, command, context, result, internal_dict):
    expression = command.strip()
    if not expression:
        result.SetError("usage: fiber-bt HANDLE")
        return

    frame = context.GetFrame()
    if not frame.IsValid():
        result.SetError("select a stopped frame containing the fiber handle")
        return

    target = context.GetTarget()
    process = context.GetProcess()

    def read_pointer(address):
        error = lldb.SBError()
        pointer = process.ReadPointerFromMemory(address, error)
        if error.Fail():
            raise ValueError(f"cannot read stack at {address:#x}: {error.GetCString()}")

        return pointer

    try:
        if hasattr(lldb, "eDILModeLegacy"):
            handle = frame.GetValueForVariablePath(expression, lldb.eNoDynamicValues, lldb.eDILModeLegacy)
        else:
            handle = frame.GetValueForVariablePath(expression, lldb.eNoDynamicValues)

        snapshot = _inspect(handle)
        for index, pc in enumerate(_core.frame_addresses(snapshot, read_pointer)):
            address = target.ResolveLoadAddress(pc - 1)
            function = address.GetFunction().GetName() or address.GetSymbol().GetName() or "<unknown>"
            line = address.GetLineEntry()
            where = f" at {line.GetFileSpec().GetFilename()}:{line.GetLine()}" if line.IsValid() else ""
            result.AppendMessage(f"#{index} {pc:#x} {function}{where}")

    except ValueError as error:
        result.SetError(str(error))


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand(f"type summary add -w fiber_context -F {__name__}.fiber_summary -x '{_core.fiber_type.pattern}'")
    debugger.HandleCommand("type category enable fiber_context")
    debugger.HandleCommand(f"command script add -o -f {__name__}.fiber_backtrace fiber-bt")

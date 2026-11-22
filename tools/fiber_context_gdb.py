#==- fiber_context_gdb.py - GDB Fiber Support -------------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Registers a pretty-printer for fiber handles and the fiber-bt command for
# backtraces on parked stacks.
#
#===------------------------------------------------------------------------===#

import importlib.util
import sys
from pathlib import Path

import gdb
import gdb.printing

_fiber_spec = importlib.util.spec_from_file_location(
    "fiber_context_gdb._fiber_debug", Path(__file__).with_name("fiber_context_debug.py")
)
_fiber_debug = importlib.util.module_from_spec(_fiber_spec)
sys.modules[_fiber_spec.name] = _fiber_debug
_fiber_spec.loader.exec_module(_fiber_debug)

class _GdbAccess:
    def __init__(self):
        pass

    @staticmethod
    def field(value, name):
        return value[name]

    @staticmethod
    def unsigned(value):
        return int(value)

    @staticmethod
    def deref(value):
        return value.dereference()

    @staticmethod
    def element(value, index):
        return value[index]

    @staticmethod
    def has_field(value, name):
        return any(field.name == name for field in value.type.strip_typedefs().fields())

    @staticmethod
    def address(value):
        if value.address is None:
            raise ValueError("fiber state address unavailable")

        return int(value.address)


def _fiber_snapshot(value):
    value_type = value.type.strip_typedefs()
    while value_type.code in (gdb.TYPE_CODE_PTR, gdb.TYPE_CODE_REF, gdb.TYPE_CODE_RVALUE_REF):
        if value_type.code == gdb.TYPE_CODE_PTR and int(value) == 0:
            raise ValueError("null fiber_context pointer")

        value = value.referenced_value()
        value_type = value.type.strip_typedefs()

    if not _fiber_debug.fiber_type.fullmatch(value_type.tag or ""):
        raise ValueError("expected a stdx::fiber_context handle")

    return _fiber_debug.inspect_fiber(value, _GdbAccess)


class _FiberPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        try:
            return _fiber_debug.summary(_fiber_snapshot(self.value))
        except (gdb.error, ValueError) as error:
            return f"fiber_context unavailable ({error})"


def _fiber_printer(value):
    if _fiber_debug.fiber_type.fullmatch(value.type.strip_typedefs().tag or ""):
        return _FiberPrinter(value)

    return None


def _fiber_frame_name(pc):
    try:
        block = gdb.block_for_pc(pc)
        while block and not block.function:
            block = block.superblock

        if block:
            name = block.function.print_name
            location = gdb.find_pc_line(pc)
            if location.symtab and location.line:
                return f"{name} at {location.symtab.filename}:{location.line}"

            return name

    except gdb.error:
        pass

    return gdb.execute(f"info symbol {pc:#x}", to_string=True).strip()


class _FiberBacktrace(gdb.Command):
    def __init__(self):
        super().__init__("fiber-bt", gdb.COMMAND_STACK, gdb.COMPLETE_EXPRESSION)

    def invoke(self, argument, from_tty):
        if not argument.strip():
            raise gdb.GdbError("usage: fiber-bt EXPR")
        try:
            with (gdb.with_parameter("may-call-functions", False),
                  gdb.with_parameter("may-write-memory", False),
                  gdb.with_parameter("may-write-registers", False)):
                snapshot = _fiber_snapshot(gdb.parse_and_eval(argument))

            inferior = gdb.selected_inferior()

            def read_pointer(address):
                return int.from_bytes(inferior.read_memory(address, 8), "little")

            for index, address in enumerate(_fiber_debug.frame_addresses(snapshot, read_pointer)):
                name = _fiber_frame_name(address - 1)
                gdb.write(f"#{index}  {address:#018x}  {name}\n")

        except (gdb.error, ValueError) as error:
            raise gdb.GdbError(str(error)) from None


_fiber_printer.name = "stdx::fiber_context"
_fiber_printer.enabled = True
gdb.printing.register_pretty_printer(None, _fiber_printer, replace=True)
_FiberBacktrace()

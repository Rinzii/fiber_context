#==- fiber_context_debug.py - Fiber Inspection and Frame Walking ------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Reads a fiber's saved state through a debugger adapter and walks the frame
# chain within the recorded stack bounds.
#
#===------------------------------------------------------------------------===#

import re
from typing import NamedTuple


fiber_type = re.compile(r"^stdx::([A-Za-z_][A-Za-z0-9_]*::)?fiber_context$")


class Fiber(NamedTuple):
    context: int
    owner: int
    stack: str
    low: int
    high: int
    sp: int
    fp: int
    pc: int


def inspect_fiber(handle, access):
    state_ptr = access.field(handle, "m_state")
    if not access.unsigned(state_ptr):
        return None

    state = access.deref(state_ptr)
    active = access.field(state, "active_stack")
    context = access.deref(active)
    registers = access.field(context, "registers")

    if access.has_field(registers, "low"):
        low = access.unsigned(access.field(registers, "low"))
        high = access.unsigned(access.field(registers, "high"))
    else:
        low = access.unsigned(access.field(context, "stack_low"))
        high = access.unsigned(access.field(context, "stack_high"))

    if access.has_field(registers, "rbp"):
        fp = access.field(registers, "rbp")
    else:
        fp = access.element(access.field(registers, "x19_to_x29"), 10)

    address = access.unsigned(active)
    if not low and not high:
        stack = "thread"
    elif address == access.address(access.field(state, "initial")):
        stack = "fiber"
    else:
        stack = "cleanup"

    return Fiber(address, access.unsigned(access.field(state, "owner")), stack, low, high,
                 access.unsigned(access.field(registers, "sp")), access.unsigned(fp),
                 access.unsigned(access.field(registers, "pc")))


def summary(fiber):
    if fiber is None:
        return "empty"

    status = "parked" if fiber.owner else "not started"
    bounds = "(bounds unknown)"

    if fiber.stack != "thread":
        bounds = f"[{fiber.low:#x}, {fiber.high:#x})"
        if 0 < fiber.low < fiber.high:
            bounds += f" ({fiber.high - fiber.low} bytes)"

    return (f"{status}, {fiber.stack} stack {bounds}, context={fiber.context:#x}, "
            f"sp={fiber.sp:#x}, fp={fiber.fp:#x}, pc={fiber.pc:#x}, owner={fiber.owner}")


def frame_addresses(fiber, read_pointer):
    if fiber is None:
        raise ValueError("empty fiber")

    if not fiber.owner:
        raise ValueError("fiber has not started")

    if not fiber.pc:
        raise ValueError("saved program counter is unavailable")

    yield fiber.pc

    # Thread stack bounds are unknown so stop at the saved PC.
    if fiber.stack == "thread":
        return

    if not 0 < fiber.low <= fiber.sp <= fiber.high or fiber.low == fiber.high:
        raise ValueError("saved stack pointer is outside the fiber stack")

    fp = fiber.fp
    for _ in range(127):
        if not fp or fp % 8 or not fiber.sp <= fp <= fiber.high - 16:
            return

        # https://github.com/ARM-software/abi-aa/blob/2025Q1/aapcs64/aapcs64.rst#L1215-L1217
        # https://gitlab.com/x86-psABIs/x86-64-ABI/-/blob/e1ce098331da5dbd66e1ffc74162380bcc213236/x86-64-ABI/low-level-sys-info.tex#L500-505
        caller = read_pointer(fp)
        pc = read_pointer(fp + 8)
        if not pc:
            return

        yield pc

        if caller < fp + 16:
            return

        fp = caller

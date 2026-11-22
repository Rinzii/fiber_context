//==- arm64_apple_canary.S - Checked Apple ARM64 Context Switch ------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Checks stack bounds and record integrity before restoring the incoming
// registers.
//
//===----------------------------------------------------------------------===//

/*
 * Apple ARM64 context record. x0 points to the outgoing record, x1 to the incoming one.
 * Byte offsets from either record.
 *
 *   Offset   | First 8 bytes     | Next 8 bytes       |
 *   ---------+-------------------+--------------------+
 *        0   | Head marker       | Stack band pointer |
 *       16   | x19               | x20                |
 *       32   | x21               | x22                |
 *       48   | x23               | x24                |
 *       64   | x25               | x26                |
 *       80   | x27               | x28                |
 *       96   | x29 (frame ptr)   | PC (saved x30)     |
 *      112   | d8                | d9                 |
 *      128   | d10               | d11                |
 *      144   | d12               | d13                |
 *      160   | d14               | d15                |
 *      176   | SP                | FPCR               |
 *      192   | Stack low         | Stack high         |
 *      208   | SP witness        | PC witness         |
 *      224   | Band witness      | Tail marker        |
 *   ---------+-------------------+--------------------+
 *      240     Total bytes
 */

#define FIBER_CONTEXT_ASM_CANARIES 1
#include "arm64_layout.h"
#include "fiber_context_asm_symbols.h"

#if defined(__arm64e__)
#error "This backend requires the ordinary arm64 ABI."
#endif

.text
.p2align 2

// x2 identifies the record in every diagnostic. Validate the band pointer
// before dereferencing it. Native thread stacks have a null band.
.macro check_record
    // The head holds this record's address and the tail holds its bitwise complement.
    ldr x9, [x2, #FIBER_CONTEXT_CTX_HEAD]
    cmp x9, x2
    b.ne Lfail_begin_marker
    ldr x9, [x2, #FIBER_CONTEXT_CTX_TAIL]
    mvn x10, x2
    cmp x9, x10
    b.ne Lfail_end_marker

    // The stored witness is the complement of the band pointer, including a null pointer.
    ldr x9, [x2, #FIBER_CONTEXT_CTX_BAND]
    mvn x10, x9
    ldr x11, [x2, #FIBER_CONTEXT_ARM_BAND_WITNESS]
    cmp x11, x10
    b.ne Lfail_stack_band
    cbz x9, 1f

    // Both band words must match: its address first, then its complement.
    ldp x11, x13, [x9]
    cmp x11, x9
    ccmp x13, x10, #0, eq
    b.ne Lfail_stack_band
1:
.endm

// x12 contains either the current physical SP or the candidate restore SP.
.macro check_stack_pointer
    tst x12, #15
    b.ne Lfail_sp_alignment

    // Two zero bounds leave only the 16-byte alignment requirement for the native stack.
    ldp x9, x10, [x2, #FIBER_CONTEXT_CTX_LOW]
    orr x11, x9, x10
    cbz x11, 2f
    cmp x12, x9
    b.lo Lfail_sp_bounds

    // A fresh downward-growing stack starts at high. Returning through x16 reads no stack slot.
    cmp x12, x10
    b.hi Lfail_sp_bounds
2:
.endm

.globl FIBER_CONTEXT_SYM_SWITCH_CONTEXT
.private_extern FIBER_CONTEXT_SYM_SWITCH_CONTEXT
FIBER_CONTEXT_SYM_SWITCH_CONTEXT:
    .cfi_startproc
    mov x12, sp

    mov x2, x0
    check_record
    check_stack_pointer

    // Bind the outgoing SP and link register to this record for the next resume.
    eor x9, x12, x2
    eor x10, x30, x2
    stp x9, x10, [x2, #FIBER_CONTEXT_CTX_SP_WITNESS]

    stp x19, x20, [x0, #FIBER_CONTEXT_ARM_X19]
    stp x21, x22, [x0, #FIBER_CONTEXT_ARM_X21]
    stp x23, x24, [x0, #FIBER_CONTEXT_ARM_X23]
    stp x25, x26, [x0, #FIBER_CONTEXT_ARM_X25]
    stp x27, x28, [x0, #FIBER_CONTEXT_ARM_X27]

    // The return address in x30 becomes the PC for the next resume.
    stp x29, x30, [x0, #FIBER_CONTEXT_ARM_X29]

    // The ABI preserves the low 64 bits of v8 through v15 across a call.
    stp d8, d9, [x0, #FIBER_CONTEXT_ARM_D8]
    stp d10, d11, [x0, #FIBER_CONTEXT_ARM_D10]
    stp d12, d13, [x0, #FIBER_CONTEXT_ARM_D12]
    stp d14, d15, [x0, #FIBER_CONTEXT_ARM_D14]
    mrs x9, fpcr
    stp x12, x9, [x0, #FIBER_CONTEXT_CTX_SP]

    // Stage the target PC and SP while the outgoing stack is still active.
    ldr x16, [x1, #FIBER_CONTEXT_CTX_PC]
    ldr x12, [x1, #FIBER_CONTEXT_CTX_SP]

    mov x2, x1
    check_record

    // Recompute both witnesses using the incoming record's address.
    ldp x9, x10, [x2, #FIBER_CONTEXT_CTX_SP_WITNESS]
    eor x11, x12, x2
    cmp x9, x11
    b.ne Lfail_sp_witness
    eor x11, x16, x2
    cmp x10, x11
    b.ne Lfail_pc_witness
    check_stack_pointer

    // Stop asynchronous unwinding while registers belong to different stacks.
    .cfi_undefined w30
    ldp x19, x20, [x1, #FIBER_CONTEXT_ARM_X19]
    ldp x21, x22, [x1, #FIBER_CONTEXT_ARM_X21]
    ldp x23, x24, [x1, #FIBER_CONTEXT_ARM_X23]
    ldp x25, x26, [x1, #FIBER_CONTEXT_ARM_X25]
    ldp x27, x28, [x1, #FIBER_CONTEXT_ARM_X27]
    ldr x29, [x1, #FIBER_CONTEXT_ARM_X29]
    ldp d8, d9, [x1, #FIBER_CONTEXT_ARM_D8]
    ldp d10, d11, [x1, #FIBER_CONTEXT_ARM_D10]
    ldp d12, d13, [x1, #FIBER_CONTEXT_ARM_D12]
    ldp d14, d15, [x1, #FIBER_CONTEXT_ARM_D14]
    ldr x9, [x1, #FIBER_CONTEXT_CTX_FPCR]
    msr fpcr, x9

    // A fresh entry has no caller. A resumed caller recovers its own LR from its frame.
    mov x30, xzr
    mov sp, x12

    // x16 holds the entry function on first use and the saved return address thereafter.
    ret x16
    .cfi_endproc

// x2 = offending record, w3 = reason. The trap precedes target restoration.
.macro record_trap setup, symbol, reason
\setup:
    mov w3, #\reason
    .globl \symbol
    .private_extern \symbol

    // The symbol names the BRK itself so the fault PC identifies this check.
\symbol:
    brk #\reason
.endm
    record_trap Lfail_begin_marker, FIBER_CONTEXT_SYM_TRAP_BEGIN_MARKER, 1
    record_trap Lfail_end_marker,   FIBER_CONTEXT_SYM_TRAP_END_MARKER,   2
    record_trap Lfail_sp_witness,   FIBER_CONTEXT_SYM_TRAP_SP_WITNESS,   3
    record_trap Lfail_pc_witness,   FIBER_CONTEXT_SYM_TRAP_PC_WITNESS,   4
    record_trap Lfail_sp_alignment, FIBER_CONTEXT_SYM_TRAP_SP_ALIGNMENT, 5
    record_trap Lfail_sp_bounds,    FIBER_CONTEXT_SYM_TRAP_SP_BOUNDS,    6
    record_trap Lfail_stack_band,   FIBER_CONTEXT_SYM_TRAP_STACK_BAND,   7

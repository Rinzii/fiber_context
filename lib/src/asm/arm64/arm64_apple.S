//==- arm64_apple.S - Apple ARM64 Context Switch ---------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Saves the outgoing registers and resumes the incoming stack through its saved
// program counter.
//
//===----------------------------------------------------------------------===//

/*
 * Apple ARM64 context record. x0 points to the outgoing record, x1 to the incoming one.
 * Byte offsets from either record.
 *
 *   Offset   | First 8 bytes     | Next 8 bytes       |
 *   ---------+-------------------+--------------------+
 *        0   | x19               | x20                |
 *       16   | x21               | x22                |
 *       32   | x23               | x24                |
 *       48   | x25               | x26                |
 *       64   | x27               | x28                |
 *       80   | x29 (frame ptr)   | PC (saved x30)     |
 *       96   | d8                | d9                 |
 *      112   | d10               | d11                |
 *      128   | d12               | d13                |
 *      144   | d14               | d15                |
 *      160   | SP                | FPCR               |
 *   ---------+-------------------+--------------------+
 *      176     Total bytes
 */

#define FIBER_CONTEXT_ASM_CANARIES 0
#include "arm64_layout.h"
#include "fiber_context_asm_symbols.h"

#if defined(__arm64e__)
#error "This backend requires the ordinary arm64 ABI."
#endif

.text
.p2align 2

.globl FIBER_CONTEXT_SYM_SWITCH_CONTEXT
.private_extern FIBER_CONTEXT_SYM_SWITCH_CONTEXT
FIBER_CONTEXT_SYM_SWITCH_CONTEXT:
    .cfi_startproc
    mov x12, sp

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

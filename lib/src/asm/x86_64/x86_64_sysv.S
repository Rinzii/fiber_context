//==- x86_64_sysv.S - System V x86-64 Context Switch -----------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Saves the outgoing registers and returns through the incoming stack to resume
// a fiber or enter its bootstrap.
//
//===----------------------------------------------------------------------===//

/*
 * x86-64 System V context record. RDI points to the outgoing record, RSI to the incoming one.
 *
 * Byte offset   | +0          +8
 * --------------+-------------------------------
 * 0x00          | RBX         RBP
 * 0x10          | R12         R13
 * 0x20          | R14         R15
 * 0x30          | SP          PC
 * 0x40          | FP control  padding
 * --------------+-------------------------------
 * Record size   | 80 bytes
 *
 * FP control slot: MXCSR (4 bytes), x87 control word (2 bytes), padding (2 bytes).
 * On the fiber stack, SP addresses the 8-byte slot where ret consumes PC.
 */

#define FIBER_CONTEXT_ASM_CANARIES 0
#include "x86_64_layout.h"
#include "fiber_context_asm_symbols.h"

#ifdef __APPLE__
#define PRIVATE(name) .private_extern name
#else
#define PRIVATE(name) .hidden name
#endif

    .text

    .p2align 4
    .globl FIBER_CONTEXT_SYM_SWITCH_CONTEXT
    PRIVATE(FIBER_CONTEXT_SYM_SWITCH_CONTEXT)

#ifndef __APPLE__
    .type FIBER_CONTEXT_SYM_SWITCH_CONTEXT, @function
#endif

FIBER_CONTEXT_SYM_SWITCH_CONTEXT:
    .cfi_startproc

    // https://gitlab.com/x86-psABIs/x86-64-ABI/-/blob/e1ce098331da5dbd66e1ffc74162380bcc213236/x86-64-ABI/low-level-sys-info.tex#L456-480
    // These are the integer registers the caller expects to survive an ordinary call.
    movq    %rbx, FIBER_CONTEXT_CTX_RBX(%rdi)
    movq    %rbp, FIBER_CONTEXT_CTX_RBP(%rdi)
    movq    %r12, FIBER_CONTEXT_CTX_R12(%rdi)
    movq    %r13, FIBER_CONTEXT_CTX_R13(%rdi)
    movq    %r14, FIBER_CONTEXT_CTX_R14(%rdi)
    movq    %r15, FIBER_CONTEXT_CTX_R15(%rdi)

    // Save the slot address and the return PC that the caller's call pushed there.
    movq    %rsp, FIBER_CONTEXT_CTX_SP(%rdi)
    movq    (%rsp), %rax
    movq    %rax, FIBER_CONTEXT_CTX_PC(%rdi)

    // Preserve the SSE and x87 rounding modes across a yield.
    stmxcsr FIBER_CONTEXT_CTX_MXCSR(%rdi)
    fnstcw  FIBER_CONTEXT_CTX_X87CW(%rdi)

    // Register restoration spans two contexts. End the unwind chain until
    // the ret completes and the target's ordinary CFI takes over.
    .cfi_undefined %rip
    ldmxcsr FIBER_CONTEXT_CTX_MXCSR(%rsi)
    fldcw   FIBER_CONTEXT_CTX_X87CW(%rsi)
    movq    FIBER_CONTEXT_CTX_RBX(%rsi), %rbx
    movq    FIBER_CONTEXT_CTX_RBP(%rsi), %rbp
    movq    FIBER_CONTEXT_CTX_R12(%rsi), %r12
    movq    FIBER_CONTEXT_CTX_R13(%rsi), %r13
    movq    FIBER_CONTEXT_CTX_R14(%rsi), %r14
    movq    FIBER_CONTEXT_CTX_R15(%rsi), %r15

    // Supply the return PC on the incoming stack, including a new fiber's first return.
    movq    FIBER_CONTEXT_CTX_PC(%rsi), %rax
    movq    FIBER_CONTEXT_CTX_SP(%rsi), %rsp
    movq    %rax, (%rsp)
    ret
    .cfi_endproc
#ifndef __APPLE__
    .size FIBER_CONTEXT_SYM_SWITCH_CONTEXT, .-FIBER_CONTEXT_SYM_SWITCH_CONTEXT
#endif

    .p2align 4
    .globl FIBER_CONTEXT_SYM_X86_BOOTSTRAP
    PRIVATE(FIBER_CONTEXT_SYM_X86_BOOTSTRAP)
#ifndef __APPLE__
    .type FIBER_CONTEXT_SYM_X86_BOOTSTRAP, @function
#endif
FIBER_CONTEXT_SYM_X86_BOOTSTRAP:
    .cfi_startproc
    .cfi_undefined %rip

    // Initialization put entry in R12. The switch's ret left RSP aligned for this call.
    call    *%r12

    // The entry routine must transfer away when the fiber finishes.
    ud2
    .cfi_endproc
#ifndef __APPLE__
    .size FIBER_CONTEXT_SYM_X86_BOOTSTRAP, .-FIBER_CONTEXT_SYM_X86_BOOTSTRAP
#endif

#ifndef __APPLE__
    .section .note.GNU-stack,"",@progbits
#endif

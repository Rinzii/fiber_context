//==- x86_64_sysv_canary.S - Checked System V x86-64 Context Switch --------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Checks the context records and return slots before restoring the incoming
// stack and registers.
//
//===----------------------------------------------------------------------===//

/*
 * x86-64 System V context record. RDI points to the outgoing record, RSI to the incoming one.
 *
 * Byte offset   | +0              +8
 * --------------+-------------------------------
 * 0x00          | head            RBX
 * 0x10          | RBP             R12
 * 0x20          | R13             R14
 * 0x30          | R15             SP
 * 0x40          | PC              FP control
 * 0x50          | SP witness      PC witness
 * 0x60          | low             high
 * 0x70          | band            band witness
 * 0x80          | bounds witness  tail
 * --------------+-------------------------------
 * Record size   | 144 bytes
 *
 * FP control slot: MXCSR (4 bytes), x87 control word (2 bytes), padding (2 bytes).
 * On the fiber stack, SP addresses the 8-byte slot where ret consumes PC.
 */

#define FIBER_CONTEXT_ASM_CANARIES 1
#include "x86_64_layout.h"
#include "fiber_context_asm_symbols.h"

#ifdef __APPLE__
#define PRIVATE(name) .private_extern name
#else
#define PRIVATE(name) .hidden name
#endif

    .text

// RDX identifies the affected record at every trap. Both records and stack
// bands are checked while RSP still points into the outgoing physical stack.
    .macro check_markers

    // The record carries its own address at the head and its bitwise complement at the tail.
    cmpq    %rdx, FIBER_CONTEXT_CTX_HEAD(%rdx)
    jne     Lbad_head
    movq    %rdx, %rax
    notq    %rax
    cmpq    %rax, FIBER_CONTEXT_CTX_TAIL(%rdx)
    jne     Lbad_tail

    // Check the band pointer against its witness before using it as an address.
    movq    FIBER_CONTEXT_CTX_BAND(%rdx), %rcx
    movq    %rcx, %rax
    xorq    %rdx, %rax
    cmpq    %rax, FIBER_CONTEXT_CTX_BAND_WITNESS(%rdx)
    jne     Lbad_band
    testq   %rcx, %rcx
    je      1f

    // The two words at the stack bottom hold the band's address and its complement.
    cmpq    %rcx, (%rcx)
    jne     Lbad_band
    movq    %rcx, %rax
    notq    %rax
    cmpq    %rax, 8(%rcx)
    jne     Lbad_band
1:
    .endm

// RCX is the address of the slot used by ret. HIGH is the first byte past
// the usable stack, and may be unaligned for a caller-provided span.
    .macro check_sp

    // A call pushes 8 bytes on a 16-byte aligned stack, leaving the return slot at 8 mod 16.
    movl    %ecx, %eax
    andl    $(15), %eax
    cmpl    $(8), %eax
    jne     Lbad_alignment

    // Bind both bounds to this record before using them to validate the return slot.
    movq    FIBER_CONTEXT_CTX_HIGH(%rdx), %rax
    xorq    FIBER_CONTEXT_CTX_LOW(%rdx), %rax
    xorq    %rdx, %rax
    cmpq    %rax, FIBER_CONTEXT_CTX_BOUNDS_WITNESS(%rdx)
    jne     Lbad_bounds

    // The original thread stack has no recorded bounds. Both fields are zero.
    movq    FIBER_CONTEXT_CTX_HIGH(%rdx), %rax
    testq   %rax, %rax
    jne     2f
    cmpq    $(0), FIBER_CONTEXT_CTX_LOW(%rdx)
    je      3f
2:
    cmpq    FIBER_CONTEXT_CTX_LOW(%rdx), %rcx
    jb      Lbad_bounds

    // Check the space remaining below HIGH without adding 8 to a possibly corrupt SP.
    subq    %rcx, %rax
    jb      Lbad_bounds
    cmpq    $(8), %rax
    jb      Lbad_bounds
3:
    .endm

    .p2align 4
    .globl FIBER_CONTEXT_SYM_SWITCH_CONTEXT
    PRIVATE(FIBER_CONTEXT_SYM_SWITCH_CONTEXT)

#ifndef __APPLE__
    .type FIBER_CONTEXT_SYM_SWITCH_CONTEXT, @function
#endif

FIBER_CONTEXT_SYM_SWITCH_CONTEXT:
    .cfi_startproc

    // The outgoing record's saved SP is stale while this fiber is running.
    movq    %rdi, %rdx
    check_markers
    movq    %rsp, %rcx
    check_sp

    // Validate the incoming SP and PC before changing stacks or writing the return slot.
    movq    %rsi, %rdx
    check_markers
    movq    FIBER_CONTEXT_CTX_SP(%rsi), %rcx
    movq    %rcx, %rax
    xorq    %rdx, %rax
    cmpq    %rax, FIBER_CONTEXT_CTX_SP_WITNESS(%rsi)
    jne     Lbad_sp
    movq    FIBER_CONTEXT_CTX_PC(%rsi), %rax
    xorq    %rdx, %rax
    cmpq    %rax, FIBER_CONTEXT_CTX_PC_WITNESS(%rsi)
    jne     Lbad_pc
    check_sp

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

    // Refresh both witnesses for the next switch back to this record.
    xorq    %rdi, %rax
    movq    %rax, FIBER_CONTEXT_CTX_PC_WITNESS(%rdi)
    movq    %rsp, %rax
    xorq    %rdi, %rax
    movq    %rax, FIBER_CONTEXT_CTX_SP_WITNESS(%rdi)

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

// Leave the reason in EAX at the faulting instruction. Each symbol has its
// own range so debuggers can distinguish the failure from the instruction PC.
    .macro trap setup, name, reason
    .globl \name
#ifdef __APPLE__
    .private_extern \name
#else
    .hidden \name
    .type \name, @function
#endif
\setup:
    movl    $\reason, %eax
\name:
    ud2
#ifndef __APPLE__
    .size \name, .-\name
#endif
    .endm
    trap Lbad_head,      FIBER_CONTEXT_SYM_TRAP_BEGIN_MARKER, 1
    trap Lbad_tail,      FIBER_CONTEXT_SYM_TRAP_END_MARKER,   2
    trap Lbad_sp,        FIBER_CONTEXT_SYM_TRAP_SP_WITNESS,   3
    trap Lbad_pc,        FIBER_CONTEXT_SYM_TRAP_PC_WITNESS,   4
    trap Lbad_alignment, FIBER_CONTEXT_SYM_TRAP_SP_ALIGNMENT, 5
    trap Lbad_bounds,    FIBER_CONTEXT_SYM_TRAP_SP_BOUNDS,    6
    trap Lbad_band,      FIBER_CONTEXT_SYM_TRAP_STACK_BAND,   7

#ifndef __APPLE__
    .section .note.GNU-stack,"",@progbits
#endif

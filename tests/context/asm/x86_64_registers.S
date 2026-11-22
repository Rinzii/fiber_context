//==- x86_64_registers.S - x86-64 Register Preservation Probe --------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Checks callee-saved registers and the SSE and x87 control settings across a
// yield callback.
//
//===----------------------------------------------------------------------===//

/*
 * Register probe stack after the six pushes and 24-byte allocation.
 * Offsets are bytes above RSP immediately before the call to yield.
 *
 * Offset   | Field at offset            | Next field
 * ---------+----------------------------+--------------------------
 *  0       | original MXCSR (4 bytes)   | original x87 CW at +4
 *  8       | expected MXCSR (4 bytes)   | expected x87 CW at +12
 * 16       | observed MXCSR (4 bytes)   | observed x87 CW at +20
 * ---------+----------------------------+--------------------------
 * 24       | caller R15                 | caller R14 at +32
 * 40       | caller R13                 | caller R12 at +48
 * 56       | caller RBP                 | caller RBX at +64
 * 72       | caller return PC           |
 *
 * Each x87 control word occupies 2 bytes. Each saved integer register occupies 8.
 */

#ifdef __APPLE__
#define TEST_PRESERVED_REGISTERS _fiber_context_test_preserved_registers
#else
#define TEST_PRESERVED_REGISTERS fiber_context_test_preserved_registers
#endif

    .text
    .p2align 4
    .globl TEST_PRESERVED_REGISTERS
#ifndef __APPLE__
    .type TEST_PRESERVED_REGISTERS, @function
#endif

// A nonzero result identifies a damaged register.
// Preserve the caller's FP environment even on failure.
TEST_PRESERVED_REGISTERS:
    .cfi_startproc
    pushq   %rbx
    .cfi_adjust_cfa_offset 8
    .cfi_offset %rbx, -16
    pushq   %rbp
    .cfi_adjust_cfa_offset 8
    .cfi_offset %rbp, -24
    pushq   %r12
    .cfi_adjust_cfa_offset 8
    .cfi_offset %r12, -32
    pushq   %r13
    .cfi_adjust_cfa_offset 8
    .cfi_offset %r13, -40
    pushq   %r14
    .cfi_adjust_cfa_offset 8
    .cfi_offset %r14, -48
    pushq   %r15
    .cfi_adjust_cfa_offset 8
    .cfi_offset %r15, -56

    // The 24-byte scratch area also aligns RSP to 16 bytes for the callback.
    subq    $24, %rsp
    .cfi_adjust_cfa_offset 24

    // Toggle a rounding-mode bit in each control word and keep the originals for cleanup.
    stmxcsr (%rsp)
    movl    (%rsp), %eax
    xorl    $0x2000, %eax
    movl    %eax, 8(%rsp)
    ldmxcsr 8(%rsp)
    fnstcw  4(%rsp)
    movzwl  4(%rsp), %eax
    xorl    $0x0400, %eax
    movw    %ax, 12(%rsp)
    fldcw   12(%rsp)

    movabsq $0x381ac46be790a55a, %rbx
    movabsq $0x5e199cb3072de618, %rbp
    movabsq $0x712c4f5610c38a7b, %r12
    movabsq $0x136d05be719f68c4, %r13
    movabsq $0x23fe79992d50b468, %r14
    movabsq $0x4c027e5aab63d914, %r15

    // Nested probes on different stacks must use different register values.
    xorq    %rsp, %rbx
    xorq    %rsp, %rbp
    xorq    %rsp, %r12
    xorq    %rsp, %r13
    xorq    %rsp, %r14
    xorq    %rsp, %r15

    // The callback yields and resumes before the probe reads the registers back.
    call    *%rdi

    .macro check_gpr reg, pattern, mask

    // Recreate the expected value from this frame and accumulate failures in EAX.
    movabsq $\pattern, %rdx
    xorq    %rsp, %rdx
    cmpq    %rdx, \reg
    je      1f
    orl     $\mask, %eax
1:
    .endm
    xorl    %eax, %eax
    check_gpr %rbx, 0x381ac46be790a55a, 1
    check_gpr %rbp, 0x5e199cb3072de618, 2
    check_gpr %r12, 0x712c4f5610c38a7b, 4
    check_gpr %r13, 0x136d05be719f68c4, 8
    check_gpr %r14, 0x23fe79992d50b468, 16
    check_gpr %r15, 0x4c027e5aab63d914, 32

    // Bits 6 and 7 of the result report MXCSR and x87 control mismatches.
    stmxcsr 16(%rsp)
    movl    16(%rsp), %edx
    xorl    8(%rsp), %edx
    testl   $0xffc0, %edx  // status flags (low six) are caller-saved
    je      2f
    orl     $64, %eax

2:
    fnstcw  20(%rsp)
    movzwl  20(%rsp), %edx
    cmpw    12(%rsp), %dx
    je      3f
    orl     $128, %eax

3:

    // Restore the caller's registers on every path, leaving the failure mask in EAX.
    ldmxcsr (%rsp)
    fldcw   4(%rsp)
    addq    $24, %rsp
    .cfi_adjust_cfa_offset -24
    popq    %r15
    .cfi_restore %r15
    .cfi_adjust_cfa_offset -8
    popq    %r14
    .cfi_restore %r14
    .cfi_adjust_cfa_offset -8
    popq    %r13
    .cfi_restore %r13
    .cfi_adjust_cfa_offset -8
    popq    %r12
    .cfi_restore %r12
    .cfi_adjust_cfa_offset -8
    popq    %rbp
    .cfi_restore %rbp
    .cfi_adjust_cfa_offset -8
    popq    %rbx
    .cfi_restore %rbx
    .cfi_adjust_cfa_offset -8
    ret
    .cfi_endproc
#ifndef __APPLE__
    .size TEST_PRESERVED_REGISTERS, .-TEST_PRESERVED_REGISTERS
    .section .note.GNU-stack,"",@progbits
#endif

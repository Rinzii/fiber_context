//==- arm64_registers.S - ARM64 Register Preservation Probe ----------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Seeds preserved registers and changes the rounding mode before yielding.
// Checks the restored values after the fiber resumes.
//
//===----------------------------------------------------------------------===//

/*
 * ARM64 register probe. x0 supplies the yield callback, w0 returns 0 on success or 1 on failure.
 * This frame holds the caller's values while the live registers hold the test patterns.
 *
 *   SP offset | First 8 bytes     | Next 8 bytes      |
 *   ----------+-------------------+-------------------+
 *           0 | Caller x29        | Caller x30        |
 *          16 | x19               | x20               |
 *          32 | x21               | x22               |
 *          48 | x23               | x24               |
 *          64 | x25               | x26               |
 *          80 | x27               | x28               |
 *          96 | d8                | d9                |
 *         112 | d10               | d11               |
 *         128 | d12               | d13               |
 *         144 | d14               | d15               |
 *         160 | Caller FPCR       | Expected FPCR     |
 *   ----------+-------------------+-------------------+
 *         176   Caller's SP
 */

.text
.p2align 2
.globl _fiber_context_test_preserved_registers
.private_extern _fiber_context_test_preserved_registers

// Seed each preserved register from this frame's address so two live probes have distinct values.
_fiber_context_test_preserved_registers:
    .cfi_startproc
    stp x29, x30, [sp, #-176]!
    .cfi_def_cfa_offset 176
    .cfi_offset w29, -176
    .cfi_offset w30, -168
    mov x29, sp
    .cfi_def_cfa_register w29
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    stp x25, x26, [sp, #64]
    stp x27, x28, [sp, #80]
    stp d8, d9, [sp, #96]
    stp d10, d11, [sp, #112]
    stp d12, d13, [sp, #128]
    stp d14, d15, [sp, #144]

    // Flip one rounding-mode bit so the resumed FPCR must differ from the caller's.
    mrs x9, fpcr
    eor x10, x9, #0x400000
    stp x9, x10, [sp, #160]
    msr fpcr, x10

    mov x9, sp

.macro seed_gpr index
    add x\index, x9, #\index
.endm

.macro seed_fpr index
    add x10, x9, #\index

    // Copy the seed bits unchanged so the check can compare them as an integer.
    fmov d\index, x10
.endm

    .irp n,19,20,21,22,23,24,25,26,27,28
        seed_gpr \n
    .endr

    .irp n,8,9,10,11,12,13,14,15
        seed_fpr \n
    .endr

    // The callback yields with all test patterns live and returns here after resumption.
    blr x0

    mov x9, sp

    // x29 should still name this 16-byte-aligned frame.
    cmp x29, x9
    b.ne Lprobe_failed
    tst x9, #15
    b.ne Lprobe_failed

.macro check_gpr index

    // Reconstruct each expected value from the resumed frame and register number.
    add x10, x9, #\index
    cmp x\index, x10
    b.ne Lprobe_failed
.endm

.macro check_fpr index
    add x10, x9, #\index
    fmov x11, d\index
    cmp x11, x10
    b.ne Lprobe_failed
.endm

    .irp n,19,20,21,22,23,24,25,26,27,28
        check_gpr \n
    .endr

    .irp n,8,9,10,11,12,13,14,15
        check_fpr \n
    .endr

    ldr x10, [sp, #168]
    mrs x11, fpcr
    cmp x11, x10
    b.ne Lprobe_failed
    mov w0, #0
    b Lprobe_restore

Lprobe_failed:
    mov w0, #1

Lprobe_restore:

    // Both outcomes restore the caller's floating-point control and register values.
    ldr x9, [sp, #160]
    msr fpcr, x9
    ldp x19, x20, [sp, #16]
    ldp x21, x22, [sp, #32]
    ldp x23, x24, [sp, #48]
    ldp x25, x26, [sp, #64]
    ldp x27, x28, [sp, #80]
    ldp d8, d9, [sp, #96]
    ldp d10, d11, [sp, #112]
    ldp d12, d13, [sp, #128]
    ldp d14, d15, [sp, #144]
    ldp x29, x30, [sp], #176
    .cfi_def_cfa sp, 0
    .cfi_restore w29
    .cfi_restore w30
    ret
    .cfi_endproc

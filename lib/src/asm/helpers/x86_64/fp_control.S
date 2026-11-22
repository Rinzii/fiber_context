//==- fp_control.S - x86-64 Floating-Point Control -------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Stores the current MXCSR and x87 control word for a new fiber.
//
//===----------------------------------------------------------------------===//

#ifdef __APPLE__
#define C_SYMBOL(name) _##name
#else
#define C_SYMBOL(name) name
#endif

    .text
    .p2align 4
    .globl C_SYMBOL(fiber_context_x86_read_fp_control)
#ifdef __APPLE__
    .private_extern C_SYMBOL(fiber_context_x86_read_fp_control)
#else
    .hidden C_SYMBOL(fiber_context_x86_read_fp_control)
    .type C_SYMBOL(fiber_context_x86_read_fp_control), @function
#endif

C_SYMBOL(fiber_context_x86_read_fp_control):
    .cfi_startproc
    stmxcsr (%rdi)
    fnstcw  (%rsi)
    ret
    .cfi_endproc

#ifndef __APPLE__
    .size C_SYMBOL(fiber_context_x86_read_fp_control), .-C_SYMBOL(fiber_context_x86_read_fp_control)
    .section .note.GNU-stack,"",@progbits
#endif

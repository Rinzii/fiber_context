//==- symbols.h - Assembly Entry Points ------------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declares the context switch and x86 bootstrap along with the canary traps
// implemented in assembly.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <__fiber_context/config.h>

#include "source.h"

#define FIBER_CONTEXT_ASM_STEM FIBER_CONTEXT_ASM_TARGET("lib/src/asm/arm64/arm64_apple", "lib/src/asm/x86_64/x86_64_sysv")
#define FIBER_CONTEXT_ASM_CANARY_BACKEND FIBER_CONTEXT_ASM_STEM "_canary.S"
#if FIBER_CONTEXT_ASM_CANARIES
    #define FIBER_CONTEXT_ASM_BACKEND FIBER_CONTEXT_ASM_CANARY_BACKEND
#else
    #define FIBER_CONTEXT_ASM_BACKEND FIBER_CONTEXT_ASM_STEM ".S"
#endif

STDX_FIBER_CONTEXT_NAMESPACE::native::asm_bridge
{

    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_BACKEND, FIBER_CONTEXT_SYM_SWITCH_CONTEXT) void switch_context(void *from, void *to) noexcept;
#if defined(__x86_64__)
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_BACKEND, FIBER_CONTEXT_SYM_X86_BOOTSTRAP)
#endif
    void x86_bootstrap();

    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_BEGIN_MARKER) void trap_begin_marker();
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_END_MARKER) void trap_end_marker();
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_SP_WITNESS) void trap_sp_witness();
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_PC_WITNESS) void trap_pc_witness();
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_SP_ALIGNMENT) void trap_sp_alignment();
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_SP_BOUNDS) void trap_sp_bounds();
    FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_CANARY_BACKEND, FIBER_CONTEXT_SYM_TRAP_STACK_BAND) void trap_stack_band();

} // namespace stdx::native::asm_bridge

#undef FIBER_CONTEXT_ASM_BACKEND
#undef FIBER_CONTEXT_ASM_CANARY_BACKEND
#undef FIBER_CONTEXT_ASM_STEM

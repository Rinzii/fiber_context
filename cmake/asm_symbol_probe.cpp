//==- asm_symbol_probe.cpp - C++ Bridge Symbol Probe -----------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Takes the address of each assembly bridge function so the object file records
// the target compiler's mangled name.
//
//===----------------------------------------------------------------------===//

#include "asm/symbols.h"

#include <__fiber_context/config.h>

#if defined(__cplusplus) && defined(__RESHARPER__)
    #define FIBER_CONTEXT_USED_IMPLICITLY [[rscpp::used_implicitly]]
#else
    #define FIBER_CONTEXT_USED_IMPLICITLY
#endif

STDX_FIBER_CONTEXT_NAMESPACE::native::asm_probe
{
    namespace bridge = asm_bridge;

    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto switch_context    = &bridge::switch_context;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto x86_bootstrap     = &bridge::x86_bootstrap;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_begin_marker = &bridge::trap_begin_marker;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_end_marker   = &bridge::trap_end_marker;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_sp_witness   = &bridge::trap_sp_witness;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_pc_witness   = &bridge::trap_pc_witness;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_sp_alignment = &bridge::trap_sp_alignment;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_sp_bounds    = &bridge::trap_sp_bounds;
    FIBER_CONTEXT_USED_IMPLICITLY extern constexpr auto trap_stack_band   = &bridge::trap_stack_band;
}

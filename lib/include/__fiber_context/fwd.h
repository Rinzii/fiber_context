//==- fwd.h - Fiber Forward Declarations -----------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declares the public handle and the internal types it references without
// requiring their definitions.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"

STDX_FIBER_CONTEXT_NAMESPACE
{
    class INTERNAL_STDX_FIBER_CONTEXT_API fiber_context;

    namespace detail
    {
        struct fiber_state;
        struct fiber_runtime;
        struct INTERNAL_STDX_FIBER_CONTEXT_HIDDEN fiber_entry;
        struct INTERNAL_STDX_FIBER_CONTEXT_HIDDEN stack_deleter;
    } // namespace detail
} // namespace STDX_FIBER_CONTEXT_NAMESPACE

//==- sanitizer.cpp - AddressSanitizer Stack Switching ---------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Notifies AddressSanitizer when execution leaves one fiber stack and arrives
// on another.
//
//===----------------------------------------------------------------------===//

#include "runtime.h"

#include <__fiber_context/config.h>

#if FIBER_CONTEXT_RUNTIME_ASAN
    #include <sanitizer/common_interface_defs.h>
#endif

STDX_FIBER_CONTEXT_NAMESPACE::runtime
{

#if FIBER_CONTEXT_RUNTIME_ASAN

    /// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.0/compiler-rt/include/sanitizer/common_interface_defs.h#L437-L455

    void start(physical_stack &from, physical_stack &to, bool retiring) noexcept
    {
        to.previous = &from;
        __sanitizer_start_switch_fiber(retiring ? nullptr : &from.fake_stack, to.bottom, to.size);
    }

    void finish(physical_stack &current) noexcept
    {
        physical_stack &previous = *current.previous;
        current.previous         = nullptr;
        __sanitizer_finish_switch_fiber(current.fake_stack, &previous.bottom, &previous.size);
    }

#endif

} // namespace stdx::runtime

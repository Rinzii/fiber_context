//==- itanium_eh.cpp - Itanium Exception State Exchange --------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Saves the thread's active exception state in the outgoing fiber and installs
// the incoming fiber's state.
//
//===----------------------------------------------------------------------===//

#include "runtime.h"

#include <__fiber_context/config.h>

#include <cstring>

// Access the thread's exception state to save and restore it per fiber.
// https://github.com/gcc-mirror/gcc/blob/releases/gcc-14.2.0/libstdc++-v3/libsupc++/cxxabi.h#L587-L613
// NOLINTBEGIN(*-reserved-identifier)
namespace __cxxabiv1
{
    struct __cxa_eh_globals;
    extern "C" __cxa_eh_globals *__cxa_get_globals() noexcept;
} // namespace __cxxabiv1
// NOLINTEND(*-reserved-identifier)

STDX_FIBER_CONTEXT_NAMESPACE::runtime
{

    void switch_exceptions(exception_state &from, const exception_state &to) noexcept
    {
        // https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html#cxx-exc-stack
        static_assert(sizeof(void *) == 8 && sizeof(unsigned int) == 4);
        static_assert(offsetof(exception_state, uncaught) == sizeof(void *));
        static_assert(sizeof(exception_state) == sizeof(void *) + sizeof(unsigned int));
        auto *globals = __cxxabiv1::__cxa_get_globals();
        std::memcpy(&from, globals, sizeof(from));
        std::memcpy(globals, &to, sizeof(to));
    }

} // namespace stdx::runtime

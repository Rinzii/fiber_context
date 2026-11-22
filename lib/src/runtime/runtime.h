//==- runtime.h - Fiber Runtime Hooks --------------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Selects the exception-state backend and declares stack-switch hooks for
// AddressSanitizer builds.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <__fiber_context/config.h>

#include <cstddef>

#if (defined(__APPLE__) && (defined(__aarch64__) || defined(__x86_64__)) && defined(_LIBCPP_VERSION)) ||                                                       \
    (defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__))
    #include "itanium_eh.h"
#else
    #error "No fiber_context exception-state backend for this target and C++ runtime"
#endif

#if defined(__SANITIZE_ADDRESS__)
    #define FIBER_CONTEXT_RUNTIME_ASAN 1
#elif defined(__has_feature)
    #if __has_feature(address_sanitizer)
        #define FIBER_CONTEXT_RUNTIME_ASAN 1
    #endif
#endif

#ifndef FIBER_CONTEXT_RUNTIME_ASAN
    #define FIBER_CONTEXT_RUNTIME_ASAN 0
#endif // FIBER_CONTEXT_RUNTIME_ASAN

#if FIBER_CONTEXT_RUNTIME_ASAN && (defined(__clang__) || defined(__GNUC__))
    #define FIBER_CONTEXT_NO_ASAN __attribute__((no_sanitize("address", "undefined"), noinline))
#elif FIBER_CONTEXT_RUNTIME_ASAN
    #error "No fiber_context fiber-switch sanitizer annotations for this compiler"
#else
    #define FIBER_CONTEXT_NO_ASAN
#endif

STDX_FIBER_CONTEXT_NAMESPACE::runtime
{

#if FIBER_CONTEXT_RUNTIME_ASAN
    struct physical_stack
    {
        const void *bottom{};
        std::size_t size{};
        void *fake_stack{};
        physical_stack *previous{};
    };

    FIBER_CONTEXT_NO_ASAN void start(physical_stack &from, physical_stack &to, bool retiring) noexcept;
    FIBER_CONTEXT_NO_ASAN void finish(physical_stack &current) noexcept;
#endif // if FIBER_CONTEXT_RUNTIME_ASAN

} // namespace stdx::runtime

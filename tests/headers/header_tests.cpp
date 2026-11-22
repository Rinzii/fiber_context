//==- header_tests.cpp - Public API Compile Checks -------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Instantiates both construction paths and checks that the public handle
// resolves to the versioned ABI type.
//
//===----------------------------------------------------------------------===//

#if defined(FIBER_CONTEXT_TEST_PLAIN_NAMESPACE)
namespace stdx
{
    class fiber_context;
}
#endif

#include <fiber_context.h>

#include <concepts>
#include <span>
#include <utility>

#if defined(FIBER_CONTEXT_TEST_PLAIN_NAMESPACE)
    #ifdef INTERNAL_STDX_FIBER_CONTEXT_ABI_NAMESPACE
        #error The source headers selected an ABI namespace without a generated configuration.
    #endif
#else
static_assert(std::same_as<stdx::fiber_context, stdx::INTERNAL_STDX_FIBER_CONTEXT_ABI_NAMESPACE::fiber_context>);
#endif

template<class Wrapper>
static void exercise_wrapper_constructors(Wrapper &original)
{
    Wrapper copy{original};
    [[maybe_unused]] Wrapper moved{std::move(copy)};
}

stdx::fiber_context exercise_fiber_context(std::span<std::byte> storage)
{
    auto yield   = [](stdx::fiber_context &&caller) { return std::move(caller); };
    auto release = [](std::span<std::byte>) noexcept {};
    stdx::detail::stored_entry<decltype(yield)> entry{yield};
    stdx::detail::stored_deleter<decltype(release)> deleter{release};
    exercise_wrapper_constructors(entry);
    exercise_wrapper_constructors(deleter);
    stdx::fiber_context owned{yield};
    owned = std::move(owned).resume();
    stdx::fiber_context supplied{yield, storage, release};
    return std::move(supplied).resume_with(yield);
}

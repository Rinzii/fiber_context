//==- fiber_function.h - Fiber Callable Constraint -------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Checks that a callable accepts a fiber_context rvalue and returns a
// fiber_context by value or rvalue reference.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "fwd.h"

#include <concepts>
#include <type_traits>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    template<class F>
    concept fiber_function = requires { requires std::same_as<std::invoke_result_t<F, fiber_context &&> &&, fiber_context &&>; };
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

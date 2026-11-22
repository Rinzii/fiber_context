//==- stored_entry_impl.h - Entry Callable Invocation ----------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Invokes the stored callable with the caller's handle and moves the callable
// if it can only be invoked as an rvalue.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "fiber_function.h"
#include "stored_entry.h"

#include <functional>
#include <utility>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    template<class F>
    fiber_context stored_entry<F>::invoke(fiber_context &&caller)
    {
        if constexpr (fiber_function<F &>)
        {
            return std::invoke(m_entry, std::move(caller));
        }
        else
        {
            return std::invoke(std::move(m_entry), std::move(caller));
        }
    }
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

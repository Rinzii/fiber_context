//==- stored_deleter.h - Stored Stack Deleter ------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Owns the user's deleter and invokes it with the supplied stack span.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "stack_deleter.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    template<class D>
    struct INTERNAL_STDX_FIBER_CONTEXT_HIDDEN stored_deleter final : stack_deleter
    {
        template<class T>
            requires(!std::same_as<std::remove_cvref_t<T>, stored_deleter>)
        explicit stored_deleter(T &&deleter) : m_deleter(std::forward<T>(deleter))
        {
        }

        void invoke(std::span<std::byte> stack) noexcept override
        {
            if constexpr (std::is_invocable_v<D &, std::span<std::byte> &>)
            {
                std::invoke(m_deleter, stack);
            }
            else
            {
                std::invoke(std::move(m_deleter), std::move(stack));
            }
        }

        D m_deleter;
    };
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

//==- stored_entry.h - Stored Entry Callable -------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Owns the user's callable and exposes it through the runtime's fiber entry
// interface.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "fiber_entry.h"

#include <concepts>
#include <type_traits>
#include <utility>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    template<class F>
    struct INTERNAL_STDX_FIBER_CONTEXT_HIDDEN stored_entry final : fiber_entry
    {
        template<class T>
            requires(!std::same_as<std::remove_cvref_t<T>, stored_entry>)
        explicit stored_entry(T &&entry) : m_entry(std::forward<T>(entry))
        {
        }

        fiber_context invoke(fiber_context &&caller) override;

        F m_entry;
    };
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

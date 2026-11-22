//==- store_for_supplied_stack.h - Supplied-Stack Callable Storage ---------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Allocates a callable wrapper and reports allocation failure as a resource
// error. Exceptions from the callable's constructor propagate.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"

#include <memory>
#include <new>
#include <system_error>
#include <utility>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    template<class Stored, class F>
    INTERNAL_STDX_FIBER_CONTEXT_HIDDEN std::unique_ptr<Stored> store_for_supplied_stack(F &&fn)
    {
        // Translate allocation failure but let constructor exceptions propagate.
        std::unique_ptr<Stored> result(new (std::nothrow) Stored(std::forward<F>(fn)));
        if (!result)
        {
            throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
        }

        return result;
    }
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

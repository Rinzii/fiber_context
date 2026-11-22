//==- stack_deleter.h - Stack Deleter Interface ----------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Lets the runtime pass a caller-supplied stack back to its deleter when the
// fiber is done with it.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "fwd.h"

#include <cstddef>
#include <span>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    struct stack_deleter
    {
        virtual ~stack_deleter()                                 = default;
        virtual void invoke(std::span<std::byte> stack) noexcept = 0;
    };
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

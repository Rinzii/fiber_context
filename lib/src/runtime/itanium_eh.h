//==- itanium_eh.h - Itanium Exception State Record ------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the per-fiber copy of the caught-exception pointer and
// uncaught-exception count.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <__fiber_context/config.h>

#include <cstddef>

STDX_FIBER_CONTEXT_NAMESPACE::runtime
{

    struct exception_state
    {
        std::byte caught[sizeof(void *)]{};
        unsigned int uncaught{};
    };

    void switch_exceptions(exception_state &from, const exception_state &to) noexcept;

} // namespace stdx::runtime

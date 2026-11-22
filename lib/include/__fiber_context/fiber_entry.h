//==- fiber_entry.h - Fiber Entry Interface --------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Lets the runtime invoke a user-supplied entry function through a common
// virtual interface.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "fwd.h"

STDX_FIBER_CONTEXT_NAMESPACE::detail
{
    struct fiber_entry
    {
        virtual ~fiber_entry()                               = default;
        virtual fiber_context invoke(fiber_context &&caller) = 0;
    };
} // namespace STDX_FIBER_CONTEXT_NAMESPACE::detail

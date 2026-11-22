//==- check.h - Test Failure Reporting -------------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Prints the failed assertion's message and exits immediately so stack
// unwinding cannot obscure the failure.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdio>
#include <cstdlib>

inline void check(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::_Exit(EXIT_FAILURE);
    }
}

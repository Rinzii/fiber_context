//==- ping_pong.cpp - Fiber Transfer Example -------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Passes control between main and a worker fiber and prints where execution
// resumes.
//
//===----------------------------------------------------------------------===//

#include <fiber_context.h>

#include <iostream>
#include <utility>

int main()
{
    using stdx::fiber_context;

    fiber_context worker{[](fiber_context &&caller)
        {
            std::cout << "worker begins\n";
            caller = std::move(caller).resume();
            std::cout << "worker finishes\n";
            return std::move(caller);
        }};

    worker = std::move(worker).resume();
    std::cout << "back in main\n";
    worker = std::move(worker).resume();

    return worker.empty() ? 0 : 1;
}

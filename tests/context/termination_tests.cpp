//==- termination_tests.cpp - Fiber Termination Scenarios ------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Exercises operations that must terminate and gives the test driver a distinct
// exit code when the handler runs.
//
//===----------------------------------------------------------------------===//

#include <fiber_context.h>

#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <utility>

using stdx::fiber_context;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return EXIT_FAILURE;
    }

    std::set_terminate([] { std::_Exit(113); });
    const std::string_view scenario = argv[1];
    const auto identity             = [](fiber_context &&caller) { return std::move(caller); };

    if (scenario == "destroy_live")
    {
        fiber_context live{identity};
    }
    else if (scenario == "assign_live")
    {
        fiber_context live{identity};
        live = fiber_context{};

        std::_Exit(0);
    }
    else if (scenario == "self_move_live")
    {
        fiber_context live{identity};
        auto &same = live;
        live       = std::move(same);

        std::_Exit(0);
    }
    else if (scenario == "empty_successor" || scenario == "throw_from_entry")
    {
        fiber_context back;
        fiber_context worker{[&](fiber_context &&caller) -> fiber_context
            {
                back = std::move(caller);
                if (scenario == "throw_from_entry")
                {
                    throw std::runtime_error("entry failure");
                }

                return {};
            }};

        worker = std::move(worker).resume();
        std::_Exit(0);
    }
    else
    {
        return EXIT_FAILURE;
    }
}

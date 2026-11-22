//==- register_tests.cpp - Register and Stack Frame Tests ------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Yields from a nested frame to check register preservation and rounding modes
// while a local object remains alive.
//
//===----------------------------------------------------------------------===//

#include <fiber_context.h>

#include "asm/source.h"
#include "check.h"

#include <cfenv>
#include <utility>

using yield_callback = void (*)();
extern "C" FIBER_CONTEXT_ASM_SOURCE(FIBER_CONTEXT_ASM_TARGET("tests/context/asm/arm64_registers.S", "tests/context/asm/x86_64_registers.S"),
    fiber_context_test_preserved_registers) int fiber_context_test_preserved_registers(yield_callback yield);

namespace
{

    using stdx::fiber_context;

    struct register_probe
    {
        fiber_context &caller;
        unsigned &destroyed;
    };

    thread_local register_probe *active_probe = nullptr;

    void yield_from_nested_frame()
    {
        struct lifetime
        {
            unsigned &count;
            ~lifetime() { ++count; }
        } local{.count = active_probe->destroyed};

        fiber_context &caller = active_probe->caller;

        caller = std::move(caller).resume();
        check(caller.can_resume(), "caller returned to nested frame");
    }

    void test_registers_and_frames()
    {
        unsigned destroyed      = 0;
        const int original_mode = std::fegetround();
        check(std::fesetround(FE_TOWARDZERO) == 0, "set rounding mode before fiber construction");

        fiber_context worker{[&](fiber_context &&caller)
            {
                check(std::fegetround() == FE_TOWARDZERO, "worker inherited rounding mode at construction");
                register_probe probe{.caller = caller, .destroyed = destroyed};
                active_probe = &probe;
                check(std::fesetround(FE_DOWNWARD) == 0, "set worker rounding mode");
                check(fiber_context_test_preserved_registers(yield_from_nested_frame) == 0, "callee-saved registers survived yield");
                check(std::fegetround() == FE_DOWNWARD, "worker rounding mode restored");
                active_probe = nullptr;
                return std::move(caller);
            }};
        check(std::fesetround(FE_UPWARD) == 0, "set main rounding mode");

        worker = std::move(worker).resume();
        check(std::fegetround() == FE_UPWARD, "main rounding mode restored");
        check(worker.can_resume() && destroyed == 0, "nested local survives suspension");

        worker = std::move(worker).resume();
        check(worker.empty() && destroyed == 1, "nested local destroyed once");
        check(std::fegetround() == FE_UPWARD, "main rounding mode restored on worker exit");
        check(std::fesetround(original_mode) == 0, "restore original rounding mode");
    }
} // namespace

int main() { test_registers_and_frames(); }

//==- runtime_tests.cpp - Fiber Runtime Tests ------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Exercises exception handling across fiber switches and reuse of supplied
// stacks. Provides a separate stack-use-after-return case for ASan.
//
//===----------------------------------------------------------------------===//

#include <fiber_context.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <span>
#include <utility>

namespace
{

    using stdx::fiber_context;

    void check(bool condition, const char *message)
    {
        if (!condition)
        {
            std::fprintf(stderr, "%s\n", message);
            std::abort();
        }
    }

    struct worker_error
    {
    };
    struct caller_error
    {
    };

    void interleaved_catches()
    {
        fiber_context worker(
            [](fiber_context &&caller)
            {
                check(!std::current_exception(), "a new fiber inherited the caller's catch");
                try
                {
                    throw worker_error{};
                }
                catch (const worker_error &original)
                {
                    const auto exception = std::current_exception();
                    for (int i = 0; i < 3; ++i)
                    {
                        caller = std::move(caller).resume();
                        check(std::current_exception() == exception, "worker catch changed after a yield");

                        try
                        {
                            throw;
                        }
                        catch (const worker_error &again)
                        {
                            check(&again == &original, "worker rethrow lost the caught object");
                        }
                    }
                }

                return std::move(caller);
            });

        try
        {
            throw caller_error{};
        }
        catch (const caller_error &original)
        {
            const auto exception = std::current_exception();
            do
            {
                worker = std::move(worker).resume();
                check(std::current_exception() == exception, "caller catch changed after a resume");

                try
                {
                    throw;
                }
                catch (const caller_error &again)
                {
                    check(&again == &original, "caller rethrow lost the caught object");
                }
            }
            while (worker);
        }

        check(!std::current_exception(), "caught exception remained after the final handler");
    }

    void suspend_during_unwind()
    {
        struct yield_on_unwind
        {
            fiber_context &caller;
            ~yield_on_unwind()
            {
                check(std::uncaught_exceptions() == 1, "worker destructor expected an active unwind");
                caller = std::move(caller).resume();
                check(std::uncaught_exceptions() == 1, "worker's unwind count was lost");
            }
        };

        fiber_context worker(
            [](fiber_context &&caller)
            {
                try
                {
                    yield_on_unwind during_unwind{caller};
                    throw worker_error{};
                }
                catch (const worker_error &)
                {
                    check(std::uncaught_exceptions() == 0, "worker catch retained an uncaught exception");
                }

                return std::move(caller);
            });

        worker = std::move(worker).resume();
        check(std::uncaught_exceptions() == 0, "caller inherited the worker's unwind count");
        worker = std::move(worker).resume();
        check(worker.empty(), "unwinding worker did not complete");
    }

    void overlapping_exception_ptr_handlers()
    {
        const auto shared = std::make_exception_ptr(worker_error{});
        fiber_context worker(
            [&](fiber_context &&caller)
            {
                try
                {
                    std::rethrow_exception(shared);
                }
                catch (const worker_error &)
                {
                    caller = std::move(caller).resume();
                    check(std::current_exception() == shared, "worker lost the shared exception");

                    try
                    {
                        throw;
                    }
                    catch (const worker_error &)
                    {
                        check(std::current_exception() == shared, "worker rethrew a different exception");
                    }
                }

                return std::move(caller);
            });

        worker = std::move(worker).resume();
        try
        {
            std::rethrow_exception(shared);
        }
        catch (const worker_error &)
        {
            worker = std::move(worker).resume();
            check(std::current_exception() == shared, "worker release changed the caller's shared catch");

            try
            {
                throw;
            }
            catch (const worker_error &)
            {
                check(std::current_exception() == shared, "caller rethrew a different exception");
            }
        }
    }

    void reuse_supplied_stack()
    {
        alignas(64) static std::array<std::byte, 512 * 1024> stack;
        int deletions = 0;
        for (int iteration = 0; iteration < 16; ++iteration)
        {
            int *suspended = nullptr;
            fiber_context worker(
                [&](fiber_context &&caller)
                {
                    int live  = iteration;
                    suspended = &live;
                    caller    = std::move(caller).resume();
                    check(live == iteration + 1, "write to a suspended frame was lost");
                    suspended = nullptr;
                    return std::move(caller);
                },
                std::span{stack},
                [&](std::span<std::byte> released) noexcept
                {
                    check(released.data() == stack.data() && released.size() == stack.size(), "deleter received a different stack");
                    ++deletions;
                });

            worker = std::move(worker).resume();
            check(suspended && *suspended == iteration, "suspended stack local lost its value");
            ++*suspended;
            worker = std::move(worker).resume();
            check(worker.empty() && deletions == iteration + 1, "stack retirement did not finish");
        }
    }

    volatile int *volatile escaped;

#ifdef _MSC_VER
    #define FIBER_CONTEXT_TEST_NOINLINE __declspec(noinline)
#else
    #define FIBER_CONTEXT_TEST_NOINLINE [[gnu::noinline]]
#endif

    FIBER_CONTEXT_TEST_NOINLINE void remember_fiber_local()
    {
        int value = 73;
        escaped   = &value;
    }

    FIBER_CONTEXT_TEST_NOINLINE int read_returned_fiber_local() { return *escaped; }

    void asan_use_after_return()
    {
        fiber_context worker(
            [](fiber_context &&caller)
            {
                remember_fiber_local();
                caller                                    = std::move(caller).resume();
                [[maybe_unused]] const volatile int value = read_returned_fiber_local();
                return std::move(caller);
            });

        worker = std::move(worker).resume();
        worker = std::move(worker).resume();
    }

} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--asan-use-after-return") == 0)
    {
        asan_use_after_return();
        return 0;
    }

    if (argc != 1)
    {
        return 2;
    }

    interleaved_catches();
    suspend_during_unwind();
    overlapping_exception_ptr_handlers();
    reuse_supplied_stack();
}

//==- context_tests.cpp - Fiber Context Behavior Tests ---------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Exercises handle moves and injected callbacks along with supplied-stack
// lifetimes and thread affinity.
//
//===----------------------------------------------------------------------===//

#include <fiber_context.h>

#include "check.h"

#include <barrier>
#include <concepts>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#ifdef FIBER_CONTEXT_TESTING_WITH_ASAN
    #include <sanitizer/asan_interface.h>
#endif

namespace
{
    using stdx::fiber_context;

    auto identity = [](fiber_context &&caller) -> fiber_context &&
    { return std::move(caller); };

    template<class T>
    concept can_resume_lvalue = requires(T &value) { value.resume(); };

    template<class T>
    concept can_inject_lvalue = requires(T &value) { value.resume_with(identity); };

    static_assert(std::is_nothrow_default_constructible_v<fiber_context>);
    static_assert(std::is_nothrow_move_constructible_v<fiber_context>);
    static_assert(std::is_nothrow_move_assignable_v<fiber_context>);
    static_assert(!std::is_copy_constructible_v<fiber_context>);
    static_assert(!std::is_copy_assignable_v<fiber_context>);
    static_assert(!std::is_convertible_v<fiber_context, bool>);
    static_assert(std::is_nothrow_swappable_v<fiber_context>);
    static_assert(!can_resume_lvalue<fiber_context> && !can_inject_lvalue<fiber_context>);
    static_assert(!noexcept(std::declval<fiber_context>().resume()));
    static_assert(!noexcept(std::declval<fiber_context>().resume_with(identity)));
    static_assert(noexcept(std::declval<const fiber_context &>().empty()));
    static_assert(noexcept(std::declval<const fiber_context &>().can_resume()));
    static_assert(noexcept(static_cast<bool>(std::declval<const fiber_context &>())));

    void test_moves()
    {
        fiber_context first;
        check(first.empty() && !first && !first.can_resume(), "default handle is empty");
        fiber_context &same = first;
        first               = std::move(same);
        check(first.empty(), "empty self-move");

        fiber_context prepared{identity};
        const auto &construction_source = prepared;
        fiber_context moved{std::move(prepared)};
        check(moved.can_resume() && construction_source.empty(), "move construction");

        swap(first, moved);
        check(first.can_resume() && moved.empty(), "ADL swap");

        moved.swap(first);
        const auto &assignment_source = moved;
        prepared                      = std::move(moved);
        check(prepared.can_resume() && assignment_source.empty() && first.empty(), "member swap and move assignment");

        prepared = std::move(prepared).resume();
        check(prepared.empty(), "rvalue-reference entry result");
    }

    void test_injection()
    {
        unsigned phase = 0;
        fiber_context saved;
        fiber_context third{[&](fiber_context &&caller)
            {
                check(phase++ == 2, "third fiber ran only when resumed");
                return std::move(caller);
            }};

        fiber_context worker{[&](fiber_context &&other)
            {
                check(phase++ == 1 && other.can_resume(), "entry receives injected handle");
                other = std::move(other).resume();
                check(phase++ == 3 && other.empty(), "completed third fiber returns empty");

                try
                {
                    saved = std::move(saved).resume();
                    check(false, "injected exception reached suspended call");
                }
                catch (const std::runtime_error &)
                {
                    check(phase++ == 6, "exception caught on target");
                }

                return std::move(saved);
            }};

        auto inject_third = [&](fiber_context &&caller)
        {
            check(phase++ == 0 && worker.empty(), "handle consumed before first injection");
            saved = std::move(caller);
            return std::move(third);
        };
        worker = std::move(worker).resume_with(inject_third);
        check(phase++ == 4 && worker.can_resume(), "worker yielded to main");

        auto inject_exception = [&](fiber_context &&caller) -> fiber_context
        {
            check(phase++ == 5 && worker.empty(), "injection runs on reactivation");
            saved = std::move(caller);
            throw std::runtime_error("injection");
        };
        worker = std::move(worker).resume_with(inject_exception);
        check(worker.empty() && phase == 7, "worker completed after injection");
    }

    void test_empty_injection()
    {
        fiber_context main_fiber;
        fiber_context worker{[&](fiber_context &&incoming)
            {
                check(incoming.empty(), "entry receives empty injection result");
                return std::move(main_fiber);
            }};

        worker = std::move(worker).resume_with(
            [&](fiber_context &&caller) -> fiber_context
            {
                main_fiber = std::move(caller);
                return {};
            });

        check(worker.empty(), "empty injection completed");
    }

    void test_recursive_injection()
    {
        unsigned phase = 0;
        fiber_context worker{[&](fiber_context &&caller)
            {
                check(phase++ == 5, "entry starts after recursive injection finishes");
                return std::move(caller);
            }};

        auto first = [&](fiber_context &&caller)
        {
            check(phase++ == 0, "first injection begins");
            caller = std::move(caller).resume_with(
                [&](fiber_context &&suspended)
                {
                    check(phase++ == 1, "nested injection receives suspended worker");
                    return std::move(suspended);
                });

            check(phase++ == 4, "first injection resumes with its caller");
            return std::move(caller);
        };

        worker = std::move(worker).resume_with(first);
        check(phase++ == 2 && worker.can_resume(), "main can resume suspended injection");

        worker = std::move(worker).resume_with(
            [&](fiber_context &&caller)
            {
                check(phase++ == 3, "reactivation injects into nested resume");
                return std::move(caller);
            });
        check(worker.empty() && phase == 6, "recursive injection completed");
    }

    void test_supplied_stack()
    {
        constexpr std::size_t size = 64 * 1024 + 7;
        constexpr auto alignment   = std::align_val_t{16};
        auto *data                 = static_cast<std::byte *>(::operator new(size, alignment));
        const std::span storage{data, size};
        const auto begin    = reinterpret_cast<std::uintptr_t>(data);
        const auto on_stack = [begin](void *pointer)
        {
#ifdef FIBER_CONTEXT_TESTING_WITH_ASAN
            if (auto *frame = __asan_addr_is_in_fake_stack(__asan_get_current_fake_stack(), pointer, nullptr, nullptr))
            {
                pointer = frame;
            }

#endif
            const auto address = reinterpret_cast<std::uintptr_t>(pointer);
            return address >= begin && address - begin < size;
        };

        bool entry_destroyed   = false;
        bool cleanup_suspended = false;
        unsigned deleted       = 0;
        fiber_context helper{[&](fiber_context &&finalizer)
            {
                check(deleted == 1, "reentrant cleanup runs after the supplied stack is released");
                cleanup_suspended = true;
                return std::move(finalizer);
            }};

        auto deleter = [&](std::span<std::byte> actual)
        {
            check(actual.data() == data && actual.size() == size, "deleter receives original span");

            std::byte marker{};
            check(!on_stack(&marker), "deleter runs off supplied stack");
            check(entry_destroyed, "entry callable destroyed before deleter starts");

            ++deleted;
            operator delete(actual.data(), alignment);
            helper = std::move(helper).resume();
            check(helper.empty(), "deleter resumes safely after helper terminates");
        };

        try
        {
            fiber_context invalid{identity, storage.subspan(1), deleter};
            check(false, "misaligned stack accepted");
        }
        catch (const std::invalid_argument &)
        {
        }

        try
        {
            fiber_context invalid{identity, storage.first(64 * 1024 - 1), deleter};
            check(false, "undersized stack accepted");
        }
        catch (const std::length_error &)
        {
        }

        check(deleted == 0, "constructor failures leave caller's storage intact");
        auto mark_destroyed = [&](bool *) { entry_destroyed = true; };
        std::unique_ptr<bool, decltype(mark_destroyed)> lifetime{&entry_destroyed, mark_destroyed};
        fiber_context worker{[&, lifetime = std::move(lifetime)](fiber_context &&caller)
            {
                check(!*lifetime, "move-only entry remains alive");

                std::byte marker{};
                check(on_stack(&marker), "entry executes on supplied stack");
                caller = std::move(caller).resume();
                return std::move(caller);
            },
            storage,
            deleter};

        worker = std::move(worker).resume();
        check(worker.can_resume() && deleted == 0, "supplied stack is live while suspended");
        worker = std::move(worker).resume();
        check(worker.empty() && deleted == 1 && cleanup_suspended, "cleanup finishes before successor runs");
    }

    thread_local int thread_value = 0;

    void test_thread_affinity()
    {
        std::barrier rendezvous{2};
        fiber_context worker{[](fiber_context &&caller)
            {
                check(thread_value == 9, "entry runs on activating thread");
                thread_value = 13;
                caller       = std::move(caller).resume();
                return std::move(caller);
            }};

        std::thread owner{[&]
            {
                thread_value = 9;
                check(worker.can_resume(), "prepared fiber can first run on another thread");
                worker = std::move(worker).resume();
                check(thread_value == 13, "thread-local shared across owner's fibers");
                rendezvous.arrive_and_wait();
                rendezvous.arrive_and_wait();
                check(worker.can_resume(), "owner can resume suspended fiber");
                worker = std::move(worker).resume();
            }};

        rendezvous.arrive_and_wait();
        check(worker && !worker.can_resume(), "other thread can observe suspended handle");

        rendezvous.arrive_and_wait();
        owner.join();
        check(thread_value == 0 && worker.empty(), "independent default fibers");
    }

} // namespace

int main()
{
    test_moves();
    test_injection();
    test_empty_injection();
    test_recursive_injection();
    test_supplied_stack();
    test_thread_affinity();
}

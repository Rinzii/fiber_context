//==- fiber_context.cpp - Fiber Context Lifecycle --------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Creates fibers and transfers control between them. Releases their stacks
// after completion.
//
//===----------------------------------------------------------------------===//

#include <__fiber_context/config.h>
#include <fiber_context.h>

#include "native/context.h"

#include <atomic>
#include <cstdint> // IWYU pragma: keep
#include <limits>
#include <optional>

STDX_FIBER_CONTEXT_NAMESPACE::detail
{

    struct explicit_fiber;

    namespace
    {
        struct incoming_transfer
        {
            fiber_state *caller{};
            void *callable{};
            fiber_context (*inject)(void *, fiber_context &&){};
            explicit_fiber *retired{};
        };
    } // namespace

    struct fiber_state
    {
        native::context initial;
        native::context *active_stack = &initial;
        runtime::exception_state exceptions;
        incoming_transfer incoming;

        // Zero lets any thread claim the fiber on its first resume.
        std::uint64_t owner{};
        explicit_fiber *fiber{};
    };

    struct explicit_fiber
    {
        fiber_state state{.fiber = this};
        std::unique_ptr<fiber_entry> entry;
        std::unique_ptr<stack_deleter> deleter;
        std::span<std::byte> supplied;
        std::optional<native::stack> allocated_stack;
        std::optional<native::stack> cleanup_stack;
        native::context cleanup;
        fiber_state *successor{};
    };

    namespace
    {

        thread_local fiber_state *current = nullptr;

        std::uint64_t thread_token() noexcept
        {
            static std::atomic<std::uint64_t> last{0};
            thread_local const auto token = []
            {
                auto value = last.load(std::memory_order_relaxed);

                for (;;)
                {
                    if (value == std::numeric_limits<std::uint64_t>::max())
                    {
                        std::terminate();
                    }

                    if (last.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
                    {
                        return value + 1;
                    }
                }
            }();

            return token;
        }

        fiber_state &running_fiber() noexcept
        {
            if (!current)
            {
                thread_local fiber_state default_fiber;
                default_fiber.owner = thread_token();
                current             = &default_fiber;
            }

            return *current;
        }

        void activate(fiber_state &from, fiber_state &to, bool retiring) noexcept
        {
            to.owner = from.owner;
            current  = &to;

            runtime::switch_exceptions(from.exceptions, to.exceptions);
            native::transfer(*from.active_stack, *to.active_stack, retiring);
        }

    } // namespace

    struct fiber_runtime
    {
        static fiber_context receive(fiber_state &self)
        {
            const incoming_transfer message = std::exchange(self.incoming, {});

            delete message.retired;
            fiber_context caller;
            caller.m_state = message.caller;
            if (message.inject)
            {
                return message.inject(message.callable, std::move(caller));
            }

            return caller;
        }

        static void depart(explicit_fiber &self) noexcept
        {
            fiber_state &successor = *self.successor;
            successor.incoming     = {.retired = &self};
            activate(self.state, successor, true);
            std::terminate();
        }

        static void run_cleanup() noexcept
        {
            explicit_fiber &self = *current->fiber;
            self.deleter->invoke(self.supplied);
            self.deleter.reset();
            depart(self);
        }

        static FIBER_CONTEXT_NO_ASAN void begin_cleanup() noexcept
        {
            native::finish(*current->active_stack);
            run_cleanup();
        }

        static void run_entry() noexcept
        {
            auto &self = *current->fiber;
            {
                fiber_context caller    = receive(self.state);
                fiber_context successor = self.entry->invoke(std::move(caller));
                if (!successor.can_resume())
                {
                    std::terminate();
                }

                self.successor = std::exchange(successor.m_state, nullptr);
            }

            self.entry.reset();
            if (self.deleter)
            {
                // The deleter may free the supplied stack, so it runs on the cleanup stack.
                // The successor destroys the retired fiber and its cleanup stack after the switch.
                self.state.active_stack = &self.cleanup;
                native::transfer(self.state.initial, self.cleanup, true);
                std::terminate();
            }

            depart(self);
        }

        static FIBER_CONTEXT_NO_ASAN void begin_entry() noexcept
        {
            native::finish(*current->active_stack);
            run_entry();
        }
    };

} // namespace stdx::detail

STDX_FIBER_CONTEXT_NAMESPACE
{

    detail::fiber_state *fiber_context::prepare(
        std::unique_ptr<detail::fiber_entry> entry, std::span<std::byte> stack, std::unique_ptr<detail::stack_deleter> deleter)
    {
        const bool supplied = static_cast<bool>(deleter);
        try
        {
            if (supplied)
            {
                native::validate(stack);
            }

            auto fiber = std::make_unique<detail::explicit_fiber>();

            if (supplied)
            {
                fiber->supplied     = stack;
                const auto &cleanup = fiber->cleanup_stack.emplace();
                native::prepare(fiber->cleanup, cleanup.data(), detail::fiber_runtime::begin_cleanup);
            }
            else
            {
                stack = fiber->allocated_stack.emplace().data();
            }

            native::prepare(fiber->state.initial, stack, detail::fiber_runtime::begin_entry);

            fiber->entry   = std::move(entry);
            fiber->deleter = std::move(deleter);

            return &fiber.release()->state;
        }
        catch (const std::bad_alloc &)
        {
            if (supplied)
            {
                throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
            }

            throw;
        }
    }

    bool fiber_context::can_resume() const noexcept { return m_state && (m_state->owner == 0 || m_state->owner == detail::thread_token()); }

    fiber_context fiber_context::transfer(void *callable, injection call)
    {
        if (!can_resume())
        {
            std::terminate();
        }

        detail::fiber_state &from = detail::running_fiber();
        detail::fiber_state &to   = *std::exchange(m_state, nullptr);

        to.incoming = {.caller = &from, .callable = callable, .inject = call};

        detail::activate(from, to, false);

        return detail::fiber_runtime::receive(from);
    }

    fiber_context fiber_context::resume() && { return transfer(nullptr, nullptr); }

} // namespace stdx

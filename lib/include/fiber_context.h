//==- fiber_context.h - Public Fiber Context Interface ---------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the move-only handle used to create and resume fibers with
// library-managed or caller-supplied stacks.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "__fiber_context/config.h"
#include "__fiber_context/fiber_function.h"
#include "__fiber_context/fwd.h"
#include "__fiber_context/store_for_supplied_stack.h"
#include "__fiber_context/stored_deleter.h"
#include "__fiber_context/stored_entry.h"

#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

STDX_FIBER_CONTEXT_NAMESPACE
{
    class INTERNAL_STDX_FIBER_CONTEXT_API fiber_context
    {

    public:
        fiber_context() noexcept = default;

        template<class F>
            requires(!std::same_as<std::remove_cvref_t<F>, fiber_context>)
        explicit fiber_context(F &&entry);

        template<class F, class D>
        fiber_context(F &&entry, std::span<std::byte> stack, D &&deleter);

        // https://wg21.link/p0876r24#page=37
        ~fiber_context()
        {
            if (m_state)
            {
                std::terminate();
            }
        }

        fiber_context(fiber_context &&other) noexcept : m_state(std::exchange(other.m_state, nullptr)) {}

        fiber_context &operator=(fiber_context &&other) noexcept
        {
            if (m_state)
            {
                std::terminate();
            }

            m_state = std::exchange(other.m_state, nullptr);
            return *this;
        }

        fiber_context resume() &&;

        template<class Fn>
        fiber_context resume_with(Fn &&fn) &&
        {
            static_assert(detail::fiber_function<std::decay_t<Fn>>);
            auto call = [&fn](fiber_context &&caller) -> fiber_context { return std::invoke(std::forward<Fn>(fn), std::move(caller)); };

            // The target invokes this callback while the caller is suspended.
            return transfer(
                &call, [](void *callable, fiber_context &&caller) -> fiber_context { return (*static_cast<decltype(call) *>(callable))(std::move(caller)); });
        }

        bool can_resume() const noexcept; // NOLINT(*-use-nodiscard)

        explicit operator bool() const noexcept { return m_state != nullptr; }

        bool empty() const noexcept { return m_state == nullptr; } // NOLINT(*-use-nodiscard)

        void swap(fiber_context &other) noexcept { std::swap(m_state, other.m_state); }

        friend void swap(fiber_context &lhs, fiber_context &rhs) noexcept { lhs.swap(rhs); }

    private:
        friend struct detail::fiber_runtime;

        using injection = fiber_context (*)(void *, fiber_context &&);

        static detail::fiber_state *prepare(
            std::unique_ptr<detail::fiber_entry> entry, std::span<std::byte> stack, std::unique_ptr<detail::stack_deleter> deleter);

        fiber_context transfer(void *callable, injection call);

        detail::fiber_state *m_state = nullptr;
    };

    template<class F>
        requires(!std::same_as<std::remove_cvref_t<F>, fiber_context>)
    fiber_context::fiber_context(F &&entry)
    {
        using callable = std::decay_t<F>;

        static_assert(std::is_constructible_v<callable, F> && detail::fiber_function<callable>);

        m_state = prepare(std::make_unique<detail::stored_entry<callable>>(std::forward<F>(entry)), {}, nullptr);
    }

    template<class F, class D>
    fiber_context::fiber_context(F &&entry, std::span<std::byte> stack, D &&deleter)
    {
        using callable = std::decay_t<F>;
        using cleanup  = std::decay_t<D>;

        static_assert(std::is_constructible_v<callable, F> && detail::fiber_function<callable>);
        static_assert(std::is_constructible_v<cleanup, D> && std::is_invocable_v<cleanup, std::span<std::byte>>);

        auto owned_entry   = detail::store_for_supplied_stack<detail::stored_entry<callable>>(std::forward<F>(entry));
        auto owned_deleter = detail::store_for_supplied_stack<detail::stored_deleter<cleanup>>(std::forward<D>(deleter));

        m_state = prepare(std::move(owned_entry), stack, std::move(owned_deleter));
    }
} // namespace STDX_FIBER_CONTEXT_NAMESPACE

#include "__fiber_context/stored_entry_impl.h"

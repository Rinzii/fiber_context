//==- context.h - Native Stack and Context Types ---------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the owned stack and selects the register record used by the native
// context.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <__fiber_context/config.h>

#include "runtime/runtime.h"

#include <cstddef>
#include <cstdint>
#include <span>

#if defined(__x86_64__) && (defined(__APPLE__) || defined(__linux__))

    #include "asm/x86_64/x86_64_layout.h"

STDX_FIBER_CONTEXT_NAMESPACE::native
{
    using register_record                              = fiber_context_x86_record;
    inline constexpr auto initialize_registers         = fiber_context_x86_initialize;
    inline constexpr std::size_t initial_stack_reserve = sizeof(std::uintptr_t);
} // namespace stdx::native

#elif defined(__APPLE__) && defined(__aarch64__)

    #include "asm/arm64/arm64_layout.h"

STDX_FIBER_CONTEXT_NAMESPACE::native
{
    using register_record                              = fiber_context_arm_record;
    inline constexpr auto initialize_registers         = fiber_context_arm_initialize;
    inline constexpr std::size_t initial_stack_reserve = 0;
} // namespace stdx::native

#else
    #error "No fiber_context assembly backend for this target"
#endif

STDX_FIBER_CONTEXT_NAMESPACE::native
{

    inline constexpr std::size_t minimum_stack_size = 64 * 1024;

    class stack
    {
    public:
        stack();
        ~stack();

        stack(const stack &)            = delete;
        stack &operator=(const stack &) = delete;

        std::span<std::byte> data() const noexcept { return m_bytes; } // NOLINT(*-use-nodiscard)

    private:
        void *m_mapping{};
        std::size_t m_mapping_size{};
        std::span<std::byte> m_bytes{};
    };

    struct context
    {
        context() noexcept;
        context(const context &)            = delete;
        context &operator=(const context &) = delete;

        register_record registers{};

#if !FIBER_CONTEXT_ASM_CANARIES

        const std::byte *stack_low{};
        const std::byte *stack_high{};

#endif // !FIBER_CONTEXT_ASM_CANARIES

#if FIBER_CONTEXT_RUNTIME_ASAN

        runtime::physical_stack physical{};

#endif // FIBER_CONTEXT_RUNTIME_ASAN
    };

    void validate(std::span<std::byte> storage);
    void prepare(context &target, std::span<std::byte> storage, void (*entry)());
    void transfer(context &from, context &to, bool retiring = false) noexcept;

#if FIBER_CONTEXT_RUNTIME_ASAN

    FIBER_CONTEXT_NO_ASAN inline void finish(context &entered) noexcept { runtime::finish(entered.physical); }

#else // if FIBER_CONTEXT_RUNTIME_ASAN

    inline void finish(context &) noexcept {}

#endif // else

} // namespace stdx::native

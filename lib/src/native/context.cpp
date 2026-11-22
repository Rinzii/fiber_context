//==- context.cpp - Native Context Preparation and Transfer ----------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Validates stack storage and prepares the initial register state. Calls the
// assembly routine to transfer control.
//
//===----------------------------------------------------------------------===//

#include "context.h"
#include "asm/symbols.h"

#include <__fiber_context/config.h>

#include <cstdint>
#include <stdexcept>

#if defined(__arm64e__) || (defined(__CET__) && __CET__)
    #error "This backend has not qualified arm64e or CET"
#endif // defined(__arm64e__) || (defined(__CET__) && __CET__)

STDX_FIBER_CONTEXT_NAMESPACE::native
{
    namespace
    {

        constexpr std::uintptr_t alignment = 16;

    } // namespace

    context::context() noexcept { initialize_registers(registers, nullptr, nullptr, nullptr, nullptr, nullptr); }

    void validate(std::span<std::byte> storage)
    {
        const auto base = reinterpret_cast<std::uintptr_t>(storage.data());
        if ((base % alignment) != 0)
        {
            throw std::invalid_argument("fiber stack base requires 16-byte alignment");
        }

        if (storage.size() < minimum_stack_size)
        {
            throw std::length_error("fiber stack needs at least 64 KiB");
        }
    }

    void prepare(context &target, std::span<std::byte> storage, void (*entry)())
    {
        validate(storage);
        std::byte *bottom    = storage.data();
        std::uintptr_t *band = nullptr;

#if FIBER_CONTEXT_ASM_CANARIES
        band = reinterpret_cast<std::uintptr_t *>(bottom);
        bottom += 2 * sizeof(std::uintptr_t);
#endif // if FIBER_CONTEXT_ASM_CANARIES

        // x86_64 reserves eight bytes below the aligned top for the address popped by ret.
        // ARM64 returns through x16, so its initial SP is the aligned top itself.
        const auto end          = reinterpret_cast<std::uintptr_t>(storage.data() + storage.size());
        const unsigned long top = end & ~(alignment - 1);
        const unsigned long sp  = top - initial_stack_reserve;

        initialize_registers(target.registers, entry, reinterpret_cast<void *>(sp), bottom, reinterpret_cast<void *>(top), band);

#if !FIBER_CONTEXT_ASM_CANARIES
        target.stack_low  = bottom;
        target.stack_high = reinterpret_cast<const std::byte *>(top);
#endif // if !FIBER_CONTEXT_ASM_CANARIES

#if FIBER_CONTEXT_RUNTIME_ASAN
        target.physical.bottom = bottom;
        target.physical.size   = top - reinterpret_cast<std::uintptr_t>(bottom);
#endif // if FIBER_CONTEXT_RUNTIME_ASAN
    }

    FIBER_CONTEXT_NO_ASAN void transfer(context &from, context &to, [[maybe_unused]] bool retiring) noexcept
    {
#if FIBER_CONTEXT_RUNTIME_ASAN
        runtime::start(from.physical, to.physical, retiring);
#endif // if FIBER_CONTEXT_RUNTIME_ASAN

        asm_bridge::switch_context(&from.registers, &to.registers);

#if FIBER_CONTEXT_RUNTIME_ASAN

        // The outgoing fiber has resumed on its own stack here.
        runtime::finish(from.physical);

#endif // if FIBER_CONTEXT_RUNTIME_ASAN
    }

} // namespace stdx::native

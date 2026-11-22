//==- canaries.cpp - Native Canary Trap Tests ------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Corrupts one saved-context invariant per child process and checks that the
// fault address identifies the expected assembly trap.
//
//===----------------------------------------------------------------------===//

#include "asm/symbols.h"
#include "native/context.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#ifdef __APPLE__
    #include <sys/ucontext.h>
#else
    #include <ucontext.h>
#endif
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    namespace native = stdx::native;
    namespace bridge = native::asm_bridge;

    using trap                      = void (*)();
    constexpr trap expected_traps[] = {bridge::trap_begin_marker,
        bridge::trap_end_marker,
        bridge::trap_sp_witness,
        bridge::trap_pc_witness,
        bridge::trap_sp_alignment,
        bridge::trap_sp_bounds,
        bridge::trap_stack_band};

    volatile std::sig_atomic_t expected_index{};

    void unexpected_entry() { _exit(2); }

    void handle_fault(int, siginfo_t *, void *machine_state)
    {
        const auto &state = *static_cast<ucontext_t *>(machine_state);
#if defined(__APPLE__) && defined(__aarch64__)
        const auto pc = __darwin_arm_thread_state64_get_pc(state.uc_mcontext->__ss);
#elif defined(__APPLE__)
        const auto pc = state.uc_mcontext->__ss.__rip;
#else
        const auto pc = state.uc_mcontext.gregs[REG_RIP];
#endif
        const auto expected = reinterpret_cast<std::uintptr_t>(expected_traps[expected_index]);
        _exit(static_cast<std::uintptr_t>(pc) == expected ? 0 : 3);
    }

    void trigger(int index)
    {
        expected_index = index;
        struct sigaction action{};
        action.sa_sigaction = handle_fault;
        action.sa_flags     = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGTRAP, &action, nullptr) || sigaction(SIGILL, &action, nullptr))
        {
            _exit(4);
        }

        native::context from;
        native::context to;
        native::stack stack;
        native::prepare(to, stack.data(), unexpected_entry);
        auto &record = to.registers;
        switch (index)
        {
            case 0: record.head ^= 1; break;
            case 1: record.tail ^= 1; break;
            case 2: record.sp_witness ^= 1; break;
            case 3: record.pc_witness ^= 1; break;
            // Keep the SP witness valid to reach the alignment and bounds traps.
            case 4:
                record.sp ^= 1;
                record.sp_witness = record.sp ^ reinterpret_cast<std::uintptr_t>(&record);
                break;
            case 5:
                record.sp         = record.low - 16 + (record.sp % 16);
                record.sp_witness = record.sp ^ reinterpret_cast<std::uintptr_t>(&record);
                break;
            case 6: stack.data().front() ^= std::byte{1}; break;
            default: break;
        }

        bridge::switch_context(&from.registers, &record);
        _exit(5);
    }
} // namespace

int main()
{
    for (int index = 0; index < 7; ++index)
    {
        const auto child = ::fork();
        if (child == 0)
        {
            trigger(index);
        }

        int status{};
        pid_t waited;
        do
        {
            waited = child < 0 ? child : waitpid(child, &status, 0);
        }
        while (child > 0 && waited < 0 && errno == EINTR);

        if (waited < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            std::fprintf(stderr, "Canary trap %d failed (wait status %d)\n", index + 1, status);
            return EXIT_FAILURE;
        }
    }
}

//==- fpcr.cpp - Arm Floating-Point Control Register Access ----------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Reads FPCR on AArch64 or extracts the control bits from FPSCR on AArch32 when
// floating-point register access is available.
//
//===----------------------------------------------------------------------===//

#include "fpcr.h"

#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__clang__) || !defined(__GNUC__)
    #include <arm_acle.h>
#endif

// https://developer.arm.com/documentation/ddi0601/2023-12/AArch64-Registers/FPCR--Floating-point-Control-Register
extern "C" uint64_t fiber_context_arm_read_fpcr() noexcept
{
#if defined(_M_ARM64) || defined(_M_ARM64EC)

    // Packed Windows register selector for FPCR.
    // https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/um/winnt.h#L6291-L6297
    constexpr int fpcr = 0x5a20;
    return static_cast<uint64_t>(_ReadStatusReg(fpcr));

#elif (defined(__aarch64__) || defined(__ARM_64BIT_STATE)) && (defined(__ARM_FP) || defined(__APPLE__) || defined(_WIN32) || defined(__ANDROID__))

    #if defined(__GNUC__) && !defined(__clang__)

    return __builtin_aarch64_get_fpcr64();

    #else

    // ACLE encoding of FPCR in an MRS instruction.
    // https://github.com/llvm/llvm-project/blob/main/llvm/lib/Target/AArch64/AArch64SystemOperands.td#L1301
    return __arm_rsr64("3:3:4:4:0");

    #endif

#elif defined(_M_ARM) || defined(__ARM_FP)

    // Keep FPSCR bits 15 through 26 and 8 through 12.
    // https://developer.arm.com/documentation/ddi0601/2023-12/AArch32-Registers/FPSCR--Floating-Point-Status-and-Control-Register
    constexpr uint32_t fpscr_control_bits = (0xfffu << 15) | (0x1fu << 8);

    #if defined(_M_ARM)

    // Coprocessor 10 encoding for VMRS from FPSCR.
    // https://github.com/google/pthreadpool/blob/main/src/threadpool-utils.h#L47-L56
    const auto fpscr = _MoveFromCoprocessor(10, 7, 1, 0, 0);

    #elif defined(__GNUC__) && !defined(__clang__)

    const auto fpscr = __builtin_arm_get_fpscr();

    #else

    const auto fpscr = __arm_rsr("fpscr");

    #endif

    // Drop condition codes and cumulative status flags.
    // https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst#L965-L974
    return fpscr & fpscr_control_bits;

#elif defined(__ARM_ARCH) || defined(__arm__) || defined(__aarch64__)

    // No floating-point register access for this target configuration.
    // https://arm-software.github.io/acle/main/acle.html#hardware-floating-point
    return 0;

#else

    #error fiber_context_arm_read_fpcr requires an Arm target

#endif
}

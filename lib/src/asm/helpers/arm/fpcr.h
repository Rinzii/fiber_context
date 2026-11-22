//==- fpcr.h - Arm Floating-Point Control Register Interface ---------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declares the C-linkage function that supplies the initial floating-point
// settings for a new context.
//
//===----------------------------------------------------------------------===//

#pragma once

#ifdef __cplusplus
    #include <cstdint>
#else
    #include <stdint.h>
#endif

extern "C" uint64_t fiber_context_arm_read_fpcr() noexcept;

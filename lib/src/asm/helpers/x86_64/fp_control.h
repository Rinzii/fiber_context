//==- fp_control.h - x86-64 Floating-Point Control -------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declares the helper used to initialize a fiber's floating-point controls.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "asm/source.h"

#include <__fiber_context/config.h>
#include <cstdint>

extern "C" FIBER_CONTEXT_ASM_SOURCE("lib/src/asm/helpers/x86_64/fp_control.S", fiber_context_x86_read_fp_control)
    INTERNAL_STDX_FIBER_CONTEXT_HIDDEN void fiber_context_x86_read_fp_control(std::uint32_t *mxcsr, std::uint16_t *x87cw) noexcept;

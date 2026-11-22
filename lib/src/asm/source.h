//==- source.h - Assembly Source Annotations -------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Connects C++ declarations to their target-specific assembly definitions for
// clangd navigation.
//
//===----------------------------------------------------------------------===//

#pragma once

#if defined(__APPLE__) && defined(__aarch64__)
    #define FIBER_CONTEXT_ASM_TARGET(arm64, x86_64) arm64
#elif defined(__x86_64__) && (defined(__APPLE__) || defined(__linux__))
    #define FIBER_CONTEXT_ASM_TARGET(arm64, x86_64) x86_64
#endif

// Points clangd from a C++ declaration to its target-specific assembly definition.
#if defined(FIBER_CONTEXT_ASM_TARGET) && __has_cpp_attribute(clang::external_source_symbol) >= 20230206
    #define FIBER_CONTEXT_ASM_SOURCE(file, label)                                                                                                              \
        [[clang::external_source_symbol(language = "Assembly", defined_in = "" file, USR = "fiber_context:" file ":" #label)]]
#elif defined(FIBER_CONTEXT_ASM_TARGET) && __has_cpp_attribute(clang::external_source_symbol)
    #define FIBER_CONTEXT_ASM_SOURCE(file, label) [[clang::external_source_symbol(language = "Assembly", defined_in = "" file)]]
#else
    #define FIBER_CONTEXT_ASM_SOURCE(file, label)
#endif

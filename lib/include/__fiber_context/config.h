//==- config.h - ABI and Symbol Visibility ---------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the inline ABI namespace and the visibility annotations for public
// and internal symbols.
//
//===----------------------------------------------------------------------===//

#pragma once

#if __has_include(<__fiber_context/abi_namespace.h>)
    #include <__fiber_context/abi_namespace.h>
#endif

#ifdef INTERNAL_CONFIG_STDX_FIBER_CONTEXT_ABI_NAMESPACE
    #define INTERNAL_STDX_FIBER_CONTEXT_ABI_NAMESPACE INTERNAL_CONFIG_STDX_FIBER_CONTEXT_ABI_NAMESPACE
    #define STDX_FIBER_CONTEXT_NAMESPACE namespace stdx::inline INTERNAL_STDX_FIBER_CONTEXT_ABI_NAMESPACE
#else
    #define STDX_FIBER_CONTEXT_NAMESPACE namespace stdx
#endif

#if defined(_WIN32) && defined(FIBER_CONTEXT_SHARED)
    #if defined(FIBER_CONTEXT_BUILDING_LIBRARY)
        #define INTERNAL_STDX_FIBER_CONTEXT_API __declspec(dllexport)
    #else
        #define INTERNAL_STDX_FIBER_CONTEXT_API __declspec(dllimport)
    #endif
#elif !defined(_WIN32) && defined(__has_attribute)
    #if __has_attribute(__visibility__)
        #define INTERNAL_STDX_FIBER_CONTEXT_API __attribute__((__visibility__("default")))
        #define INTERNAL_STDX_FIBER_CONTEXT_HIDDEN __attribute__((__visibility__("hidden")))
    #endif
#endif

#ifndef INTERNAL_STDX_FIBER_CONTEXT_API
    #define INTERNAL_STDX_FIBER_CONTEXT_API
#endif

#ifndef INTERNAL_STDX_FIBER_CONTEXT_HIDDEN
    #define INTERNAL_STDX_FIBER_CONTEXT_HIDDEN
#endif

#==- AsmNamespace.cmake - Assembly Bridge Namespace -------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Configures the inline namespace shared by the public headers and assembly labels.
#
#===------------------------------------------------------------------------===#

set(FIBER_CONTEXT_ABI_NAMESPACE "v1" CACHE STRING "C++ inline namespace for the fiber_context ABI")
if(NOT FIBER_CONTEXT_ABI_NAMESPACE MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
    message(FATAL_ERROR "FIBER_CONTEXT_ABI_NAMESPACE must be a single C++ identifier. Got '${FIBER_CONTEXT_ABI_NAMESPACE}'.")
endif()
set(FIBER_CONTEXT_ASM_SCOPE "stdx::${FIBER_CONTEXT_ABI_NAMESPACE}::native::asm_bridge::")

function(fiber_context_configure_abi_namespace)
    include(CheckCXXSourceCompiles)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    set(check "FIBER_CONTEXT_VALID_ABI_NAMESPACE_${FIBER_CONTEXT_ABI_NAMESPACE}")
    check_cxx_source_compiles("
        #ifdef ${FIBER_CONTEXT_ABI_NAMESPACE}
        #error ABI namespace expands to a macro
        #endif
        namespace stdx::inline ${FIBER_CONTEXT_ABI_NAMESPACE} {
            class fiber_context;
            namespace native {}
            namespace detail {}
            namespace runtime {}
        }
        using stdx::fiber_context;
        namespace native = stdx::native;
        namespace detail = stdx::detail;
        namespace runtime = stdx::runtime;
    " ${check})
    if(NOT ${check})
        message(FATAL_ERROR "Cannot use '${FIBER_CONTEXT_ABI_NAMESPACE}' as the fiber_context C++ inline namespace.")
    endif()

    set(include_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
    set(header "${include_dir}/__fiber_context/abi_namespace.h")
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../lib/include/__fiber_context/abi_namespace.h.in" "${header}" @ONLY)
    set(FIBER_CONTEXT_GENERATED_INCLUDE_DIR "${include_dir}" PARENT_SCOPE)
    set(FIBER_CONTEXT_ABI_HEADER "${header}" PARENT_SCOPE)
endfunction()

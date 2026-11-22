#==- CMakeLists.txt - Public Header Compile Checks --------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Compiles the same consumer with static-library and DLL import and export
# settings.
#
#===------------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.23)
project(fiber_context_header_tests LANGUAGES CXX)

# Discard the old default include path only when it no longer exists.
if(FIBER_CONTEXT_TEST_INCLUDE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/../../include" AND NOT IS_DIRECTORY "${FIBER_CONTEXT_TEST_INCLUDE_DIR}")
    unset(FIBER_CONTEXT_TEST_INCLUDE_DIR CACHE)
endif()
set(FIBER_CONTEXT_TEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/include" CACHE PATH "Public fiber_context include directory")

file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/include" source_include_dir)
file(REAL_PATH "${FIBER_CONTEXT_TEST_INCLUDE_DIR}" test_include_dir)
set(header_test_includes "${test_include_dir}")
if(test_include_dir STREQUAL source_include_dir)
    if(NOT DEFINED FIBER_CONTEXT_GENERATED_INCLUDE_DIR)
        include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/AsmNamespace.cmake")
        fiber_context_configure_abi_namespace()
    endif()
    list(APPEND header_test_includes "${FIBER_CONTEXT_GENERATED_INCLUDE_DIR}")

    add_library(header_plain_namespace OBJECT header_tests.cpp)
    target_include_directories(header_plain_namespace PRIVATE "${source_include_dir}")
    target_compile_definitions(header_plain_namespace PRIVATE FIBER_CONTEXT_TEST_PLAIN_NAMESPACE=1)
    target_compile_features(header_plain_namespace PRIVATE cxx_std_20)
endif()

foreach(linkage IN ITEMS static dll_producer dll_consumer)
    add_library(header_${linkage} OBJECT header_tests.cpp)
    target_include_directories(header_${linkage} PRIVATE ${header_test_includes})
    target_compile_features(header_${linkage} PRIVATE cxx_std_20)
endforeach()

target_compile_definitions(header_dll_producer PRIVATE FIBER_CONTEXT_SHARED=1 FIBER_CONTEXT_BUILDING_LIBRARY=1)
target_compile_definitions(header_dll_consumer PRIVATE FIBER_CONTEXT_SHARED=1)

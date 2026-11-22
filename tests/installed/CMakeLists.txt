#==- CMakeLists.txt - Installed Package Consumer ----------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Builds the ping-pong example against the installed CMake package and registers
# it as a consumer test.
#
#===------------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.23)
project(fiber_context_consumer LANGUAGES CXX)

find_package(fiber_context CONFIG REQUIRED)
add_executable(installed_consumer ../../examples/ping_pong.cpp)
target_link_libraries(installed_consumer PRIVATE fiber_context::fiber_context)

enable_testing()
add_test(NAME installed_consumer COMMAND installed_consumer)

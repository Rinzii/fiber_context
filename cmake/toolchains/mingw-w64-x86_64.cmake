#==- mingw-w64-x86_64.cmake - MinGW-w64 Cross Toolchain ---------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Selects the MinGW-w64 C++ compiler and assembler driver for an x86-64 Windows
# target.
#
#===------------------------------------------------------------------------===#

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_ASM_COMPILER x86_64-w64-mingw32-gcc)

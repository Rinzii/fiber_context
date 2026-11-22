#==- compare-asm-symbols.cmake - Assembly Label Comparison ------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Regenerates labels with a second symbol reader and compares the output byte
# for byte with the reference header.
#
#===------------------------------------------------------------------------===#

cmake_minimum_required(VERSION 4.0)

include("${GENERATOR}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${REFERENCE}" "${OUTPUT}"
    RESULT_VARIABLE different)

if(different)
    message(FATAL_ERROR "Assembly labels from ${READER} differ from ${REFERENCE}. See ${OUTPUT}.")
endif()

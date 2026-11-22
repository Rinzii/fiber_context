#==- expect-asan.cmake - ASan Diagnostic Check ------------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Requires AddressSanitizer to identify a read from a fiber stack frame after
# its function has returned.
#
#===------------------------------------------------------------------------===#

set(ENV{ASAN_OPTIONS} "detect_stack_use_after_return=1:symbolize=1")
execute_process(COMMAND ${EMULATOR} "${PROGRAM}" --asan-use-after-return
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)

if("${status}" STREQUAL "0" OR
   NOT diagnostic MATCHES "AddressSanitizer: stack-use-after-return" OR
   NOT diagnostic MATCHES "read_returned_fiber_local")
    message(FATAL_ERROR "Expected ASan to identify the returned fiber local.\n${output}\n${diagnostic}")
endif()

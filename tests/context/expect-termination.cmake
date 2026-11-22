#==- expect-termination.cmake - Termination Exit Checks ---------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Runs each termination scenario in a separate process and checks the exit code
# set by its terminate handler.
#
#===------------------------------------------------------------------------===#

foreach(scenario IN ITEMS destroy_live assign_live self_move_live empty_successor throw_from_entry)
    execute_process(COMMAND ${EMULATOR} "${PROGRAM}" "${scenario}" RESULT_VARIABLE status)
    if(NOT status STREQUAL "113")
        message(FATAL_ERROR "${scenario}: expected std::terminate to exit with 113, got ${status}")
    endif()
endforeach()

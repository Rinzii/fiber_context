#==- SelectAsmSymbols.cmake - Assembly Symbol Tool Selection ----------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Chooses the C++ symbol ABI and an object-file reader that can produce labels
# for the target assembler.
#
#===------------------------------------------------------------------------===#

set(FIBER_CONTEXT_ASM_ABI AUTO CACHE STRING "C++ ABI used to generate assembly labels")
set_property(CACHE FIBER_CONTEXT_ASM_ABI PROPERTY STRINGS AUTO ITANIUM MSVC)
string(TOUPPER "${FIBER_CONTEXT_ASM_ABI}" FIBER_CONTEXT_SYMBOL_ABI)

if(FIBER_CONTEXT_SYMBOL_ABI STREQUAL "AUTO")

    if(MINGW OR CMAKE_CXX_COMPILER_TARGET MATCHES "-(windows-gnu|w64-mingw32)")
        set(FIBER_CONTEXT_SYMBOL_ABI ITANIUM)
    elseif(MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_TARGET MATCHES "-windows-msvc")
        set(FIBER_CONTEXT_SYMBOL_ABI MSVC)
    elseif(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(FIBER_CONTEXT_SYMBOL_ABI ITANIUM)
    else()
        set(FIBER_CONTEXT_SYMBOL_ABI UNKNOWN)
    endif()

elseif(NOT FIBER_CONTEXT_SYMBOL_ABI STREQUAL "ITANIUM" AND NOT FIBER_CONTEXT_SYMBOL_ABI STREQUAL "MSVC")

    message(FATAL_ERROR "Invalid FIBER_CONTEXT_ASM_ABI='${FIBER_CONTEXT_ASM_ABI}'. Select AUTO, ITANIUM or MSVC.")

endif()

function(fiber_context_select_symbol_reader)

    if(FIBER_CONTEXT_SYMBOL_ABI STREQUAL "UNKNOWN")
        message(FATAL_ERROR "Unknown C++ ABI for '${CMAKE_SYSTEM_NAME}' target '${CMAKE_CXX_COMPILER_TARGET}'. Set FIBER_CONTEXT_ASM_ABI to ITANIUM or MSVC.")
    endif()

    set(FIBER_CONTEXT_SYMBOL_READER "" CACHE STRING "Assembly symbol reader executable or program name (empty for automatic selection)")
    set(symbol_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>")

    if(FIBER_CONTEXT_SYMBOL_ABI STREQUAL "MSVC")
        set(dialect MASM)
        set(FIBER_CONTEXT_LLVM_NM "" CACHE FILEPATH "LLVM nm executable for MSVC assembly labels")
        set(candidates "${CMAKE_NM}" llvm-nm dumpbin "${CMAKE_OBJDUMP}" llvm-objdump)
    else()
        set(dialect GNU)
        set(candidates "${CMAKE_NM}" "${CMAKE_OBJDUMP}" "${CMAKE_READELF}" llvm-nm llvm-objdump llvm-readelf nm objdump readelf)
    endif()

    if(FIBER_CONTEXT_SYMBOL_READER)
        set(candidates "${FIBER_CONTEXT_SYMBOL_READER}")
    elseif(dialect STREQUAL "MASM" AND FIBER_CONTEXT_LLVM_NM)
        set(candidates "${FIBER_CONTEXT_LLVM_NM}")
    endif()

    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_dir)
    list(REMOVE_DUPLICATES candidates)
    set(CMAKE_FIND_REQUIRED OFF)

    foreach(candidate IN LISTS candidates)
        if(NOT candidate)
            continue()
        endif()

        set(reader reader-NOTFOUND)

        # The reader runs on the host and must understand the target object format.
        find_program(reader NAMES "${candidate}" HINTS "${compiler_dir}" NO_CACHE NO_CMAKE_FIND_ROOT_PATH)

        if(NOT reader)
            continue()
        endif()

        cmake_path(GET reader FILENAME name)
        string(TOLOWER "${name}" name)

        if(name MATCHES "(^|-)g?(nm|objdump|readelf)(-[0-9.]+)?([.]exe)?$")
            string(TOUPPER "${CMAKE_MATCH_2}" kind)
        elseif(name MATCHES "^dumpbin([.]exe)?$")
            set(kind DUMPBIN)
        elseif(candidate STREQUAL CMAKE_NM OR candidate STREQUAL FIBER_CONTEXT_LLVM_NM)
            set(kind NM)
        elseif(candidate STREQUAL CMAKE_OBJDUMP)
            set(kind OBJDUMP)
        elseif(candidate STREQUAL CMAKE_READELF)
            set(kind READELF)
        else()
            continue()
        endif()

        if(kind STREQUAL "READELF" AND (dialect STREQUAL "MASM" OR APPLE OR WIN32))
            continue()
        elseif(kind STREQUAL "DUMPBIN" AND NOT dialect STREQUAL "MASM")
            continue()
        elseif(dialect STREQUAL "MASM" AND NOT kind STREQUAL "DUMPBIN")
            execute_process(COMMAND "${reader}" --version RESULT_VARIABLE status OUTPUT_VARIABLE version ERROR_QUIET)
            if(NOT status STREQUAL "0" OR NOT version MATCHES "LLVM|llvm-nm")
                continue()
            endif()
        endif()

        message(STATUS "Assembly symbol reader: ${reader} (${kind})")
        set(FIBER_CONTEXT_SYMBOL_DIR "${symbol_dir}" PARENT_SCOPE)
        set(FIBER_CONTEXT_SYMBOL_HEADER "${symbol_dir}/fiber_context_asm_symbols.h" PARENT_SCOPE)
        set(FIBER_CONTEXT_SYMBOL_DIALECT "${dialect}" PARENT_SCOPE)
        set(FIBER_CONTEXT_SYMBOL_READER_EXECUTABLE "${reader}" PARENT_SCOPE)
        set(FIBER_CONTEXT_SYMBOL_READER_KIND "${kind}" PARENT_SCOPE)
        return()

    endforeach()

    message(FATAL_ERROR "No compatible assembly symbol reader for ${FIBER_CONTEXT_SYMBOL_ABI} on ${CMAKE_SYSTEM_NAME}. "
        "Tried: ${candidates}. Set FIBER_CONTEXT_SYMBOL_READER to a target-compatible executable.")

endfunction()

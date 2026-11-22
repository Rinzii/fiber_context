#==- GenerateAsmSymbols.cmake - Assembly Label Generation -------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Pairs each bridge declaration with its spelling in the target object file and
# writes the aliases used by the assembler.
#
#===------------------------------------------------------------------------===#

cmake_minimum_required(VERSION 4.0)

include("${CMAKE_CURRENT_LIST_DIR}/AsmNamespace.cmake")

function(read_nm_symbols result)

    # Preserve symbol order to pair raw labels with demangled names.
    execute_process(COMMAND "${READER}" -u -p ${ARGN} "${OBJECT}"
        OUTPUT_VARIABLE listing OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE status ERROR_VARIABLE diagnostic ERROR_STRIP_TRAILING_WHITESPACE)

    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "Symbol reader '${READER}' failed on '${OBJECT}' (${status}). "
            "Set FIBER_CONTEXT_SYMBOL_READER to a target-compatible executable.\n${diagnostic}")
    endif()

    string(REPLACE "\r\n" "\n" listing "${listing}")

    string(REGEX REPLACE "(^|\n)[ \t]*[UwWvV][ \t]+" "\\1" listing "${listing}")
    string(REPLACE "\n" ";" listing "${listing}")
    set(${result} "${listing}" PARENT_SCOPE)
endfunction()

if(NOT READER AND NM)
    set(READER "${NM}")
    set(READER_KIND NM)
endif()

if(READER_KIND STREQUAL "NM")
    read_nm_symbols(raw_symbols)
    read_nm_symbols(demangled_symbols -C)
elseif(READER_KIND STREQUAL "OBJDUMP" OR READER_KIND STREQUAL "READELF")
    include("${CMAKE_CURRENT_LIST_DIR}/ReadObjectSymbols.cmake")
    fiber_context_read_object_symbols(raw_symbols demangled_symbols)
elseif(READER_KIND STREQUAL "DUMPBIN")
    include("${CMAKE_CURRENT_LIST_DIR}/ReadDumpbinSymbols.cmake")
    fiber_context_read_dumpbin_symbols(raw_symbols demangled_symbols)
else()
    message(FATAL_ERROR "Unknown assembly symbol reader kind '${READER_KIND}' for '${READER}'.")
endif()

list(LENGTH raw_symbols raw_count)
list(LENGTH demangled_symbols demangled_count)

if(NOT raw_count EQUAL demangled_count)
    message(FATAL_ERROR "${READER} reported inconsistent symbol counts for ${OBJECT}")
endif()

set(signatures
    "switch_context(void*, void*)"
    "x86_bootstrap()"
    "trap_begin_marker()"
    "trap_end_marker()"
    "trap_sp_witness()"
    "trap_pc_witness()"
    "trap_sp_alignment()"
    "trap_sp_bounds()"
    "trap_stack_band()")

if(ASM_DIALECT STREQUAL "MASM")
    set(label_pattern "^[?][A-Za-z0-9_?$@]+$")
else()
    set(label_pattern "^[A-Za-z_.$][A-Za-z0-9_.$]*$")
endif()

foreach(raw demangled IN ZIP_LISTS raw_symbols demangled_symbols)
    if(ASM_DIALECT STREQUAL "MASM")
        if(NOT raw MATCHES "^[?][^@]+@asm_bridge@native@${FIBER_CONTEXT_ABI_NAMESPACE}@stdx@@")
            continue()
        endif()

        if(NOT demangled MATCHES "^void __cdecl (${FIBER_CONTEXT_ASM_SCOPE}.*)$")
            message(FATAL_ERROR "Unexpected assembly declaration '${demangled}' in ${OBJECT}")
        endif()

        set(demangled "${CMAKE_MATCH_1}")
        string(REPLACE "(void)" "()" demangled "${demangled}")
        string(REPLACE "void *" "void*" demangled "${demangled}")
    endif()

    if(NOT demangled MATCHES "^${FIBER_CONTEXT_ASM_SCOPE}(([a-zA-Z0-9_]+)\\(.*\\))$")
        continue()
    endif()

    set(signature "${CMAKE_MATCH_1}")
    set(name "${CMAKE_MATCH_2}")

    # The assembly alias uses only the function name so overloads would collide.
    if(NOT signature IN_LIST signatures OR DEFINED symbol_${name})
        message(FATAL_ERROR "Unexpected or ambiguous assembly declaration '${demangled}' in ${OBJECT}")
    endif()

    if(NOT raw MATCHES "${label_pattern}")
        message(FATAL_ERROR "Unusable assembly label '${raw}' for '${demangled}'")
    endif()

    set(symbol_${name} "${raw}")
endforeach()

if(ASM_DIALECT STREQUAL "MASM")
    set(comment ";")
    set(opener ";==- fiber_context_asm_symbols.h - MASM Labels ------------------------------==;")
    set(closer ";===------------------------------------------------------------------------===;")
else()
    set(comment "//")
    set(opener "//==- fiber_context_asm_symbols.h - GNU Assembly Labels --------------------==//")
    set(closer "//===----------------------------------------------------------------------===//")
endif()

string(CONCAT header
    "${opener}\n${comment}\n"
    "${comment} Copyright (c) 2026 Ian Pike\n"
    "${comment} Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.\n"
    "${comment} See LICENSE in the project root for license information.\n"
    "${comment} SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception\n"
    "${comment}\n${closer}\n${comment}\n"
    "${comment} Maps assembly labels to linker names read from the target C++ compiler's\n"
    "${comment} object file.\n"
    "${comment}\n${closer}\n\n")

if(NOT ASM_DIALECT STREQUAL "MASM")
    string(APPEND header "#pragma once\n")
endif()

foreach(signature IN LISTS signatures)
    string(REGEX MATCH "^[a-zA-Z0-9_]+" name "${signature}")
    if(NOT DEFINED symbol_${name})
        message(FATAL_ERROR "${READER} could not find '${FIBER_CONTEXT_ASM_SCOPE}${signature}' in ${OBJECT}")
    endif()

    string(TOUPPER "${name}" macro)
    if(ASM_DIALECT STREQUAL "MASM")
        string(APPEND header "FIBER_CONTEXT_SYM_${macro} TEXTEQU <${symbol_${name}}>\n")
    else()
        string(APPEND header "#define FIBER_CONTEXT_SYM_${macro} ${symbol_${name}}\n")
    endif()

endforeach()

# Expand the header once so CMake preserves the at signs in MSVC symbols.
file(CONFIGURE OUTPUT "${OUTPUT}" CONTENT "@header@" @ONLY NEWLINE_STYLE UNIX)

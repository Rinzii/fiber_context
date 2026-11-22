#==- ReadDumpbinSymbols.cmake - COFF Symbol Reader --------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Extracts undefined externals from dumpbin output and pairs their decorated
# names with their C++ declarations.
#
#===------------------------------------------------------------------------===#

function(fiber_context_read_dumpbin_symbols raw_result demangled_result)
    if(NOT ASM_DIALECT STREQUAL "MASM")
        message(FATAL_ERROR "DUMPBIN /SYMBOLS requires the MSVC assembly ABI.")
    endif()

    execute_process(COMMAND "${READER}" /NOLOGO /SYMBOLS "${OBJECT}"
        RESULT_VARIABLE status OUTPUT_VARIABLE listing ERROR_VARIABLE diagnostic)

    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "'${READER}' /SYMBOLS failed on '${OBJECT}' (${status}).\n${diagnostic}${listing}")
    endif()

    set(raw_symbols "")
    set(demangled_symbols "")
    string(REGEX MATCHALL
            "(^|[\r\n])[ \t]*[0-9a-fA-F]+[ \t]+0+[ \t]+UNDEF[ \t]+[^|\r\n]*[ \t]+(External|WeakExternal)[ \t]*[|][ \t]+[^\r\n]+"
            entries
            "${listing}"
    )

    foreach(entry IN LISTS entries)
        string(REGEX MATCH "[|][ \t]+([^ \t]+)(.*)$" symbol "${entry}")
        set(raw "${CMAKE_MATCH_1}")
        string(STRIP "${CMAKE_MATCH_2}" declaration)

        if(declaration MATCHES "^[(](.+)[)]$")
            set(declaration "${CMAKE_MATCH_1}")
            string(REGEX REPLACE ",[ \t]*" ", " declaration "${declaration}")

            # Only the default pointer width is implicit in the expected C++ signature.
            if(POINTER_SIZE STREQUAL "8")
                string(REPLACE "* __ptr64" "*" declaration "${declaration}")
            elseif(POINTER_SIZE STREQUAL "4")
                string(REPLACE "* __ptr32" "*" declaration "${declaration}")
            endif()

        else()
            set(declaration "${raw}")
        endif()

        list(APPEND raw_symbols "${raw}")
        list(APPEND demangled_symbols "${declaration}")

    endforeach()

    set(${raw_result} "${raw_symbols}" PARENT_SCOPE)
    set(${demangled_result} "${demangled_symbols}" PARENT_SCOPE)
endfunction()

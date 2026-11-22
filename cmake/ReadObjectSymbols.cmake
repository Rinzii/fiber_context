#==- ReadObjectSymbols.cmake - Object Symbol Readers ------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Extracts undefined symbols with objdump or readelf and obtains their demangled
# names for signature matching.
#
#===------------------------------------------------------------------------===#

function(fiber_context_find_reader_demangler result)
    cmake_path(GET READER PARENT_PATH reader_dir)
    cmake_path(GET READER FILENAME reader_name)
    string(REPLACE "llvm-objdump" "llvm-cxxfilt" sibling "${reader_name}")
    string(REPLACE "objdump" "c++filt" sibling "${sibling}")
    set(names llvm-cxxfilt c++filt)

    if(NOT sibling STREQUAL reader_name)
        list(PREPEND names "${sibling}")
    endif()

    find_program(demangler NAMES ${names} PATHS "${reader_dir}" NO_DEFAULT_PATH NO_CACHE NO_CMAKE_FIND_ROOT_PATH)
    find_program(demangler NAMES ${names} NO_CACHE NO_CMAKE_FIND_ROOT_PATH REQUIRED)
    set(${result} "${demangler}" PARENT_SCOPE)
endfunction()

function(fiber_context_read_object_symbols raw_result demangled_result)
    if(READER_KIND STREQUAL "READELF")
        set(options --syms --wide)
        set(row_prefix "[ \t]*[0-9]+:[^\n]*[ \t]UND[ \t]+")
    elseif(READER_KIND STREQUAL "OBJDUMP")
        set(options --syms)
        set(elf_columns "[0-9A-Fa-f]+[ \t]+(\\.(hidden|protected|internal)[ \t]+)?")
        set(undefined "[0-9A-Fa-f]+[^\n]*[ \t]\\*UND\\*[ \t]+(${elf_columns})?")
        set(coff "\\[[ \t]*[0-9]+\\]\\(sec[ \t]+0\\)[^\n]*\\(scl[ \t]+(2|105)\\)[ \t]*\\(nx[ \t]+[0-9]+\\)[ \t]+0x0+[ \t]+")
        set(row_prefix "(${undefined}|${coff})")
    else()
        message(FATAL_ERROR "Unknown object symbol reader kind '${READER_KIND}'")
    endif()

    foreach(form IN ITEMS raw demangled)
        if(form STREQUAL "demangled")
            list(APPEND options --demangle)
        endif()

        execute_process(COMMAND "${READER}" ${options} "${OBJECT}"
            OUTPUT_VARIABLE listing RESULT_VARIABLE status ERROR_VARIABLE diagnostic
            ERROR_STRIP_TRAILING_WHITESPACE)

        if(NOT status STREQUAL "0")
            message(FATAL_ERROR "Symbol reader '${READER}' failed on '${OBJECT}' (${status}): ${diagnostic}")
        endif()

        string(REGEX MATCHALL "\n${row_prefix}[^ \t\r\n][^\r\n]*" rows "\n${listing}")
        set(symbols "")

        foreach(row IN LISTS rows)
            string(REGEX REPLACE "^\n${row_prefix}" "" symbol "${row}")

            if(form STREQUAL "demangled" AND ASM_DIALECT STREQUAL "MASM" AND symbol MATCHES "^[?][^ \t]+[ \t]+\\((.+)\\)$")
                set(symbol "${CMAKE_MATCH_1}")
            elseif(form STREQUAL "demangled" AND ASM_DIALECT STREQUAL "GNU" AND symbol MATCHES "^_?(_Z[^ \t]+)$")

                # Strip the optional leading underscore before demangling the Itanium name.
                set(mangled "${CMAKE_MATCH_1}")

                if(NOT cxxfilt)
                    fiber_context_find_reader_demangler(cxxfilt)
                endif()

                execute_process(COMMAND "${cxxfilt}" -n "${mangled}"
                    OUTPUT_VARIABLE symbol OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
            endif()

            list(APPEND symbols "${symbol}")
        endforeach()
        set(${${form}_result} "${symbols}" PARENT_SCOPE)

    endforeach()
endfunction()

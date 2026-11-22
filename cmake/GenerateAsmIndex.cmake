#==- GenerateAsmIndex.cmake - Clangd Assembly Index -------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Maps bridge functions to label locations in the assembly sources so clangd can
# navigate to their definitions.
#
#===------------------------------------------------------------------------===#

function(fiber_context_asm_file_uri path result)
    if(path MATCHES [=[^([A-Za-z]:[/\\]|\\\\)]=])
        string(REPLACE "\\" "/" path "${path}")
    endif()
    if(path MATCHES "^//[^/]")
        set(uri "file:")
    elseif(path MATCHES "^[A-Za-z]:/")
        set(uri "file:///")
    else()
        set(uri "file://")
    endif()
    set(unreserved "/:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~")
    string(LENGTH "${path}" length)
    math(EXPR last "${length} - 1")
    foreach(offset RANGE 0 ${last})
        string(SUBSTRING "${path}" ${offset} 1 byte)
        string(FIND "${unreserved}" "${byte}" allowed)
        if(allowed EQUAL -1)
            string(HEX "${byte}" hex)
            string(TOUPPER "${hex}" hex)
            string(APPEND uri "%${hex}")
        else()
            string(APPEND uri "${byte}")
        endif()
    endforeach()
    set(${result} "${uri}" PARENT_SCOPE)
endfunction()

function(fiber_context_generate_asm_index output)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${output}")

    # All build trees write to the index named in .clangd.
    file(LOCK "${output}.lock" GUARD FUNCTION)
    cmake_path(GET CMAKE_CURRENT_FUNCTION_LIST_DIR PARENT_PATH root)
    string(CONCAT index
        "#==- asm.idx - clangd Assembly Index ----------------------------------------==#\n#\n"
        "# Copyright (c) 2026 Ian Pike\n"
        "# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.\n"
        "# See LICENSE in the project root for license information.\n"
        "# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception\n"
        "#\n#===------------------------------------------------------------------------===#\n#\n"
        "# Maps clangd symbol IDs to their defining labels in the assembly sources.\n"
        "#\n#===------------------------------------------------------------------------===#\n\n")
    list(REMOVE_DUPLICATES ARGN)
    list(SORT ARGN)

    foreach(source IN LISTS ARGN)
        set(path "${root}/${source}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
        fiber_context_asm_file_uri("${path}" uri)
        file(READ "${path}" remaining)
        string(APPEND remaining "\n")
        set(line_number -1)

        while(NOT remaining STREQUAL "")
            string(FIND "${remaining}" "\n" newline)
            string(SUBSTRING "${remaining}" 0 ${newline} line)
            math(EXPR next "${newline} + 1")
            string(SUBSTRING "${remaining}" ${next} -1 remaining)
            math(EXPR line_number "${line_number} + 1")

            if(line MATCHES "^[ \t]*(FIBER_CONTEXT_SYM_[A-Z0-9_]+|TEST_PRESERVED_REGISTERS|_?fiber_context_test_preserved_registers)[ \t]*:")
                set(label "${CMAKE_MATCH_1}")
            elseif(line MATCHES "^[ \t]*C_SYMBOL\\((fiber_context_[a-z0-9_]+)\\)[ \t]*:")
                set(label "${CMAKE_MATCH_1}")
            elseif(line MATCHES "^[ \t]*(record_trap|trap)[ \t]+[A-Za-z0-9_]+[ \t]*,[ \t]*(FIBER_CONTEXT_SYM_[A-Z0-9_]+)[ \t]*,")
                set(label "${CMAKE_MATCH_2}")
            else()
                continue()
            endif()

            if(label MATCHES "^FIBER_CONTEXT_SYM_([A-Z0-9_]+)$")
                string(TOLOWER "${CMAKE_MATCH_1}" name)
                set(scope "stdx::native::asm_bridge::")
                set(key "${label}")
            else()
                if(label STREQUAL "TEST_PRESERVED_REGISTERS")
                    set(name "fiber_context_test_preserved_registers")
                else()
                    string(REGEX REPLACE "^_" "" name "${label}")
                endif()
                set(scope "")
                set(key "${name}")
            endif()

            # Hash the same symbol identifier that the C++ declaration gives clangd.
            # https://github.com/llvm/llvm-project/blob/llvmorg-21.1.0/clang-tools-extra/clangd/index/SymbolID.cpp#L16-L20
            string(SHA1 digest "fiber_context:${source}:${key}")

            # https://github.com/llvm/llvm-project/blob/llvmorg-21.1.0/clang-tools-extra/clangd/index/SymbolID.h#L47-L49
            string(SUBSTRING "${digest}" 0 16 id)
            string(TOUPPER "${id}" id)
            string(FIND "${line}" "${label}" column)
            string(LENGTH "${label}" width)
            math(EXPR end_column "${column} + ${width}")
            string(APPEND index
                "--- !Symbol\nID: '${id}'\nName: '${name}'\nScope: '${scope}'\n"
                "SymInfo: {Kind: Function, Lang: Cpp}\nDefinition:\n  FileURI: '${uri}'\n"
                "  Start: {Line: ${line_number}, Column: ${column}}\n"
                "  End: {Line: ${line_number}, Column: ${end_column}}\n")

        endwhile()

    endforeach()
    file(CONFIGURE OUTPUT "${output}" CONTENT "${index}" @ONLY NEWLINE_STYLE UNIX)
endfunction()

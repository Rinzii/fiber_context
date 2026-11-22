#==- CMakeLists.txt - Symbol Reader Test Registration -----------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Registers comparisons between the configured assembly symbol reader and other
# readers available on the host.
#
#===------------------------------------------------------------------------===#

if(CMAKE_CROSSCOMPILING OR NOT FIBER_CONTEXT_SYMBOL_DIALECT STREQUAL "GNU")
    return()
endif()

foreach(kind IN ITEMS NM OBJDUMP READELF)
    if(kind STREQUAL FIBER_CONTEXT_SYMBOL_READER_KIND OR NOT EXISTS "${CMAKE_${kind}}" OR CMAKE_${kind} STREQUAL FIBER_CONTEXT_SYMBOL_READER_EXECUTABLE)
        continue()
    endif()
    if(kind STREQUAL "READELF" AND NOT CMAKE_EXECUTABLE_FORMAT STREQUAL "ELF")
        continue()
    endif()

    string(TOLOWER "${kind}" name)
    add_test(NAME asm_symbols_${name} COMMAND "${CMAKE_COMMAND}"
        "-DREADER=${CMAKE_${kind}}" "-DREADER_KIND=${kind}"
        "-DFIBER_CONTEXT_ABI_NAMESPACE=${FIBER_CONTEXT_ABI_NAMESPACE}"
        "-DASM_DIALECT=${FIBER_CONTEXT_SYMBOL_DIALECT}" "-DPOINTER_SIZE=${CMAKE_SIZEOF_VOID_P}"
        "-DOBJECT=$<TARGET_OBJECTS:fiber_context_symbol_probe>" "-DREFERENCE=${FIBER_CONTEXT_SYMBOL_HEADER}"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.h"
        "-DGENERATOR=${FIBER_CONTEXT_SYMBOL_SCRIPT}" -P "${CMAKE_CURRENT_SOURCE_DIR}/compare-asm-symbols.cmake")
endforeach()

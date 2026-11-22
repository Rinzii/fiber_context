#==- AsmSymbols.cmake - Assembly Label Build Rules --------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Compiles the symbol probe and regenerates the assembler include when the probe
# or its inputs change.
#
#===------------------------------------------------------------------------===#

fiber_context_select_symbol_reader()

add_library(fiber_context_symbol_probe OBJECT "${CMAKE_CURRENT_LIST_DIR}/asm_symbol_probe.cpp")
target_compile_features(fiber_context_symbol_probe PRIVATE cxx_std_20)
target_compile_definitions(fiber_context_symbol_probe PRIVATE FIBER_CONTEXT_ASM_CANARIES=$<BOOL:${FIBER_CONTEXT_ASM_CANARIES}>)
target_include_directories(fiber_context_symbol_probe PRIVATE
    "${FIBER_CONTEXT_GENERATED_INCLUDE_DIR}" "${CMAKE_CURRENT_LIST_DIR}/../lib/src" "${CMAKE_CURRENT_LIST_DIR}/../lib/include")

# Keep the probe readable by symbol tools without an LTO plugin.
set_target_properties(fiber_context_symbol_probe PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    target_compile_options(fiber_context_symbol_probe PRIVATE -fno-lto)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(fiber_context_symbol_probe PRIVATE /GL-)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(fiber_context_symbol_probe PRIVATE /clang:-fno-lto)
endif()

set(FIBER_CONTEXT_SYMBOL_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/GenerateAsmSymbols.cmake")
add_custom_command(OUTPUT "${FIBER_CONTEXT_SYMBOL_HEADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${FIBER_CONTEXT_SYMBOL_DIR}"
    COMMAND "${CMAKE_COMMAND}" "-DREADER=${FIBER_CONTEXT_SYMBOL_READER_EXECUTABLE}" "-DREADER_KIND=${FIBER_CONTEXT_SYMBOL_READER_KIND}"
            "-DFIBER_CONTEXT_ABI_NAMESPACE=${FIBER_CONTEXT_ABI_NAMESPACE}"
            "-DASM_DIALECT=${FIBER_CONTEXT_SYMBOL_DIALECT}" "-DPOINTER_SIZE=${CMAKE_SIZEOF_VOID_P}"
            "-DOBJECT=$<TARGET_OBJECTS:fiber_context_symbol_probe>" "-DOUTPUT=${FIBER_CONTEXT_SYMBOL_HEADER}"
            -P "${FIBER_CONTEXT_SYMBOL_SCRIPT}"

    # Refresh the timestamp even when the rebuilt probe yields identical labels.
    COMMAND "${CMAKE_COMMAND}" -E touch_nocreate "${FIBER_CONTEXT_SYMBOL_HEADER}"
    DEPENDS fiber_context_symbol_probe $<TARGET_OBJECTS:fiber_context_symbol_probe> "${FIBER_CONTEXT_SYMBOL_SCRIPT}" "${FIBER_CONTEXT_SYMBOL_READER_EXECUTABLE}"
            "${CMAKE_CURRENT_LIST_DIR}/AsmNamespace.cmake" "${FIBER_CONTEXT_ABI_HEADER}"
            "${CMAKE_CURRENT_LIST_DIR}/ReadObjectSymbols.cmake" "${CMAKE_CURRENT_LIST_DIR}/ReadDumpbinSymbols.cmake"
    COMMENT "Generating assembly labels from the target C++ declarations"
    VERBATIM)

add_custom_target(fiber_context_asm_symbols ALL DEPENDS "${FIBER_CONTEXT_SYMBOL_HEADER}")

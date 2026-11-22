#==- SelectBackend.cmake - Native Backend Selection -------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Matches the target platform and ABI to a context switch and its supporting
# sources. Canary builds select the checked assembly variant.
#
#===------------------------------------------------------------------------===#

if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(FIBER_CONTEXT_ARCH "${CMAKE_OSX_ARCHITECTURES}")
elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    set(FIBER_CONTEXT_ARCH "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
elseif(CMAKE_VS_PLATFORM_NAME)
    set(FIBER_CONTEXT_ARCH "${CMAKE_VS_PLATFORM_NAME}")
elseif(CMAKE_ANDROID_ARCH_ABI)
    set(FIBER_CONTEXT_ARCH "${CMAKE_ANDROID_ARCH_ABI}")
elseif(CMAKE_CXX_COMPILER_TARGET)
    string(REGEX MATCH "^[^-]+" FIBER_CONTEXT_ARCH "${CMAKE_CXX_COMPILER_TARGET}")
else()
    set(FIBER_CONTEXT_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

list(TRANSFORM FIBER_CONTEXT_ARCH TOLOWER)
list(TRANSFORM FIBER_CONTEXT_ARCH REPLACE "^(amd64|x64|x86-64)$" "x86_64")
list(TRANSFORM FIBER_CONTEXT_ARCH REPLACE "^aarch64$" "arm64")
list(REMOVE_DUPLICATES FIBER_CONTEXT_ARCH)
list(LENGTH FIBER_CONTEXT_ARCH FIBER_CONTEXT_ARCH_COUNT)

if(FIBER_CONTEXT_ARCH_COUNT GREATER 1)
    list(JOIN FIBER_CONTEXT_ARCH ", " arch_names)
    message(FATAL_ERROR "Multiple target architectures (${arch_names}) require separate assembly symbol probes. Select one architecture per build tree.")
endif()

set(FIBER_CONTEXT_SYSTEM "${CMAKE_SYSTEM_NAME}")
if(FIBER_CONTEXT_SYSTEM STREQUAL "Darwin")

    # Check the SDK because Darwin alone does not identify macOS.
    string(REGEX REPLACE "/+$" "" sdk "${CMAKE_OSX_SYSROOT}")
    cmake_path(GET sdk FILENAME sdk)
    string(TOLOWER "${sdk}" sdk)

    if(sdk MATCHES "^(iphone|appletv|watch|xr|driverkit)")
        set(FIBER_CONTEXT_SYSTEM "${sdk}")
    elseif(CMAKE_CXX_COMPILER_TARGET MATCHES "-apple-(ios|tvos|watchos|xros|driverkit|visionos)")
        set(FIBER_CONTEXT_SYSTEM "${CMAKE_MATCH_1}")
    endif()

endif()

set(FIBER_CONTEXT_COMPILER "${CMAKE_CXX_COMPILER_ID}")
if(NOT FIBER_CONTEXT_COMPILER)
    set(FIBER_CONTEXT_COMPILER "${CMAKE_CXX_COMPILER}")
endif()

if(NOT FIBER_CONTEXT_ARCH)
    set(FIBER_CONTEXT_ARCH "unknown")
endif()

string(STRIP "${FIBER_CONTEXT_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}" FIBER_CONTEXT_COMPILER)
if(CMAKE_SIZEOF_VOID_P)
    string(APPEND FIBER_CONTEXT_COMPILER ", ${CMAKE_SIZEOF_VOID_P}-byte pointers")
endif()

set(FIBER_CONTEXT_TARGET "${FIBER_CONTEXT_SYSTEM}/${FIBER_CONTEXT_ARCH} (${FIBER_CONTEXT_COMPILER}, ${FIBER_CONTEXT_SYMBOL_ABI} C++ ABI)")

function(fiber_context_register_backend arch)
    cmake_parse_arguments(PARSE_ARGV 1 backend "" "ASSEMBLER;SYMBOL_ABI;POINTER_BYTES;SWITCH;CANARY;REGISTER_TEST;CANARY_TEST" "SYSTEMS;SOURCES")

    # Collect every backend for clangd before selecting one to build.
    set(asm_sources ${backend_SOURCES})
    list(FILTER asm_sources INCLUDE REGEX "\\.S$")
    list(APPEND FIBER_CONTEXT_INDEX_SOURCES ${backend_SWITCH} ${backend_CANARY} ${backend_REGISTER_TEST} ${asm_sources})
    set(FIBER_CONTEXT_INDEX_SOURCES "${FIBER_CONTEXT_INDEX_SOURCES}" PARENT_SCOPE)
    if(FIBER_CONTEXT_TOOLCHAIN_ONLY OR NOT FIBER_CONTEXT_SYSTEM IN_LIST backend_SYSTEMS OR
       NOT FIBER_CONTEXT_ARCH STREQUAL arch OR NOT FIBER_CONTEXT_SYMBOL_ABI STREQUAL backend_SYMBOL_ABI)
        return()
    endif()

    if(CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL backend_POINTER_BYTES)
        return()
    endif()

    if(FIBER_CONTEXT_SWITCH)
        message(FATAL_ERROR "Multiple native fiber backends match ${FIBER_CONTEXT_TARGET}.")
    elseif(FIBER_CONTEXT_ASM_CANARIES AND NOT backend_CANARY)
        message(FATAL_ERROR "No canary assembly implementation for ${FIBER_CONTEXT_TARGET}.")
    elseif(FIBER_CONTEXT_ASM_CANARIES)
        set(FIBER_CONTEXT_SWITCH "${backend_CANARY}" PARENT_SCOPE)
    else()
        set(FIBER_CONTEXT_SWITCH "${backend_SWITCH}" PARENT_SCOPE)
    endif()

    set(FIBER_CONTEXT_ASM_LANGUAGE "${backend_ASSEMBLER}" PARENT_SCOPE)
    set(FIBER_CONTEXT_BACKEND_SOURCES "${backend_SOURCES}" PARENT_SCOPE)
    set(FIBER_CONTEXT_REGISTER_TEST "${backend_REGISTER_TEST}" PARENT_SCOPE)
    set(FIBER_CONTEXT_CANARY_TEST "${backend_CANARY_TEST}" PARENT_SCOPE)

endfunction()

set(FIBER_CONTEXT_SWITCH "")
set(FIBER_CONTEXT_INDEX_SOURCES "")

set(FIBER_CONTEXT_POSIX_BACKEND
    ASSEMBLER ASM SYMBOL_ABI ITANIUM POINTER_BYTES 8
    SOURCES lib/src/platform/posix_stack.cpp lib/src/runtime/itanium_eh.cpp
    CANARY_TEST tests/native/posix/canaries.cpp)

fiber_context_register_backend(x86_64 SYSTEMS Darwin Linux
    SOURCES lib/src/asm/helpers/x86_64/fp_control.S
    SWITCH lib/src/asm/x86_64/x86_64_sysv.S
    CANARY lib/src/asm/x86_64/x86_64_sysv_canary.S
    REGISTER_TEST tests/context/asm/x86_64_registers.S
    ${FIBER_CONTEXT_POSIX_BACKEND})

set(FIBER_CONTEXT_ARM_FPCR_SOURCE lib/src/asm/helpers/arm/fpcr.cpp)

fiber_context_register_backend(arm64 SYSTEMS Darwin
    SOURCES ${FIBER_CONTEXT_ARM_FPCR_SOURCE}
    SWITCH lib/src/asm/arm64/arm64_apple.S
    CANARY lib/src/asm/arm64/arm64_apple_canary.S
    REGISTER_TEST tests/context/asm/arm64_registers.S
    ${FIBER_CONTEXT_POSIX_BACKEND})

if(NOT FIBER_CONTEXT_TOOLCHAIN_ONLY AND NOT FIBER_CONTEXT_SWITCH)
    message(FATAL_ERROR "No native fiber backend is implemented for ${FIBER_CONTEXT_TARGET}. Use -DFIBER_CONTEXT_TOOLCHAIN_ONLY=ON for the public API checks and assembly labels.")
endif()

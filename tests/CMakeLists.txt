#==- CMakeLists.txt - Test Suite Registration -------------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Registers the fiber tests with CTest and enables the debugger and canary
# checks when their prerequisites are available.
#
#===------------------------------------------------------------------------===#

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
endif()

add_subdirectory(cmake)

find_package(Threads REQUIRED)
add_executable(fiber_context_tests context/context_tests.cpp headers/header_tests.cpp)
target_link_libraries(fiber_context_tests PRIVATE fiber_context::fiber_context Threads::Threads)
add_test(NAME context COMMAND fiber_context_tests)

function(fiber_context_add_subprocess_test name target script)
    set(emulator "")
    if(CMAKE_CROSSCOMPILING)

        # The script needs the emulator because CTest runs CMake on the host.
        set(emulator "$<TARGET_GENEX_EVAL:${target},$<TARGET_PROPERTY:${target},CROSSCOMPILING_EMULATOR>>")
    endif()

    add_test(NAME ${name} COMMAND "${CMAKE_COMMAND}"
        "-DEMULATOR=${emulator}" "-DPROGRAM=$<TARGET_FILE:${target}>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/${script}")
endfunction()

add_executable(fiber_context_termination_tests context/termination_tests.cpp)
target_link_libraries(fiber_context_termination_tests PRIVATE fiber_context::fiber_context)
fiber_context_add_subprocess_test(context_termination fiber_context_termination_tests context/expect-termination.cmake)

if(FIBER_CONTEXT_REGISTER_TEST)
    add_executable(fiber_context_register_tests context/register_tests.cpp "${PROJECT_SOURCE_DIR}/${FIBER_CONTEXT_REGISTER_TEST}")
    target_include_directories(fiber_context_register_tests PRIVATE "${PROJECT_SOURCE_DIR}/lib/src")
    target_link_libraries(fiber_context_register_tests PRIVATE fiber_context::fiber_context)
    add_test(NAME registers COMMAND fiber_context_register_tests)
endif()

add_executable(fiber_context_runtime_tests runtime/runtime_tests.cpp)
target_link_libraries(fiber_context_runtime_tests PRIVATE fiber_context::fiber_context)
add_test(NAME runtime COMMAND fiber_context_runtime_tests)

include(CheckCXXSourceCompiles)

# Recheck ASan when compiler flags change in this build tree.
unset(FIBER_CONTEXT_TESTING_WITH_ASAN CACHE)
check_cxx_source_compiles([=[
    #ifndef __has_feature
    #define __has_feature(x) 0
    #endif
    #if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    #error ASan is disabled
    #endif
    int main() { return 0; }
]=] FIBER_CONTEXT_TESTING_WITH_ASAN)

if(FIBER_CONTEXT_TESTING_WITH_ASAN)
    target_compile_definitions(fiber_context_tests PRIVATE FIBER_CONTEXT_TESTING_WITH_ASAN=1)
    fiber_context_add_subprocess_test(asan_stack_use_after_return fiber_context_runtime_tests runtime/expect-asan.cmake)
endif()

if(FIBER_CONTEXT_ASM_CANARIES AND FIBER_CONTEXT_CANARY_TEST)

    # Link the native objects directly to reach the hidden trap labels.
    add_executable(fiber_context_canary_tests "${PROJECT_SOURCE_DIR}/${FIBER_CONTEXT_CANARY_TEST}" $<TARGET_OBJECTS:fiber_context_native>)

    target_include_directories(fiber_context_canary_tests PRIVATE "${PROJECT_SOURCE_DIR}/lib/src" "${PROJECT_SOURCE_DIR}/lib/include"
        "${FIBER_CONTEXT_GENERATED_INCLUDE_DIR}")

    target_compile_features(fiber_context_canary_tests PRIVATE cxx_std_20)
    target_compile_definitions(fiber_context_canary_tests PRIVATE FIBER_CONTEXT_ASM_CANARIES=1)

    add_test(NAME canaries COMMAND fiber_context_canary_tests)
endif()

if(TARGET fiber_context_ping_pong)
    add_test(NAME ping_pong COMMAND fiber_context_ping_pong)
endif()

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME debugger_frames COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/debugger/debugger_frames.py")

    if(TARGET fiber_context_ping_pong AND NOT CMAKE_CROSSCOMPILING)
        find_program(FIBER_CONTEXT_LLDB_EXECUTABLE lldb)

        if(FIBER_CONTEXT_LLDB_EXECUTABLE)
            add_test(NAME debugger_lldb COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/debugger/debugger_lldb.py"
                "${FIBER_CONTEXT_LLDB_EXECUTABLE}" "$<TARGET_FILE:fiber_context_ping_pong>" "$<CONFIG>")
            set_tests_properties(debugger_lldb PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
        endif()
    endif()
endif()

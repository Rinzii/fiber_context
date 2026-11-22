cmake_minimum_required(VERSION 4.0)
project(fiber_context VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

option(FIBER_CONTEXT_TOOLCHAIN_ONLY "Build the public API compile checks and generate native assembly labels" OFF)
option(FIBER_CONTEXT_ASM_CANARIES "Validate saved registers and stack boundaries at each switch" OFF)
option(FIBER_CONTEXT_BUILD_EXAMPLE "Build the ping-pong example" ${PROJECT_IS_TOP_LEVEL})
option(FIBER_CONTEXT_CLANGD_ASM_INDEX "Generate the project-local assembly index for clangd" ${PROJECT_IS_TOP_LEVEL})

include(cmake/AsmNamespace.cmake)
fiber_context_configure_abi_namespace()

include(cmake/SelectAsmSymbols.cmake)
include(cmake/SelectBackend.cmake)
include(cmake/AsmSymbols.cmake)
if(FIBER_CONTEXT_TOOLCHAIN_ONLY)
    block(PROPAGATE FIBER_CONTEXT_TARGET_IS_ARM)

        # Detect Arm without requiring a working target linker.
        set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
        try_compile(FIBER_CONTEXT_TARGET_IS_ARM SOURCE_FROM_CONTENT arm.cpp [=[
            #if !defined(__ARM_ARCH) && !defined(__arm__) && !defined(__aarch64__) && !defined(_M_ARM) && !defined(_M_ARM64) && !defined(_M_ARM64EC)
            #error The compiler targets a different architecture
            #endif
            int fiber_context_arm_target;
        ]=] NO_CACHE)

    endblock()

    if(FIBER_CONTEXT_TARGET_IS_ARM)
        add_library(fiber_context_arm_fpcr OBJECT "${FIBER_CONTEXT_ARM_FPCR_SOURCE}")
        target_compile_features(fiber_context_arm_fpcr PRIVATE cxx_std_20)
    endif()

    add_subdirectory(tests/headers)
    return()
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

if(FIBER_CONTEXT_CLANGD_ASM_INDEX)
    include(cmake/GenerateAsmIndex.cmake)
    fiber_context_generate_asm_index("${CMAKE_CURRENT_SOURCE_DIR}/build/clangd/asm.idx" ${FIBER_CONTEXT_INDEX_SOURCES})
endif()

if(FIBER_CONTEXT_ASM_LANGUAGE STREQUAL "ASM" AND CMAKE_CXX_COMPILER_TARGET AND NOT DEFINED CMAKE_ASM_COMPILER_TARGET)
    set(CMAKE_ASM_COMPILER_TARGET "${CMAKE_CXX_COMPILER_TARGET}")
endif()

enable_language(${FIBER_CONTEXT_ASM_LANGUAGE})

if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    include(CheckCXXCompilerFlag)

    # The debugger follows saved frame pointers to unwind a parked fiber.
    check_cxx_compiler_flag("-fno-omit-frame-pointer" FIBER_CONTEXT_HAS_FRAME_POINTER_FLAG)
    if(FIBER_CONTEXT_HAS_FRAME_POINTER_FLAG)
        add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug,RelWithDebInfo>>:-fno-omit-frame-pointer>")
    endif()
endif()

add_library(fiber_context_native OBJECT
    lib/src/native/context.cpp
    ${FIBER_CONTEXT_BACKEND_SOURCES}
    lib/src/runtime/sanitizer.cpp
    ${FIBER_CONTEXT_SWITCH})
add_dependencies(fiber_context_native fiber_context_asm_symbols)
target_compile_features(fiber_context_native PRIVATE cxx_std_20)
target_include_directories(fiber_context_native PRIVATE lib/src lib/include "${FIBER_CONTEXT_GENERATED_INCLUDE_DIR}" "${FIBER_CONTEXT_SYMBOL_DIR}")

# Match the C++ record layout to the selected assembly variant.
target_compile_definitions(fiber_context_native PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:FIBER_CONTEXT_ASM_CANARIES=$<BOOL:${FIBER_CONTEXT_ASM_CANARIES}>>")
set_target_properties(fiber_context_native PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_library(fiber_context lib/src/fiber_context.cpp $<TARGET_OBJECTS:fiber_context_native>)
add_library(fiber_context::fiber_context ALIAS fiber_context)
set(FIBER_CONTEXT_NATVIS "${CMAKE_CURRENT_BINARY_DIR}/generated/fiber_context.natvis")
configure_file(tools/fiber_context.natvis.in "${FIBER_CONTEXT_NATVIS}" @ONLY)
target_sources(fiber_context PUBLIC
    "$<BUILD_INTERFACE:${FIBER_CONTEXT_NATVIS}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_DATADIR}/fiber_context/debuggers/fiber_context.natvis>")
target_compile_features(fiber_context PUBLIC cxx_std_20)
target_include_directories(fiber_context PRIVATE lib/src PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/include>
    "$<BUILD_INTERFACE:${FIBER_CONTEXT_GENERATED_INCLUDE_DIR}>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(fiber_context PRIVATE FIBER_CONTEXT_ASM_CANARIES=$<BOOL:${FIBER_CONTEXT_ASM_CANARIES}>)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(fiber_context PUBLIC FIBER_CONTEXT_SHARED)
    set_target_properties(fiber_context PROPERTIES DEFINE_SYMBOL FIBER_CONTEXT_BUILDING_LIBRARY)
endif()
set_target_properties(fiber_context PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

if(FIBER_CONTEXT_BUILD_EXAMPLE)
    add_executable(fiber_context_ping_pong examples/ping_pong.cpp)
    target_link_libraries(fiber_context_ping_pong PRIVATE fiber_context::fiber_context)
endif()

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(tests)
endif()

install(TARGETS fiber_context EXPORT fiber_contextTargets)
install(DIRECTORY lib/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" PATTERN "*.in" EXCLUDE)
install(FILES "${FIBER_CONTEXT_ABI_HEADER}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/__fiber_context")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/fiber_context")
install(FILES tools/fiber_context_debug.py tools/fiber_context_lldb.py tools/fiber_context_gdb.py "${FIBER_CONTEXT_NATVIS}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/fiber_context/debuggers")
install(EXPORT fiber_contextTargets NAMESPACE fiber_context:: FILE fiber_contextConfig.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fiber_context")

# fiber_context

`fiber_context` is a reference implementation of the stackful fibers proposed in [P0876](https://wg21.link/p0876) that is written in C++20.

## Support

| Operating system | Architecture | Compiler and standard library | Runtime support |
| --- | --- | --- | --- |
| macOS | arm64, x86-64 | Apple Clang or LLVM Clang with libc++ | Backend implemented |
| macOS | arm64, x86-64 | GCC with libstdc++ | Unsupported at this time|
| Linux | x86-64 | GCC or Clang with libstdc++ | Backend implemented|
| Linux | x86-64 | Clang with libc++ | Unsupported at this time|
| Windows | x86-64 | MSVC, clang-cl, or MinGW-w64 (GCC or Clang) | Build rules for header checks and assembly label generation only |

## Setup

### Unix systems

| Software | Minimum requirement for the fiber runtime |
| --- | --- |
| CMake | 4.0 or newer |
| Build tool | Ninja for the supplied presets. Use 1.11 or newer with [C++ module scanning][module-scanning]. |
| macOS toolchain | Apple Clang or LLVM Clang in C++20 mode with libc++ and the macOS SDK. The runtime supports arm64 and x86-64. |
| Linux toolchain | GCC or Clang in C++20 mode with libstdc++ and the system development headers. The runtime supports x86-64. |
| Assembler | An assembler that accepts the target's preprocessed .S files. Clang's integrated assembler works. |
| Symbol reader | A target-compatible nm or objdump. On Linux, readelf also works for ELF objects. |

Run these commands from the project root to build the runtime with canary checks.

```sh
cmake --preset checked
cmake --build --preset checked
```

CMake selects a symbol reader automatically. To choose one, pass its name or full path at configure time.

```sh
cmake --preset checked -DFIBER_CONTEXT_SYMBOL_READER=llvm-objdump
```

### Windows

Windows currently supports toolchain checks only. These compile the public headers and generate assembly labels.

| Software | Minimum requirement for the toolchain checks |
| --- | --- |
| CMake | 4.0 or newer |
| Build tool | Ninja for the supplied presets. Use 1.11 or newer with [C++ module scanning][module-scanning]. |
| MSVC toolchain option | MSVC or clang-cl in C++20 mode with the MSVC standard library and Windows SDK. |
| MinGW-w64 toolchain option | GCC or Clang in C++20 mode with MinGW-w64 target headers and libraries. |
| Symbol reader for MSVC or clang-cl | One of dumpbin, llvm-nm, or llvm-objdump. |
| Symbol reader for MinGW-w64 | A COFF-capable nm or objdump from GNU Binutils or LLVM. If objdump leaves names mangled, also install c++filt or llvm-cxxfilt. |

Configure with FIBER_CONTEXT_TOOLCHAIN_ONLY=ON for MSVC or clang-cl. The mingw-toolchain preset sets it automatically for MinGW-w64 GCC.

## License

fiber_context is licensed under the [Apache License 2.0 with the LLVM exception](LICENSE).

[module-scanning]: https://cmake.org/cmake/help/v4.0/manual/cmake-cxxmodules.7.html#limitations

# CubicSDR Project Evaluation

**Project:** CubicSDR v0.2.8
**License:** GPL-2.0+
**Language:** C++ (C++11/14)

## Architecture

- Well-organized modules: SDR I/O, demodulation, audio, visualization, and modem plugins
- Threading model: Producer-consumer pattern with blocking queues for SDR -> demod -> audio pipeline
- Build: Single monolithic CMakeLists.txt supporting Windows/macOS/Linux
- Vendored deps: lodepng, tinyxml, rtaudio, cubicvr2, hamlib, liquid-dsp in `external/`

## Key Weaknesses

| Issue | Severity |
|-------|----------|
| **Zero test coverage** — no test framework, no test files, CI only builds | Critical |
| **Monolithic files** — `AppFrame.cpp` is ~2,700 lines | High |
| **Memory safety** — raw `new`/`delete` for threads, `reinterpret_cast` type punning in DataTree, suppressed MSVC C4996 warnings | High |
| **Outdated CMake** — targets 2.8.12, uses `-std=c++0x` draft flag, deprecated patterns | Medium |
| **Incomplete .gitignore** — missing IDE, OS, and build artifact patterns | Medium |
| **Minimal docs** — no CONTRIBUTING, CHANGELOG, API docs, or Doxygen | Medium |
| **Open TODOs** in project code, including acknowledged bugs | Low-Medium |
| **No input validation** on XML config files | Medium |

## Recommendations (Priority Order)

1. **Add unit tests** — Start with `DataTree`, `Timer`, `ThreadBlockingQueue`, frequency conversion utilities
2. **Fix `.gitignore`** — Add `.vs/`, `*.obj`, `CMakeCache.txt`, `.DS_Store`, IDE files
3. **Modernize CMake** — Bump to 3.10+, use `target_compile_features(cxx_std_14)`, replace deprecated variables
4. **Replace raw `new`/`delete`** with `std::unique_ptr`/`std::shared_ptr` for thread objects in `CubicSDR.cpp`
5. **Split `AppFrame.cpp`** — Extract menu handling, keyboard shortcuts, device management into separate files
6. **Remove MSVC C4996 suppression** — Address the underlying unsafe CRT calls (`sprintf` -> `snprintf`, etc.)
7. **Replace `reinterpret_cast` type punning** in `DataTree.h` with `memcpy`-based serialization (avoids UB)
8. **Use git submodules or a package manager** for vendored external dependencies
9. **Add CI test execution** after builds
10. **Resolve or convert to tracked issues** the open TODOs

## Detailed Findings

### Memory Safety Concerns

- **`DataTree.h`** contains multiple `reinterpret_cast` operations for type punning — potential aliasing violations and undefined behavior. Use `memcpy` or `std::bit_cast` instead.
- **`CubicSDR.cpp`** allocates thread objects (`std::thread*`) with raw `new` and manual `delete`. Any exception between allocation and deletion causes a leak. Should use `std::unique_ptr<std::thread>`.
- **`IOThread.cpp`** `catch(...)` re-throws after setting termination flags — correct for cleanup, but callers rarely handle the propagated exception
- MSVC C4996 warning is globally suppressed, hiding potential buffer overflow risks from unsafe CRT functions.

### Build System Issues

- `cmake_minimum_required(VERSION 2.8.12)` — should be 3.10+
- Uses `ADD_DEFINITIONS(-std=c++0x)` instead of `target_compile_features()` — `c++0x` is the C++11 draft, not the finalized standard
- Uses deprecated `CMAKE_CREATE_WIN32_EXE` variable
- Uses global `include_directories()` instead of `target_include_directories()`
- Hardcoded library paths: `link_directories(/usr/local/lib)`, ALSA paths at `/usr/include` and `/usr/lib`
- Header/source file lists in CMakeLists.txt have mismatches: `SoapySDRThread.h` and `MeterPanel.h` are listed as sources; `SoapySDRThread.cpp`, `UITestCanvas.cpp`, and `UITestContext.cpp` are listed as headers

### Documentation

- README.md is minimal with no inline build instructions
- No CONTRIBUTING.md, CHANGELOG.md, CODE_OF_CONDUCT.md, or architecture docs
- No Doxygen or API documentation
- Build instructions exist only on external wiki; user manual in separate repository

### CI/CD

- CI exists (`.circleci/`) but only builds — no test execution, no `.github/workflows/` or `.travis.yml`
- Recent commits are macOS-focused (bundling, code signing)

### .gitignore

Currently only covers `build/`, `cmake_build/`, `dist/`. Missing entries for:
- IDE files (`.vs/`, `*.suo`, `.idea/`, `*.xcworkspace`, `*.xcodeproj`)
- OS files (`.DS_Store`, `Thumbs.db`, `desktop.ini`)
- Compiled artifacts (`*.o`, `*.obj`, `*.dll`, `*.so`, `*.dylib`, `*.exe`, `*.lib`, `*.a`)
- CMake generated files (`CMakeCache.txt`, `CMakeFiles/`, `cmake_install.cmake`, `Makefile`)
- Package files (`*.deb`, `*.rpm`, `*.dmg`)

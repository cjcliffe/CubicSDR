# CubicSDR Project Evaluation

**Project:** CubicSDR v0.2.8
**License:** GPL-2.0+
**Language:** C++14

## Architecture

- Well-organized modules: SDR I/O, demodulation, audio, visualization, and modem plugins
- Threading model: Producer-consumer pattern with blocking queues for SDR -> demod -> audio pipeline
- Build: single CMakeLists.txt plus CMakePresets.json, driven by the unified stdlib-only build.py for Windows/macOS/Linux
- Vendored deps: lodepng, tinyxml, rtaudio, cubicvr2, hamlib, liquid-dsp in `external/`

## Key Weaknesses

| Issue | Severity |
|-------|----------|
| **Zero test coverage** — no test framework, no test files, CI only builds | Critical |
| **Monolithic files** — `AppFrame.cpp` is ~3,200 lines | High |
| **Memory safety** — raw `new`/`delete` for threads | High |
| **Incomplete .gitignore** — missing IDE, OS, and build artifact patterns | Medium |
| **Minimal docs** — no CONTRIBUTING, CHANGELOG, API docs, or Doxygen | Medium |
| **Open TODOs** in project code, including acknowledged bugs | Low-Medium |
| **No input validation** on XML config files | Medium |

## Recommendations (Priority Order)

1. **Add unit tests** — Start with `DataTree`, `Timer`, `ThreadBlockingQueue`, frequency conversion utilities
2. **Fix `.gitignore`** — Add `.vs/`, `*.obj`, `CMakeCache.txt`, `.DS_Store`, IDE files
3. **Replace raw `new`/`delete`** with `std::unique_ptr`/`std::shared_ptr` for thread objects in `CubicSDR.cpp`
4. **Split `AppFrame.cpp`** — Extract menu handling, keyboard shortcuts, device management into separate files
5. **Use git submodules or a package manager** for vendored external dependencies
6. **Add CI test execution** after builds
7. **Resolve or convert to tracked issues** the open TODOs

## Detailed Findings

### Memory Safety Concerns

- **`CubicSDR.cpp`** allocates thread objects (`std::thread*`) with raw `new` and manual `delete`. Any exception between allocation and deletion causes a leak. Should use `std::unique_ptr<std::thread>`.
- **`IOThread.cpp`** `catch(...)` re-throws after setting termination flags — correct for cleanup, but callers rarely handle the propagated exception

### Documentation

- README.md carries basic build instructions and links to the extended BUILD.md guide
- No CONTRIBUTING.md, CHANGELOG.md, or CODE_OF_CONDUCT.md
- No Doxygen or API documentation
- User manual in separate repository

### CI/CD

- CI exists (`.circleci/`) but only builds — no test execution, no `.github/workflows/` or `.travis.yml`

### .gitignore

Currently only covers `build/`, `cmake_build/`, `dist/`. Missing entries for:
- IDE files (`.vs/`, `*.suo`, `.idea/`, `*.xcworkspace`, `*.xcodeproj`)
- OS files (`.DS_Store`, `Thumbs.db`, `desktop.ini`)
- Compiled artifacts (`*.o`, `*.obj`, `*.dll`, `*.so`, `*.dylib`, `*.exe`, `*.lib`, `*.a`)
- CMake generated files (`CMakeCache.txt`, `CMakeFiles/`, `cmake_install.cmake`, `Makefile`)
- Package files (`*.deb`, `*.rpm`, `*.dmg`)

# Plan: Add Unit Tests

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md)

## Current State

- No C++ test coverage: no C++ test framework, files, or CMakeLists.txt targets yet. Build-system coverage exists in `tests/test_build.py` (stdlib `unittest`, no third-party dependencies).
- CI exists (`.circleci/`) but only builds — no test execution.

## Implementation Plan

### Phase 1: Set up test infrastructure

1. Add a `tests/` directory at the project root.
2. Vendor the doctest single header under `external/doctest/`. It needs no compiled library and no network at configure time.
3. Add a `tests/CMakeLists.txt` with a single `cubicsdr_tests` target. Use one `tests/main.cpp` holding `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`; all other test files only include the header. Group cases with a `TEST_SUITE` per file and register the binary once with CTest.
4. Compile only the sources under test into `cubicsdr_tests` (plus bundled `external/tinyxml/` where needed). Never link wxWidgets, OpenGL, SoapySDR, or liquid-dsp into the test target.
5. Add `option(BUILD_TESTING ...)` defaulting to `OFF` to the root `CMakeLists.txt` and guard `enable_testing()` plus `add_subdirectory(tests)` behind it, so existing and packaging builds are unaffected. (`enable_testing()` must run at root scope, otherwise CTest never writes the top-level test file and reports no tests.)
6. Wire `BUILD_TESTING=ON` through the preset flow. The app builds through `CMakePresets.json` presets (driven by `build.py`), so the plan must cover where the flag is set: either a preset `cacheVariables` entry or an extra `-D` on the preset configure invocation, plus a matching `build.py` passthrough if CI keeps driving through `build.py`.
7. Keep test targets free of wxWidgets, OpenGL, SoapySDR, and liquid-dsp. Phase 2 and Phase 3 sources below were selected because they avoid those dependencies. Local run: `cmake --preset <name> -DBUILD_TESTING=ON`, `cmake --build --preset <name>`, `ctest --test-dir build/<name> --output-on-failure`.

### Phase 2: Core utility tests (highest value, lowest effort)

**Test file: `tests/test_SpinMutex.cpp`**
- `src/util/SpinMutex.h` — header-only, depends only on `<atomic>`.
- Tests:
  - Single-thread: `lock()` makes `try_lock()` fail; after `unlock()`, `try_lock()` succeeds.
  - `std::lock_guard<SpinMutex>` and `std::unique_lock<SpinMutex>` usage, since `ThreadBlockingQueue` relies on both plus `condition_variable_any`.
  - Multi-threaded counter increment; verify the final count.
  - Copy construction and assignment are deleted.

**Test file: `tests/test_Gradient.cpp`**
- `src/util/Gradient.h` + `src/util/Gradient.cpp` — self-contained.
- Tests:
  - Two-color gradient produces linear interpolation across the requested length.
  - Output vector length matches the requested length.
  - Out-of-range color components clamp to the valid range.
  - `clear()` resets state; `generate()` after `clear()` documents empty-array behavior.
  - Multi-stop gradient interpolates across each segment.
- Note: `generate()` divides by `colors.size() - 1`, so zero- and one-color inputs are currently unsafe. Decide the intended behavior first (early return or defined single-color fill), implement it, then lock it in with tests. Do not assert single-color output against the current implementation.

**Test file: `tests/test_ThreadBlockingQueue.cpp`**
- `src/util/ThreadBlockingQueue.h` — depends on `SpinMutex.h` plus standard headers.
- Tests against the actual API:
  - `push`/`pop` ordering, `size()`, `empty()`/`full()`, `flush()`.
  - `try_push()`/`try_pop()` fast paths when full or empty.
  - Timeout overloads, including the `NON_BLOCKING_TIMEOUT` threshold behavior.
  - `set_max_num_items()` only raises the limit; tests must not assume it can lower the limit.
  - Multi-threaded producer-consumer transfer, with generous timeouts so slow CI runners stay stable.
- Note: the header uses `std::shared_ptr` without including `<memory>`. Fix the include as part of this work if the isolated test target exposes the gap.

**Test file: `tests/test_DataTree.cpp`**
- `src/util/DataTree.h` + `src/util/DataTree.cpp` — depends on bundled `external/tinyxml/`.
- Covers `DataElement`, `DataNode`, and `DataTree` in one file.
- Tests:
  - `DataElement::set()`/`get()` for scalar types and `getDataType()` for each type.
  - `DataElement` vector and string overloads.
  - `DataNode` tree building: `newChild()`, `child()` by name and index, `numChildren()`.
  - `DataNode` iteration: `hasAnother()`/`getNext()`/`rewind()`, including `findAll()` and multi-level nesting.
  - `DataTree` XML round-trip through `SaveToFileXML()` / `LoadFromFileXML()` using a portable temp-file helper (C++14 has no `std::filesystem`); clean up the temp file after each run.

**Test file: `tests/test_Timer.cpp`**
- `src/util/Timer.h` + `src/util/Timer.cpp` — platform-specific timing backend.
- Use `lockFramerate()` for deterministic assertions.
- Tests:
  - `start()`/`reset()` behavior and `getNumUpdates()` counting.
  - Locked framerate advances a fixed step per `update()`.
  - `paused()` freezes `getMilliseconds()` while `totalMilliseconds()` keeps advancing.
  - `setMilliseconds()`/`setSeconds()` force specific values.
  - Avoid exact wall-clock assertions on the unlocked path; prefer locked-mode checks or tolerances.

### Phase 3: Threading helper tests (after Phase 2 is stable)

**Test file: `tests/test_IOThread.cpp`**
- `src/IOThread.h` + `src/IOThread.cpp` — covers both `IOThread` and `ReBuffer<T>`, which share that header.
- `IOThread` tests:
  - `setInputQueue()` / `getInputQueue()` and `setOutputQueue()` / `getOutputQueue()` by name, including rebinding an existing name.
  - `terminate()` sets the stopping flag.
  - A minimal subclass `run()` returning normally leads `threadMain()` to set the terminated flag; verify `isTerminated()` with and without a wait, using generous waits so slow CI runners stay stable.
- `ReBuffer<T>` tests:
  - `getBuffer()` allocates when the pool is empty and reuses entries whose `use_count` dropped back to the pool-only reference.
  - Multiple buffers in flight simultaneously.
  - `purge()` clears the pool.
  - Age-out behavior around `REBUFFER_GC_LIMIT`, observed through pool size.
  - Concurrent `getBuffer()` calls from multiple threads.
- Note: compiling this test pulls in `Timer.cpp` as well, but still avoids wxWidgets and GL.

### Phase 4: Update CI

- Update `.circleci/config.yml` (the active CI config) to configure with `BUILD_TESTING=ON`, build, then run `ctest --test-dir build/<preset> --output-on-failure`.
- If CI keeps driving through `build.py`, add the flag passthrough there.
- Keep the default local build without tests unchanged so packaging builds pay no test cost.

## Verification Criteria

- CMake configure with `BUILD_TESTING=ON` succeeds through the preset flow.
- `cmake --build` compiles the `cubicsdr_tests` target without new warnings.
- `ctest --test-dir build/<preset> --output-on-failure` runs the Phase 2 tests and they pass.
- Phase 3 tests compile and pass.
- CI runs the test step after build and reports results.

## Rollback Strategy

This plan only adds new files and a CMake option. If tests fail or cause build issues:
1. Disable `BUILD_TESTING` (set to `OFF`) to skip test compilation entirely.
2. Or remove the `add_subdirectory(tests)` line from `CMakeLists.txt`.
3. No existing source code is modified, except for the small fixes noted above (`<memory>` include, `Gradient` degenerate-input guard) which can each revert independently.

## Files to Create/Modify

| File | Action |
|------|--------|
| `tests/CMakeLists.txt` | Create |
| `tests/main.cpp` (doctest entry point) | Create |
| `external/doctest/doctest.h` (vendored single header) | Vendor |
| `tests/test_SpinMutex.cpp` | Create |
| `tests/test_Gradient.cpp` | Create |
| `tests/test_ThreadBlockingQueue.cpp` | Create |
| `tests/test_DataTree.cpp` | Create |
| `tests/test_Timer.cpp` | Create |
| `tests/test_IOThread.cpp` (covers `IOThread` and `ReBuffer`) | Create (Phase 3) |
| `CMakeLists.txt` | Add `BUILD_TESTING` option and guarded `add_subdirectory(tests)` |
| `CMakePresets.json` and/or `build.py` | Wire `BUILD_TESTING` through the preset flow |
| `.circleci/config.yml` | Add test execution step |
| `src/util/ThreadBlockingQueue.h` | Add missing include if isolated build requires it |
| `src/util/Gradient.cpp` | Guard degenerate `generate()` inputs |

# Plan: Add Unit Tests

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers adding unit test infrastructure and initial test coverage.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md)

**Last Updated:** 2026-07-23

## Current State

- Zero test coverage. No test framework, no test files, no test targets in CMakeLists.txt.
- CI (CircleCI) only builds — no test execution.

## Implementation Plan

### Phase 1: Set up test infrastructure

1. Add a `tests/` directory at the project root.
2. Add a test framework. **Recommendation: Catch2 v3** (header-only, single header, modern C++14, good assertion macros, CMake integration via `FetchContent` or bundled header).
3. Add a `tests/CMakeLists.txt` with a `cubicsdr_tests` target.
4. Add `add_subdirectory(tests)` to the root `CMakeLists.txt` (guarded by an option like `BUILD_TESTING`).
5. Update CircleCI config to run `ctest` after build.

### Phase 2: Core utility tests (highest value, lowest effort)

**Test file: `tests/test_SpinMutex.cpp`**
- `src/util/SpinMutex.h` — header-only, zero external dependencies (`<atomic>` only).
- Tests:
  - Single-thread: `lock()` → `try_lock()` returns false → `unlock()` → `try_lock()` returns true.
  - Multi-thread: two threads incrementing a shared counter under the lock; verify final count equals sum of increments.
  - Verify `lock_guard<SpinMutex>` works (satisfies C++ Lockable concept).
  - Verify copy construction and assignment are deleted (compile-time).

**Test file: `tests/test_Gradient.cpp`**
- `src/util/Gradient.h` + `src/util/Gradient.cpp` — self-contained, no project dependencies.
- Tests:
  - Two-color gradient (black to white) produces linear interpolation [0.0 → 1.0].
  - Output array length matches requested `len`.
  - Values clamped to [0.0, 1.0] for edge cases.
  - `clear()` resets state; `generate()` after `clear()` produces empty arrays.
  - Multi-color gradient (3+ stops) produces correct interpolation.
  - Single-color gradient returns an array of the single color (no interpolation needed).

**Test file: `tests/test_ThreadBlockingQueue.cpp`**
- `src/util/ThreadBlockingQueue.h` — depends only on `SpinMutex.h` (also header-only).
- Tests:
  - Single-threaded: `push`/`pop` ordering, `size()` tracking, `empty()`/`full()` states, `flush()`.
  - Capacity: `set_max_num_items()`, `try_push()` returns false when full.
  - Multi-threaded producer-consumer: one thread pushes N items, another pops N items; verify all items received.
  - Timeout: `push` with short timeout returns false when queue is full.
  - `NON_BLOCKING_TIMEOUT` path: verify immediate fail when queue is full/empty.

**Test file: `tests/test_DataTree.cpp`**
- `src/util/DataTree.h` + `src/util/DataTree.cpp` — depends on bundled `external/tinyxml/`.
- Tests:
  - `DataElement::set()`/`get()` for all scalar types (char, int, float, double, etc.).
  - `DataElement::getDataType()` returns correct enum for each type.
  - `DataNode` tree building: `newChild()`, `child()` by name/index, `numChildren()`.
  - `DataNode` iteration: `hasAnother()`/`getNext()`/`rewind()`.
  - `DataTree::SaveToFileXML()` / `LoadFromFileXML()` round-trip.
  - `DataElement` vector types: `set()`/`get()` for `vector<int>`, `vector<float>`, etc.

**Test file: `tests/test_Timer.cpp`**
- `src/util/Timer.h` + `src/util/Timer.cpp` — platform-specific (`<windows.h>` on Windows).
- Tests (using `lockFramerate()` for determinism):
  - `start()` resets timer; `getMilliseconds()` returns 0 after start.
  - `lockFramerate(30.0)` → `update()` advances by ~33.33ms each call.
  - `paused(true)` freezes `getMilliseconds()` while `totalMilliseconds()` continues.
  - `getNumUpdates()` counts `update()` calls correctly.
  - `setMilliseconds()`/`setSeconds()` force specific values.

### Phase 3: Additional module tests (after Phase 2 is stable)

**Test file: `tests/test_FreqConversion.cpp`**
- Frequency conversion utilities used throughout the SDR pipeline.
- Tests:
  - `freqToHz()` / `hzToFreq()` round-trip for common frequencies (1 MHz, 144 MHz, 440 MHz)
  - Edge cases: DC (0 Hz), Nyquist boundary, negative frequencies
  - Frequency formatting for display strings

**Test file: `tests/test_DataNode.cpp`**
- `DataNode` tree structure operations (beyond basic DataTree round-trip in Phase 2).
- Tests:
  - `newChild()` with various types (string, int, float, vector)
  - `child()` by name and by index
  - `numChildren()` accuracy after insertions
  - Iterator pattern: `hasAnother()` / `getNext()` / `rewind()`
  - Deep tree traversal (3+ levels nesting)
  - `DataNode` copy semantics

**Test file: `tests/test_IOThread.cpp`**
- `IOThread` base class queue binding and lifecycle.
- Tests:
  - `setInputQueue()` / `getInputQueue()` by name
  - `setOutputQueue()` / `getOutputQueue()` by name
  - `stopping` flag transitions
  - `terminated` flag set after `run()` completes
  - `isTerminated()` timeout behavior
  - Named queue rebinding (replace a bound queue)

**Test file: `tests/test_ReBuffer.cpp`**
- `ReBuffer<T>` buffer pool allocation and garbage collection.
- Tests:
  - `getBuffer()` returns new buffer when pool is empty
  - `getBuffer()` reuses buffer when use_count drops to 1
  - Multiple buffers in flight simultaneously
  - `GC_LIMIT` age-out behavior (mock or observe pool size)
  - Thread-safe concurrent `getBuffer()` calls

### Phase 4: Update CI

- Add `BUILD_TESTING=ON` to CircleCI build steps.
- Add `ctest --test-dir build --output-on-failure` after build.
- Consider adding a separate "test" job that depends on the "build" job.

## Verification Criteria

- `cmake -B build -DBUILD_TESTING=ON` configures without errors.
- `cmake --build build` compiles all test files and the `cubicsdr_tests` target.
- `ctest --test-dir build --output-on-failure` runs all Phase 2 tests and they pass.
- Phase 3 tests compile and pass when enabled.
- CI pipeline (`.circleci/config.yml`) runs tests after build and reports results.

## Rollback Strategy

This plan only adds new files and a CMake option. If tests fail or cause build issues:
1. Disable `BUILD_TESTING` (set to `OFF`) to skip test compilation entirely.
2. Or remove the `add_subdirectory(tests)` line from `CMakeLists.txt`.
3. No existing source code is modified.

## Files to Create/Modify

| File | Action |
|------|--------|
| `tests/CMakeLists.txt` | Create |
| `tests/test_SpinMutex.cpp` | Create |
| `tests/test_Gradient.cpp` | Create |
| `tests/test_ThreadBlockingQueue.cpp` | Create |
| `tests/test_DataTree.cpp` | Create |
| `tests/test_Timer.cpp` | Create |
| `tests/test_FreqConversion.cpp` | Create (Phase 3) |
| `tests/test_DataNode.cpp` | Create (Phase 3) |
| `tests/test_IOThread.cpp` | Create (Phase 3) |
| `tests/test_ReBuffer.cpp` | Create (Phase 3) |
| `CMakeLists.txt` | Add `add_subdirectory(tests)` and `BUILD_TESTING` option |
| `.circleci/config.yml` | Add test execution step |

## Testability Assessment

| Module | Self-Contained? | External Deps | Testability |
|--------|----------------|---------------|-------------|
| `SpinMutex.h` | Yes (header-only) | None (`<atomic>` only) | Excellent |
| `Gradient.h/.cpp` | Yes | None (`<vector>` only) | Excellent |
| `ThreadBlockingQueue.h` | Mostly | `SpinMutex.h` (in-project) | Excellent |
| `DataTree.h/.cpp` | Mostly | `tinyxml.h` (bundled) | Good |
| `Timer.h/.cpp` | Mostly | Platform APIs | Moderate |
| Freq conversion | Yes | None | Excellent |
| `DataNode` | Mostly | `DataElement` (in-project) | Good |
| `IOThread.h/.cpp` | Mostly | `ThreadBlockingQueue` (in-project) | Good |
| `ReBuffer.h` | Yes (header-only) | `shared_ptr` only | Good |

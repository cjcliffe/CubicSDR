# Plan: Add Unit Tests

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers adding unit test infrastructure and initial test coverage.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

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
  - Document bug: `generate()` with single color causes divide-by-zero (`colors.size() - 1 == 0`).

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

### Phase 3: Update CI

- Add `BUILD_TESTING=ON` to CircleCI build steps.
- Add `ctest --test-dir build --output-on-failure` after build.
- Consider adding a separate "test" job that depends on the "build" job.

## Files to Create/Modify

| File | Action |
|------|--------|
| `tests/CMakeLists.txt` | Create |
| `tests/test_SpinMutex.cpp` | Create |
| `tests/test_Gradient.cpp` | Create |
| `tests/test_ThreadBlockingQueue.cpp` | Create |
| `tests/test_DataTree.cpp` | Create |
| `tests/test_Timer.cpp` | Create |
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

# Testing

See also: [Architecture Overview](README.md) | [Threading Model](threading.md) | [Configuration System](configuration-system.md)

## Layout

- C++ tests live in `tests/`: one `TEST_SUITE` per file, a single `cubicsdr_tests` binary, and one CTest entry.
- `external/doctest/` vendors the doctest single header, so tests need no compiled library and no network access at configure time.
- `tests/test_build.py` covers the build-system helpers with stdlib `unittest`.

## Running the suite

Configure with `BUILD_TESTING=ON` (default `OFF`), build, then run `ctest --test-dir build/<preset> --output-on-failure`. The `build` CI job enables the flag and runs the suite after the application build; a failing test fails the step and the job.

## Scope and conventions

- The test target compiles only the sources under test plus the doctest entry point; it never links wxWidgets, OpenGL, SoapySDR, or liquid-dsp.
- Covered areas include the spin mutex, gradient generator, thread-safe queue (ordering, timeouts, and blocking waits), data elements and nodes with XML round-trips, timers, IO threads, and the buffer pool.
- Timer tests run locked at a fixed framerate so results stay deterministic; the unlocked production path is intentionally untested.
- Threaded tests favor join-based assertions over timing asserts, with generous waits where a thread must park first.
- DataTree round-trip tests use a temporary file and remove it afterwards.
- `enable_testing()` runs at root scope inside the `BUILD_TESTING` guard; CTest only writes the top-level test file then, otherwise reporting no tests.
- Windows test builds define `NOMINMAX` so the `windows.h` min/max macros do not clash with `std::max`; the application itself inherits that define through wxWidgets.

# Plan: Fix Undefined Behavior in DataTree Serialization

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers eliminating undefined-behavior `reinterpret_cast` type punning in `src/util/DataTree.h` by replacing read-side aliasing violations with `std::memcpy`-based deserialization.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md)

**Last Updated:** 2026-07-23

## Current State

`src/util/DataTree.h` contains 20 `reinterpret_cast` operations:
- **18 read-side casts** (in `get<T>()` methods) — alias typed scalars through `unsigned char` buffers, violating strict aliasing rules (undefined behavior)
- **2 write-side casts** (in `set<T>()` methods) — cast to `unsigned char*`, which is well-defined per [basic.lval]/11

The 18 read-side casts are the actual UB. The write-side casts are safe and idiomatic.

## Implementation Plan

1. Create a helper template function in `DataTree.h` (or a new `DataTreeUtil.h`):
    ```cpp
    template <typename T>
    T readFromBuffer(const std::vector<unsigned char>& buf, size_t offset = 0) {
        T value;
        std::memcpy(&value, buf.data() + offset, sizeof(T));
        return value;
    }

    template <typename T>
    void writeToBuffer(std::vector<unsigned char>& buf, const T& value) {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
        buf.insert(buf.end(), bytes, bytes + sizeof(T));
    }
    ```

    **Note:** `writeToBuffer` retains a `reinterpret_cast` to `const unsigned char*`. This cast is well-defined per [basic.lval]/11 — casting to `unsigned char*` (or `std::byte*`) is the one exception to strict aliasing. It is safe and idiomatic. Only the read-side casts need replacement.
2. Replace all 18 `reinterpret_cast` read operations in `get<T>()` methods with calls to `readFromBuffer<T>()`.
3. Replace the 2 `reinterpret_cast` write operations in `set<T>()` methods with calls to `writeToBuffer<T>()`.
4. Add `#include <cstring>` to `DataTree.h` for `std::memcpy`.

## Verification Criteria

- Zero `reinterpret_cast` read operations remain in `DataTree.h` (the 2 write-side casts to `unsigned char*` are expected).
- Build succeeds on MSVC, GCC, and Clang with `-fstrict-aliasing` enabled.
- Existing serialization round-trip: `DataTree::SaveToFileXML()` / `LoadFromFileXML()` still produces identical output for config and bookmark files.
- No new compiler warnings under `-Wall -Wextra`.

## Rollback Strategy

This is a low-risk, localized change (single file). If regressions appear:
1. `git revert` the commit.
2. The original `reinterpret_cast` code is functionally equivalent on all major compilers despite being technically UB — the revert is safe.

## Files to Modify

| File | Action |
|------|--------|
| `src/util/DataTree.h` | Add helper functions, replace 18 read-side reinterpret_casts |

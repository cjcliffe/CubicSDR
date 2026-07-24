# Plan: Replace reinterpret_cast Type Punning in DataTree

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers replacing undefined-behavior `reinterpret_cast` type punning in `src/util/DataTree.h` with safe `std::memcpy`-based serialization.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

`src/util/DataTree.h` contains 20 `reinterpret_cast` operations (lines 209, 230, 306-398) that perform type-punning between `unsigned char` byte buffers and typed scalar values. This violates C++ strict aliasing rules and can cause undefined behavior.

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

    **Note:** The `writeToBuffer` helper retains a `reinterpret_cast` for the write side. Casting to `unsigned char*` (or `std::byte*`) is well-defined per [basic.lval]/11 — it is the one exception to strict aliasing. The goal here is to eliminate the **20 read-side** `reinterpret_cast`s that alias typed scalars through `unsigned char` buffers, which are the actual aliasing violations. The write-side cast is safe and idiomatic.
2. Replace all 20 `reinterpret_cast` read operations (lines 306-398) with calls to `readFromBuffer<T>()`.
3. Replace the 2 `reinterpret_cast` write operations (lines 209, 230) with calls to `writeToBuffer<T>()` (which uses a safe cast to `unsigned char*`).
4. Add `#include <cstring>` to `DataTree.h` for `std::memcpy`.
5. Build and verify no regressions.

## Files to Modify

| File | Action |
|------|--------|
| `src/util/DataTree.h` | Add helper functions, replace all reinterpret_casts |

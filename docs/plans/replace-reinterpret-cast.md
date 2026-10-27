# Plan: Fix Undefined Behavior in DataTree Serialization

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md)

## Current State

`DataElement` in `src/util/DataTree.h` stores scalar and vector values as byte buffers (`data_val`, `data_val_vector`).

- The `DataElement::set()` overloads encode values by reading them through `unsigned char` pointers, which the aliasing rules permit. The change there is for symmetry with the read path, not a UB fix on its own.
- The `DataElement::get()` overloads decode by casting the byte buffer to the stored scalar type for the matching type branch, then converting that value to the requested output type. Dereferencing a pointer into a byte buffer as a larger scalar type relies on undefined behavior (alignment and effective type). Byte copying with `memcpy` provides the same result with defined behavior.
- The in-memory byte representation stays native-endian and platform-sized (notably `long` widths); the portable form is the XML text path in `DataTree.cpp`, which is unaffected.
- The project targets C++14, so the fix uses `memcpy` helpers rather than a newer facility such as `std::bit_cast`.

## Implementation Plan

1. Add includes for `std::memcpy` (`<cstring>`) and the trivially-copyable trait (`<type_traits>`) to `DataTree.h`.
2. Add small header-local helpers in a detail namespace (for example `DataTreeDetail`), defined after the `DataException` classes the guard throws and before `DataElement`, so the member templates can call them without depending on the nested buffer typedef:
    ```cpp
    namespace DataTreeDetail {
    template <typename S>
    S readScalar(const std::vector<unsigned char>& buf) {
        static_assert(std::is_trivially_copyable<S>::value, "scalar storage must be trivially copyable");
        if (buf.size() < sizeof(S)) {
            throw DataException("Cannot get() the scalar, stored buffer is smaller than the stored type!");
        }
        S tmp{};
        std::memcpy(&tmp, buf.data(), sizeof(S));
        return tmp;
    }

    template <typename T>
    void writeScalar(std::vector<unsigned char>& buf, const T& value) {
        // No static_assert here: unsupported input types must keep throwing
        // DataTypeMismatchException at runtime from the determine* calls.
        buf.resize(sizeof(T));
        std::memcpy(buf.data(), &value, sizeof(T));
    }
    }
    ```
    Put the size check inside `readScalar` so scalar and per-element vector reads share one guard. The guard throws `DataException`, consistent with the existing empty-buffer checks. Keep the existing scalar empty-buffer check; the `readScalar` guard is the second layer that catches short buffers. Do not put a `static_assert` on `writeScalar`: unsupported input types must keep throwing `DataTypeMismatchException` at runtime from the existing `determineScalarDataType` / `determineVectorDataType` calls, and a `static_assert` there would convert those runtime errors into compile errors instead. The `static_assert` stays only on `readScalar`, where the stored type is always one of the supported kinds named at the call site. The detail namespace keeps the helpers callable from the committed tests with an undersized buffer; the truncated-buffer path is not reachable through the public `set()` API, which always writes full-size buffers. The buffer parameters stay `std::vector<unsigned char>` to match `DataElementBuffer`; the nested typedef cannot be named from before `DataElement`.
3. In the scalar `DataElement::get()` overload, convert every stored-type branch to copy the stored type and then convert to the output, preserving the existing conversion behavior relied on by callers such as `DataElement::toString()`:
    ```cpp
    if (storageType == DataElement::Type::DATA_CHAR) {
        scalar_out = T(DataTreeDetail::readScalar<char>(data_val));
    } else if (storageType == DataElement::Type::DATA_UCHAR) {
        scalar_out = T(DataTreeDetail::readScalar<unsigned char>(data_val));
    } else if (storageType == DataElement::Type::DATA_INT) {
        scalar_out = T(DataTreeDetail::readScalar<int>(data_val));
    } else if (storageType == DataElement::Type::DATA_UINT) {
        scalar_out = T(DataTreeDetail::readScalar<unsigned int>(data_val));
    } else if (storageType == DataElement::Type::DATA_LONG) {
        scalar_out = T(DataTreeDetail::readScalar<long>(data_val));
    } else if (storageType == DataElement::Type::DATA_ULONG) {
        scalar_out = T(DataTreeDetail::readScalar<unsigned long>(data_val));
    } else if (storageType == DataElement::Type::DATA_LONGLONG) {
        scalar_out = T(DataTreeDetail::readScalar<long long>(data_val));
    } else if (storageType == DataElement::Type::DATA_FLOAT) {
        scalar_out = T(DataTreeDetail::readScalar<float>(data_val));
    } else if (storageType == DataElement::Type::DATA_DOUBLE) {
        scalar_out = T(DataTreeDetail::readScalar<double>(data_val));
    }
    ```
    Preserve the existing behavior for unmatched storage kinds (for example scalar `get()` on string, void, null, or vector storage): leave the output untouched rather than introducing a new throw here. Value-initialize the locals in the one-line helpers (for example `int i_get{};` in `getInt()`) so the unmatched-storage path returns a defined zero instead of reading uninitialized memory; behavior for matched storage is unchanged. Only the generic `get<T>` overload changes here; the `string`, `wstring`, and `vector<string>` overloads dispatch separately and stay untouched. Remove the smarter-way-with-templates TODO in this overload; the helper is that resolution.
4. In the vector `DataElement::get()` overload, apply the same per-element pattern against each element buffer for every stored vector kind (`DATA_CHAR_VECTOR`, `DATA_UCHAR_VECTOR`, `DATA_INT_VECTOR`, `DATA_UINT_VECTOR`, `DATA_LONG_VECTOR`, `DATA_ULONG_VECTOR`, `DATA_LONGLONG_VECTOR`, `DATA_FLOAT_VECTOR`, `DATA_DOUBLE_VECTOR`), reusing the `readScalar` size guard per element so the per-element empty check becomes redundant and can be dropped (an empty element buffer trips the shared guard, since its size is below every stored type size; note the thrown message changes to the shared guard text, so tests should assert the exception type rather than the old per-element message). Value-initialize the per-element output (`T scalar_out{};`) so an unmatched stored kind pushes one default value per element instead of copying uninitialized memory; the element count behavior stays as before. Pushing one default per element is a deliberate behavior-preserving choice: counts stay stable and the UB goes away without adding a new throw. Throwing `DataTypeMismatchException` on unmatched storage (as the `string` overloads do) was considered and deferred; revisit if callers want strictness. Iterate the element buffers by const reference rather than by value to avoid copying each buffer per iteration. Remove the smarter-way-with-templates TODO in this overload.
5. In the scalar and vector `DataElement::set()` overloads, replace pointer aliasing with `writeScalar`, preserving the current overwrite semantics for `data_val` and for each per-element buffer. Use const references for the element loop variables in the touched overloads to avoid copying each element per iteration; behavior is unchanged.

Out of scope: the C-style interior-pointer cast in `getDataPointer()`, the `calloc` / multibyte-conversion handling in the `wstring` `set()` / `get()` overloads, `wsEncode` / `wsDecode`, and the `toString()` `WSTRING` branch. Those stay untouched, so this plan does not resolve the `toString` forced-cast TODO; that entry stays as a convert-to-issue item in the resolve-todos plan. The two smarter-way-with-templates TODOs in `DataTree.h` are resolved here by the helpers: split the combined `DataTree.h` row in the resolve-todos plan so those two entries are marked resolved by this plan, while the `DataTree.cpp` forced-cast and stack-recursion rows remain convert-to-issue items. Also out of scope: the pre-existing `DATA_UINT_VECTOR` defect in `nodeToXML()`, which streams `*tmp_intvect_i` while iterating `tmp_uintvect_i` — dereferencing an empty vector's iterator, not just producing wrong values — and the missing `vector<long long>` convenience operators on `DataNode`. File those separately if they are not already tracked.

## Verification Criteria

- Build through `python build.py` succeeds on the touched platforms.
- Extend the committed doctest suite in `tests/test_DataTree.cpp`, focusing on the gaps in the existing coverage rather than duplicating it: cross-type reads of stored values (for example integer storage read as a wider integer, float storage read as double, as relied on by `toString()`), the vector kinds without round-trip coverage yet (char, uchar, uint, long, ulong, longlong, double), and the truncated-buffer guard. Cover the guard by calling the detail-namespace `readScalar` helper directly with an undersized buffer and checking for a `DataException` throw, since that path is unreachable through the public `set()` API. Exercise `vector<long long>` through `element()->set` / `get` directly, since `DataNode` has no convenience operators for that type. Run the suite with the canonical CI steps (configure the top-level with `BUILD_TESTING=ON`, build, and run `ctest --output-on-failure`) and keep it green; the `cubicsdr_tests` binary directly stays available for reruns without a rebuild. A future `--run-tests` entry for `build.py` (see [Add --run-tests](add-run-tests.md)) may wrap those steps.
- Run the suite under UBSan (plus ASan where available): this is a UB fix, and the wx-free `cubicsdr_tests` target keeps a sanitizer run cheap. Keep it green alongside the canonical run.
- Confirm no `reinterpret_cast` remains in `src/` with a repository-wide search (for example `git grep -n reinterpret_cast -- src`, or `grep -rn reinterpret_cast src/` where grep is available); vendored code under `external/` is excluded. Remaining C-style casts in the out-of-scope paths stay by design.
- XML persistence through `DataTree::SaveToFileXML()` / `DataTree::LoadFromFileXML()` continues to work for config, session, and bookmark files. The XML text inference is lossy by design (for example narrow integer text may reload with a narrower type), so success means files load and save without errors with values spot-checked, not exact type equality. Avoid `DATA_UINT_VECTOR` content in XML spot-checks: the known out-of-scope `nodeToXML()` defect above dereferences an empty vector's iterator there. Use multi-element vectors in XML spot-checks: a single-element numeric vector serializes as bare text that decodes back as a scalar. Avoid exact `long long` vector round-trips through XML: the decode path accumulates into a `long` container and sets `DATA_LONG_VECTOR` (never `DATA_LONGLONG_VECTOR`), and narrows through `long`, which truncates on platforms with narrower `long` widths.
- No new compiler warnings from the touched translation units. The MSVC C4996 suppression stays in place; removing it belongs to its own plan.
- The resolve-todos plan no longer routes the two `DataTree.h` template TODOs to issues: its combined `DataTree.h` row is split, step 1 names this plan, and this plan is linked under Related Plans there.

## Rollback Strategy

This is a localized change to one header plus tests. If regressions appear, `git revert` the commit to restore the prior behavior.

## Files to Modify

| File | Action |
|------|--------|
| `src/util/DataTree.h` | Add includes and byte-copy helpers, convert `set()` / `get()` branches, remove the two smarter-way-with-templates TODOs |
| `tests/test_DataTree.cpp` | Add round-trip, cross-type, direct-helper truncated-buffer, and XML cases for the touched paths |
| `docs/plans/resolve-todos.md` | Split the combined `DataTree.h` row so the two template TODOs are marked resolved by this plan; keep the `DataTree.cpp` forced-cast and stack-recursion rows as convert-to-issue; name this plan in step 1 alongside the AppFrame split and link it under Related Plans |

# Plan: Centralize Frequency Helpers

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md) | [Testing](../design/testing.md)

## Current State

- `frequencyToStr()` and `strToFrequency()` are defined in `CubicSDR.cpp` and declared in `CubicSDR.h`, so reusing them pulls in the application translation unit.
- Formatting call sites are spread across `AppFrame.cpp` (bandwidth menu labels, demodulator titles), `BookmarkMgr.cpp`, and `FrequencyDialog.cpp`.
- Alongside the shared helpers, several places redo the same logic by hand: inline sample-rate formatting streaming its own `kHz` suffix, offset conversions dividing by 1000 with hand-written labels, and inconsistent suffix casing across sites.

## Implementation Plan

### Phase 1: Inventory formatting and parsing sites

Collect every site that formats or parses frequencies: callers of `frequencyToStr()` and `strToFrequency()`, inline divisions by thousands/millions/billions for display, and hand-written unit suffixes. The result is the migration checklist for Phase 3.

### Phase 2: Add a frequency utility

Add `src/util/Frequency.h` + `src/util/Frequency.cpp` using only the standard library (no wxWidgets):

- Format an integer Hz value with GHz/MHz/KHz thresholds.
- Parse a suffixed string back to Hz.
- Move the existing `frequencyToStr()` / `strToFrequency()` implementations there and update all callers; drop the old declarations from `CubicSDR.h`.
- Settle suffix casing once, in the utility.

### Phase 3: Migrate call sites

Replace each inventoried inline conversion with the utility, including the bandwidth menu labels, demodulator titles, bookmark strings, frequency dialog strings, and offset display conversions.

### Phase 4: Add tests

Add `tests/test_Frequency.cpp` on the doctest infrastructure described in [Testing](../design/testing.md):

- Round-trip formatting and parsing for representative frequencies across the amateur bands.
- Threshold boundaries between plain Hz, KHz, MHz, and GHz output.
- Edge cases: zero, sub-kHz values, and negative offsets.
- Parser variants: suffix casing, missing suffix, and invalid input.

## Verification Criteria

- The full application configures and builds with the new sources.
- Re-running the Phase 1 inventory search finds no remaining inline frequency formatting.
- `cubicsdr_tests` passes, including the new round-trip cases.
- UI spot-check shows identical strings in the bandwidth menu, demodulator titles, bookmarks, and frequency dialog.

## Rollback Strategy

- The utility is additive until callers migrate; each migrated site can revert to its previous inline code independently.
- If the migration causes display regressions, revert the affected call site first rather than the whole utility.

## Files to Create/Modify

| File | Action |
|------|--------|
| `src/util/Frequency.h` | Create |
| `src/util/Frequency.cpp` | Create |
| `tests/test_Frequency.cpp` | Create |
| `CMakeLists.txt` | Add new sources to `cubicsdr_sources` and headers |
| `src/CubicSDR.cpp` | Remove moved implementations |
| `src/CubicSDR.h` | Remove moved declarations |
| `src/AppFrame.cpp` | Use the utility at inventoried sites |
| `src/BookmarkMgr.cpp` | Use the utility at inventoried sites |
| `src/FrequencyDialog.cpp` | Use the utility at inventoried sites |

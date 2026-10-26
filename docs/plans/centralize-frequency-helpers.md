# Plan: Centralize Frequency Helpers

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md) | [Testing](../design/testing.md)

## Current State

- `frequencyToStr()` and `strToFrequency()` are defined in `CubicSDR.cpp` and declared in `CubicSDR.h`, so reusing them pulls in the application translation unit.
- `filterChars()` lives alongside them in the same two files and is shared: `strToFrequency()` depends on it, and the clipboard-paste handler in `FrequencyDialog.cpp` calls it directly. Moving the frequency helpers without it leaves a dangling declaration or a duplicated helper.
- Formatting call sites are spread across `AppFrame.cpp` (bandwidth menu labels, demodulator titles), `DemodulatorInstance.cpp` (demodulator titles), `BookmarkMgr.cpp`, `BookmarkView.cpp` (range, bookmark, and demodulator frequency/bandwidth display strings), `SDRDevices.cpp` (device sample-rate option names), and `FrequencyDialog.cpp`.
- `DemodulatorInstance::updateLabel()` rolls its own MHz label formatting (fixed three-decimal megahertz value, no suffix) for bookmark labels. It is a separate display format from `frequencyToStr()`, so it is inventoried but out of scope for migration (see Phase 3).
- Alongside the shared helpers, several RF display sites redo the same logic by hand: offset conversions dividing by 1000 with hand-written labels (in both the `AppFrame.cpp` offset menu/dialog and the `SDRDevices.cpp` device offset property), and inconsistent suffix casing across sites (`KHz` in the helper, offset labels, and the modem bandwidth dialog title versus `kHz` in the audio menu).
- Audio sample-rate formatting (the audio device menu, debug output in `AudioThread.cpp`) is explicitly out of scope for this plan. It formats audio rates, not RF frequencies, with different precision, and migrating it would change unrelated UI strings.

## Implementation Plan

### Phase 1: Inventory formatting and parsing sites

Collect every RF site that formats or parses frequencies: callers of `frequencyToStr()` and `strToFrequency()`, the `filterChars()` paste call in `FrequencyDialog.cpp`, inline divisions by thousands/millions/billions for display, and hand-written unit suffixes. Include `DemodulatorInstance::updateLabel()` and the offset display conversions in the inventory so the exclusion decisions are recorded. Exclude static human-readable labels (such as bookmark range display names) and all audio-rate formatting. The result is the migration checklist for Phase 3.

### Phase 2: Add a frequency utility

Add `src/util/Frequency.h` + `src/util/Frequency.cpp` using only the standard library (no wxWidgets):

- Format an integer Hz value with GHz/MHz/kHz thresholds.
- Parse a suffixed string back to Hz, preserving the existing implicit rule that a bare number containing a decimal point or at or below 3000 means megahertz.
- Move the existing `frequencyToStr()` / `strToFrequency()` implementations there, together with `filterChars()`, and update all callers; drop the old declarations from `CubicSDR.h`.
- Settle suffix casing once, in the utility: SI-correct lowercase-`k` `kHz` everywhere. This intentionally changes the current `KHz` output of the helper and the offset labels; Phase 4 pins the new casing with golden-string tests.

### Phase 3: Migrate call sites

Replace each inventoried RF conversion with the utility, including the bandwidth menu labels, demodulator titles, bookmark strings, bookmark view display strings, device sample-rate option names, and frequency dialog strings. Normalize the static modem bandwidth dialog title suffix to SI `kHz` as part of the casing sweep.

Explicitly out of scope, with reasons:

- `DemodulatorInstance::updateLabel()`: its fixed three-decimal bare-MHz bookmark label is a different format; converting it to suffixed output would change persisted bookmark label strings. Leave it as-is; a follow-up can add a dedicated label formatter if wanted.
- Offset kHz-unit dialogs: the `AppFrame.cpp` offset menu label plus entry dialog, and the `SDRDevices.cpp` device offset property, present the value in whole kHz (integer division for display, multiply-back by 1000 on confirm). Keep those kHz-unit interactions unchanged and only normalize their suffixes to SI `kHz`. Rationale: the dialog range deliberately exceeds 32-bit Hz range for down/upconverter shifts, and switching to suffixed-Hz parsing would change units, rounding, and range handling for no display benefit.
- Audio sample-rate formatting: RF only for now (see Current State).

### Phase 4: Add tests

Add `tests/test_Frequency.cpp` on the doctest infrastructure described in [Testing](../design/testing.md):

- Golden-string vectors pinning the SI `kHz` casing and the threshold boundaries between plain Hz, kHz, MHz, and GHz output.
- Approximate round-trip checks for representative frequencies across the amateur bands (exact round-trip does not hold at all magnitudes because the formatter uses significant-digits precision, so assert within a documented tolerance).
- Parser vectors for the implicit megahertz rule for bare input, suffix casing variants, missing suffix, invalid input, zero, sub-kHz values, and negative offsets.

## Verification Criteria

- The full application configures and builds with the new sources.
- Re-running the Phase 1 inventory search over computed RF conversions finds no remaining inline frequency formatting, excluding the explicitly out-of-scope sites (static display-name labels, `updateLabel()`, the offset kHz-unit dialog, audio rates).
- `cubicsdr_tests` passes, including the new golden-string and parser-vector cases.
- UI spot-check shows identical strings in the bandwidth menu, demodulator titles, bookmarks, bookmark view, device settings, and frequency dialog; offset labels and the modem bandwidth title show the intentional `KHz` to `kHz` casing change and unchanged numeric values.

## Rollback Strategy

- The utility is additive until callers migrate; each migrated site can revert to its previous inline code independently.
- If the migration causes display regressions, revert the affected call site first rather than the whole utility.

## Files to Create/Modify

| File | Action |
|------|--------|
| `src/util/Frequency.h` | Create |
| `src/util/Frequency.cpp` | Create |
| `tests/test_Frequency.cpp` | Create |
| `tests/CMakeLists.txt` | Register new test file and utility sources |
| `CMakeLists.txt` | Add new sources to `cubicsdr_sources` and headers |
| `src/CubicSDR.cpp` | Remove moved implementations |
| `src/CubicSDR.h` | Remove moved declarations |
| `src/AppFrame.cpp` | Use the utility at inventoried RF sites; normalize offset suffix to `kHz` |
| `src/BookmarkMgr.cpp` | Use the utility at inventoried sites |
| `src/forms/Bookmark/BookmarkView.cpp` | Use the utility at inventoried sites |
| `src/forms/SDRDevices/SDRDevices.cpp` | Use the utility for sample-rate option names; normalize offset property suffix to `kHz` |
| `src/FrequencyDialog.cpp` | Use the utility at inventoried sites, including the `filterChars()` paste handler |
| `src/demod/DemodulatorInstance.cpp` | Use the utility at inventoried `frequencyToStr()` title sites |
| `src/CubicSDR.cpp` | Normalize static modem bandwidth dialog title suffix to `kHz` |

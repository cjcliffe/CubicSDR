# Plan: Centralize Frequency Helpers

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md) | [Testing](../design/testing.md)

## Current State

- `frequencyToStr()` and `strToFrequency()` are defined in `CubicSDR.cpp` and declared in `CubicSDR.h`, so reusing them pulls in the application translation unit.
- `filterChars()` lives alongside them in the same two files and is shared: `strToFrequency()` depends on it, and the clipboard-paste handler in `FrequencyDialog.cpp` calls it directly. Moving the frequency helpers without it leaves a dangling declaration or a duplicated helper.
- Formatting call sites are spread across `AppFrame.cpp` (bandwidth menu labels, demodulator titles), `DemodulatorInstance.cpp` (demodulator titles), `BookmarkMgr.cpp`, `BookmarkView.cpp` (range, bookmark, and demodulator frequency/bandwidth display strings), `SDRDevices.cpp` (device sample-rate option names), and `FrequencyDialog.cpp`. The demodulator-title `frequencyToStr()` sites in `AppFrame.cpp` and `DemodulatorInstance.cpp` sit inside `#if ENABLE_DIGITAL_LAB` guards, so the test target (which never compiles that path) covers them only indirectly.
- The GL demodulator label in `PrimaryGLContext.cpp` (`std::to_string((double)freq/1000000.0)`) is another computed RF display with the same bare-MHz shape as `updateLabel()`; it is inventoried in Phase 1 and out of scope for migration (see Phase 3). A second GL path there renders `demod->getLabel()`, which is downstream of `updateLabel()` and covered by that exclusion.
- The manual sample-rate entry dialog in `AppFrame.cpp` shows computed `min`/`max` bounds as plain `"... Hz"` strings, and the Rig SDR-IF entry dialog uses the same whole-Hz `Frequency (Hz)` pattern. They describe RF values but use whole-Hz entry, not the suffixed helper format; Phase 1 records them and Phase 3 leaves the prompts unchanged.
- `DemodulatorInstance::updateLabel()` rolls its own MHz label formatting (fixed three-decimal megahertz value, no suffix) for the demodulator auto-label. It is a separate display format from `frequencyToStr()`, so it is inventoried but out of scope for migration (see Phase 3).
- Spectrum axis tick labels in `SpectrumPanel.cpp` are bare-MHz numbers derived from MHz-scale grid math. They are a separate axis format from `frequencyToStr()`, so they are inventoried but out of scope for migration (see Phase 3).
- Alongside the shared helpers, several RF display sites redo the same logic by hand: offset conversions dividing by 1000 with hand-written labels (in both the `AppFrame.cpp` offset menu/dialog and the `SDRDevices.cpp` device offset property), and inconsistent suffix casing across sites (`KHz` in the helper, offset labels, and the modem bandwidth dialog title versus `kHz` in the audio menu).
- Audio sample-rate formatting (the audio device menu, debug output in `AudioThread.cpp`) is explicitly out of scope for this plan. It formats audio rates, not RF frequencies, with different precision, and migrating it would change unrelated UI strings.

## Implementation Plan

### Phase 1: Inventory formatting and parsing sites

Collect every RF site that formats or parses frequencies: callers of `frequencyToStr()` and `strToFrequency()`, the `filterChars()` paste call in `FrequencyDialog.cpp`, inline divisions by thousands/millions/billions for display (including `/1000000.0` with `to_string` and `/1000` integer-division offsets), and hand-written unit suffixes. Include `DemodulatorInstance::updateLabel()` (which also covers the `getLabel()` GL path derived from it), the `PrimaryGLContext.cpp` GL demodulator label, the spectrum axis tick labels in `SpectrumPanel.cpp`, the `AppFrame.cpp` manual sample-rate `min`/`max` Hz prompts and Rig SDR-IF Hz entry, the offset display conversions, and the plain-Hz stored `sample_rate` value in `SDRDevices.cpp` (a machine value carried with `Hz` units, not a display string) in the inventory so the exclusion decisions are recorded. Exclude static human-readable labels (such as bookmark range display names), all audio-rate formatting, and the non-RF `FrequencyDialog` targets (waterfall lines-per-second, spectrum average, gain), which use plain `stoi`/`stod`/`to_string` rather than frequency parsing. The result is the migration checklist for Phase 3.

### Phase 2: Add a frequency utility

Add `src/util/Frequency.h` + `src/util/Frequency.cpp` using only the standard library (no wxWidgets):

- Format an integer Hz value with GHz/MHz/kHz thresholds, preserving the existing `frequencyToStr()` signatures so callers move without conversion churn. Document the negative-input behavior (today a negative value falls through to a plain-Hz string with no suffix) and keep it unchanged.
- Parse a suffixed string back to Hz, preserving the existing implicit rule that a bare number containing a decimal point or at or below 3000 means megahertz.
- Define the canonical parser behavior where the current code is inconsistent, and keep it fixed for the migration: the existing filter strips `-` (so negative input cannot round-trip today), and the paste allow-list accepts `Z`/`z` while the parser filter does not. Note that `filterChars()` takes its input by reference and filters it in place, so in `strToFrequency()` the fallback length and the numeric/suffix slicing all operate on the already-filtered text rather than the raw input. The utility keeps one documented rule for all callers: strip a leading `-` (preserve current behavior; negative values do not round-trip) instead of copying the mismatch per site. The no-suffix default is a `FrequencyUnit` parameter (`Hz`/`kHz`/`MHz`/`GHz`/`Auto`, defaulting to `MHz`); `Auto` keeps the historical conditional rule above, and the dialog call sites pass `Auto` explicitly so their behavior is unchanged. The range split on `-` in `FrequencyDialog::OnChar()` stays in the dialog: it divides ranged input before parsing, exempts a leading dash from splitting, and passes each half to the utility as a single value.
- Move the existing `frequencyToStr()` / `strToFrequency()` implementations there, together with `filterChars()`, and update all callers; drop the old declarations from `CubicSDR.h`. Each migrated translation unit includes `util/Frequency.h` for the moved declarations.
- Settle suffix casing once, in the utility: SI-correct lowercase-`k` `kHz` everywhere. This intentionally changes the current `KHz` output of the helper everywhere it appears (bandwidth menu labels, demodulator titles, bookmark strings, device sample-rate names, frequency dialog strings) as well as the offset labels; Phase 4 pins the new casing with golden-string tests.

### Phase 3: Migrate call sites

Replace each inventoried RF conversion with the utility, including the bandwidth menu labels, demodulator titles (noting the `ENABLE_DIGITAL_LAB` guards carry over unchanged), bookmark strings, bookmark view display strings, device sample-rate option names, and frequency dialog strings. Normalize the static modem bandwidth dialog title suffix to SI `kHz` as part of the casing sweep.

Explicitly out of scope, with reasons:

- `DemodulatorInstance::updateLabel()`: its fixed three-decimal bare-MHz demodulator auto-label is a different format; converting it to suffixed output would change GL demodulator labels and recording filenames derived from it. Leave it as-is; a follow-up can add a dedicated label formatter if wanted.
- `PrimaryGLContext.cpp` GL demodulator label: same bare-MHz shape as `updateLabel()` (`to_string` of MHz with full double precision). Leave it as-is alongside `updateLabel()`; a follow-up can route both through one label formatter if wanted. The other GL demodulator path there renders `demod->getLabel()` and is covered by the `updateLabel()` exclusion above.
- Manual sample-rate `min`/`max` Hz prompts and Rig SDR-IF Hz entry in `AppFrame.cpp`: whole-Hz entry bounds, not suffixed display strings. Leave the prompts unchanged.
- Spectrum axis tick labels in `SpectrumPanel.cpp`: bare-MHz axis numbers, not suffixed display strings. Leave them as-is; a follow-up can route them through one label formatter with `updateLabel()` if wanted.
- Offset kHz-unit dialogs: the `AppFrame.cpp` offset menu label plus entry dialog, and the `SDRDevices.cpp` device offset property, present the value in whole kHz (integer division for display, multiply-back by 1000 on confirm). Keep those kHz-unit interactions unchanged and only normalize their suffixes to SI `kHz`. Rationale: the dialog range deliberately exceeds 32-bit Hz range for down/upconverter shifts, and switching to suffixed-Hz parsing would change units, rounding, and range handling for no display benefit.
- Stored `sample_rate` value in `SDRDevices.cpp`: a plain-Hz machine value (carried with `Hz` units), not a suffixed display string. Leave it as `to_string` output.
- Audio sample-rate formatting: RF only for now (see Current State).

### Phase 4: Add tests

Add `tests/test_Frequency.cpp` on the doctest infrastructure described in [Testing](../design/testing.md), following the one-`TEST_SUITE`-per-file convention:

- Golden-string vectors pinning the SI `kHz` casing and the threshold boundaries between plain Hz, kHz, MHz, and GHz output, including negative-input formatting (plain-Hz fall-through, unchanged).
- Approximate round-trip checks for representative frequencies across the amateur bands (exact round-trip does not hold at all magnitudes because the formatter uses significant-digits precision, so assert within a documented tolerance).
- Parser vectors for the implicit megahertz rule for bare input (passed as `FrequencyUnit::Auto`), suffix casing variants, missing suffix, invalid input, zero, and sub-kHz values, plus the documented leading-`-` rule from Phase 2 (a leading `-` is stripped, so negative input does not round-trip). Default-unit vectors pinning the `MHz` default and explicit `Hz`/`kHz`/`MHz`/`GHz` defaults, including a suffixed value beating a non-megahertz default. Cover single values only; ranged `freq-freq2` input keeps its existing split in `FrequencyDialog::OnChar()` before parsing, so no ranged cases belong in the utility tests.

## Verification Criteria

- The full application configures and builds through `python build.py` with the new sources.
- Re-running the Phase 1 inventory search over computed RF conversions finds no remaining inline frequency formatting, excluding the explicitly out-of-scope sites (static display-name labels, `updateLabel()` and the `getLabel()` GL path derived from it, the `PrimaryGLContext.cpp` GL label, the spectrum axis tick labels, the manual sample-rate `min`/`max` prompts and Rig SDR-IF entry, the offset kHz-unit dialog, the stored `sample_rate` value, non-RF dialog targets, audio rates). Repeatable searches: `frequencyToStr|strToFrequency|filterChars` for helper use (still hits the new utility and its migrated callers by design); `to_string.*1000000|1000000|/ ?1000` alongside hand-written `KHz|kHz` suffixes for inline conversions, scoped to RF display code since bare `/1000` also matches non-RF code; the bare `1000000` alternative covers stringstream-based MHz formatting such as `updateLabel()`.
- `cubicsdr_tests` passes, including the new golden-string and parser-vector cases.
- UI spot-check shows unchanged numeric values everywhere; strings keep their layout and only the suffix casing changes from `KHz` to `kHz` (in helper output across the bandwidth menu, demodulator titles, bookmarks, bookmark view, device settings, and frequency dialog, plus the offset labels and the modem bandwidth title).

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
| `docs/design/testing.md` | Extend the covered-areas list with the frequency utility |
| `CMakeLists.txt` | Add new sources to `cubicsdr_sources` and headers |
| `src/CubicSDR.cpp` | Remove moved implementations; normalize static modem bandwidth dialog title suffix to `kHz` |
| `src/CubicSDR.h` | Remove moved declarations |
| `src/AppFrame.cpp` | Use the utility at inventoried RF sites; normalize offset suffix to `kHz`; leave manual sample-rate `min`/`max` Hz prompts and Rig SDR-IF Hz entry unchanged |
| `src/BookmarkMgr.cpp` | Use the utility at inventoried sites |
| `src/forms/Bookmark/BookmarkView.cpp` | Use the utility at inventoried sites |
| `src/forms/SDRDevices/SDRDevices.cpp` | Use the utility for sample-rate option names; normalize offset property suffix to `kHz` |
| `src/FrequencyDialog.cpp` | Use the utility at inventoried sites, including the `filterChars()` paste handler |
| `src/demod/DemodulatorInstance.cpp` | Use the utility at inventoried `frequencyToStr()` title sites (guards unchanged) |
| `src/visual/PrimaryGLContext.cpp` | Inventory only; leave GL label as-is (out of scope) |
| `src/panel/SpectrumPanel.cpp` | Inventory only; leave axis tick labels as-is (out of scope) |

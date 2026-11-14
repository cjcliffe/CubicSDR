## Goal

Silence the cheap, unambiguous MSVC `/analyze` findings in app code:
an uninitialized struct member, deliberately-discarded return values,
a dead temporary, plus a fail-soft policy for unchecked `malloc`/`calloc`.

## Success Criteria

- The three findings no longer fire; no behavior change.
- Build and both suites stay green.

## Approach

Run `/analyze` on the app target in an isolated worktree (existing
deps prefix reused) and triage by code, `src/` only. Fix only what is
provably intention-preserving:

- *Uninitialized member:* `WaterfallRing::UploadPlan` gets default
  member initializers. Both fields are always assigned by
  `planUpload()` today, so this only hardens future paths; `consumed`
  rides along since it shares the identical defect.
- *Discarded returns:* the three `modf()` calls in the tuner digit
  handler use only the out-param. Cast to `(void)` to document that
  the fractional return is intentionally ignored.
- *Dead temporary:* the unnamed `wxClientDC` in the waterfall paint
  path constructs and immediately destroys a device context. It dates
  to 2015 and has no effect; delete it.
- *Copy/paste memset:* the FFT setup zeroed `fftInput` four times
  instead of each fresh buffer. Each `memset` now targets its own
  `malloc`, confirmed against the author's intent; a sweep found no
  other mismatched pairs in the process sources.
- *Uninitialized drag width:* `dragBW` was the one drag member missing
  from the constructor init list. A demod appearing mid-press (after
  the press-time snapshot was skipped) could feed garbage to
  `setBandwidth()`; now zero-initialized alongside `dragOfs`, which
  clamps to minimum bandwidth. The adjacent mid-drag demod-switch
  quirk stays as observed behavior.
- *Narrowing arithmetic:* triaged all `C26451` sites. Fixed the new
  code (peak products computed in `double`, ring indexes widened
  before operating) and every allocation-length shape (stream-MTU
  `malloc`, waterfall texture `new`/`memset`, callback `memset`).
  The MTU value is driver-supplied but tiny in practice; the cast is
  hygiene for the exact shape the check exists for. The remaining
  sites are bounded index/pixel/frequency math (glyph atlas, spectrum
  points, stream copies, menu IDs) — reviewed and left as noise.

- *Unchecked allocation:* allocate-then-commit with fail-soft
  bail. `SpectrumVisualProcessor::setup()` returns `bool`; it builds
  all FFT buffers before touching live state, frees partial results
  and returns false on failure, and `process()` re-arms the resize
  strobe on false so the next frame retries with current buffers
  intact. `DataTree::wsEncode()`/`wsDecode()` return empty on
  `calloc` failure. `SDRThread::updateSettings()` allocates the
  stream buffer to a local and commits (free old, store MTU, resize
  overflow) only on success; on failure it logs once and returns
  with current buffers live and no update notify. No re-arm there:
  retrying would repeat the device deactivate/set/activate dance
  every turn; the next rate change retries the switch. First-run
  failure still leaves null buffers (nothing to preserve); the
  `overflowBuffer` vector growth throws rather than returning null,
  so a uniform OOM policy for exceptions stays future work.

## Steps

1. Branch per the Fix Synopsis convention; apply the three fixes.
2. Write the synopsis; build, run suites, commit on request.

## Validation Plan

- Full Windows build plus C++ suite and `test_build.py` green.
- Re-running `/analyze` shows the three codes gone from these sites
  (spot-check, not a full second pass).

## Risks / Open Questions

- Risk: none identified; all three edits are behavior-preserving by
  construction.
- Open questions: none.

## Outcome

Implemented as planned. Build green with no new warnings; suites
green. No new maintained test: the `setup()` contract needs
liquid-dsp linkage the test target excludes by design, the `ws`
guards are private with happy paths already covered by the XML
wstring round-trips, and the stream block needs a live device.

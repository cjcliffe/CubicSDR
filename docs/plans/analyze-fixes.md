## Goal

Silence the cheap, unambiguous MSVC `/analyze` findings in app code:
an uninitialized struct member, deliberately-discarded return values,
and a dead temporary.

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

Left out deliberately: unchecked `malloc`/`calloc` results and the
`C26451` bulk — each needs author judgment or a wider pass.

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
green. The deferred findings above remain future work.

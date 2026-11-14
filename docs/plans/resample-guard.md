## Goal

Fix the GH#1052 segfault (`firdespm_create(), invalid bands` at narrow
bandwidths or deep zoom) by keeping resample ratios inside a designable
range and failing soft when liquid-dsp refuses a design.

## Root Cause

CubicSDR never calls `firdespm` directly. On liquid-dsp through v1.8.2,
every `msresamp_*_create` decomposes its ratio into 2x halfband stages,
each designed via Parks-McClellan (`resamp2` ->
`liquid_firdespm_halfband_as`). Extreme ratios (tiny IQ ratio from
narrow bandwidth against a wide device rate, huge audio ratio from the
same, tiny spectrum ratio from deep zoom) push the per-stage designs
out of spec, `firdespm_create` rejects the bands and returns NULL, and
the NULL propagates unchecked into a segfault. Our vendored liquid-dsp
v1.8.3 switched `resamp2` to a Kaiser design (a speed-motivated change
that silently removed this crash class), so the crash only reproduces
against older system liquid — but the unchecked NULLs and unbounded
ratios are ours in every version.

## Approach

- *New `ResampleGuard.h`:* `clampResampleRatio()` bounds any ratio to
  a fixed halfband stage budget in both directions (garbage maps to
  the floor); `minResampleBandwidth()` is the matching integer-Hz
  floor for a given device rate. Both are pure and covered by
  `test_ResampleGuard.cpp`.
- *Worker IQ path:* the bandwidth is raised to the stage-budget floor
  before the modem kit is built, so kit and resampler stay coherent;
  the ratio is clamped again at the `msresamp_create` call, and a NULL
  result is logged and propagated (the PreThread already keeps old
  filters on NULL).
- *PreThread:* the DSP block is skipped while no resampler exists, so
  a first-build failure drops frames instead of crashing.
- *Modem kits (Analog, FMStereo, CW):* audio ratios clamped at
  creation, NULL creates logged, destroys guarded, and executes
  replaced with zero-length output on NULL (silence, no crash).

## Validation Plan

- New suite green (written failing-first against an identity stub),
  full C++ suite and `test_build.py` green, Windows build with no new
  warnings.
- Live check on Windows with narrow bandwidths; the clamp only bites
  past the previous crash range, so normal operation is unchanged.

## Risks / Open Questions

- The stage budget is heuristic: grounded between observed-good and
  observed-crashing ratios, not derived from liquid internals.
- On old liquid the extreme-narrow case now mutes or raises bandwidth
  instead of crashing; that degradation is intentional but unverified
  against system liquid locally.

## Outcome

Implemented as planned. Retire this synopsis on merge.

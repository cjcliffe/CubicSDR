# Visual Clarity Improvement Plan

## Goal

Close the spectrum/waterfall visibility gap reported in GH#989: weak signals — especially adjacent to strong ones — should be clearly distinguishable instead of smeared, washed out, or compressed into the noise floor.

## Success Criteria

- A weak signal next to a strong one reads clearly on both spectrum and waterfall.
- No single artifact (DC spike) can compress the display range.
- Zooming and panning stay responsive with stable levels.
- Every phase lands build-green, suite-green, and live-checked on Windows.

## Approach

Work proceeds in ordered phases, each on a sub-branch off `visual-clarity`, merged back when its validation passes. Order is deliberate: un-compress the scale first so later changes read correctly, then cut leakage, then contrast, then resolution, then performance, then cosmetics. DSP changes reuse the linked liquid-dsp API and each carries a doctest in the existing harness. All phases flow through `SpectrumVisualProcessor`, which also feeds the demod views — they inherit the improvements.

## Steps

1. DC-ceiling exclusion. Skip DC-adjacent bins in the auto floor/ceiling scan when DC hiding is on, so the spike stops pinning the scale. Surfaces: the min/max scan and the hide path in `SpectrumVisualProcessor`. Test: synthetic DC spike no longer sets the ceiling.
2. FFT window. Precompute Hann taps per FFT size in `setup()` and apply before `fft_execute`, replacing the rectangular window's sidelobe smear. liquid-dsp already provides the window types. Test: leakage reduction on a synthetic tone.
3. Peak decimation. Reduce bins to pixels with max instead of mean so narrow peaks survive zoom-out. Surfaces: the pixel accumulation loop in `process()`. Test: narrow peak preserved through N:1 reduction.
4. Log-domain averaging. Average in dB rather than linear magnitude for standard video-averaging contrast. Test: noise-floor behavior on synthetic noise plus tone.
5. Configurable FFT size. Add a menu-configurable spectrum FFT size setting (1024 to 8192 in powers of two, defaulting to `DEFAULT_FFT_SIZE`), persisted with the app config. Measure CPU cost and waterfall texture growth per step; keep power-of-two sizing. Validation is measurement plus the existing ring tests.
6. Zoom rebuild cost. Throttle resampler recreation during zoom gestures (minimum interval between rebuilds, reusing the existing rate gate) while view tracking stays live. Validation is live feel plus a zoom sweep without freezes or jumps, with the waterfall flowing throughout.
7. Rendering polish. Filled spectrum trace, waterfall top-color reach, peak-hold default review. GL-only; validated live.

## Validation Plan

Each phase: `python build.py` build stays green; new focused doctest plus the full suite green; live check on Windows with a weak signal adjacent to a strong one and a zoom sweep. Highest-risk validation is the phase 5 performance measurement, which decides the new default size.

## Risks / Open Questions

- Larger FFT sizes raise CPU and texture memory; measurement in phase 5 gates the default.
- Log-domain averaging changes the long-standing look of the noise floor; final feel call happens live.
- Resampler level normalization must not shift calibrated levels; verify against a known carrier.
- Open questions: None.

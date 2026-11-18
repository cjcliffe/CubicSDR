## Goal

Fix the FMS stereo decoder's poor separation, phasiness, and hiss,
verified against the liquid-dsp v1.8.3 source and the ITU-R BS.450
pilot-tone spec.

## Root Cause

Four defects, all in the stereo difference path of
`ModemFMStereo`:

- *Uncalibrated 38 kHz reference:* the subcarrier is sine-phased
  with the pilot (BS.450), and doubling the pilot PLL doubles the
  pilot filter phase, so the downconverted difference lands
  rotated: S = m·sin(2φ) + H(m)·cos(2φ), a mix of the message and
  its Hilbert transform governed by the IIR phase φ at 19 kHz.
  The old matrix also assumed the wrong sign.
- *Mono/stereo time skew:* the Hilbert R2C/C2R pair delays the
  difference by 2·(2m+1) = 22 samples while the mono path has no
  delay, collapsing separation above the bass.
- *Ungated stereo:* with a weak or absent pilot the difference
  channel plays noise into L/R, including on mono stations.
- *Overwide pilot PLL:* bandwidth 0.25 snaps the 38 kHz
  reference to noise (loop gains derive straight from it).

## Approach

- *Reference calibration:* `buildKit` measures the pilot filter
  phase at 19 kHz through a scratch twin and locks the PLL at
  `stereoReferencePhase` (φ − π/4), decoding +S; the matrix
  becomes L = M + S, R = M − S with the old trim kept as
  `STEREO_OUT_GAIN` (M/S balance is unity by construction).
- *Delay alignment:* a `MonoDelayLine` holds 22 samples of mono
  history so both resamplers process the same time window.
- *Pilot blend:* a `StereoBlender` fades S to mono below a pilot
  power floor (fast attack, slow release).
- *PLL narrowed* to 0.02, still acquiring in milliseconds.
- The helpers live liquid-free in `StereoDsp.h` with doctest
  coverage in `test_StereoDsp.cpp`, since `cubicsdr_tests`
  never links liquid-dsp.

## Validation Plan

- Full C++ suite and `test_build.py` green, Windows build with no
  new warnings. DSP correctness needs ears and RF: on a strong
  stereo station confirm stable centered vocals, wide
  undistorted sides, and no phasiness on transients; on a mono
  or weak station confirm hiss fades instead of pumping; on
  tune confirm stereo returns within a fraction of a second.

## Risks / Open Questions

- Blend thresholds and ballistics are set from nominal pilot
  levels; tune by ear on weak stations if the stereo image
  pumps or drops out.
- L/R polarity follows the BS.450 phase derivation; confirm
  with known-polarity program material when available.

## Outcome

Implemented as planned and verified live on broadcast FM.
Retire this synopsis on merge.

# FMS stereo level vs sample rate — synopsis

## Symptom

FMS loudness tracked the audio device rate: noticeably louder at 96kHz and
quiet at 22.05kHz relative to the mid rates. FM mono was unaffected.

## Root cause

`ModemFMStereo::buildKit` feeds `liquid_firdes_kaiser` taps directly into the
stereo L/R lowpass filters. Those taps are not unity-gain: their sum grows
with filter length, and length grows with the audio rate (the transition width
is a fixed 1kHz, so higher rates need longer filters). Measured tap sums were
1.00, 1.38, 1.50, and 3.00 at 22.05, 44.1, 48, and 96kHz — roughly +0, +2.8,
+3.5, and +9.5dB of unintended gain. The tap buffer was also leaked on every
kit rebuild.

## Investigation

Exonerated in order: peak metering (max-abs, rate-independent), the mixer
(pure copy with rate filtering only), de-emphasis (gain-1 at DC at every rate,
checked mathematically and via demph-off runs), the liquid resampler (unity
within 0.0005 at ratios from 0.11 to 16, measured), and the FM mono path
(flat across rates, measured).

The discriminating probe compiled the real modem sources against the built
liquid library and fed them synthetic FM: FM peaks held constant while FMS
peaks scaled from 0.277 at 22.05kHz to 0.871 at 96kHz, tracking the tap sums.

## Fix

Normalize the designed taps to DC gain 1 (divide by their sum, guarded against
zero) before creating the filters, and free the tap buffer. One file, additive
only.

## Verification

- Probe re-run against the fixed code: FMS peaks flat (0.277 to 0.290) across
  all rates; FM unchanged.
- Full application build plus the C++ and build suites green.
- Live check: switching audio device rates on an FMS station holds constant
  loudness.
- No maintained unit test covers this: the modem sources require liquid-dsp,
  which the unit harness deliberately excludes. The standalone probe plus the
  live check are the verification evidence.

## Follow-ups

None.

## Goal

Clean up analog voice quality: fix the shared auto-gain leveling
used by AM/DSB/LSB/USB, migrate CW onto it with a sideband
selection (GH#919), harden DSB against carrier leak, and flatten
SSB bass response.

## Root Cause

- *Peak detector half-blind:* the level follower tracked positive
  peaks only, so asymmetric voice ran hot on the negative side.
- *Unbounded gain:* with no ceiling, long silence drove gain
  toward infinity — burst on return, NaN in the limit. (A 20x
  ceiling tried first proved too low for weak signals and was
  raised to 60 dB.)
- *Symmetric block-rate smoothing:* one slow coefficient gave
  slow attack (loud onsets blip) and slow release (pumping).
- *DSB DC path:* the Costas output (verified present and correct
  in liquid-dsp) had no DC blocker, so residual carrier and
  mistuning rumble reached the peak detector.
- *SSB edge on carrier:* the sideband filter cutoff sat exactly
  on the shifted carrier, shelving bass −3 dB at DC.
- *CW gain in log domain:* auto gain ran the same follower but
  through 10·log10, heading to +inf in silence; manual gain
  applied 10^(dB/10), twice as hot as its amplitude-dB label.
- *CW single sideband:* upper only, so mirrored chains (I/Q
  swap, GH#919) flip pitch direction with no recourse; the
  suggested negative-beep workaround would only silence output.
- *CW offset rebuild:* retuning the sidetone rebuilt the kit,
  flushing the resampler for no reason (NCO retunes are
  phase-continuous).
- *CW transient desense:* unshaped keying edges ring hotter than
  the tone and arrive faster than the release sheds them, so a
  wideband follower parks on clicks and plays the beep quiet.

## Approach

- *`AnalogAgc` helper:* peak follower with |.| detection, fast
  attack (2 ms) and slow release (300 ms) timed in seconds from
  the kit rate, target 0.5 preserving nominal loudness, gain
  ceiling 60 dB so weak signals reach target. State lives on
  the modem so kit rebuilds do not re-adapt. Liquid-free in
  `AnalogAgc.h` with doctest coverage.
- *`DcBlocker` helper:* one-pole DC blocker on the DSB output,
  corner a few Hz at voice rates, with doctest coverage.
- *SSB guard:* shift 0.24 against the 0.25 cutoff in both LSB
  and USB, confining leakage below ~50 Hz and flattening bass.
- *CW migration:* auto gain runs the shared helper at the audio
  rate and reports clamped amplitude-dB, capped back at the
  manual 40 dB range for keying-transient headroom; manual
  apply moves to 10^(dB/20) with the per-sample pow hoisted
  out of the loop; the old follower state is deleted.
- *CW sideband:* a side setting (upper default) mirrors the NCO
  and keeps the matching Hilbert output; offset changes no
  longer rebuild the kit.
- *CW tone level:* auto gain follows a leaky resonator at the
  beep frequency instead of wideband peaks, immune to clicks
  and out-of-band noise, with doctest coverage.

## Validation Plan

- Full C++ suite and `test_build.py` green, Windows build with no
  new warnings. DSP behavior needs ears and RF: hot asymmetric
  voice no longer clips negatives, long pauses return without
  a burst, loud onsets settle promptly without pumping, DSB
  holds level on residual-carrier signals, SSB bass sounds full.
  CW upper matches previous pitch direction, lower mirrors it,
  manual gain needs re-tweaking (recalibrated), offset retunes
  without gaps. CW beep plays at the same level as an SSB tone
  on the same carrier, dits stay clean, no pumping between
  elements.

## Risks / Open Questions

- Release ballistics are set from convention; shorten if levels
  pump audibly on ragchew pauses, lengthen if weak-signal fading
  breathes.
- A per-sample limiter after the block gain would tame
  single-block overshoots further; deferred as future work.

## Outcome

Implemented as planned and verified live on broadcast and weak
signals. Retire this synopsis on merge.

## Goal

Fix GH#844 (center frequency can't go below half the sample rate) by
clamping the device LO to the driver's real range instead of the
assumed `rate/2` floor, and add the missing high-side clamp so an
accidental tune past the device max can't take down a throwing driver.

## Root Cause

Three low clamps (`CubicSDR::setFrequency`, `SDRThread::setFrequency`,
and the canvas view floors) all assumed the view's left edge must stay
above 0 Hz, blocking HF-capable hardware. Nothing clamped the top end
at all, and the streaming retune called `device->setFrequency` with no
exception guard while `SDRThread::run` has no catch-all — a driver that
throws on an out-of-range tune terminates the whole app.

## Approach

- *Device range query:* new `SDRDeviceInfo::getFrequencyRange()` merges
  the bare and `"RF"` Soapy ranges (bogus spans dropped, throwing
  drivers reported as unknown). `SDRThread` caches the span at device
  start so tuning never queries the driver per tick.
- *Low-end policy:* the floor is `min(driverMin, rate/2)` — the driver
  can only ever open more low tuning than legacy, never take it away.
  This keeps unpatched drivers (SoapyRTLSDR reporting the tuner range
  in direct-sampling mode) and mode gaps (bladeRF + XB-200) working
  exactly as today. The high end always trusts the driver.
- *Shared math:* header-only `TuneRange.h` (`clampTuneFrequency` applies
  the offset-aware device clamp, `deviceTuneFloor` the low policy,
  `pinViewCenter`/`pinDemodCenter` the view and drag pins), covered by
  `test_TuneRange.cpp`, written failing-first against a stub.
- *Views:* the `bandwidth/2` floors are gone; after each retune the view
  is pinned back into the live data window, so sub-zero spectrum (noise
  and rolloff — the FFT bins exist across the whole window) displays
  without ever rendering past the data.
- *Demods:* the zero floor in `setFrequency` is removed so drops land
  where clicked, and frequency/range drags pin the center to the view —
  previously a drag could push a demod past the data window where the
  post thread deactivates it.
- *Labels:* spectrum axis ticks stop at zero (no mirrored negative
  labels) with the `0.0` tick marking the boundary, snapped so float
  residue never renders `-0.0`.
- *Retune guard:* the streaming tune catches driver exceptions, logs,
  and keeps the old frequency; buffered samples are still pre-tune so
  skipping the drain is correct.

## Validation Plan

- New matcher suites green, full C++ suite and `test_build.py` green,
  Windows build with no new warnings.
- Live on Windows with an RTL-SDR: normal tuning unchanged, high end
  stops at the device max, and with the companion SoapyRTLSDR
  `range-reporting` branch (mode-aware `[0, max]`) low tunes continue
  through 24 MHz into auto-switched direct sampling.

## Risks / Open Questions

- Honest high-minimum tuners (E4000, FC parts) keep the legacy floor,
  so the pre-existing E4000 below-range wedge persists; that tuner is
  discontinued and out of scope.
- At extremely wide zooms the grid phase can skip over 0, leaving no
  `0.0` marker (labels still stop either way).
- Offset (upconverter) changes don't re-clamp the frequency, a
  pre-existing gap unchanged by this work.

## Outcome

Implemented as planned. Retire this synopsis on merge.

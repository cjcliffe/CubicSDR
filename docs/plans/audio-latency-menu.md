## Goal

Add an audio-latency selector as a submenu under Settings, with a few buffer-size levels defaulting to today's behavior (1024 frames, ~21ms at 48kHz), so users on saturated boxes can trade latency for underrun margin.

## Success Criteria

- Settings menu gains a latency submenu; switching applies live with only a brief dropout (same UX as the existing audio-sample-rate switch).
- Fresh config behaves exactly as today (1024 frames) — no latency change without user action.
- Choice persists across restarts; C++ and build suites stay green.

## Approach

Mirror the existing audio-sample-rate path end to end: menu → static setter → command queue → controller stream reopen. The reopen already does stop/close/open/start plus `resetAllPrebuffers()`; a latency change is the same operation with a different `nBufferFrames`. Global setting (all devices), not per-device — simpler, and latency is a user tolerance, not a device property.

Levels: 512 / 1024 / 2048 / 4096 frames. Each label shows the frame count plus the real latency computed from the current output device rate, e.g. `1024 (~21ms at 48kHz)`. Honest expectation: bigger buffers absorb small event jitter, not 150ms pile-on stalls — physics, not a defect.

## Steps

1. **Config**: add `get/setAudioBufferFrames` accessor pair on the app config object (pattern: `setRecordingFileTimeLimit`), default 1024, persisted with existing settings.
2. **AudioThread**: add a static desired-buffer-frames value applied at `openStream`; add an `AUDIO_THREAD_CMD_SET_BUFFER_FRAMES` command handled by the controller thread — stop/close/open/start with the new size plus `resetAllPrebuffers()`, mirroring `setSampleRate` minus the demod-rate loop (buffer size doesn't change demod rates; `mixVoice` already takes frame count as a parameter and its stall margin is time-based).
3. **Menu**: add an Audio Latency submenu under Settings where the menu is rebuilt (alongside the existing frame/DSP/pacing submenus), radio items, current level checked. Labels are computed as `<frames> (~<ms>ms at <rate>kHz)` from the active demod's output device rate (rounded ms with `~` prefix); fall back to the default output device rate, then to a 48kHz reference when no device is open.
4. **Label freshness**: refresh the labels when the menu opens (menu-open handler recomputes from the live device rate), so changing the audio sample rate can't leave stale ms values.
5. **Wiring**: menu action stores the config value and dispatches the command to every live controller (global setting, unlike the per-device rate menu).
6. **Edge handling**: apply the stored value when a controller first opens its stream (covers device start after a cold launch); confirm DirectSound honors the requested size at open.

## Validation Plan

- Build with the SDRPlay module set, then the C++ suite and `test_build.py` all passing.
- Live on Windows with SDRPlay: open Settings, confirm the latency submenu with computed labels; switch levels mid-stream, confirm brief dropout then clean audio at each level; confirm the highest level audibly reduces realistic-load skips versus 1024.
- Labels: at 48kHz device rate confirm `1024 (~21ms at 48kHz)` etc.; switch the audio sample rate, reopen the menu, confirm the ms values recompute.
- Restart the app, confirm the level persists and the menu check matches; delete config, confirm fresh default is 1024.
- Highest-risk check: the live switch while demods are running — must not wedge the controller or desync producers (time-based margins should make this a non-issue, but ears confirm it).

## Risks / Open Questions

- Risk: 512 frames will underrun *more* on a loaded box — inherent to the option; the ms labels make the tradeoff visible.
- Risk: none to existing behavior — the default path is untouched.
- Open questions: none.

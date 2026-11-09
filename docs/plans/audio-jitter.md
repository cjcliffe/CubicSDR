# Audio Jitter on App Switch

Merged record: branch `audio-mixer-model` into `ux_cleanup_v0_3`. Symptom, proven mechanisms, fix stack, dead ends, the CPU-saga outcome, and deferred items. The mixer work stands on ears-first verification; the CPU detour below concluded environmental and changed no code.

## Outcome

Final stack: resume play-through + 150 ms prebuffer + strand fix + time-based drain + above-normal process class + headless `AudioMixer` model with unit tests (`tests/test_AudioMixer.cpp`). The MMCSS guard was evaluated and removed (see Dead ends). Design docs updated (`audio-subsystem.md`, `threading.md`).

Measured live: 2 MHz RTLSDR single FMS silent under rapid switching (was 1–2 s gaps per switch); 10 MHz SDRPlay clean alone up to about 10 FMS streams; residual gaps under heavy VSCode activity at high load declared machine wall for this unit — the correlation probe was never built.

Branch history: `e80be44` (resume play-through), `74c9cec` (mixer model, prebuffer, strand fix, time drain, tests). The `audio-prebuffer`, `audio-producer-priority`, and `audio-depth-probe` pointers plus the MMCSS-guard and depth-probe stashes are superseded and droppable.

## Symptom

Brief audio gaps when switching between CubicSDR and other apps, worst on the CubicSDR-to-VSCode path; the visual pause happens both directions (DWM composition) while the audio gap mostly fires when leaving CubicSDR. Before the time-based drain there was also an occasional skip-ahead at high rates. Damage scales with load: device sample rate, simultaneous stream count, and other-app activity (VSCode text selection sustains it, not just momentary spikes). Motivating use case: unattended multi-channel feeds into decoders and I/Q consumers must never lose a sample.

## Proven mechanisms

- Thin mixer margin. The mixer input queue rides a few buffers (tens of ms); any producer stall longer than the margin gaps. Reference: 1024-frame RtAudio callbacks run about 47–48 per second, so each skipped callback is about 21 ms of silence.
- Resume amplifier. On starvation `currentInput` nulled; the next callback drained the whole queue to newest and skipped playback: a full silent callback plus destroyed margin per event. Fixed by draining only deep backlog and playing immediately.
- Empty-queue stranding. The mixer skipped the entire callback on momentary queue-dry even with playable samples still in `currentInput`. Bursty production (one buffer per IQ block against 21 ms callbacks) made this fire constantly under load. Fixed: an empty queue alone never skips, since every pop site already handles one.
- Count-cap truncation and skip-ahead. Draining to a fixed buffer count kept too little at high rates (small buffers truncated the lead) and deleted live audio whenever bursty production sat above the cap mid-stream (skip-ahead). Fixed: the drain keeps about 200 ms by production timestamp at any rate.
- One-shot lead dynamics. The prebuffer lead burns on the first stalls and only rebuilds via upstream catch-up in quiet gaps (the demod runs faster than real-time when unblocked). Stall-every-second bouts keep it pinned near zero; it recovers when switching stops (observed). Squelch-idle also drains it, and the first transmission after bind misses about 150 ms of head while the lead builds (one time).
- Healthy callback. RtAudio 5.2.0 DirectSound backend; its output-underflow bit is real cursor-based xrun detection and always read zero with steady callback counts, so the device is fed on time and the gaps are silent mixer data.
- Saturation model. Damage tracks total pressure: one or two streams shrug VSCode off, eight are stable alone but garble under VSCode, nine or ten lag alone (pre-MMSS). VSCode stays usable during our gaps, so cycles exist that our chain cannot convert — pointing at blocking or bandwidth rather than descheduling.

## Fix stack (all measured live)

- Resume play-through (committed): long pauses became brief jitters.
- 150 ms prebuffer (per-mixer hold, reset on bind and on stream restart): covers single transients; costs monitoring latency only, since recording and I/Q feeds use separate queues.
- Strand fix: jitter became fewer distinct skips.
- Drain cap raise, then time-based drain: full lead at any rate; skip-ahead gone.
- Headless mixer model with unit tests: the callback keeps locking, thread state, and device rate; all per-voice logic moved into testable `AudioMixer::mixVoice`/`normalize`.
- Above-normal process class on Windows: marginal on the VSCode band; other apps stay usable, no hogging observed.
- MMCSS "Pro Audio" guard on the sample-path threads: removed again (see Dead ends).

## Dead ends (with cause of death)

- MMCSS for switch spikes: spikes are saturation, where all our threads starve equally.
- MMCSS guard as a whole: harmful, removed. Elevating producers while visual consumers stay at normal priority inverts the `try_push` drain loops: spinners hog cores while the threads that would drain the queues starve. Measured effect was CPU way up and hard waterfall lag. Per-thread priorities stay default; in-process ordering cannot be bought this way while producers shed load by spinning.
- Process class for the VSCode band: marginal; with VSCode usable, the problem is not preemption.
- RtAudio update, WASAPI switch, bigger device buffers: the device never starved (underflow bit zero), so delivery is not the fault. Modernization stands on its own, not as this fix.
- Dropout concealment (repeating audio across gaps): hides gaps while duplicating or losing samples — the wrong direction for decoder feeds.
- Linux realtime priorities: deliberately untouched (Windows-only testing; CI builds but never runs audio).
- Task Manager CPU readings: coarse sampling aliases against bursty loads; identical states read differently run to run. Kernel CPU times and TMOG trusted instead.
- "Warmup gap" app theories in sequence: clock governor (frequency flat across the gap), paging/OS warmup (faults flat), starved demod-visual thread (toggling visuals moves nothing), slow startup task (3-minute no-touch run stayed high), pacer spin (loop plus pacing model prove at most a 2 ms sleep-poll). All killed by measurement; see CPU outcome.
- The `currentModem`-null strand after delete: real (delete paths null the selection and nothing re-selects a survivor, freezing the demod view), but toggling visuals moves CPU by nothing — a UX defect, not the CPU cause. Deferred, not dead.

## CPU outcome (environmental, no code change)

Per-thread kernel times plus a Process Explorer thread stack named the ~8 % burner: an NVIDIA OpenGL driver worker spinning in `nvoglv64.dll!DrvPresentBuffers`, born at first GL use, wedged by a startup present-handoff race. A focus switch (or any composition/scheduling perturbation) parks it in a proper wait — one way, hence the "warmup gap" shape. The waterfall flows throughout; the spin is pure waste off the critical path. Not our code; nothing to fix for correctness. Possible mitigations if ever wanted: Threaded optimization off in the driver profile, fewer `SwapBuffers` on hidden/static canvases, a docs note for the quirk.

## Deferred

- Re-select a survivor on delete instead of stranding a null selection (both delete paths; care for the terminate-all loop). Freezes the demod view and detaches selection-targeted controls today.
- The UI-tick versus mixer-dry correlation probe was never built; the 10 MHz VSCode residual stands as machine wall for this unit.
- Above-normal process stays unconditional with no config toggle; running hungry decoder apps alongside is untested.

## Notes (latency, UX, policy)

- The 150 ms monitoring latency buys single-transient cover; raise it toward 200–300 ms only with stall-length data, since sustained bouts need seconds of lead, which is not buyable.
- A background-visual throttle would freeze the unfocused window; that UX needs sign-off before implementing.
- Callback-behavior unit tests remain deferred; coverage is the doctest gate plus live listening.

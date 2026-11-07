# UX cleanup v0.3.0

## Goal

Discovery and clean-up for improvement of UX as the base for the next major
release version.

## Review Coverage

Issues surfaced in the review and feedback session, and what each fix
established:

- Hover re-routed visuals and selection, leaving the active demodulator
  ambiguous — hovering no longer drives anything but local canvas state.
- The waterfall crawled with sharp intermittent jumps — it now advances
  evenly at the requested lines-per-second with no fast-forward bursts after
  stalls and no starvation pauses.
- Animation froze while menus were open — the frame timer keeps ticking
  inside modal menu loops.
- Sliders had no reset shortcut, and the bookmark panel toggle was
  menu-only — double-click restores documented defaults and Ctrl+B shares
  the menu toggle path.
- Module loading rebuilt the device tree on every timer tick, and probing
  blocked the UI with the pre-click selection still painted — the tree now
  holds a static placeholder, selection paints before probing with busy
  feedback, and the layout lives in the form project.
- New demodulators played seconds of stale audio behind the visuals — the
  audio callback now drops backlog to the newest buffer on every fresh
  start.
- Right-dragging gain on the waterfall snapped back on release while the
  spectrum held — drag distance is now tracked by hand and right-click
  resets both views together.
- Zooming the waterfall needed the wheel — middle-drag now glide-zooms with
  a distance-proportionate rate.
- Default band data was wrong and wide ranges overshot small devices — 17m
  corrected to the ITU allocation, microwave and utility bands added, and
  range selection clamps to the sample rate.
- The frame pump ran full rate with nothing to show — the timer now parks
  at an idle rate with no device streaming, and follows the Frame Rate
  setting otherwise.
- The devices dialog had no way back to factory settings, forgot the last
  device, and titled the window with the driver label — Reset Properties
  restores defaults including the live sample rate, Start doubles as Stop,
  rename syncs the tree and title, and the active or last-used device is
  pre-selected with persistence across restarts.
- One CPU-usage option drove both pacing and DSP quality — frame rate
  (Low/Normal/High) and DSP mode (Normal/Enhanced) are now separate
  settings, with the old key migrating to both.
- The devices dialog dismissed on Start and had no keyboard path back —
  Start and Stop stay open with status-line progress, ESC and double-click
  dismiss, and Ctrl+D opens or focuses the dialog; the startup-shown
  dialog still dismisses on Start while manual re-opens stay up.
- Switching devices left the spectrum and waterfall at the old sample
  rate — device start now primes the SDR thread with the new rate and
  antenna before launch.
- Large device MTUs stalled the stream into burst-and-gap delivery —
  Soapy reads are now sliced to the frame size so blocks go out evenly
  at the display rate regardless of MTU.
- Sliced reads still delivered blocks in lumps with dry gaps between —
  SDRPacerThread meters full frames at a fixed gate so DSP, visuals, and
  scope advance together, with a Stream Pacing toggle to bypass it.
- Rate switches played wrong-speed audio and left the waterfall
  permanently lagged — the driver ring drains before the switch, the new
  rate applies at a block boundary, visual pushes drop oldest, and stale
  visual backlog flushes on stream changes.
- Fast tuning left the display ahead of the hardware — the flag clear
  acknowledged tunes requested during the slow driver call, so the tune
  flag now clears before the call and mid-call tunes stay pending for
  the next turn, with one tune per turn keeping drags live.
- The macOS audio-thread cleanup joined the wrong thread with the wrong
  API — the audio thread is a `std::thread` on every platform, so the
  platform split is gone and the join/delete path is now shared.
- Reset Session produced FM demods instead of FMS — the mode selector
  matches by 1-based value, so the reset selected FM and the frame sync
  copied it back over the FMS default; the reset now selects by name.
- Switching to a narrower device crashed on a stale rate, and any
  driver throw during start/stop killed the process — the pending rate
  is adopted before configuring, lifecycle driver calls are contained,
  and every mid-stream flag clears before applying.
- Stopping the device spooled stale waterfall frames for a second or
  two — `stopDevice` never flushed the SDR-to-post backlog that
  `setDevice` already flushed; it now discards it so downstream stops
  with the device.
- Tuning left the waterfall behind the knob — queued lines carry the
  old frequency, so every retune now drops the waterfall backlog and
  the display tracks live input instead of spooling history.
- Low frame rates could not honor high line rates — the insertion cap
  assumed ~20fps pacing, so it now scales with measured frame time
  (with an anti-burst ceiling) and the slider stays truthful.

## Work Plan

Landed units:

- [x] Remove hover re-route of demodulator visuals (`6e7c004`,
  `b5cba36`) — collapse `DemodulatorMgr` to a single `currentModem`; keep
  hover as local canvas state only; visual queue follows selection with
  detach on deselect; keep the ghost preview alongside selection.
- [x] Steady inactive selection rendering
  - active demod holds its normal look while aiming select/create; red
    reserved for an imminent move
  - hover tint distinct from selection; spectrum selection uses the yellow
    ghost tint
  - zero the wash alpha on leave to kill the flash-fade
- [x] Hover-targeted quick keys, record button, tooltips (`a019d13`)
  - `M`/`V`/`R` act on the hovered demod in the waterfall, with
    active-demod fallback elsewhere
  - `D` hover-only; `C` hover-or-cursor; nudge `[`/`]` hover in the
    waterfall with active fallback outside
  - `V` works everywhere; green `R` record button above the `M` mute
    button
  - descriptive tips with key names for Peak/Mute/Solo/Record/Pin
- [x] Single-selection design docs (`cb803d2`).
- [x] Bookmark in-place refresh (`b9ad1e1`) — refresh rows in place
  instead of rebuilding the tree on every nudge.
- [x] Frame-timer pacing (`9e476ea`, `fe649df`, `b96e786`, `28ac78b`)
  - one timer drives `processFrame` plus ordered canvas updates instead of
    idle events
  - uncapped swap interval applied per GL context; 1ms timer resolution
    on Windows
  - ticks from a multimedia timer after measurement showed `WM_TIMER`
    firing a 33ms request at ~47ms here
  - ticks delivered via a window message so animation continues inside
    modal menu loops
- [x] Waterfall consumer hardening (`b96e786`)
  - cap insertions per frame relative to the requested rate
  - forgive banked catch-up credit instead of bursting; bound the backlog
    with oldest dropped
  - fix wrap-crossing uploads to fill newest-first from the new ring top
    (`WaterfallPanel`)
- [x] Slider double-click reset (`02848d9`) — optional `MeterCanvas`
  default restored on double-click with the trailing release suppressed;
  waterfall speed (30 lps), spectrum averaging (0.65), demod gain (1.0),
  squelch off (-100, below any live floor).
- [x] Ctrl+B bookmark toggle and squelch-off reset (`55d8347`) — shared
  `toggleBookmarks()` path for the Display menu item and Ctrl+B (menu label
  shows the shortcut); squelch double-click default is off (-100, below any
  live floor); handler claims only the B key below the modem-props/bookmark
  focus pass-throughs. Live-verified.
- [x] Design docs updated for this branch: frame timer (threading),
  bookmark panel toggle and in-place refresh (bookmark system), slider reset
  defaults (visual rendering); retired hover-reroute plan removed, its
  decisions recorded in the checklist above.
- [x] `tickFrame()` declared on all platforms (`93e88e6`) — the declaration
  sat in a Windows-only guard while the caller was shared, breaking the
  Linux build.
- [x] Steady devices dialog (`eb3fcec`, `8929484`, `2ade52f`)
  - tree holds static "Initializing" while modules load; status line keeps
    the live progress
  - selection paints before probing with busy-cursor feedback during the
    blocking hardware probe
  - list fixed at minimum width, property grid takes extra space, dialog
    floor and grid minimum declared in the form project
- [x] Audio backlog drop on stream start (`f17a40b`) — callback drains to
  the newest queued buffer, so playback starts live; no-op in steady
  streaming.
- [x] Waterfall gain reset parity (`2286ec9`)
  - right-click without drag resets the shared scale factor via the
    spectrum canvas, easing both views back together
  - drag distance tracked by hand since the pointer warp pins the origin
    delta; status tip matches the spectrum wording
- [x] Middle-drag glide zoom (`e7adb01`)
  - holding glides the zoom at a rate accelerating with vertical distance from
    the press point, up zooms in and down zooms out
  - releasing or leaving the window stops at the current bandwidth
- [x] Band defaults and range clamp (`b4820b7`) — 17m corrected to
  18.068–18.168 MHz; 9/6/3/1.2 cm plus CB, FM, airband, marine, and weather
  bands added; range selection centers on the middle and caps width at the
  sample rate.
- [x] Record-button pulse and no-path gating (`7cfa707`) — green `R` breathes
  between its off and on looks on a shared two-second cycle with the
  spectrum recording glow; the button dims and swallows clicks until a
  recording path is set, with the tip explaining what is missing and no
  dialog opened from the frame tick; spectrum glow phase fixed to the
  same cycle with integer-millisecond precision.
- [x] FM broadcast startup defaults (`2d04a8a`) — center 98 MHz,
  the middle of the FM broadcast band; default demod type stereo (`FMS`),
  including the Reset action; `FMS` de-emphasis default 75us
- [x] Demod-view canvas keys (`47fcc9a`) — `D`/`C`/`E` (and `B` on its
  spectrum) work with the cursor in the demod waterfall/spectrum views
  via the extended key re-dispatch chain, matching the status tips
- [x] Waterfall pacing model (`73e218a`) — `processInputQueue`
  scheduling extracted to headless `WaterfallPacer` with doctest coverage
  of the cap, backlog bound, and credit-forgiveness rules
- [x] Waterfall ring assembly (`a3ce133`, `684dc20`) — wrap geometry
  extracted to headless `WaterfallRing` in `visual/` with doctest coverage
  of the assembly rules; canvas keeps sizing checks and GL upload.
- [x] Idle-aware frame pump (`59ae542`) — timer parks at a low rate with no
  device streaming, following the Frame Rate setting otherwise.
- [x] Devices dialog reset (`cf73a10`)
  - `DeviceConfig::resetToDefaults()` restores factory state, keeping the
    device id; reset stores the concrete nearest-to-default sample rate so
    it applies instead of falling back to the live rate
  - Reset Properties sits under the property grid and defers past click
    completion via the device timer; reset mirrors the per-field edit path
    so active devices update live
  - PPM and AGC added to General Settings; Start doubles as Stop on the
    active device with label sync
  - rename syncs the tree row and (when active) the window title, which
    uses the custom name via a shared title helper
  - dialog pre-selects the active device, else the last started one
    (`window/last_device`, persisted across restarts)
  - config file written only on close; eager saves removed, so a crash
    reverts to the last saved state
- [x] Frame rate / DSP split (`b9da6dd`) — separate Frame Rate (Low 15fps,
  Normal 30fps, High 60fps) and DSP (Normal/Enhanced channelizer) settings
  menus; legacy `perf_mode` migrates (Low to frame Low, High to frame High
  plus DSP Enhanced).
- [x] Devices dialog stay-open (`3fc26f3`)
  - Start/Stop keep the dialog open with Starting/Stopping status, settling
    to Stop/Streaming once the stream thread reports active
  - ESC dismisses via a character hook, except while editing a property
  - tree double-click starts and dismisses, or just dismisses when active
  - Ctrl+D opens/focuses the dialog with a menu shortcut label; repeat
    clicks inside the double-click window act once
  - startup-shown dialog dismisses on Start; manual re-opens stay open
    for further tweaks
- [x] Device-start thread priming (`655dc4e`) — sample rate and antenna
  forwarded to the SDR thread object before launch, so a switched device
  no longer inherits the old rate; empty antenna keeps the driver default.
- [x] Frame-sized Soapy reads (`399785f`) — each read requests only what
  the frame still needs instead of a full MTU, so the read itself paces
  at the display rate; the shrink-your-driver-buffers workaround and its
  TODOs are gone.
- [x] Stream pacing and rate-switch hygiene (`9f5626c`)
  - `SDRPacerThread` meters full frames at a fixed 60Hz gate between
    `SDRThread` and `SDRPostThread`; waits on input when dry, drop-oldest past
    cap, bypass mode for the toggle, selective flush on discontinuity
  - headless `SDRPacer` model with doctest coverage of gating, backlog
    bound, and credit-forgiveness rules
  - pre-switch non-blocking driver drain plus block-boundary rate apply,
    ending wrong-speed audio bursts across switches
  - drop-oldest visual pushes plus visual flush on stream reinit, ending
    permanent waterfall lag
- [x] Stream-pacing design docs — signal flow (new stage, queue wiring,
  drop-oldest visuals), thread inventory, and the `stream_pacing`
  config key.
- [x] Tune-flag clear-before-apply — the worker clears `freq_changed`
  before the slow driver call, so tunes requested mid-call stay pending
  for the next turn and rapid drags converge on the latest frequency;
  one tune per turn keeps the stream live mid-drag, and a post-tune
  drain discards old-frequency backlog so the display never lags the
  knob.
- [x] macOS audio-thread join fix — the copy-pasted
  `pthread_join(t_PreDemod)` in the audio cleanup is replaced by the
  shared `t_Audio` join/delete; the stale Known Issues entry is gone.
- [x] Reset Session FMS parity — the reset selects the mode by name
  instead of a magic 1-based value, so new demods after a reset are
  FMS like everywhere else.
- [x] Device lifecycle robustness
  - `init()` adopts the pending rate before configuring, so a switch
    never blasts the previous device's rate at the new driver
  - init sequence, `deinit()`, and the rate block's
    deactivate/activate contain driver throws (log + graceful failure
    instead of `std::terminate`)
  - `drainStream()` skips fresh unallocated streams; `updateGains()`
    moved inside the read-loop guard
  - all mid-stream flags clear before applying, closing the lost-update
    race for antenna, offset, rate, PPM, AGC, gains, and settings
- [x] Flush backlog on device stop — `stopDevice` requests the same
  pacer/input/output flush `setDevice` already did, plus the waterfall
  FFT pipeline flush, so the waterfall freezes instead of spooling
  stale frames after stop.
- [x] Flush waterfall backlog on retune — `setFrequency` drops the
  distributor, FFT, and canvas queues via a new pipeline flush, so the
  display tracks the knob instead of spooling old-frequency lines.
- [x] Time-scaled waterfall insertion cap — the per-frame cap covers
  the frame's time share plus one banked line (floored, with a
  twice-steady ceiling against hitches), so maximum lines-per-second
  renders fully even at the lowest frame rate; pacer tests updated
  and extended.
- [x] Demod-type re-click reset (`e0266f5`) — clicking the active type
  drops its saved settings and bandwidth and rebuilds the modem with
  defaults; the selector reports the re-click and the tips mention it.
  Live-verified.

## Validation Plan

- `python build.py --skip-deps` links clean; `cubicsdr_tests.exe` passes the
  full doctest suite.
- Live pass with a device:
  - hover vs click vs SHIFT-create vs drag vs delete; TAB selection
  - M/V/R/D/C and nudge on hovered vs active vs empty waterfall
  - squelch-break steal; rig follow
  - bookmark nudge without tree flashing
  - menu-open animation; waterfall evenness under drag and load
  - slider resets; Ctrl+B
  - low/normal/high frame rates
  - frame pump at rest with no device; streaming and minimized behavior
  - devices dialog reset idle and streaming; Start/Stop cycling; rename in
    tree and title; auto-select on open and across restart
  - switching devices across sample rates; views track the new rate
  - high sample rates at default driver MTU; smooth visuals without
    shrinking driver buffers
  - Stream Pacing on vs off; rate switches up and down; waterfall live
    after the switch with no stale backlog

## Risks / Open Questions

- Almost all timing evidence is from one Windows machine; the 47ms `WM_TIMER`
  behavior and the multimedia-timer fix need confirmation on other systems,
  especially Linux/macOS compositors and vsync handling.
- The multimedia timer keeps the process timer quantum at 1ms; confirm no
  adverse power/CPU impact on laptops.
- High line-rate settings depend on sample rate for actual throughput; the
  consumer cap keeps motion smooth but cannot invent lines the samples do not
  support.

## Amendments

Chronological log of branch changes; newest last. The Work Plan checklist
above always reflects the current state.

- Hover re-route removal (`6e7c004`, `b5cba36`): single-selection model in
  `DemodulatorMgr`; ghost preview kept alongside selection. Live-verified.
- Selection rendering: active demod holds its normal look while aiming a
  select or SHIFT-create; red means an imminent move; spectrum selection
  uses the yellow ghost tint; wash alpha zeroed on leave. Live-verified.
- Quick keys, record button, tooltips (`a019d13`): hover-targeted `M`/`V`/`R`
  with active fallback outside the waterfall, hover-only `D`,
  hover-or-cursor `C`, nudge with active fallback outside, global `V`,
  green `R` button, descriptive tips. Live-verified.
- Single-selection design docs (`cb803d2`).
- Bookmark in-place refresh (`b9ad1e1`): nudge no longer rebuilds the tree.
  Live-verified.
- Frame-timer pacing (`9e476ea`, `fe649df`, `28ac78b`): one timer driving
  `processFrame` plus ordered canvas updates, uncapped swap, modal-safe tick
  delivery. Live-verified.
- Waterfall consumer hardening (`b96e786`): rate-relative insertion cap,
  forgiven catch-up credit, bounded backlog, wrap-order upload fix.
  Live-verified.
- Slider double-click reset (`02848d9`): waterfall speed, spectrum averaging,
  demod gain, squelch defaults. Live-verified.
- Ctrl+B handler narrowed to claim only the B key and moved below the
  modem-props/bookmark focus pass-throughs, so other Ctrl combinations keep
  their previous behavior.
- Design docs updated for the frame timer (threading), bookmark panel toggle
  and in-place refresh (bookmark system), and slider reset defaults (visual
  rendering).
- Removed the retired hover-reroute plan; its decisions (ghost alongside
  selection, distinct hover tint, squelch-break steal kept, detach on
  deselect) are recorded in the checklist above.
- Ctrl+B bookmark toggle and squelch-off reset (`55d8347`): shared toggle
  path, menu shortcut label, squelch-off default; handler narrowed post-
  verification (claims only B, below focus pass-throughs), preserving the
  verified path. Live-verified.
- Detach-on-deselect confirmed fine as-is; menu-open cadence confirmed
  steady (will re-raise if it recurs on another platform); scope question
  dropped — additions by request.
- `tickFrame()` declared on all platforms (`93e88e6`) after the Linux CI
  build failed without it.
- Devices dialog steadied (`eb3fcec`): static placeholder tree, paint-first
  selection, busy feedback during probes. Live-verified.
- Dialog layout declared in the form project (`8929484`, `2ade52f`):
  minimum-width list, expanding parameters, dialog floor. Live-verified.
- Audio backlog drop on stream start (`f17a40b`): newest buffer wins.
  Live-verified.
- Waterfall gain reset parity (`2286ec9`): right-click resets both views;
  drag distance tracked by hand past the pointer warp. Live-verified.
- Middle-drag glide zoom (`e7adb01`): accelerating rate, stops on release.
  Live-verified.
- Band defaults and range clamp (`b4820b7`): 17m corrected, microwave and
  utility bands added, view capped at the sample rate. Live-verified.
- Record-button pulse and no-path gating: green `R` breathes between its
  off and on looks on a shared two-second cycle with the spectrum
  recording glow; the button dims and swallows clicks until a recording
  path is set, with the tip explaining what is missing and no dialog opened
  from the frame tick; spectrum glow phase fixed to the same cycle.
  Live-verified.
- FM broadcast startup defaults (`2d04a8a`): 98 MHz center, stereo default
  demod, Reset action parity, 75us de-emphasis default. Live-verified.
- Demod-view canvas keys: `D`/`C`/`E` re-dispatched to the demod
  waterfall canvas under the cursor, `B` to its spectrum; matches the
  hover status tips. Live-verified.
- Waterfall pacing model: insert/drop schedule unit-tested;
  canvas keeps the queue drain, sizing checks, and GL upload.
- Waterfall ring assembly (`a3ce133`, `684dc20`): wrap geometry extracted
  to headless `WaterfallRing`; canvas keeps sizing and GL upload.
  Live-verified.
- Idle-aware frame pump (`59ae542`): timer parks at a low rate with no
  device, following the Frame Rate setting when streaming.
  Live-verified.
- Devices dialog reset (`cf73a10`): factory-default reset with deferred
  apply, PPM/AGC settings, Start/Stop toggle, rename sync, active/last
  pre-select with persistence, close-only config writes. Live-verified.
- `MeterCanvas` init-list order fixed (`487d999`): declaration order
  restored, silencing the `-Wreorder-ctor` from the slider-reset members.
- Packaging artifacts collected by glob (`b7d1dfb`): each job moves its
  versioned output into `dist/` via basename substitution, so CI carries
  no hardcoded package version after the 0.3.0 rename break.
- Windows liquid-dsp from source (`e2b2188`, `b063b51`, `81344a6`):
  VS-bundled clang-cl builds v1.8.3 static for x64 and Win32; vendored
  binaries removed, no DLL in the link or installer. Live-verified.
- Frame rate / DSP split (`b9da6dd`): separate pacing and channelizer
  settings with legacy key migration.
- Devices dialog stay-open (`3fc26f3`): status-line Start/Stop progress,
  ESC and double-click dismiss, Ctrl+D shortcut, click debounce.
  Live-verified.
- Device-start thread priming (`655dc4e`): new rate and antenna reach the
  thread object before launch; empty antenna keeps the driver default.
- Frame-sized Soapy reads (`399785f`): reads sliced to the frame so a
  large MTU bounds instead of setting the cadence. Live-verified.
- Stream pacing and rate-switch hygiene (`9f5626c`): fixed-gate frame
  metering with a unit-tested model, driver drain plus boundary rate
  apply, drop-oldest visuals with stream-change flush; signal-flow,
  threading, and configuration docs updated. Live-verified.
- Startup dialog dismisses on Start while manual re-opens stay open;
  `deviceSelector()` carries the mode per showing.
- Tune-flag clear-before-apply: mid-call tunes stay pending so fast
  drags land the hardware on the displayed frequency without stalling
  the stream mid-drag; post-tune drain drops old-frequency backlog so
  no lag accumulates behind the knob.
- macOS audio-thread join fix: shared join/delete path, stale Known
  Issues entry removed.
- Reset Session FMS parity: select-by-name instead of magic value.
- Device lifecycle robustness: pending-rate adoption on switch,
  lifecycle containment, drain guard, uniform clear-before-apply.
  Live-verified (aggressive stop/start/switch cycling, no crashes).
- Flush backlog on device stop: pacer/input/output plus the waterfall
  FFT pipeline discarded so the waterfall freezes with the device.
- Flush waterfall backlog on retune: display tracks the knob.
- Time-scaled waterfall insertion cap: requested LPS honored at any
  frame rate, hitch ceiling preserved, tests extended.

- Demod-type re-click reset (`e0266f5`): saved settings dropped,
  modem rebuilt with defaults. Live-verified.

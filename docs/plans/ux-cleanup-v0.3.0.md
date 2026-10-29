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
  - `D`/`C` and nudge `[`/`]` hover-only in the waterfall
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

Remaining units:

- [ ] Validate frame pacing and vsync behavior on Linux and macOS.
- [ ] Before the final PR, delete implementation plan docs from `docs/plans/`
  per repo convention, including this file once its record has served.

## Alternatives rejected (waterfall stutter)

The symptom throughout was the same: the waterfall crawled with sharp
intermittent jumps instead of advancing evenly. Each hypothesis below was
tested against that symptom and ruled out. Do not retry without new evidence:

- Texture filtering/alignment as the jump source: `GL_NEAREST` A/B showed no
  change in the jump pattern; reverted.
- Producer burst pacing as the jump source: capping insertions per frame
  showed no change on <=2-line frames, so the producer wasn't the source;
  superseded by the final consumer-side hardening.
- Repaint contention from other canvases as the jump source: waterfall-only
  repaints showed no change; reverted.
- `UpdateUI` event storm as the tick driver: throttling 8x showed no effect
  on the crawl; reverted.
- Upload-vs-draw race as the jump source: upload and draw share the UI thread
  and one mutex, so the render can't read a half-uploaded texture; ruled out
  by construction.

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
  - high/normal/low performance modes

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
  with active fallback outside the waterfall, hover-only `D`/`C`/nudge,
  global `V`, green `R` button, descriptive tips. Live-verified.
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
- Current state: `AppFrame` toggle/reset committed; design doc updates, old-
  plan removal, and plan updates committed; `tickFrame()` declaration moved
  out of the Windows-only guard (uncommitted) after the Linux CI build
  failed without it; rebuilt green with the unit suite passing.

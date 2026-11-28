# GPU Port Plan

Target state: the main window runs on bgfx for panels with SDL3 for windowing
and input plus ImGui for widgets and dialogs, with wxWidgets eliminated,
keeping current behavior and opening low-overhead targets (including X-less
Raspberry Pi via SDL's KMS/DRM path).

Early spikes used Raylib to validate the tested models; the renderer and
windowing choice has since moved to bgfx plus SDL while ImGui stays. Each
panel was proven by a spike under `spike/`, built with
`python build.py --spike <name>`; a spike passed when it looked and felt
like the app widget it mirrors. The panel spikes have been retired now
that their migrations are done, and the spike build harness stays so a
future spike is one dropped-in source file.

## Completed spikes

- Tuner (`spike/raylib-tuner`): tuning strip from `TunerStep`/`TunerLayout`
  plus `TuneRange` limits, with a minimal local state struct. Proved the
  models run with no wx/GL in the include path.
- Mode selector (`spike/raylib-modesel`): choice list from the `ModeSelector`
  model, including toggle, pulse, disabled, and highlight-override modes.
- Spectrum (`spike/raylib-spectrum`): spectrum view from `SpectrumTicks` with
  a synthetic carrier-plus-noise feed, pan, visual gain, dB labels, peak hold.
- Waterfall (`spike/raylib-waterfall`): scrolling view reusing the headless
  `WaterfallRing`, `WaterfallPacer`, `Gradient`, and `ColorTheme` sources
  unchanged, with fake demodulators driving the hover/drag/placement models
  and `DemodMarkers` overlays.
- Scope (`spike/raylib-scope`, partial): waveform and spectrum panels with a
  synthetic feed verified at rest; the 3D carousel posed through rlgl's
  implicit state never rendered reliably, which triggered the bgfx pivot.
- Scope on bgfx (`--spike bgfxscope`): SDL3 window with the bgfx OpenGL
  renderer drawing carousel-posed textured quads from `ScopeCarousel` with
  explicit glm matrices. Verified live: fit, rotation, color, drag feel.
- Tuner on ImGui (`--spike imgui`): custom ImGui backend over bgfx (texture
  protocol, transient buffers, Vera Sans Mono digits) drawing the full tuning
  strip via draw lists from `TunerStep`/`TunerLayout`/`TuneRange`, plus the
  ImGui demo window as widget proof. Verified live.
- Gain/meter on ImGui (`--spike gainmeter`): gain strips from `GainSlider`
  plus meter sliders with drag, wheel, double-click/middle-click default
  reset, and right-click squelch snap, reusing the backend and shaders
  unchanged. Verified live.
- Spectrum on ImGui (`--spike spectrum`): per-pixel fill columns,
  pre-blended grid, peak hold, and tracked tick labels from
  `SpectrumTicks`/`DemodMarkers`. Verified live.
- Waterfall on ImGui (`--spike waterfall`): ring-fed scrolling texture
  with hover/drag placement and demod overlays from the waterfall
  models. Verified live.
- Scope on ImGui (`--spike scope`): waveform/XY/constellation carousel
  panels with rotating text from `ScopeCarousel`. Verified live.
- Mode selector on ImGui (`--spike mode`): choice lists from the
  `ModeSelector` model with toggle, pulse, and disabled states.
  Verified live.

The panel spikes were removed after their migrations landed; git history
keeps them and the harness remains for future spikes.

## Portability findings

- The viewmodel headers, `TuneRange`, the waterfall ring and pacer, and the
  gradient/theme sources all compile standalone. Reuse them directly; do not
  re-derive their logic.
- Raylib defines `PI` as a macro, which collides with the model's typed
  constant. Decision: the model's constant moves out of the way (rename it);
  raylib's macro wins at the boundary.
- Raylib's additive blend mode matches the overlay blending the GL code uses,
  so marker and selector overlays transfer pixel-exactly.
- Non-view canvas getters derive from the device; the stored members only
  matter inside zoomed views and the zoom anchor math. Port the derivation,
  not just the members.
- The waterfall texture plan executes against any texture API: palette-expand
  each upload op on the CPU and draw the halves with wrap-aware rects.
- Cursor warping during drags is a Windows-only behavior; the equivalent state
  is freezing the hover at the press origin.
- Dialogs (direct frequency input, labels, device setup) and the top-level
  layout have no spike yet; they are the largest unknown.
- Raylib/rlgl fights custom render state: its implicit batching and matrix
  stack never reliably drew 2D shape calls under a custom modelview, while
  textured quads plus scripted-pose screenshots were the workable loop.
- Decision: bgfx for rendering (explicit views, programs, and state), SDL3
  for windowing and input, ImGui for widgets and panel content drawing. The
  Raylib spikes were removed at the pivot, and the ImGui panel spikes were
  removed after their migrations landed. Git history keeps all of them.
- Decision: CubicVR math stays stranded in the old shell (it was never
  finished or tested, and its quirks already bit once); all new code uses
  glm, and `external/cubicvr2` goes away with wx.
- Decision: commit precompiled shaders; the shaderc chain is opt-in via a
  build flag so clean builds skip the toolchain.
- The ImGui backend uses only public API: the `ImTextureData` texture
  protocol for the font atlas plus transient vertex/index buffers per draw
  list. Fonts bake lazily, so the backend must honor `WantUpdates` for
  glyphs first used after startup, not just the initial upload.
- Draw lists offer no blend control, so the tuner's additive bars transfer
  as opaque gradient quads with the background lift and under-color stack
  baked in; digits, grid, and snap box transfer exactly.
- Vera Sans Mono (the same family as the GL bitmap pages, embedded as
  a header) replaces the GL bitmap font for tuner digits; legibility
  verified live at strip size.
- bgfx marks textures created with initial data immutable and silently
  skips later updates in release builds, so the atlas is created empty and
  filled with a full-rect update; update memory must exactly cover the
  rect or the upload is skipped the same way. Missing late-baked glyphs
  are the symptom when either rule is broken.

## Phases

- [x] Spike the interactive controls (tuner, mode selector).
- [x] Spike the spectrum view with a synthetic feed.
- [x] Spike the waterfall with ring, pacer, and demod interaction.
- [x] Spike a bgfx plus SDL window drawing one textured quad with explicit
  matrices, re-proving the scope carousel motion on the new stack.
- [x] Spike the ImGui backend with the tuner drawn via draw lists.
- [x] Spike the remaining views (gain/meter) or defer them behind the state
  extraction if they stop teaching anything new.
- [x] Extract the application state interface the spikes specified: device and
  view frequencies, demodulator list and selection, visual gain, snap, theme,
  and status routing, behind an API both shells can drive.
- [ ] Spike one ImGui dialog against the state interface to prove the widget
  story before committing to it.
- [x] Decide the shell strategy: panel-by-panel replacement inside the
  existing frame.
- [x] Port panels in dependency order, keeping the wx app working throughout.

## State extraction

The spikes drive state that is currently scattered: device settings on
the app object, zoomed-view copies on each canvas with manual sync
between them, gains and meter inputs on frame children, theme in a
singleton, status text routed to the frame. The port needs it behind
one portable API both shells can drive. `DemodulatorMgr` is already
portable and stays as the demod list plus selection; scalar storage
moves into a new portable owner, and view access goes through the same
API with the app object delegating to the canvases:

- Device: frequency, sample rate, PPM, offset.
- Main view: center, bandwidth, zoomed-view set/reset. Unzoomed views
  keep falling back to device frequency and sample rate.
- Tuning aids: frequency snap.
- Gains: per-device gain values plus the changed flags.
- Meter inputs: the four slider positions (waterfall speed, spectrum
  average, demod gain, squelch) stay canvas-local; their consumers and
  mappings are already portable.
- Theme: current theme colors.
- Status: one text sink per shell.

Slices in dependency order; the wx app keeps working throughout:

1. Device plus view plus snap state (done): scalars moved into the
   portable owner; view API declared with canvas delegation; canvas
   zoom logic stays put for the new shell to reimplement on models.
2. Demodulator access plus a selection-change notification (done):
   `DemodulatorMgr` carries an atomic selection serial bumped on real
   selection changes, including delete fallback; the bump rule is a
   tested pure function.
3. Gains and meter inputs (done, no code): gain values and ranges
   already portable via the SDR thread and device info; slider
   positions stay canvas-local for the new shell to reimplement.
4. Theme and status sink (done): themes already portable and tested;
   status text added to `AppServices` with the frame status bar behind
   it, alongside the existing message box sink.

## Panel migration

Decision: incremental replacement inside the wx frame, not a parallel
shell. Each canvas becomes a bgfx view hosted in its window, reusing
the spike-proven view code; the wx frame, dialogs, and menus go last.
Rationale: every step ships a working app, and the hardest part
(dialogs, layout) gets maximum design time.

Hosting design:

- New wxWindow-based host: bgfx renders to the native handle, so
  migrating canvases rebase off wxGLCanvas (two GL contexts cannot
  share one window). bgfx inits headless; each host owns a framebuffer
  recreated on resize.
- One imgui context for all hosted views; one fixed borderless imgui
  window per view. Spike view bodies move over verbatim; wx events
  translate to the spike edge structs.
- Each host pumps `bgfx::frame()` from its own paint handler.
- Hosted views run without vsync: a vsync wait inside every paint
  stalls the UI thread and pauses the other canvases.
- View state flows through `CoreState` and `DemodulatorMgr`; zoom
  stays canvas-local.
- Where bgfx cannot start, the host paints a flat theme-background
  fill. macOS needs Metal shaders, which are blocked on the pinned
  shaderc until the bgfx update.

Order: tuner, gain/meter, spectrum, waterfall, scope, mode selector (all
done, with demod views). Spectrum, waterfall, and scope share the common
draw helpers.

## Open questions

- The frame, dialogs, menus, and layout are still wx; what order do they go in?
- Which platforms must the new shell cover on day one?

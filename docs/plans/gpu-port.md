# GPU Port Plan

Target state: the main window runs on bgfx for panels with SDL3 for windowing
and input plus ImGui for widgets and dialogs, with wxWidgets eliminated,
keeping current behavior and opening low-overhead targets (including X-less
Raspberry Pi via SDL's KMS/DRM path).

Early spikes used Raylib to validate the tested models; the renderer and
windowing choice has since moved to bgfx plus SDL while ImGui stays. Spikes
live under `spike/`, each built with `python build.py --spike <name>`. A
spike passes when it looks and feels like the app widget it mirrors.

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
  protocol, transient buffers, RobotoMono digits) drawing the full tuning
  strip via draw lists from `TunerStep`/`TunerLayout`/`TuneRange`, plus the
  ImGui demo window as widget proof. Verified live.
- Gain/meter on ImGui (`--spike gainmeter`): gain strips from `GainSlider`
  plus meter sliders with drag, wheel, double-click/middle-click default
  reset, and right-click squelch snap, reusing the backend and shaders
  unchanged. Verified live.

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
  Raylib spikes stay as model-validation records.
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
- RobotoMono (bundled with bgfx examples) replaces the GL bitmap font for
  tuner digits; legibility verified live at strip size.
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
- [ ] Extract the application state interface the spikes specified: device and
  view frequencies, demodulator list and selection, visual gain, snap, theme,
  and status routing, behind an API both shells can drive.
- [ ] Spike one ImGui dialog against the state interface to prove the widget
  story before committing to it.
- [ ] Decide the shell strategy: parallel bgfx shell alongside wx, or
  panel-by-panel replacement inside the existing frame.
- [ ] Port panels in dependency order, keeping the wx app working until the
  swap.

## Open questions

- Which views, if any, still need spikes before extraction starts?
- Parallel shell or incremental replacement?
- What provides dialogs and menus in the interim if panels move first?
- Which platforms must the new shell cover on day one?

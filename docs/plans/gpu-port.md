# Raylib Port Plan

Target state: the main window runs on Raylib for panels plus ImGui for widgets
and dialogs, with wxWidgets eliminated, keeping current behavior and opening
low-overhead targets.

This plan is driven by throwaway spikes under `spike/`, each built with
`python build.py --spike <name>`. A spike passes when it looks and feels like
the app widget it mirrors.

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

## Phases

- [x] Spike the interactive controls (tuner, mode selector).
- [x] Spike the spectrum view with a synthetic feed.
- [x] Spike the waterfall with ring, pacer, and demod interaction.
- [ ] Spike the remaining views (scope, gain/meter) or defer them behind the
  state extraction if they stop teaching anything new.
- [ ] Extract the application state interface the spikes specified: device and
  view frequencies, demodulator list and selection, visual gain, snap, theme,
  and status routing, behind an API both shells can drive.
- [ ] Spike one ImGui dialog against the state interface to prove the widget
  story before committing to it.
- [ ] Decide the shell strategy: parallel Raylib shell alongside wx, or
  panel-by-panel replacement inside the existing frame.
- [ ] Port panels in dependency order, keeping the wx app working until the
  swap.

## Open questions

- Which views, if any, still need spikes before extraction starts?
- Parallel shell or incremental replacement?
- What provides dialogs and menus in the interim if panels move first?
- Which platforms must the new shell cover on day one?

# Visual Rendering

This document describes CubicSDR's OpenGL rendering system, canvas hierarchy, GL panel system, font rendering, color themes, and rendering flow. For the visual data processing pipeline (VisualProcessor, distributors, FFT processing), see [visual-data-pipeline.md](visual-data-pipeline.md).

## Overview

The visual system is built on wxWidgets' OpenGL integration (`wxGLCanvas`/`wxGLContext`) and uses a **pull-based** architecture: worker threads produce FFT/scope data into queues, and UI canvases poll those queues in their `OnIdle()` or `OnPaint()` handlers. Worker threads never push data to the UI.

The visual data processing pipeline — how raw IQ samples become display-ready spectrum/scope data, the VisualProcessor template, and the FFT/scope processing internals — is documented in [visual-data-pipeline.md](visual-data-pipeline.md).

## Canvas Class Hierarchy

```
wxGLCanvas
  |
  +-- InteractiveCanvas (base: mouse tracking, frequency mapping, key state)
        |
        +-- WaterfallCanvas (waterfall display, demod creation, drag operations)
        +-- SpectrumCanvas (FFT spectrum display, linked to WaterfallCanvas)
        +-- ScopeCanvas (oscilloscope/spectrum display for demod output)
        +-- MeterCanvas (signal level meter)
        +-- TuningCanvas (fine tuning control)
        +-- ModeSelectorCanvas (modem type selection buttons)
        +-- GainCanvas (per-gain-stage slider)
        +-- UITestCanvas (test/development canvas in src/ui/)
```

### InteractiveCanvas (`src/visual/InteractiveCanvas.h`)

Base class for all interactive GL canvases. Provides:

- **Frequency mapping:** `getFrequencyAt(x)` converts a normalized x-position [0..1] to a frequency in Hz, given the current center frequency and bandwidth
- **View state:** `setView()` / `disableView()` for zoomed sub-band views vs. full-band views
- **Mouse tracking:** `MouseTracker` instance with position, button state, drag deltas
- **Key state:** `shiftDown`, `altDown`, `ctrlDown` flags updated on key/mouse events
- **Status bar:** `setStatusText()` writes to the AppFrame status bar

### WaterfallCanvas (`src/visual/WaterfallCanvas.h`)

The primary display canvas. Handles:

- **FFT data consumption:** Pops `SpectrumVisualData` from `visualDataQueue` at a rate-limited pace (`linesPerSecond`)
- **Waterfall rendering:** Delegates to `WaterfallPanel` for texture-based scrolling waterfall
- **Demodulator markers:** Draws all active demodulators via `PrimaryGLContext::DrawDemod()`
- **Frequency selector:** Draws hover/active frequency markers via `DrawFreqSelector()`
- **Drag operations:**
  - `WF_DRAG_FREQUENCY` — move demodulator frequency
  - `WF_DRAG_BANDWIDTH_LEFT` / `WF_DRAG_BANDWIDTH_RIGHT` — resize demodulator bandwidth
  - `WF_DRAG_RANGE` — create new demodulator by range selection
- **Zoom:** Mouse wheel adjusts `mouseZoom`, which smoothly animates to the target zoom level
- **Frequency nudge:** Arrow keys shift center frequency by half/full bandwidth
- **Visual gain:** Shift+Up/Down adjusts `scaleMove` (drives visual gain animation toward target scale factor)

**Drag state machine:**
```
WF_DRAG_NONE ──(mouse hover over demod)──> WF_DRAG_FREQUENCY / WF_DRAG_BANDWIDTH_*
WF_DRAG_NONE ──(Alt held)──> WF_DRAG_RANGE
WF_DRAG_* ──(mouse released)──> WF_DRAG_NONE
```

### SpectrumCanvas (`src/visual/SpectrumCanvas.h`)

Displays the FFT spectrum line plot. Linked to `WaterfallCanvas`:

- Shares the same center frequency and bandwidth
- Receives FFT data from its own `visualDataQueue`
- Supports dB scale display and dB offset
- Right-drag adjusts visual gain (vertical scale factor)
- Uses `SpectrumPanel` for rendering

### ScopeCanvas (`src/visual/ScopeCanvas.h`)

Displays demodulated audio as oscilloscope or spectrum:

- Pops `ScopeRenderData` from `inputData` queue
- Toggles between scope mode (waveform) and spectrum mode (FFT)
- Supports PPM mode for frequency calibration display
- Uses both `ScopePanel` and `SpectrumPanel` in a composite layout
- `GLPanel` hierarchy: `parentPanel` → `scopePanel` + `spectrumPanel` (children); `bgPanel` is drawn independently with its own identity transform before `parentPanel`

## GLPanel System (`src/ui/GLPanel.h`)

A lightweight retained-mode UI system for OpenGL rendering. Panels form a tree: each panel has position, size, fill, and optional children. The tree is traversed recursively each frame, with each panel computing its transform relative to its parent.

### Base Class Properties

```
GLPanel
├── pos[2], size[2], rot[3]       // placement in parent coordinates
├── fill[2] (RGBA4f)              // fill colors (solid or gradient endpoints)
├── fillType                       // rendering mode (see below)
├── borderColor, borderPx          // border color (RGBA4f) and per-edge border widths in pixels
├── marginPx                       // margin in pixels
├── coord                          // coordinate system (see below)
├── visible, contentsVisible       // visibility flags
├── srcBlend, dstBlend             // custom blend modes (GLuint)
├── children (vector<GLPanel*>)    // child panels
├── transform, transformInverse    // computed global matrices
├── localTransform                 // pos/size/rot composed
├── vmin, vmax                     // computed screen-space extents
├── umin, umax, ucenter            // computed unit-space extents
├── view[2]                        // viewport dimensions
├── min, mid, max                  // coordinate system extents (-1/0/1 or 0/0.5/1)
├── pdim, pvec                     // pixel dimensions and pixel vector
└── (private) glPoints, glColors   // pre-computed vertex/color arrays for fill rendering
```

**Derived panels:**
- `GLTextPanel` — text rendering with alignment (left/right/center, top/bottom); `useNativeFont` bypasses scale factor when true
- `GLTestPanel` — debug/test rendering

**Panel classes:**
| Panel | File | Purpose |
|-------|------|---------|
| `WaterfallPanel` | `src/panel/WaterfallPanel.h` | Scrolling waterfall texture rendering |
| `SpectrumPanel` | `src/panel/SpectrumPanel.h` | FFT line plot rendering |
| `ScopePanel` | `src/panel/ScopePanel.h` | Oscilloscope waveform rendering |
| `MeterPanel` | `src/panel/MeterPanel.h` | Signal level meter rendering |

### Fill Types

| Enum | Vertices | Description |
|------|----------|-------------|
| `GLPANEL_FILL_NONE` | 8 (allocated, not drawn) | Transparent — arrays allocated but `draw()` skips rendering |
| `GLPANEL_FILL_SOLID` | 4 (1 quad) | Single color (`fill[0]`) |
| `GLPANEL_FILL_GRAD_X` | 4 (1 quad) | Horizontal gradient: `fill[0]` (left) → `fill[1]` (right) |
| `GLPANEL_FILL_GRAD_Y` | 4 (1 quad) | Vertical gradient: `fill[0]` (bottom) → `fill[1]` (top) |
| `GLPANEL_FILL_GRAD_BAR_X` | 8 (2 quads) | Symmetric horizontal bar: gradient from edges to center |
| `GLPANEL_FILL_GRAD_BAR_Y` | 8 (2 quads) | Symmetric vertical bar: gradient from edges to center |

The bar fill types create a symmetric gradient: `fill[0]` at both edges, `fill[1]` at the center line. This is used for tuning bar indicators where the center value is the strongest.

### Coordinate Systems

The `GLPanelCoordinateSystem` enum controls how the [-1,1] unit square maps to screen space. Each system applies a different matrix to transform content coordinates before drawing:

| Enum | Mapping | Y Direction | Use Case |
|------|---------|-------------|----------|
| `GLPANEL_Y_DOWN_ZERO_ONE` | [0,1] → [-1,1] | Down | Waterfall — origin at top-left |
| `GLPANEL_Y_UP_ZERO_ONE` | [0,1] → [-1,1] | Up | Most panels — origin at bottom-left |
| `GLPANEL_Y_UP` | [-1,1] passthrough | Up | Raw unit coordinates |
| `GLPANEL_Y_DOWN` | [-1,1] flip Y | Down | Raw coordinates with Y flipped |

The coordinate matrix is applied after the panel's transform: `finalMatrix = transform * coordMatrix`. Content drawn in `drawPanelContents()` uses this coordinate space.

### Transform Pipeline: `calcTransform()` + `draw()`

The render pipeline executes in two phases per panel:

**Phase 1 — `calcTransform(parentTransform)`** (called by parent before drawing children):

1. Compose local transform: `localTransform = translate(pos) * scale(size)` (optionally `* rotate(rot)`)
2. Compute global transform: `transform = parentTransform * localTransform`
3. Read viewport: `glGetIntegerv(GL_VIEWPORT, vp)`
4. Transform coordinate-system corners through `transform` to get screen-space `vmin`/`vmax` (corners are `(min,min)` and `(max,max)` where min/max depend on the coordinate system: -1/+1 or 0/+1)
5. Compute pixel extents: `pdim = vec2((vmax.x - vmin.x) * 0.5 * vp_width, ...)`
6. Apply margin: shrink transform proportionally by `marginPx * 2 * pvec` in each axis (pixel-vector-scaled margin)
7. Compute `transformInverse = mat4::inverse(transform)` for hit-testing

**Phase 2 — `draw()`** (called recursively):

1. Load modelview matrix: `glLoadMatrixf(transform.to_ptr())`
2. If `visible` and fill type is not NONE:
   - Enable `GL_BLEND` with the panel's blend mode
   - Draw pre-computed vertex/color arrays via `glDrawArrays(GL_QUADS, ...)` (arrays built earlier when fill properties changed via `genArrays()` in `setFill()`/`setFillColor()`/`setCoordinateSystem()`)
   - If any border is non-zero: enable `GL_LINE_SMOOTH`, draw border edges with `glBegin(GL_LINES)`
3. If `contentsVisible`:
   - Build coordinate matrix from the panel's coordinate system
   - `glLoadMatrixf((transform * coordMatrix).to_ptr())`
   - Call `drawPanelContents()` (virtual — overridden by subclasses)
4. Base `drawPanelContents()` calls `drawChildren()`, which iterates `children` and calls `calcTransform(this->transform)` + `draw()` on each child

**Composition pattern**: The root panel's `draw()` is called once. It renders its own fill, then its `drawPanelContents()` triggers `drawChildren()`. Each child computes its transform relative to the parent and draws itself, creating a recursive tree traversal.

### Hit-Testing

`GLPanel::hitTest(vec2 screenPos, vec2 &localResult)` transforms a screen-space point into the panel's local [-1,1] coordinate system using the precomputed `transformInverse`. If the transformed point falls within the unit square, the hit succeeds and the caller receives the local coordinates.

Note: GLPanel hit-testing is used by `MeterPanel` (in `isMeterHit()` and `getMeterHitValue()`) for click-to-set-level interaction within `GainCanvas`. Other canvases (WaterfallCanvas, SpectrumCanvas, MeterCanvas, etc.) use `MouseTracker` with their own coordinate mapping instead.

## PrimaryGLContext (`src/visual/PrimaryGLContext.h`)

Shared OpenGL context providing drawing primitives. All canvases share this context via `wxGLContext` sharing. Uses legacy OpenGL fixed-function pipeline (no shaders).

### Drawing Methods

| Method | Purpose |
|--------|---------|
| `BeginDraw(r, g, b)` | Clear color+depth buffers, load identity modelview |
| `EndDraw()` | Finalize frame (currently empty — was `glFlush()`) |
| `DrawDemod(demod, color, center_freq=-1, srate=0)` | Draw demodulator bandwidth indicator with label |
| `DrawDemodInfo(demod, color, center_freq=-1, srate=0, centerline=false)` | Draw demodulator info label (frequency, type, bandwidth) |
| `DrawFreqSelector(uxPos, color, w=0, center_freq=-1, srate=0)` | Draw frequency selection marker (center line + BW edges) |
| `DrawRangeSelector(uxPos1, uxPos2, color)` | Draw range selection overlay (two vertical boundary lines) |
| `DrawFreqBwInfo(freq, bw, color, center_freq=-1, srate=0, stack=false, centerline=false)` | Draw frequency and bandwidth text with shadow |

### GL State Management

**`BeginDraw(r, g, b)`** (called at frame start):
```
glClearColor(r, g, b, 1)
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
glMatrixMode(GL_MODELVIEW)
glLoadIdentity()
```

**Drawing primitives use immediate-mode OpenGL:**
- `glEnable(GL_BLEND)` / `glBlendFunc()` — additive blending for overlays
- `glDisable(GL_TEXTURE_2D)` before line drawing
- `glBegin(GL_LINES)` / `glVertex3f()` — immediate mode for demod lines
- `glBegin(GL_QUADS)` / `glVertex3f()` — immediate mode for filled regions
- `glColor4f()` / `glColor3f()` — immediate mode color setting
- `glEnable(GL_LINE_SMOOTH)` — antialiased lines for borders and demod edges
- `glLineWidth()` — variable width for demod edge emphasis

**Blend modes used:**

| Mode | Components | Use |
|------|------------|-----|
| Additive | `GL_SRC_ALPHA, GL_ONE` | Demod bands, frequency selectors |
| Alpha | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` | Labels, backgrounds |
| Opaque | `GL_ONE, GL_ZERO` | Shadow text (then switched to alpha for main text) |

### Buffer Strategy

**No persistent GPU buffers** for drawing primitives. All PrimaryGLContext drawing is immediate-mode (`glBegin/glEnd`) for lines and quads. The only persistent GPU resources in the visual system are:
- **Font atlas textures** — one per font size, uploaded once in `loadFontOnce()`
- **Waterfall textures** — two double-buffered textures per waterfall panel, updated in `WaterfallPanel::update()`

### ScopeContext Extensions

`ScopeContext` (subclass of `PrimaryGLContext`) adds scope-specific drawing:
- `DrawBegin(clear)` — clears with scope background color, disables texture
- `DrawTunerTitles(ppmMode)` — draws "Frequency", "Bandwidth", "Center Frequency" labels
- `DrawDeviceName(name)` — draws device name in top-right
- `DrawDivider()` — single white vertical line at center
- `DrawEnd()` — finalize scope frame (currently empty, like `EndDraw()`)

### MeterContext (`src/visual/MeterContext.h`)

Context for the signal level meter canvas. Minimal extension — no font rendering, purely filled geometry.

- `DrawBegin()` — disables `GL_CULL_FACE` and `GL_DEPTH_TEST`, clears with theme background (`generalBackground`), loads identity modelview, disables texturing
- `Draw(r, g, b, a, level)` — draws a vertical meter bar split at x=0 into left/right halves; each half uses a horizontal gradient (dim at outer edges, full brightness at center x=0). Additive blending (`GL_ONE, GL_ONE`). The `level` parameter (0.0–1.0) controls fill height. Note: the two halves are not symmetric — the left half dims only RGB at the outer edge, while the right half dims both RGB and alpha.
- `DrawEnd()` — no-op

### TuningContext (`src/visual/TuningContext.h`)

Context for the fine-tuning bar canvas. Renders per-digit frequency display with adaptive font sizing.

- `DrawBegin()` — clears with theme background, disables texturing
- `Draw(r, g, b, a, p1, p2)` — draws a horizontal tuning bar between positions `p1`–`p2` (normalized 0–1), split at y=0 with additive blending gradient (dim at edges, full at center)
- `DrawTuner(freq, count, displayPos, displayWidth)` — renders a frequency value as individual digits, each in its own column with vertical grid lines at 25% alpha. Font size adapts to viewport via two independent paths: width-based (≥500px → 32px, ≥300px → 24px, else → 18px) and height-based (≥28px → 18px, ≥24px → 16px, else → 12px); whichever threshold is met first determines the size
- `static DrawTunerDigitBox(index, count, displayPos, displayWidth, color)` — draws a red highlight box around a single digit position via `GL_LINE_STRIP` (note: `color` parameter is currently ignored; highlight is always red)
- `GetTunerDigitIndex(mPos, count, displayPos, displayWidth)` — hit-test utility converting mouse position to digit index
- `DrawTunerBarIndexed(start, end, count, displayPos, displayWidth, color, alpha, top, bottom)` — draws a colored bar for a range of digit indices on the top/bottom half with additive blending (note: `alpha` parameter is currently ignored; hardcoded to 0.6)
- `DrawDemodFreqBw(freq, bw, center)` — composite method drawing three tuner displays side-by-side: demod frequency, bandwidth, and center frequency
- `DrawEnd()` — no-op

### ModeSelectorContext (`src/visual/ModeSelectorContext.h`)

Context for the modem type selection button canvas. Unique dual-drawing mode for active/inactive states.

- `DrawBegin()` — clears with theme background, disables texturing
- `DrawSelector(label, c, cMax, on, r, g, b, a, px, py)` — draws a single mode button. When `on=true`, draws a filled quad (active); when `on=false`, draws only an outline (`GL_LINE_LOOP`). Semi-transparent modes enable alpha blending. Text is black when active, otherwise drawn in the specified color. Buttons are distributed evenly across full height based on `cMax`
- `DrawEnd()` — no-op

### UITestContext (`src/ui/UITestContext.h`)

Context for the UI test/development canvas. The only context that uses the `GLPanel` widget hierarchy rather than raw GL calls.

- Constructor builds test panel members: root `testPanel` (solid blue, 10px margin), `testChildPanel` (gradient bar, Y-down coords), `testChildPanel2` (gradient, red border, with `testText1` child showing "Testing 123.."), `testChildPanel3` (gradient, red border), and `testMeter` (meter widget). Currently only `testMeter` is parented to `testPanel`; the three child panels are instantiated but not added to the rendered tree (commented out)
- `DrawBegin()` — disables cull face/depth test, clears with theme background, loads identity modelview
- `Draw()` — computes root panel transform via identity matrix, then calls `testPanel.draw()` which recursively renders the entire GLPanel tree
- `DrawEnd()` — no-op

### Hover Alpha

A `hoverAlpha` float controls the transparency of hover indicators, animated smoothly over time for visual feedback on demodulator hover states.

## GLFont System (`src/util/GLFont.h`)

Bitmap font rendering using BMFont format. Fonts are stored in `font/` as PNG + definition files. 11 bitmap font sizes (12-96px) are loaded on demand via `loadFontOnce()` (lazy initialization). Font file search order:
1. `RES_FOLDER/fonts/*`
2. `[exe_path]/RES_FOLDER/fonts/*`
3. `[exe_path]/fonts/*`

### Font Sizes

| Enum | Pixels | Use Case |
|------|--------|----------|
| `GLFONT_SIZE12` | 12px | Small labels |
| `GLFONT_SIZE16` | 16px | Status text |
| `GLFONT_SIZE18` | 18px | Medium labels |
| `GLFONT_SIZE24` | 24px | Frequency display |
| `GLFONT_SIZE27` | 27px | — |
| `GLFONT_SIZE32` | 32px | Large labels |
| `GLFONT_SIZE36` | 36px | — |
| `GLFONT_SIZE48` | 48px | Header text |
| `GLFONT_SIZE64` | 64px | — |
| `GLFONT_SIZE72` | 72px | — |
| `GLFONT_SIZE96` | 96px | — |

### Font Scaling

`GLFont::GLFontScale` provides user-configurable scaling:
- `GLFONT_SCALE_NORMAL` — 1.0x
- `GLFONT_SCALE_MEDIUM` — 1.5x
- `GLFONT_SCALE_LARGE` — 2.0x

Changing the scale factor calls `clearAllCaches()` to flush all string caches, since cached vertex data is baked at a specific scale.

### Drawer Proxy

`GLFont::Drawer` is a lightweight proxy that selects the appropriate font size and scale factor:

```cpp
GLFont::Drawer drawer = GLFont::getFont(24, scaleFactor);
drawer.drawString("Hello", x, y, GLFONT_ALIGN_LEFT, GLFONT_ALIGN_TOP);
```

**Font selection algorithm** (`getFont()`):

1. Compute `targetSize = round(requestedSize * scaleFactor)`
2. Scan the `fonts[]` array to find the largest font where `pixHeight <= targetSize`
3. Compute `renderingFontScaleFactor = (double) targetSize / rawPixHeight`
4. The Drawer stores the selected font index and scale factor; `drawString()` applies the scale to the final rendering

This means requesting size 24 with scale 1.5x selects the 36px font (if available) or the nearest smaller font scaled up.

### String Caching

Each font maintains a `stringCache` map (`wstring → GLFontStringCache`) for pre-computed vertex/UV data. The cache key encodes viewport dimensions, pixel height, and the string text.

**Cache entry structure:**
- `drawlen` — number of renderable characters
- `vpx`, `vpy` — viewport dimensions at cache time (invalidates on resize)
- `pxHeight` — pixel height at cache time
- `msgWidth` — precomputed string width
- `gc` — atomic age counter for eviction
- `gl_vertices` — pre-transformed vertex positions
- `gl_uv` — texture coordinates

**Cache hit path:** `drawCacheString()` renders all characters in a single `glDrawArrays(GL_QUADS, 0, 4 * drawlen)` call. No per-character transforms needed — vertices are pre-baked at cache creation.

**Cache miss path:** `cacheString()` pre-transforms all character vertices through the final scale+translate matrix, stores them, then draws.

**Cache eviction policy:**

```
Every ~50 cacheable draw calls (GC_DRAW_COUNT_PERIOD, fires when gcCounter > 50):
    │
    ▼
doCacheGC() — single pass through stringCache map:
    │
    ├── For each entry: decrement gc counter
    │
    └── Remove first entry where gc < -GC_DRAW_COUNT_LIMIT (-10)
        (not drawn for 10+ GC cycles = 500+ draw calls)
```

Eviction is **amortized** — at most one entry removed per GC cycle, preventing frame spikes. On cache hit, `gc` resets to 0. This LRU-like policy keeps frequently used strings (demod labels, frequency readouts) cached while slowly evicting stale entries (old demod labels, transient status text).

### Thread Safety

| Mechanism | Protects | Type |
|-----------|----------|------|
| `cache_busy` | String cache map access | SpinMutex (atomic flag spinlock) |
| `currentScale` | Scale factor reads | std::atomic<GLFontScale> |
| Font data (characters, textures) | Read-only after load | Inherently thread-safe |

The `Drawer` object is stack-local (not shared between threads), so creating a Drawer per-frame is safe. The SpinMutex ensures only one thread can modify the cache map at a time, while concurrent reads of the font atlas texture are safe since it's immutable after upload.

## ColorTheme System (`src/visual/ColorTheme.h`)

### Theme Structure

`ColorTheme` defines all colors used by the visual system:

| Property | Used By |
|----------|---------|
| `waterfallGradient` | Waterfall color mapping (256-entry LUT) |
| `waterfallHighlight` | Active demodulator marker |
| `waterfallNew` | New demodulator being created |
| `waterfallHover` | Hovered demodulator / frequency selector |
| `waterfallDestroy` | Demodulator being deleted |
| `wfHighlight` | Waterfall highlight overlay |
| `fftLine` | Spectrum line color |
| `fftHighlight` | Highlighted spectrum point |
| `scopeLine` | Oscilloscope trace color |
| `tuningBarLight` / `tuningBarDark` | Tuning bar gradient |
| `tuningBarUp` / `tuningBarDown` | Fine tuning direction indicators |
| `meterLevel` / `meterValue` | Signal meter colors |
| `text` | General text color |
| `freqLine` | Frequency grid lines |
| `button` / `buttonHighlight` | UI button colors |
| `scopeBackground` | Scope canvas background |
| `fftBackground` | Spectrum canvas background |
| `generalBackground` | General background color |
| `name` | Theme display name (string) |

### Available Themes

| ID | Name | Class | Description |
|----|------|-------|-------------|
| 0 | Default | `DefaultColorTheme` | Google Turbo colormap (rainbow) |
| 1 | DefaultJet | `DefaultColorThemeJet` | Original CubicSDR jet colormap |
| 2 | Black & White | `BlackAndWhiteColorTheme` | Grayscale waterfall |
| 3 | Sharp | `SharpColorTheme` | High-contrast blue-white-yellow-red |
| 4 | Rad | `RadColorTheme` | Blue-green-orange-red-white |
| 5 | Touch | `TouchColorTheme` | Dark purple/cyan/green/yellow/red |
| 6 | HD | `HDColorTheme` | Blue-green-red-yellow-white |
| 7 | Radar | `RadarColorTheme` | Green monochrome radar style |

### ThemeMgr

Global singleton `ThemeMgr::mgr` manages theme selection:
- `setTheme(themeId)` sets `currentTheme` pointer
- `getTheme()` returns current `themeId`
- `themes` map holds all `ColorTheme*` instances
- Current theme is persisted in `AppConfig`

## Rendering Flow

### Per-Frame Rendering (WaterfallCanvas)

All canvases follow the same rendering pattern in `OnPaint()`:

```
1. glContext->SetCurrent(*this)          // bind GL context
2. initGLExtensions()                    // WGL swap interval (Windows)
3. glViewport(0, 0, ClientSize.x, ClientSize.y)

4. glContext->BeginDraw(r, g, b)        // clear buffers, load identity

5. panelTree.calcTransform(identity)     // recursive transform computation
6. panelTree.draw()                      // recursive panel rendering

7. glContext->DrawDemod(...)             // demodulator overlay markers
8. glContext->DrawFreqSelector(...)      // frequency selection indicators
9. glContext->DrawRangeSelector(...)     // range selection overlay (if active)

10. glContext->EndDraw()                 // finalize (empty)
11. SwapBuffers()                        // present frame
```

**Key detail:** The panel tree is drawn first (step 5-6), then overlays are drawn on top (steps 7-9) using `PrimaryGLContext` immediate-mode drawing. This two-pass approach ensures overlays always render above the waterfall/spectrum content. Note: `DrawFreqBwInfo` is called by `SpectrumCanvas` (not `WaterfallCanvas`) to show frequency/bandwidth readouts at the cursor position.

### OnIdle Processing

Each canvas registers its own `EVT_IDLE` handler. wxWidgets fires `OnIdle` when the event loop is empty, giving canvases a chance to poll for new data:

| Canvas | OnIdle Behavior |
|--------|----------------|
| `WaterfallCanvas` | `processInputQueue()` + `Refresh()` |
| `SpectrumCanvas` | `Refresh()` + `event.RequestMore()` |
| `ScopeCanvas` | `Refresh()` + `event.RequestMore()` |

`WaterfallCanvas` is the only canvas that polls its input queue in `OnIdle` via `processInputQueue()`. `SpectrumCanvas` and `ScopeCanvas` process their input queues inside `OnPaint()` instead (via `try_pop()` at the start of the paint handler). All canvases use non-blocking `try_pop()` to avoid stalling the UI thread. `Refresh()` is always called to drive animations (zoom interpolation, hover alpha decay, frequency nudge velocity) even when no new data is available.

### WaterfallCanvas Paint Flow (Detailed)

```
OnPaint():
    ├── Apply zoom interpolation: mouseZoom += (1.0 - mouseZoom) * 0.2
    ├── Apply frequency nudge velocity: centerFreq += bandwidth * freqMove * 0.01
    ├── Apply visual gain animation: scaleFactor interpolation
    │
    ├── GL setup (viewport, clear)
    │
    ├── WaterfallPanel.draw()           // texture-based scrolling waterfall
    │   ├── Upload new FFT rows as texture strips
    │   └── Draw with texture coordinate scrolling
    │
    ├── Draw demodulator markers        // one DrawDemod() per active demod
    │   └── Color-coded: highlight (active), hover, new, destroy
    │
    ├── Draw frequency selector         // hover/active frequency marker
    │
    ├── Draw frequency/bandwidth info   // text readout at cursor position
    │
    └── SwapBuffers()
```

### ScopeCanvas Paint Flow (Detailed)

```
OnPaint():
    ├── Pop ScopeRenderData from inputData queue
    │
    ├── GL setup (viewport, clear)
    │
    ├── bgPanel.draw()                  // independent identity transform, scope background
    │
    ├── Compute panel positions (spring animation)
    │   ├── ctr += (ctrTarget - ctr) * 0.2
    │   └── Apply 3D perspective: lookat(0, 0, -1.205) + rotation
    │
    ├── Set panel visibility based on scopeVisible()/spectrumVisible()
    │   ├── scopePanel.contentsVisible = scopeVisible()
    │   └── spectrumPanel.contentsVisible = spectrumVisible()
    │
    ├── parentPanel.draw()              // recursive: scopePanel + spectrumPanel
    │   └── Uses ScopeVisualProcessor output data
    │
    ├── ScopeContext overlay drawing
    │   ├── DrawDeviceName()
    │   └── DrawTunerTitles(ppmMode)
    │
    └── SwapBuffers()
```

### Data Flow Timing

The pull-based architecture creates a producer-consumer pipeline with these timing characteristics:

```
SDRThread (producer) → SDRPostThread (channelizer/passthrough)
    │
    ├── IQ data at sample rate (e.g., 2.4 Msps → 60 packets/sec at 40,000 samples/packet)
    │
    ▼
SpectrumVisualProcessor (worker thread)
    │
    ├── Processes at pace set by upstream FFTDataDistributor (~30-60/sec)
    ├── Applies smoothing (double EMA) to reduce flicker
    │
    ▼
WaterfallCanvas (consumer, UI thread)
    │
    ├── OnIdle() polls queue with try_pop()
    ├── Rate-limits waterfall line insertion (linesPerSecond, default 30)
    ├── Triggers Refresh() → OnPaint() → SwapBuffers()
    │
    ▼
Display (~30-60 FPS depending on system)
```

For details on the visual data processing pipeline internals (VisualProcessor, distributors, FFT processing, ScopeVisualProcessor processing logic), see [visual-data-pipeline.md](visual-data-pipeline.md).

## Mouse Interaction

### MouseTracker (`src/util/MouseTracker.h`)

All canvases share a `MouseTracker` instance that normalizes mouse coordinates and tracks drag state:

- **Coordinates:** `mouseX`/`mouseY` normalized to [0,1] with Y flipped (`1.0 - clientY/height`)
- **Deltas:** `deltaMouseX`/`deltaMouseY` computed each frame
- **Origin:** `originMouseX`/`originMouseY` recorded on mouse-down for drag distance
- **Button state:** `isMouseDown`, `isMouseRightDown`, `isMouseInView`
- **Drag locks:** `vertDragLock`/`horizDragLock` — on Windows, physically warp cursor back to lock axis via `WarpPointer()`; on macOS/Linux, drag locks are not enforced visually

### Event Propagation

All canvas event handlers follow the same pattern:
1. Call `InteractiveCanvas::OnMouseMoved(event)` — updates `mouseTracker` coordinates and modifier key flags (`shiftDown`, `altDown`, `ctrlDown`)
2. Perform canvas-specific logic

Modifier keys are tracked as member booleans, updated on every mouse/key event.

### WaterfallCanvas Drag State Machine

```
enum DragState {
    WF_DRAG_NONE,
    WF_DRAG_BANDWIDTH_LEFT,
    WF_DRAG_BANDWIDTH_RIGHT,
    WF_DRAG_FREQUENCY,
    WF_DRAG_RANGE
};
```

**Hover state resolution** (`updateHoverState()`):

```
altDown? ──yes──> WF_DRAG_RANGE
    │no
    ▼
Cursor over demodulator (within initial 15 kHz filter, then dynamic buffer of `halfBw + 10kHz * (currentBw / globalBw)`)? ──no──> WF_DRAG_NONE
    │yes
    ▼
!shiftDown? ──no──> WF_DRAG_NONE (shift held bypasses demod-hover entirely)
    │yes
    ▼
|freqDiff| > bandwidth/3? ──yes──> WF_DRAG_BANDWIDTH_LEFT or RIGHT
    │no                              (based on which side of center;
    ▼                                LSB/USB mode exceptions for direction)
WF_DRAG_FREQUENCY
```

**Mouse-down:** Captures `nextDragState` into `dragState`, records `dragOfs` (offset from cursor to demod center) and `dragBW` (initial bandwidth for BW resize).

**Mouse-move while dragging:**

| State | Behavior |
|-------|----------|
| `WF_DRAG_BANDWIDTH_LEFT` | Resize from left edge: `bwDiff = deltaMouseX * bandwidth * 2`, clamp to [`MIN_BANDWIDTH`, `CHANNELIZER_RATE_MAX`] Hz |
| `WF_DRAG_BANDWIDTH_RIGHT` | Resize from right edge: same formula, opposite sign (direction exceptions for LSB/USB modes) |
| `WF_DRAG_FREQUENCY` | Move demod center: compute target freq from cursor, apply frequency snap |
| `WF_DRAG_RANGE` | Visual range selection — highlight region between origin and current cursor |
| (right-drag) | Forward to `spectrumCanvas->updateScaleFactorFromYMove(deltaMouseY)` for visual gain |

**Mouse-release:**

| State | Action |
|-------|--------|
| Zero-drag (click) | Create new demodulator or move existing one to cursor frequency |
| `WF_DRAG_RANGE` | Create demodulator spanning the selected frequency range |
| All states | Reset `dragState = WF_DRAG_NONE` |

**Frequency snap:** When moving a demodulator, the frequency snaps to the nearest configurable snap value (powers of 10: 1, 10, 100, 1000, etc.). Default is 1 Hz (no snapping); the user sets this via the tuning canvas, and it is persisted in `AppConfig`.

### Frequency Zoom State Machine (Mouse Wheel)

```
Mouse wheel delta
    │
    ▼
mouseZoom = 1.0 - (wheelRotation / linesPerAction) / 1000.0
    │
    ▼ (in OnPaint, per-frame interpolation)
mouseZoom += (1.0 - mouseZoom) * 0.2   // smooth animation toward 1.0
    │
    ▼
|mouseZoom - 1.0| < 0.01? ──yes──> snap to 1.0, exit view mode if zoomed out past sample rate
    │no
    ▼
Apply zoom: bw = ceil(baseBandwidth * currentZoom), clamp to minBandwidth (default 30000 Hz)
```

The zoom keeps the frequency under the mouse cursor stable by adjusting the center frequency proportionally to the bandwidth change.

### Keyboard Navigation

Arrow keys control frequency and display in the WaterfallCanvas:

| Key | Condition | Action |
|-----|-----------|--------|
| LEFT | View mode | Pan center frequency by ±1.0% of bandwidth (±5.0% with Shift) |
| LEFT | Not in view | Jump by half bandwidth left (10x bandwidth with Shift) |
| RIGHT | View mode | Pan center frequency right |
| RIGHT | Not in view | Jump by half bandwidth right (10x bandwidth with Shift) |
| UP | No modifier | Zoom in (reduce bandwidth): `zoom *= 0.95` |
| DOWN | No modifier | Zoom out (increase bandwidth): `zoom *= 1.05` |
| UP | Shift held | Increase visual gain: `scaleMove = 1.0` |
| DOWN | Shift held | Decrease visual gain: `scaleMove = -1.0` |

All keyboard controls are **momentary** — on key-up, `freqMove`, `scaleMove`, and `zoom` reset to their default (0, 0, 1.0). Frequency movement has velocity decay: `freqMove -= freqMove * 0.2` per frame, stopping at 0.01.

### SpectrumCanvas Interactions

| Input | Action |
|-------|--------|
| Left-drag | Horizontal pan: `moveCenterFrequency(deltaX * bandwidth)` |
| Right-drag | Visual gain: `updateScaleFactorFromYMove(deltaMouseY)`, clamped to [0.25, 10.0] (requires `scaleFactorEnabled`) |
| Right-click (no vertical drag) | Animate scale factor back to 1.0, toggle peak hold on spectrum/demod processors (only triggers when `originDeltaMouseY == 0`) |
| Mouse wheel | Forwarded to `WaterfallCanvas::OnMouseWheelMoved()` for coordinated zoom |
| 'B' key | Toggle dB display mode |

The spectrum canvas tracks cumulative bandwidth change (`bwChange`) and resets the scale factor when bandwidth changes exceed 400,000 Hz to prevent the display from becoming unreadable.

### ScopeCanvas Interactions

| Input | Action |
|-------|--------|
| Left-drag | Panel slide with inertia: `dragAccel = 4.0 * deltaMouseX`, momentum-based snap |
| Mouse wheel | Not wired in event table (handler exists but `EVT_MOUSEWHEEL` is absent from the event table) |

Panel position animates with spring-like behavior: `ctr += (ctrTarget - ctr) * 0.2`. Panels use 3D perspective projection with `lookat(0, 0, -1.205, ...)` and rotation via `atan2(pos, 1.2)` for a card-flip effect. The two panels (scope + spectrum) are separated by `panelSpacing = 0.4f` (total interval = `panelWidth * 2.0 + panelSpacing`).

## Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEFAULT_WATERFALL_LPS` | 30 | Default waterfall lines per second |
| `MIN_BANDWIDTH` | 500 | Minimum demodulator bandwidth |
| `CHANNELIZER_RATE_MAX` | 500000 | Maximum channelizer sample rate |

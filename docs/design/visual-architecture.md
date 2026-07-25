# Visual Architecture

This document describes CubicSDR's OpenGL rendering system, canvas hierarchy, font rendering, color themes, and the visual data processing pipeline.

## Overview

The visual system is built on wxWidgets' OpenGL integration (`wxGLCanvas`/`wxGLContext`) and uses a **pull-based** architecture: worker threads produce FFT/scope data into queues, and UI canvases poll those queues in their `OnIdle()` handlers. Worker threads never push data to the UI.

```
SDRPostThread
    |
    +--[pipeIQVisualData]--------> SpectrumVisualProcessor --> SpectrumCanvas
    +--[pipeWaterfallIQVisualData]-> FFTVisualDataThread --> WaterfallCanvas
    +--[pipeDemodIQVisualData]----> SpectrumVisualProcessor --> Demod spectrum

DemodulatorThread
    |
    +--[audioVisOutputQueue]------> ScopeVisualProcessor --> ScopeCanvas
```

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
- **Scale factor:** Shift+Up/Down adjusts visual gain (FFT display scaling)

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
- `GLPanel` hierarchy: `parentPanel` → `scopePanel` + `spectrumPanel` + `bgPanel`

## GLPanel System (`src/ui/GLPanel.h`)

A lightweight retained-mode UI system for OpenGL rendering:

**Base class `GLPanel`:**
- Position (`pos[2]`), size (`size[2]`), rotation (`rot[3]`)
- Fill modes: none, solid, gradient X/Y, gradient bar
- Border and margin in pixels
- Coordinate system options (Y-up/Y-down, zero-one or signed)
- Transform matrix stack (`CubicVR::mat4`)
- Child panel hierarchy (`children` vector)
- Hit testing for mouse interaction

**Derived panels:**
- `GLTextPanel` — text rendering with alignment (left/right/center, top/bottom)
- `GLTestPanel` — debug/test rendering

**Panel classes:**
| Panel | File | Purpose |
|-------|------|---------|
| `WaterfallPanel` | `src/panel/WaterfallPanel.h` | Scrolling waterfall texture rendering |
| `SpectrumPanel` | `src/panel/SpectrumPanel.h` | FFT line plot rendering |
| `ScopePanel` | `src/panel/ScopePanel.h` | Oscilloscope waveform rendering |
| `MeterPanel` | `src/panel/MeterPanel.h` | Signal level meter rendering |

## PrimaryGLContext (`src/visual/PrimaryGLContext.h`)

Shared OpenGL context providing drawing primitives. All canvases share this context via `wxGLContext` sharing.

**Drawing methods:**
| Method | Purpose |
|--------|---------|
| `BeginDraw(r, g, b)` | Clear screen and set up OpenGL state |
| `EndDraw()` | Finalize frame |
| `DrawDemod()` | Draw demodulator bandwidth indicator with label |
| `DrawDemodInfo()` | Draw demodulator info label (frequency, type, bandwidth) |
| `DrawFreqSelector()` | Draw frequency selection marker |
| `DrawRangeSelector()` | Draw range selection overlay |
| `DrawFreqBwInfo()` | Draw frequency and bandwidth text |

**Hover alpha:** A `hoverAlpha` float controls the transparency of hover indicators, animated smoothly over time.

## GLFont System (`src/util/GLFont.h`)

Bitmap font rendering using BMFont format. Fonts are stored in `font/` as PNG + definition files.

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

`GLFont::getFont(requestedSize, scaleFactor)` selects the best matching font from the available sizes and applies scaling.

### String Caching

Each font maintains a `stringCache` map (`wstring → GLFontStringCache`) for pre-computed vertex/UV data:
- `cacheString()` generates OpenGL vertex and texture coordinate arrays
- `drawCacheString()` renders from cache
- `doCacheGC()` evicts old entries using an atomic garbage collection counter
- Cache is per-font, invalidated on font reload

### Drawer Proxy

`GLFont::Drawer` is a lightweight proxy that selects the appropriate font size and scale factor:
```cpp
GLFont::Drawer drawer = GLFont::getFont(24, scaleFactor);
drawer.drawString("Hello", x, y, GLFONT_ALIGN_LEFT, GLFONT_ALIGN_TOP);
```

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
- `themes` map holds all `ColorTheme*` instances
- Current theme is persisted in `AppConfig`

## Visual Data Processing Pipeline

### VisualProcessor Template (`src/process/VisualProcessor.h`)

Base template for all visual data processors:

```
InputQueue → process() → distribute() → OutputQueues[]
```

- `setInput()` — attach input queue
- `attachOutput()` / `removeOutput()` — manage output queues
- `process()` — pure virtual, implemented by subclasses
- `distribute()` — pushes output to all attached output queues

### Specializations

| Class | Purpose |
|-------|---------|
| `VisualDataDistributor<T>` | 1:N shared-pointer dispatch (no copy) |
| `VisualDataReDistributor<T>` | 1:N deep-copy dispatch via `ReBuffer` pool |

### Concrete Processors

**`SpectrumVisualProcessor`:**
- Input: `DemodulatorThreadInputQueue` (IQ data)
- Output: `SpectrumVisualDataQueue` (FFT points)
- Computes FFT, applies windowing, generates spectrum points
- Supports configurable FFT size, floor/ceiling, scale factor

**`ScopeVisualProcessor`:**
- Input: `DemodulatorThreadOutputQueue` (audio data)
- Output: `ScopeRenderDataQueue` (waveform/spectrum data)
- Computes scope waveform or FFT of demodulated audio
- Supports multiple display modes (scope, spectrum, waterfall)

**`FFTVisualDataThread`:**
- Dedicated thread running `SpectrumVisualProcessor`
- Rate-limits waterfall updates (one FFT per frame)
- Redistributes FFT data to multiple consumers

**`SpectrumVisualDataThread`:**
- Dedicated thread running `SpectrumVisualProcessor`
- Main spectrum display computation

**`FFTDataDistributor`:**
- Distributes IQ data to multiple FFT processors
- Uses `VisualDataDistributor` for shared-pointer distribution

## Rendering Flow

### Per-Frame Rendering (WaterfallCanvas)

1. `OnIdle()` calls `processInputQueue()` then `Refresh()`
2. `processInputQueue()` pops FFT data at rate-limited intervals
3. `OnPaint()`:
   - Apply zoom and frequency nudge animations
   - Set GL context and viewport
   - Call `waterfallPanel.draw()` for waterfall texture
   - Draw demodulator markers via `PrimaryGLContext::DrawDemod()`
   - Draw frequency selector / hover indicators
   - Call `SwapBuffers()`

### OnIdle Processing

Each canvas registers its own `EVT_IDLE` handler:
- `WaterfallCanvas::OnIdle` → `processInputQueue()` + `Refresh()`
- `SpectrumCanvas::OnIdle` → `processInputQueue()` + `Refresh()`
- `ScopeCanvas::OnIdle` → `processInputQueue()` + `Refresh()`

All use non-blocking `try_pop()` to avoid stalling the UI thread.

## Mouse Interaction Summary

| Canvas | Left Click | Left Drag | Right Drag | Wheel |
|--------|-----------|-----------|------------|-------|
| Waterfall | Set freq / create demod | Move demod / resize BW / range select | Adjust visual gain | Zoom |
| Spectrum | Set freq | Move demod | Adjust visual gain | Zoom |
| Scope | — | — | — | — |

## Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `DEFAULT_WATERFALL_LPS` | 30 | Default waterfall lines per second |
| `MIN_BANDWIDTH` | 30000 | Minimum demodulator bandwidth |
| `CHANNELIZER_RATE_MAX` | varies | Maximum channelizer sample rate |

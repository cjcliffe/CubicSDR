## Goal

Fix two mouse-wheel bugs: tuner digits skipping under multi-event
wheels (#846), and waterfall zoom speed collapsing when the OS
"lines per scroll" setting is raised.

## Approach

- *Tuner digit skip:* `TuningCanvas::OnMouseWheelMoved` stepped a
  full digit on every wheel event using only the rotation's sign.
  Smooth-scroll and multi-line drivers deliver several
  small-rotation events per physical notch, so one notch became
  several digit steps (double-subscription and event forwarding
  were ruled out first). Rotation is now accumulated per axis and
  applied once per wheel-delta, carrying the remainder; direction
  mapping per axis is unchanged.
- *Waterfall zoom speed:* the zoom handler divided rotation by
  `GetLinesPerAction()`, inverting the OS setting's meaning — a
  higher setting slowed zooming instead of speeding it up. It now
  normalizes to wheel notches and scales by a fixed factor matching
  the previous feel at the platform default, so zoom speed is
  constant per notch on every platform. The setting's wide range
  makes honoring it directly unusable for zoom, hence the fixed
  factor. Other canvases use rotation proportionally rather than
  stepped and were left alone.

## Validation Plan

- Windows build clean; C++ suite green.
- Live check: slow wheel rolls advance tuner digits exactly one
  per notch; fast spins track proportionally; waterfall zoom
  distance is identical at low and high OS line settings.

## Risks / Open Questions

- None identified; both edits preserve default feel and only
  remove setting-dependent variance.

## Outcome

Implemented as planned. Retire this synopsis on merge.

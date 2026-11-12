## Goal

Apply the swap-interval call once per GL context instead of on every context
activation.

## Success Criteria

- Identical visuals; ~1000+/sec redundant driver calls eliminated at 60fps;
  C++ and build suites stay green.

## Approach

Add a `mutable bool` guard in `PrimaryGLContext::SetCurrent`: apply
`wglSwapIntervalEXT(0)` on first successful activation, skip after. Sound
because every call site activates its own canvas (`*this`), contexts are
created and destroyed with their canvases, and all calls run on the UI thread
(no atomics needed).

## Steps

1. Branch per the Fix Synopsis convention; add the member and guard; build.
2. Write the synopsis; commit, merge, and retire on request.

## Validation Plan

- Full build plus C++ suite and `test_build.py` green (wx/GL-bound, no unit test).
- Live smoke: streaming session plus resize and device restart to exercise
  context activation paths; no visual change expected.

## Risks / Open Questions

- Risk: negligible — context and window lifetimes are paired, so the flag
  cannot go stale within a context's life.
- Open questions: none.

## Outcome

Implemented as planned (+4/−3). Suites green. Live smoke test passed with no
visual change.

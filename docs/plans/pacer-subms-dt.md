## Goal

Feed the waterfall pacer sub-millisecond frame deltas instead of the
millisecond-quantized `Timer`.

## Success Criteria

- Waterfall crawl smooth and rate-correct at LPS 30/60 on every frame-rate
  setting; C++ and build suites stay green.

## Approach

Replace the `gTimer` pacer feed in `WaterfallCanvas` with a `steady_clock`
member timestamp, reset in `setup()`; pass seconds as a double into the
unchanged `WaterfallPacer::plan` interface. Remove the `gTimer` member (its
only use is the pacer feed). Demod waterfall views share the class and inherit
the change.

## Steps

1. Branch per the Fix Synopsis convention; swap the clock; build.
2. Write the synopsis; commit, merge, and retire on request.

## Validation Plan

- Full build plus C++ suite and `test_build.py` green. No new unit test: the
  pacer model tests cover the credit math, and the clock source is live-only.
- Live: waterfall crawl at LPS 30/60 with frame rate NORMAL and HIGH; confirm
  smooth crawl and sane rate over a minute of viewing.

## Risks / Open Questions

- Risk: none to the pacing math — same credit integrator, finer input.
- Open questions: none.

## Outcome

Implemented as planned (+9/−5). Suites green. Live crawl check passed at LPS
30/60 on NORMAL and HIGH with no drift.

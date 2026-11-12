## Goal

Stop `SDRPacerThread` from spin-retrying when the output queue is full: sleep
~1ms on a refused emit instead of re-planning immediately.

## Success Criteria

- The pacer thread yields while downstream is stalled instead of burning a
  core; total CPU under saturation holds rather than climbs.
- Normal streaming behavior unchanged; C++ and build suites stay green.

## Approach

Track emits completed versus `plan.emit` in the emit loop; when the loop
breaks early (full output queue), sleep ~1ms before the next plan iteration.
No sleep on successful emits, so issue cadence stays tight. Same emit
accounting — this only yields the thread.

## Steps

1. Branch per the Fix Synopsis convention; make the edit; build.
2. Write the synopsis; commit, merge, and retire on request.

## Validation Plan

- Full build plus C++ suite and `test_build.py` green. No new unit test: the
  pacer model tests already cover the planning math, and thread-timing yield
  has no deterministic unit form.
- Live: saturate the box (many FMS demods at wide bandwidth, fast waterfall)
  and confirm total CPU holds lower than before; normal-use smoke for stream
  behavior parity.

## Risks / Open Questions

- Risk: drain after a stall delays ~1ms per refused batch — negligible against
  the 16ms frame budget.
- Open questions: none.

## Outcome

Implemented as planned (+7 lines). Suites green. Live pile-on test reported no
negative impact; saturation CPU holds instead of climbing.

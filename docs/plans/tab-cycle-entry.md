## Goal

Make TAB (and shift-TAB) select a demodulator when there is no active
selection — e.g. after the active demod is tuned out of view — instead of
doing nothing.

## Success Criteria

- TAB with no active demod selects the first in-view demodulator by
  frequency; shift-TAB selects the last.
- Normal wrap cycling with an active selection is unchanged.
- C++ and build suites stay green.

## Approach

Trace the dead path: `SDRPostThread` marks the out-of-view demod inactive,
`updateLastState` nulls `currentModem`, and then both the TAB handler's
`if (!lastDemod)` early break and the `getNext`/`getPrevious`
`getCurrentModem` guards return nothing.

- *DemodCycle:* new header-only stdlib model (`step(count, current,
  forward)`) holding the cycle arithmetic: wrap on a usable selection,
  enter at the front (forward) or back (backward) on none/stale, nothing
  on empty. Mirrors the `DemodReselect` convention so the wx-free test
  binary can drive it.
- *DemodulatorMgr:* `getNextDemodulator`/`getPreviousDemodulator` map the
  demod to a position in the ordered actives (null or missing yields a
  no-selection sentinel) and step through the model. The stale-current
  transient and the dead `if (actives) {}` block go away with the rewire.
- *AppFrame:* drop the TAB handler's null-current early break so a null
  selection reaches the model; no canvas handles TAB, so consuming the
  key is safe.

## Steps

1. Branch per the Fix Synopsis convention; add the model with old
   semantics plus the new-behavior tests and confirm they fail.
2. Flip the model to entry behavior; rewire the manager and TAB handler.
3. Write the synopsis; commit, merge, and retire on request.

## Validation Plan

- New `DemodCycle` doctest suite (empty, wrap both directions, single
  entry, forward/backward entry, stale re-entry) plus full build, C++
  suite, and `test_build.py` green.
- Live: tune the active demod out of view, TAB lands on the
  lowest-frequency in-view demod, shift-TAB on the highest; normal
  cycling still wraps.

## Risks / Open Questions

- Risk: `getFirstDemodulator` still calls `front()` on a possibly empty
  ordered list — pre-existing, untouched, no caller feeds it an empty
  list on this path.
- Open questions: none.

## Outcome

Implemented as planned. Entry cases failed against old semantics before
the flip, then the full suites went green. Live check confirmed TAB
re-entry lands naturally and normal cycling is unchanged.

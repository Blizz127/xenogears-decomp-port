Optional integrated engine measurement uses `XENO_HD2D_BATCH_STATS=1` together
with `XENO_HD2D_OCTOPATH=1` when launching the normal port field benchmark.
Capture stderr and keep the field/map/checkpoint, effect configuration, frame
count and executable identity with the measurements. This records PsyCross
`g_splitIndex`, not the standalone enhancement backend's synthetic counters.

The lane patch `batch_stats.patch` observes the completed batch just before
`DrawAllSplits` calls `ClearSplits`, including intermediate flushes from DR_MOVE,
and observes the existing capacity-rejection branch. It does not write split,
vertex, blend, texture, draw-environment or GL state. Instrumentation remains
inactive without the explicit environment opt-in or while the master is OFF.

Logs use `[hd2d-gpu-batches]` and report new observed peaks, allocation rejections
and a normal-exit summary. `peak_splits` counts allocated slots, including empty
splits that still occupy the array. `array_limit=4096` includes reserved sentinel
slot zero; `usable_slots=4095` is the largest valid `g_splitIndex`.
`capacity_rejections` counts denied split allocations, rather than hiding them
behind a peak clamped to the valid maximum. `peak_vertices` is observed at the
same flush/rejection points. `completed_batches` counts flushes, not frames.

A captured integrated summary establishes the measured engine peak for that
run. Unit tests exercise telemetry gating and counter handling; they do not
establish an integrated field peak. Forced termination may prevent the final
summary; new-peak/rejection records still report observations before that point.

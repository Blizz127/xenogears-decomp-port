CPU sprite-hook acceptance does not establish GPU replacement consumption.
The prior pipeline cleared registrations in postprocess while field was
presenting the previous frame. Actual field sequence builds sprite OT packets,
then calls Vsync(0) (`src/field/main/misc2.c`, around line 2250), then submits the
new OT with DrawOTag (around line 2355). Clearing all pending packets at that
pre-submit present prevented the GPU hook from finding them.

The corrected lifetime preserves unconsumed registrations through presents.
The native OT parser calls `PcPort_Hd2dSpriteGpuWalkDone` after a completed walk;
only registered addresses actually queried in that walk expire. Unrelated OT
walks preserve unseen registrations. A queried registration also expires when
scope, snapshot, metadata or UV validation rejects it. Full packet snapshot
comparison remains strict, including its tag; inspection found no native packet
copy or identified tag mutation that warranted relaxing identity checks.
Enable-state transitions explicitly invalidate everything via the CPU-only
`PcPort_Hd2dSpriteReset` API.

Pointer reuse with changed bytes can be refreshed by a new producer registration
that already passed the exact verified-part profile matcher. Identical duplicate
registration is ambiguous: it returns retail and removes the old token rather
than letting the fallback inherit stale art. The registry has a fixed bound and
fails to retail when capacity is unavailable. No global retail-UV match exists.

`XENO_HD2D_SPRITE_DIAG=1` enables separate registered/query/accepted counts,
miss reasons, premature-present observations, completed OT walks and rejected
seen-entry expiration. `first_consumed` prints the actual nonzero texture and
registered map/actor/frame/part. Snapshot counters print on changed registrations
or acceptance at presents, so timeout-based runs retain evidence; normal exit
also prints a summary. OFF returns before counters, environment or GL work.
`PcPort_Hd2dSpriteDiagnosticsGet` provides the same counters to test tooling.

A valid live sprite validation must establish nonzero GPU acceptance plus
visibly different original replacement art. Byte-identical retail/replacement
captures and CPU acceptance alone cannot substantiate successful replacement or
silhouette preservation. Unit tests establish the repaired lifetime and exact
packet identity behavior, while root's integrated runs establish actual visuals.

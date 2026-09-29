# World-map forbidden-hit harnesses (W18I strict model)

Runtime harnesses that prove forbidden world-map targets were never
dispatched. **Rule:** a forbidden target is proven absent only when its
instrumentation was successfully registered, remained active for the entire
measured interval, and its measured hit count is zero. Unregistered or
failed instrumentation reports `NOT_INSTRUMENTED` and fails any run where
the target is required — it is never printed as numeric zero.

## Files

| File | Purpose |
| --- | --- |
| `forbidden_instrumentation.py` | Reusable strict manifest / registration proof / interval bookkeeping / summary emitter, imported by every harness |
| `w18b_natural.gdb` | Hardened W18B natural Lahan route (cut at retail `0x80072490`, placeholder stable 120 Vsyncs) |
| `w18b_hold_enabled.gdb` | Hardened hold-enabled route (`XENO_FIELD_HOLD_TRANSITION=1`, world never entered, 1400 frames) |
| `w18b_gate_off.gdb` | Hardened gate-off smoke (`XENO_WORLD_FT4_POOLS=1` only; W17B cut `0x80072488`) |
| `w18b_controls.gdb` | Fault-injection controls (`XENO_W18I_CTRL=<scenario>`), see `scratchpad/w18i_forbidden_instrumentation/NEGATIVE_CONTROLS.md` |
| `run_w18i.sh` | Full suite: checker unit fixtures, all harnesses, all control scenarios |

## Invocation

From the repo root, on the host graphical session (never Docker):

```sh
DISPLAY=:10 timeout -s KILL 290 gdb -batch \
  -x pc_port/tools/world_harness/w18b_natural.gdb \
  --args pc_port/build_native/xeno-port
```

The gdb exit code carries the verdict (0 = every required target
ZERO_VERIFIED). Check a log afterwards:

```sh
python3 pc_port/tools/check_forbidden_instrumentation.py <log>
```

## Target mechanisms

* `native_symbol_bp` — GDB breakpoint on a native symbol
  (`wm_*_should_not_run` stub family). `set breakpoint pending off`;
  breakpoint number/enabled/location verified at interval start; liveness
  re-checked every Vsync and at interval end.
* `native_route_counter` — counter for a retail step with no native code
  (e.g. `0x80074E58`). Backed by the exported `g_wm_forbidden_targets`
  registry in `pc_port/src/world_map_init.c`; the harness reads the
  baseline at interval start and the delta at interval end directly from
  memory (no inferior calls).
* `shared_symbol_attr_bp` — breakpoint on a shared function
  (`ArchiveCdDataSync`, `DrawOTag`); each hit is classified by backtrace:
  world-update-family callers count as forbidden hits, field/overlay/
  placeholder callers are allowed and reported as `allowed=N`.
* `gdb_window_counter` — harness-side accumulator gated by a window the
  harness delimits (e.g. `rand_in_w18b`).

Raw PSX retail addresses are never used as native breakpoints; the native
process does not execute code at those addresses.

## Summary format

```
<TAG>_FORBIDDEN_SUMMARY_BEGIN
required=<comma-separated labels>
target label=<l> status=<ZERO_VERIFIED|HIT|NOT_INSTRUMENTED|INSTRUMENTATION_ERROR> registered=<0|1> hits=<n|undefined> method=<m> reason=<r>
<TAG>_FORBIDDEN_SUMMARY_END
<TAG>_FORBIDDEN_VERDICT verdict=<PASS|FAIL> exit=<n> ...
```

The checker (`pc_port/tools/check_forbidden_instrumentation.py`) rejects
duplicate labels, missing/unregistered required labels, undefined hits on
registered targets, negative counts, unknown statuses, status/count
contradictions, and legacy bare numeric dictionaries.

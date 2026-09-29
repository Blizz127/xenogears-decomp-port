# N5 deterministic boot/opening harness

Fail-closed harness for the boot -> title -> prologue (Map 4) -> Lahan
opening (Map 2) route.  The port already owns the deterministic seams used
here; this directory only adds evidence generation and verdict logic, plus a
schedule generator.

## Seams

| Env | Purpose |
| --- | --- |
| `XENO_FIELD_MAP=4` | Boot override straight into the prologue map after the retail movie (field-test roadmap skipped on purpose) |
| `XENO_FIELD_TEST_INPUT` | 4096-step, field-frame-relative held-button schedule merged in `FieldPollControllers` (2-on/2-off Circle `0x20`) |
| `XENO_FIELD_CAPTURE_DIR/EVERY` | Present-time PNG captures from `PcPort_FieldCaptureOnVsync` |
| `XENO_NPC_EVENT_DUMP=1` | `dialog-open box=.. str=..` progression emitted by `text_box.c` |

`XENO_FIELD_TEST` is deliberately never set to 1: that value also selects the
developer KernelMenu boot path, which is not the retail chain.

## Files

| File | Purpose |
| --- | --- |
| `schedule.py` | Deterministic 2-on/2-off Circle schedule up to the 4096-step cap; `--self-test` |
| `analyze_log.py` | Verdict clauses: enabled schedule, ordered FieldLoad chain, no fault markers, monotonic dialog progression, nonblack captures, optional field 3/13 |
| `test_n5_opening.py` | Pytest fixtures for the analyzer and generator |

Runners live in `pc_port/tests/`:

| Runner | Scope |
| --- | --- |
| `run_w34d1_opening_a.sh` | Lane A: Map 4 -> Map 2, bounded 300 s |
| `run_w34d1_opening_b.sh` | Lane B: normal boot -> Movie -> Map 490 title preflight, bounded 400 s |
| `run_w34d1_opening_selfcheck.sh` | Offline schedule + analyzer tests |

## Fail-closed gates

* Lane A never claims playable Lahan from a Map 2 load alone; the verdict
  requires the ordered chain `4 -> 2` (optionally followed by Map 2's possible
  exits `3`/`13`), enabled input, nonblack captures, and monotonic dialog
  progression.  Another Map 4 after Map 2 is treated as a route misload.
* Lane B reports `NEW_GAME=NOT_RUN` and `FIELD_CHAIN=NOT_RUN`.  The title
  menu's `func_801C7D78` overwrites `g_Menu->input` every frame, and the raw
  pad -> `ControllerPoll` -> queue path is the only thing it drains; the field
  schedule is merged after that drain, so no existing PC seam can confirm New
  Game deterministically.  Wiring that is a separate seam change, not forced
  here.
* `A7_CONTROL=NOT_RUN` always: no existing diagnostic observes the retail
  control-opcode A7 path safely.
* `MISSING_PRIM_INSTRUMENT=NOT_RUN`: the `missing D_8004FE50`/`xeno-ot`
  diagnostics were removed after the row-4 fill, so log absence is a software
  regression gate, not a registered zero-proof.

## Running

Lane A (needs a built `pc_port/build_native/xeno-port` or `XENO_PORT_BINARY`):

```sh
pc_port/tests/run_w34d1_opening_a.sh
```

Lane B:

```sh
pc_port/tests/run_w34d1_opening_b.sh
```

Offline self-checks (no binary needed):

```sh
pc_port/tests/run_w34d1_opening_selfcheck.sh
```

## Exit status

`0` = PASS, `1` = FAIL, `2` = NOT_RUN / prerequisite / IO.  The runners emit a
machine-readable `overall=` line and, on prerequisite failure, `NOT_RUN_RC=2`
with `BUILD=NONE` (binary absent) or `BUILD=PRESENT` + `DISPLAY=NONE` (no X11
and no `xvfb-run`).  The analyzer returns `2` for a missing log or an
`overall=NOT_RUN` result.  This matches the repository's existing fail-closed
convention
(`pc_port/tools/check_forbidden_instrumentation.py`: 0 PASS, 1 FAIL,
2 usage/IO).

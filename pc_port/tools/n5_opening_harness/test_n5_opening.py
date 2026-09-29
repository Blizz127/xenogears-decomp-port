import os
import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

from analyze_log import (
    Verdict,
    no_fault_markers,
    parse_lines,
    run_analysis,
)
from schedule import CIRCLE, generate_steps, to_csv


def test_schedule_determinism_and_cap():
    a = generate_steps()
    b = generate_steps()
    assert a == b
    assert len(a) <= 4096
    assert a[0] == (0, CIRCLE)
    frames = [frame for frame, _ in a]
    assert all(x < y for x, y in zip(frames, frames[1:]))
    values = [value for _, value in a]
    assert values[:4] == [CIRCLE, 0, CIRCLE, 0]
    # The schedule string must be parseable by the port's strtoul loop.
    csv = to_csv(a)
    assert csv.startswith("0:0x20,2:0x0,4:0x20")
    assert "::" not in csv and not csv.endswith(",")
    assert csv.count(",") + 1 == len(a)


def test_parse_lines():
    text = textwrap.dedent(
        """\
        [field-test-input] enabled steps=128
        [field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
        [field-diag] FieldLoad begin field=2 mapBuf=0x8005A4E0
        [npc-event] dialog-open box=0 str=4 lock=0x1 vis=1
        [npc-event] dialog-open box=0 str=9 lock=0x1 vis=1
        [field-diag] actor3 ip=120 flags4=0 status=0
        """
    )
    fields, input_steps, dialogs, ips = parse_lines(text)
    assert fields == [4, 2]
    assert input_steps == [128]
    assert dialogs == [(0, 4), (0, 9)]
    assert ips == [120]


def test_no_fault_markers():
    good, found = no_fault_markers("clean log\n[field-diag] frame=1\n")
    assert found == []
    bad_markers = (
        "missing D_8004FE50",
        "ptag length is not valid",
        "malloc_printerr",
        "SIGSEGV",
        "Aborted",
    )
    for marker in bad_markers:
        _good, found = no_fault_markers(marker)
        assert any(marker.lower() in f.lower() for f in found)


@pytest.fixture()
def lane_a_pass_log(tmp_path):
    log = tmp_path / "lane-a-pass.log"
    log.write_text(textwrap.dedent(
        """\
        [field-test-input] enabled steps=4096
        [field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
        [npc-event] dialog-open box=0 str=0 lock=0x1 vis=1
        [npc-event] dialog-open box=0 str=2 lock=0x1 vis=1
        [npc-event] dialog-open box=0 str=6 lock=0x1 vis=1
        [field-diag] FieldLoad begin field=2 mapBuf=0x8005A4E0
        """
    ))
    return log


def test_lane_a_pass(lane_a_pass_log):
    result = run_analysis(lane_a_pass_log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.PASS
    assert by_name["INPUT_SCHEDULE"] == Verdict.PASS
    assert by_name["FIELD_CHAIN"] == Verdict.PASS
    assert by_name["DIALOG_PROGRESS"] == Verdict.PASS
    assert by_name["NO_FAULTS"] == Verdict.PASS
    assert by_name["NONBLACK_CAPTURES"] == Verdict.NOT_RUN
    assert result["a7_control"] == Verdict.NOT_RUN


def test_lane_a_fail_missing_map2(lane_a_pass_log, tmp_path):
    log = tmp_path / "lane-a-fail.log"
    log.write_text("""[field-test-input] enabled steps=1
[field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
""")
    result = run_analysis(log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.FAIL
    assert by_name["FIELD_CHAIN"] == Verdict.FAIL


def test_lane_a_empty_log_fails_closed(tmp_path):
    log = tmp_path / "lane-a-empty.log"
    log.write_text("")
    result = run_analysis(log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.FAIL
    assert by_name["INPUT_SCHEDULE"] == Verdict.FAIL
    assert by_name["FIELD_CHAIN"] == Verdict.FAIL
    assert by_name["FIELD3_OR_13"] == Verdict.NOT_RUN


def test_lane_a_no_field_load_markers_fails_closed(tmp_path):
    log = tmp_path / "lane-a-no-fields.log"
    log.write_text("[field-test-input] enabled steps=4096\n")
    result = run_analysis(log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.FAIL
    assert by_name["INPUT_SCHEDULE"] == Verdict.PASS
    assert by_name["FIELD_CHAIN"] == Verdict.FAIL
    assert result["fields"] == []


def test_analyzer_missing_log_returns_not_run_io_code(tmp_path):
    missing = tmp_path / "does-not-exist.log"
    proc = subprocess.run(
        [sys.executable, str(Path(__file__).with_name("analyze_log.py")),
         str(missing)],
        capture_output=True, text=True, check=False,
        env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"},
    )
    assert proc.returncode == 2
    assert "overall=FAIL log_missing=" in proc.stdout


@pytest.mark.parametrize("runner", [
    Path(__file__).parents[2] / "tests" / "run_w34d1_opening_a.sh",
    Path(__file__).parents[2] / "tests" / "run_w34d1_opening_b.sh",
])
def test_runner_binary_missing_returns_not_run_code(runner, tmp_path):
    proc = subprocess.run(
        ["bash", str(runner)],
        capture_output=True, text=True, check=False,
        env={
            **os.environ,
            "XENO_PORT_BINARY": str(tmp_path / "missing-xeno-port"),
            "XENO_N5_OUTDIR": str(tmp_path / "out"),
            "PYTHONDONTWRITEBYTECODE": "1",
        },
    )
    assert proc.returncode == 2
    assert "overall=NOT_RUN" in proc.stdout
    assert "NOT_RUN_RC=2" in proc.stdout
    assert "BUILD=NONE" in proc.stdout


def test_lane_a_tail_transition_to_field3(tmp_path):
    log = tmp_path / "lane-a-tail.log"
    log.write_text("""[field-test-input] enabled steps=4096
[field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
[field-diag] FieldLoad begin field=2 mapBuf=0x8005A4E0
[field-diag] FieldLoad begin field=3 mapBuf=0x8005A4E0
""")
    result = run_analysis(log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.PASS
    assert by_name["FIELD_CHAIN"] == Verdict.PASS
    assert by_name["FIELD3_OR_13"] == Verdict.PASS


def test_lane_a_reload_to_map4_fails(tmp_path):
    log = tmp_path / "lane-a-reload.log"
    log.write_text("""[field-test-input] enabled steps=4096
[field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
[field-diag] FieldLoad begin field=2 mapBuf=0x8005A4E0
[field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
""")
    result = run_analysis(log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert by_name["FIELD_CHAIN"] == Verdict.FAIL
    assert result["overall"] == Verdict.FAIL


def test_lane_b_title_preflight_not_run(tmp_path):
    log = tmp_path / "lane-b.log"
    log.write_text("""[field-test-input] enabled steps=4096
[field-diag] FieldLoad begin field=490 mapBuf=0x8005A4E0
[xeno-port][menu] func_801C58EC title loop enter choice=1 D_80059460=2
""")
    result = run_analysis(log, None, "B")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.PASS
    assert by_name["TITLE_REACHED"] == Verdict.PASS
    assert by_name["NEW_GAME"] == Verdict.NOT_RUN
    assert by_name["FIELD_CHAIN"] == Verdict.NOT_RUN
    assert result["a7_control"] == Verdict.NOT_RUN


def test_lane_b_full_chain(tmp_path):
    log = tmp_path / "lane-b-chain.log"
    log.write_text(textwrap.dedent(
        """\
        [field-test-input] enabled steps=4096
        [field-diag] FieldLoad begin field=490 mapBuf=0x8005A4E0
        [xeno-port][menu] func_801C58EC title loop enter choice=2 D_80059460=2
        New Game -> Field
        [field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
        [field-diag] FieldLoad begin field=2 mapBuf=0x8005A4E0
        """
    ))
    result = run_analysis(log, None, "B")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert result["overall"] == Verdict.PASS
    assert by_name["NEW_GAME"] == Verdict.PASS
    assert by_name["FIELD_CHAIN"] == Verdict.PASS


def test_lane_a_fault_marker_fails(lane_a_pass_log, tmp_path):
    log = tmp_path / "lane-a-fault.log"
    log.write_text("""[field-test-input] enabled steps=1
[field-diag] FieldLoad begin field=4 mapBuf=0x8005A4E0
[field-diag] FieldLoad begin field=2 mapBuf=0x8005A4E0
SIGSEGV
""")
    result = run_analysis(log, None, "A")
    by_name = {name: status for name, status, _ in result["checks"]}
    assert by_name["NO_FAULTS"] == Verdict.FAIL
    assert result["overall"] == Verdict.FAIL

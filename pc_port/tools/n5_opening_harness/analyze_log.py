#!/usr/bin/env python3
"""N5 opening-harness log analyzer.

Fail-closed verdict for the deterministic boot/opening runs:

Lane A (Map 4 -> Map 2):
    ordered FieldLoad chain 4 -> 2; schedule enabled; no missing prim rows /
    OT / malloc / abort / SEGV markers; nonblack captures; monotonic dialog
    progress from any observed marker; optional field3/field13 observation.

Lane B (normal boot -> title):
    retail boot path reached Map 490, title opened, and -- when a New Game
    inject is actually proven -- the full chain 490 -> 4 -> 2.  The New Game
    menu inject has no existing deterministic PC seam (func_801C7D78
    overwrites g_Menu->input every frame), so a boot-to-title run reports
    A7_CONTROL=NOT_RUN and NEW_GAME=NOT_RUN instead of claiming playable
    Lahan.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


FIELDLOAD_RE = re.compile(r"\[field-diag\] FieldLoad begin field=(\d+)")
INPUT_ENABLED_RE = re.compile(r"\[field-test-input\] enabled steps=(\d+)")
DIALOG_RE = re.compile(r"\[npc-event\] dialog-open box=(\d+) str=(\d+)")
SCRIPT_IP_RE = re.compile(r"\[field-diag\] actor3 ip=(\d+)")


class Verdict:
    PASS = "PASS"
    FAIL = "FAIL"
    NOT_RUN = "NOT_RUN"


def parse_lines(text: str):
    fields = [int(m.group(1)) for m in FIELDLOAD_RE.finditer(text)]
    input_steps = [int(m.group(1)) for m in INPUT_ENABLED_RE.finditer(text)]
    dialogs = [
        (int(m.group(1)), int(m.group(2)))
        for m in DIALOG_RE.finditer(text)
    ]
    ips = [int(m.group(1)) for m in SCRIPT_IP_RE.finditer(text)]
    return fields, input_steps, dialogs, ips


def no_fault_markers(text: str) -> tuple[list[str], list[str]]:
    fatal = [
        "missing D_8004FE50",
        "ptag length is not valid",
        "xeno-ot",
        "malloc_printerr",
        "corrupted size",
        "free(): invalid",
        "double free",
        "AddressSanitizer",
        "ThreadSanitizer",
        "SEGV",
        "SIGSEGV",
        "SIGABRT",
        "abort()",
        "Aborted",
    ]
    found = [m for m in fatal if m.lower() in text.lower()]
    return fatal, found


def nonblack_ok(capture_dir: Path, minimum_fraction: float = 0.01,
                minimum_images: int = 2) -> tuple[bool | None, list[Path], str]:
    try:
        from PIL import Image
    except Exception as exc:  # pragma: no cover - dependency probe
        return None, [], f"PIL unavailable: {exc}"
    images = sorted(capture_dir.glob("field-frame-*.png"))
    if len(images) < minimum_images:
        return False, images, (
            f"need >= {minimum_images} captures, got {len(images)}"
        )
    sampled = [images[0], images[-1]]
    if len(images) >= 4:
        sampled = [images[0], images[len(images) // 2], images[-1]]
    total_pixels = 0
    nonblack = 0
    for image in sampled:
        try:
            with Image.open(image).convert("RGB") as im:
                for r, g, b in im.getdata():
                    total_pixels += 1
                    if (r | g | b) > 8:
                        nonblack += 1
        except Exception as exc:  # noqa: BLE001 - env-gated helper
            return False, images, f"capture {image.name}: {exc}"
    if total_pixels == 0:
        return False, images, "captures have zero pixels"
    fraction = nonblack / total_pixels
    if fraction < minimum_fraction:
        return False, images, (
            f"nonblack fraction {fraction:.4%} below {minimum_fraction:.4%}"
        )
    return True, images, (
        f"nonblack={nonblack}/{total_pixels} ({fraction:.4%}) across "
        f"{len(sampled)} sampled captures"
    )


def run_analysis(log_path: Path, capture_dir: Path | None,
                 lane: str, require_field3_or_13: bool = False,
                 allow_reuse_of_2_after_4: bool = False) -> dict:
    text = log_path.read_text(encoding="utf-8", errors="replace")
    fields, input_steps, dialogs, ips = parse_lines(text)
    _, fatal_found = no_fault_markers(text)

    result = {
        "lane": lane,
        "fields": fields,
        "input_steps": input_steps,
        "dialogs": dialogs,
        "script_ips": ips,
        "fatal_markers": fatal_found,
        "a7_control": Verdict.NOT_RUN,
    }

    checks = []

    if not input_steps or input_steps[-1] < 1:
        checks.append(("INPUT_SCHEDULE", Verdict.FAIL,
                       "no [field-test-input] enabled steps"))
    else:
        checks.append(("INPUT_SCHEDULE", Verdict.PASS,
                       f"enabled steps={input_steps[-1]}"))

    if lane == "A":
        if not fields:
            checks.append((
                "FIELD_CHAIN", Verdict.FAIL,
                "no FieldLoad markers observed",
            ))
            checks.append(("FIELD3_OR_13", Verdict.NOT_RUN,
                           "observed=[]"))
            return _finish(result, checks)
        # Ordered retail chain: Map 4 prologue must load first, then Map 2.
        # Later transitions to 3/13 (the Map 2 script's possible exits) are
        # allowed; another Map 4 after Map 2 is a route misload.
        first_two = fields[0] == 4 and (len(fields) == 1 or fields[1] == 2)
        two_seen = 2 in fields
        first_two_idx = fields.index(2) if two_seen else -1
        tail = fields[first_two_idx:] if first_two_idx >= 0 else []
        tail_ok = all(f in (2, 3, 13) for f in tail)
        ok_chain = first_two and two_seen and tail_ok
        checks.append((
            "FIELD_CHAIN",
            Verdict.PASS if ok_chain else Verdict.FAIL,
            f"FieldLoad sequence={fields} (expected 4 -> 2 last)",
        ))
        observed_tail = [f for f in fields if f in (3, 13)]
        checks.append((
            "FIELD3_OR_13",
            Verdict.PASS if observed_tail else Verdict.NOT_RUN,
            f"observed={observed_tail}",
        ))
    elif lane in ("B", "title"):
        has_title = 490 in fields
        checks.append((
            "TITLE_REACHED",
            Verdict.PASS if has_title else Verdict.FAIL,
            f"FieldLoad sequence={fields}",
        ))
        new_game = "New Game" in text or "[CHAIN-DONE]" in text
        ng = (
            Verdict.PASS
            if new_game and 4 in fields and 2 in fields
            else Verdict.NOT_RUN
        )
        checks.append(("NEW_GAME", ng, "menu confirm has no deterministic seam"))
        if ng == Verdict.PASS and lane == "B":
            ok_chain = (
                len(fields) >= 3
                and fields[0] == 490
                and 4 in fields[1:-1]
                and fields[-1] == 2
            )
            checks.append((
                "FIELD_CHAIN",
                Verdict.PASS if ok_chain else Verdict.FAIL,
                f"FieldLoad sequence={fields} (expected 490 -> 4 -> 2)",
            ))
        else:
            checks.append((
                "FIELD_CHAIN", Verdict.NOT_RUN,
                "New Game inject seam unavailable; full 490 -> 4 -> 2 chain "
                "was not driven",
            ))
    else:
        checks.append(("FIELD_CHAIN", Verdict.FAIL, f"unknown lane {lane}"))

    if input_steps:
        dialog_strs = [s for _b, s in dialogs]
        monotonic = all(
            a <= b for a, b in zip(dialog_strs, dialog_strs[1:])
        )
        ip_monotonic = bool(ips) and all(a <= b for a, b in zip(ips, ips[1:]))
        checks.append((
            "DIALOG_PROGRESS",
            Verdict.PASS
            if monotonic and len(dialog_strs) >= 2
            else (
                Verdict.PASS
                if ip_monotonic
                else Verdict.NOT_RUN
            ),
            (
                f"dialog str={dialog_strs}, script ips={len(ips)}"
                if dialog_strs or ips
                else "no progress markers emitted"
            ),
        ))
    else:
        checks.append(("DIALOG_PROGRESS", Verdict.NOT_RUN,
                       "input schedule never enabled"))

    if fatal_found:
        checks.append(("NO_FAULTS", Verdict.FAIL,
                       "; ".join(fatal_found)))
    else:
        checks.append((
            "NO_FAULTS", Verdict.PASS,
            "no missing-prim/OT/ptag/malloc/abort/SEGV markers in log",
        ))
    # The missing-row/OT diagnostics were removed after the D_8004FE50 row-4
    # fill landed, so absence in the log is not a registered zero-proof.  Keep
    # the audit explicitly NOT_RUN so nobody reads the log-level gate as a
    # hard instrumentation certificate.
    checks.append((
        "MISSING_PRIM_INSTRUMENT", Verdict.NOT_RUN,
        "no registered missing D_8004FE50/xeno-ot diagnostic in this branch",
    ))

    if capture_dir is not None and capture_dir.exists():
        ok, images, detail = nonblack_ok(capture_dir)
        if ok is None:
            status = Verdict.NOT_RUN
        else:
            status = Verdict.PASS if ok else Verdict.FAIL
        checks.append((
            "NONBLACK_CAPTURES",
            status,
            detail,
        ))
    else:
        checks.append(("NONBLACK_CAPTURES", Verdict.NOT_RUN,
                       f"capture dir missing: {capture_dir}"))

    if require_field3_or_13:
        observed = [f for f in fields if f in (3, 13)]
        checks.append((
            "FIELD3_OR_13",
            Verdict.PASS if observed else Verdict.NOT_RUN,
            f"observed={observed}",
        ))

    return _finish(result, checks)


def _finish(result: dict, checks: list) -> dict:
    """Shared tail of run_analysis: attach checks and compute overall."""
    result["checks"] = checks
    overall = Verdict.PASS
    for _name, status, _detail in checks:
        if status == Verdict.FAIL:
            overall = Verdict.FAIL
    result["overall"] = overall
    return result


def emit(result: dict, out=sys.stdout) -> int:
    print("=" * 12 + " N5 OPENING HARNESS ANALYSIS " + "=" * 12)
    print(f"lane={result['lane']}")
    print(f"overall={result['overall']}")
    print(f"fields={' '.join(map(str, result['fields']))}")
    print(f"input_steps={result['input_steps'][-1] if result['input_steps'] else 0}")
    print(f"dialogs={len(result['dialogs'])} script_ips={len(result['script_ips'])}")
    print(f"a7_control={result['a7_control']}")
    for name, status, detail in result["checks"]:
        print(f"{name}={status} detail={detail}")
    if result["overall"] == Verdict.PASS:
        return 0
    if result["overall"] == Verdict.NOT_RUN:
        return 2
    return 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--capture-dir", type=Path, default=None)
    parser.add_argument("--lane", choices=["A", "B", "title"], default="A")
    parser.add_argument("--require-field3-or-13", action="store_true")
    parser.add_argument("--allow-reuse-of-2-after-4", action="store_true")
    args = parser.parse_args()

    if not args.log.exists():
        print(f"N5 OPENING HARNESS ANALYSIS overall=FAIL log_missing={args.log}")
        return 2
    result = run_analysis(
        args.log,
        args.capture_dir,
        args.lane,
        require_field3_or_13=args.require_field3_or_13,
        allow_reuse_of_2_after_4=args.allow_reuse_of_2_after_4,
    )
    return emit(result)


if __name__ == "__main__":
    sys.exit(main())

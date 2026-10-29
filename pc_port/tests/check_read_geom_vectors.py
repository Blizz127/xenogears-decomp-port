#!/usr/bin/env python3
"""Test-only vector/provenance checker with built-in negative controls.

usage: check_read_geom_vectors.py <read-geom output dir>
Positive: read-geom.jsonl + read-geom-provenance.json must satisfy the sealed
expectations (1430 cases: 904 ReadGeomOffset / 526 ReadGeomScreen, EVERY case
reaching coverage mask 0xFF resp. 0x7, alias both ways, GTE control unchanged,
only read/write/cfc2 events, provenance SHA == file SHA, reachable words 8 + 3).
Controls (each runs on a corrupted COPY and must FAIL): one vector value altered
with the original provenance; provenance with a wrong vector SHA; one
ReadGeomScreen coverage mask reduced to 0x3; one row removed. Results are
merged into controls.json (with the generator's runner-side controls, if any).
Exit 0 only when the positive check passes AND every control failed as expected.
No retail bytes are read or written here; only generated I/O vectors."""
import hashlib
import json
import pathlib
import sys
import tempfile

EXPECTED_COUNTS = {"ReadGeomOffset": 904, "ReadGeomScreen": 526}
EXPECTED_COVERAGE = {"ReadGeomOffset": 0xFF, "ReadGeomScreen": 0x7}
EXPECTED_WORDS = {"ReadGeomOffset": 8, "ReadGeomScreen": 3}


def digest(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def check(vectors, provenance):
    """Raise AssertionError describing the first violated expectation."""
    receipt = json.loads(pathlib.Path(provenance).read_text())
    assert receipt["vector_sha256"] == digest(vectors), "provenance vector_sha256 differs from the vector file"
    assert receipt.get("reachable_words") == EXPECTED_WORDS, "provenance reachable word counts differ"
    rows = [json.loads(line) for line in pathlib.Path(vectors).read_text().splitlines()]
    assert receipt["case_count"] == len(rows), "provenance case_count differs"
    counts, coverage, aliases = {}, {}, set()
    for row in rows:
        name = row["function"]
        counts[name] = counts.get(name, 0) + 1
        coverage[name] = coverage.get(name, 0) | row["instruction_coverage_mask"]
        # Straight-line leaf functions: EVERY case must fetch every reachable word, so a
        # single case that skipped a reachable instruction is a defect, not noise.
        assert row["instruction_coverage_mask"] == EXPECTED_COVERAGE.get(name), (
            "case %s of %s reached mask 0x%X, expected 0x%X" % (row.get("case"), name, row["instruction_coverage_mask"], EXPECTED_COVERAGE.get(name, 0)))
        assert row["final_control"] == row["input_control"], "GTE control state changed"
        assert all(e["op"] in ("read", "write", "cfc2") for e in row["trace"]), "unexpected trace op"
        if name == "ReadGeomOffset":
            aliases.add(row["alias"])
        else:
            assert "return_u32" in row, "ReadGeomScreen row without return value"
    assert counts == EXPECTED_COUNTS, "case counts %r" % counts
    assert coverage == EXPECTED_COVERAGE, "coverage masks %r" % coverage
    assert aliases == {True, False}, "alias coverage incomplete"
    return {"cases": counts, "coverage_masks": coverage, "vector_sha256": receipt["vector_sha256"]}


def corrupt_controls(vectors, provenance, scratch):
    """Return control results; each must raise inside check()."""
    lines = pathlib.Path(vectors).read_text().splitlines()
    receipt = json.loads(pathlib.Path(provenance).read_text())
    results = []

    def run(name, rows_text, receipt_obj):
        v = scratch / (name + ".jsonl"); v.write_text(rows_text)
        p = scratch / (name + ".json"); p.write_text(json.dumps(receipt_obj))
        try:
            check(v, p)
            results.append({"control": name, "expected": "FAIL", "observed": "PASS", "ok": False})
        except AssertionError as error:
            results.append({"control": name, "expected": "FAIL", "observed": "FAIL: " + str(error), "ok": True})

    altered = json.loads(lines[-1])
    altered["return_u32"] = (altered.get("return_u32", 0) + 1) & 0xFFFFFFFF
    altered["output"] = altered["output"][:-1] + ("0" if altered["output"][-1] != "0" else "1")
    run("vector-value-altered", "\n".join(lines[:-1] + [json.dumps(altered, separators=(",", ":"))]) + "\n", receipt)
    run("provenance-wrong-sha", "\n".join(lines) + "\n", dict(receipt, vector_sha256="0" * 64))
    reduced = None
    for i, line in enumerate(lines):
        row = json.loads(line)
        if row["function"] == "ReadGeomScreen":
            row["instruction_coverage_mask"] = 0x3
            reduced = lines[:i] + [json.dumps(row, separators=(",", ":"))] + lines[i + 1:]
            break
    text = "\n".join(reduced) + "\n"
    run("coverage-mask-reduced", text, dict(receipt, vector_sha256=hashlib.sha256(text.encode()).hexdigest()))
    text = "\n".join(lines[1:]) + "\n"
    run("row-removed", text, dict(receipt, vector_sha256=hashlib.sha256(text.encode()).hexdigest()))
    return results


def main():
    out = pathlib.Path(sys.argv[1])
    vectors, provenance = out / "read-geom.jsonl", out / "read-geom-provenance.json"
    report = {"positive": None, "controls": [], "status": None}
    existing = out / "controls.json"
    if existing.is_file():                         # generator-side controls recorded by the run script
        report["controls"] = json.loads(existing.read_text()).get("controls", [])
    try:
        report["positive"] = {"status": "PASS", **check(vectors, provenance)}
    except AssertionError as error:
        report["positive"] = {"status": "FAIL", "reason": str(error)}
    with tempfile.TemporaryDirectory(prefix="read-geom-controls.") as scratch:
        report["controls"] += corrupt_controls(vectors, provenance, pathlib.Path(scratch))
    ok = report["positive"]["status"] == "PASS" and all(c["ok"] for c in report["controls"])
    report["status"] = "all controls behaved; positive PASS" if ok else "GATE FAILED"
    existing.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""W18I forbidden-instrumentation summary checker.

Validates the machine-readable summary emitted by the world-map runtime
harnesses (pc_port/tools/world_harness/*.gdb). A forbidden target is proven
absent only when its instrumentation was successfully registered and its
measured hit count is zero: required targets pass only as ZERO_VERIFIED.

Rules enforced:
  * exactly one summary block, parseable target records
  * no duplicate labels
  * every required label present
  * required labels registered (registered=1)
  * no undefined hit counts on registered targets
  * no negative hit counts
  * no unknown status strings
  * no status/count contradictions
  * no legacy bare numeric dictionaries anywhere in the log
  * every required target ZERO_VERIFIED

Exit status: 0 = PASS, 1 = FAIL, 2 = usage/IO error.
"""

import argparse
import re
import sys

VALID_STATUSES = ("ZERO_VERIFIED", "HIT", "NOT_INSTRUMENTED",
                  "INSTRUMENTATION_ERROR")

SUMMARY_BEGIN_RE = re.compile(r"^(\w+)_FORBIDDEN_SUMMARY_BEGIN$")
TARGET_RE = re.compile(r"^target\s+(.*)$")
KV_RE = re.compile(r"(\w+)=(\S+)")

# Legacy false-zero shape: a Python dict literal with quoted (often hex)
# labels mapped to bare integers, e.g. forbid={'74e58': 0, '863e0': 0}.
LEGACY_DICT_RE = re.compile(r"['\"]([0-9a-fA-Fx_]{3,32})['\"]\s*:\s*-?\d+")


class CheckFailure(Exception):
    pass


def parse_summary(lines):
    """Returns (tag, required_labels, records) or raises CheckFailure."""
    begin_idx = None
    tag = None
    for i, line in enumerate(lines):
        m = SUMMARY_BEGIN_RE.match(line.strip())
        if m:
            if begin_idx is not None:
                raise CheckFailure("multiple summary blocks found")
            begin_idx = i
            tag = m.group(1)
    if begin_idx is None:
        raise CheckFailure("no *_FORBIDDEN_SUMMARY_BEGIN block found")

    end_marker = "%s_FORBIDDEN_SUMMARY_END" % tag
    end_idx = None
    for i in range(begin_idx + 1, len(lines)):
        if lines[i].strip() == end_marker:
            end_idx = i
            break
    if end_idx is None:
        raise CheckFailure("summary block not terminated by %s" % end_marker)

    required_labels = []
    records = {}
    for raw in lines[begin_idx + 1:end_idx]:
        line = raw.strip()
        if not line:
            continue
        if line.startswith("required="):
            value = line.split("=", 1)[1].strip()
            if value and value != "-":
                required_labels = [x for x in value.split(",") if x]
            continue
        m = TARGET_RE.match(line)
        if not m:
            raise CheckFailure("unparseable summary line: %r" % line)
        fields = dict(KV_RE.findall(m.group(1)))
        label = fields.get("label")
        if not label:
            raise CheckFailure("summary record without label: %r" % line)
        if label in records:
            raise CheckFailure("duplicate label in summary: %s" % label)
        records[label] = fields
    return tag, required_labels, records


def parse_hits(value, label):
    if value == "undefined":
        return None
    try:
        return int(value)
    except (TypeError, ValueError):
        raise CheckFailure("label %s: malformed hits value %r" %
                           (label, value))


def check_record(label, fields):
    """Returns a list of violation strings for one record."""
    problems = []
    status = fields.get("status")
    registered_raw = fields.get("registered")
    hits_raw = fields.get("hits")

    if status not in VALID_STATUSES:
        problems.append("label %s: unknown status %r" % (label, status))
        return problems

    if registered_raw not in ("0", "1"):
        problems.append("label %s: malformed registered %r" %
                        (label, registered_raw))
        return problems
    registered = registered_raw == "1"

    hits = parse_hits(hits_raw, label)
    if hits is not None and hits < 0:
        problems.append("label %s: negative hit count %d" % (label, hits))

    if registered and hits is None:
        problems.append("label %s: registered target with undefined hits" %
                        label)

    if status == "ZERO_VERIFIED":
        if not registered:
            problems.append("label %s: ZERO_VERIFIED without registration" %
                            label)
        if hits != 0:
            problems.append("label %s: ZERO_VERIFIED but hits=%r" %
                            (label, hits_raw))
    elif status == "HIT":
        if not registered:
            problems.append("label %s: HIT without registration" % label)
        elif hits is not None and hits <= 0:
            problems.append("label %s: HIT but hits=%r" % (label, hits_raw))
    elif status == "NOT_INSTRUMENTED":
        if registered:
            problems.append("label %s: NOT_INSTRUMENTED but registered=1" %
                            label)
        if hits is not None and hits != 0:
            problems.append("label %s: NOT_INSTRUMENTED but hits=%r" %
                            (label, hits_raw))
    # INSTRUMENTATION_ERROR: registered may be either; hits may be undefined.
    return problems


def scan_legacy_dicts(lines):
    """Lines that look like the pre-W18I bare numeric forbid dictionaries."""
    hits = []
    for i, line in enumerate(lines):
        if "{" not in line:
            continue
        matches = LEGACY_DICT_RE.findall(line)
        # A single quoted-number pair can appear in legitimate text; the
        # legacy shape is a brace group with at least one such pair attached
        # to an identifier (forbid=..., hits=...).
        if matches and re.search(r"\w+\s*=\s*\{", line):
            hits.append((i + 1, line.strip()))
    return hits


def check_log(text, required_override=None):
    """Returns (exit_code, report_lines)."""
    lines = text.splitlines()
    report = []

    legacy = scan_legacy_dicts(lines)
    if legacy:
        for lineno, line in legacy[:5]:
            report.append("REJECT legacy numeric dictionary at line %d: %s" %
                          (lineno, line))
        report.append("RESULT FAIL legacy_numeric_dictionary")
        return 1, report

    try:
        tag, required_labels, records = parse_summary(lines)
    except CheckFailure as exc:
        report.append("REJECT %s" % exc)
        report.append("RESULT FAIL unparseable")
        return 1, report

    if required_override is not None:
        required_labels = required_override

    problems = []
    for label, fields in records.items():
        problems.extend(check_record(label, fields))

    for label in required_labels:
        if label not in records:
            problems.append("required label missing from summary: %s" % label)
            continue
        fields = records[label]
        if fields.get("registered") != "1":
            problems.append("required label not registered: %s" % label)
        if fields.get("status") != "ZERO_VERIFIED":
            problems.append("required label not ZERO_VERIFIED: %s (status=%s)"
                            % (label, fields.get("status")))

    if problems:
        for p in problems:
            report.append("REJECT %s" % p)
        report.append("RESULT FAIL required=%d targets=%d problems=%d" %
                      (len(required_labels), len(records), len(problems)))
        return 1, report

    report.append("OK tag=%s required=%d targets=%d all required "
                  "ZERO_VERIFIED" % (tag, len(required_labels), len(records)))
    report.append("RESULT PASS")
    return 0, report


def main(argv):
    parser = argparse.ArgumentParser(
        description="Check a world forbidden-instrumentation summary log.")
    parser.add_argument("logfile", help="gdb harness stdout log")
    parser.add_argument("--required",
                        help="comma-separated required labels (overrides the "
                             "summary's required= line)")
    args = parser.parse_args(argv)

    try:
        with open(args.logfile, "r", errors="replace") as f:
            text = f.read()
    except OSError as exc:
        print("ERROR cannot read %s: %s" % (args.logfile, exc))
        return 2

    override = None
    if args.required:
        override = [x for x in args.required.split(",") if x]

    code, report = check_log(text, override)
    for line in report:
        print(line)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

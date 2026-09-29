#!/usr/bin/env python3
"""Audit pc_port/tests/run_*.sh for the "a partial run looks like a pass" shape.

Three dead gates were found in one session, all with the same structure: the
suite emits PASS text EARLY -- from inside a test binary, or from an echo after
the first regime -- and then abandons later regimes, mutant controls or whole
link steps. Under `set -e` the script does exit nonzero, so the script is not
lying; but a reader (or an agent writing an evidence document) sees PASS lines
and records a pass.

A suite is flagged UNDER-RUN-RISK when BOTH hold:

  * it emits PASS/OK/CERTIFICATE text somewhere that is NOT its last statement
    -- either the runner echoes it early, or a test binary it runs prints its
    own PASS; and
  * it has more work afterwards: another regime, a mutant/negative control, or
    any further executable statement.

That is the exact shape of run_battle_graphics_abi_test.sh, whose O0 binary
printed two PASS lines and returned 1 with no message, silently skipping O2,
UBSan and both mutant controls.

Usage:  python3 pc_port/tests/lib/underrun_audit.py [--list]
"""
import os
import re
import subprocess
import sys

TESTS = "pc_port/tests"
PASSY = re.compile(r"\b(PASS|CERTIFICATE)\b")
# a test binary printing its own success marker
CPASS = re.compile(r"""(?:puts|printf|fputs)\s*\(\s*"[^"]*\b(?:PASS|CERTIFICATE)\b""")


def statements(path):
    """non-blank, non-comment lines with their line numbers"""
    out = []
    for i, raw in enumerate(open(path, errors="replace"), 1):
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        out.append((i, s))
    return out


def referenced_tests(src):
    return set(re.findall(r"pc_port/tests/([A-Za-z0-9_]+\.c)", src))


def main():
    show = "--list" in sys.argv
    runners = sorted(
        f for f in os.listdir(TESTS)
        if f.startswith("run_") and f.endswith(".sh") and f != "run_ubsan_sweep.sh"
    )
    flagged, clean = [], []
    for r in runners:
        p = os.path.join(TESTS, r)
        src = open(p, errors="replace").read()
        st = statements(p)
        if not st:
            continue
        last_line = st[-1][0]

        early = []
        # (a) the runner itself echoes PASS before its final statement
        for ln, s in st[:-1]:
            if re.match(r"(echo|printf)\b", s) and PASSY.search(s):
                early.append(("runner echo", ln))
        # (b) a test binary it builds prints its own PASS, and the runner
        #     runs binaries more than once (multiple regimes)
        runs = len(re.findall(r'"\$(?:OUT|BUILD_DIR|TEST_OUT)[^"]*/\$?\{?\w+\}?"', src))
        for c in referenced_tests(src):
            cp = os.path.join(TESTS, c)
            if os.path.isfile(cp) and CPASS.search(open(cp, errors="replace").read()):
                if runs >= 2:
                    early.append((f"{c} prints PASS, {runs} binary invocations", 0))
                break

        if early:
            flagged.append((r, early, last_line))
        else:
            clean.append(r)

    print("== under-run audit of pc_port/tests/run_*.sh ==")
    print(f"runners audited      : {len(runners)}")
    print(f"UNDER-RUN-RISK       : {len(flagged)}")
    print(f"no early PASS text   : {len(clean)}")
    print()
    print("UNDER-RUN-RISK means: this suite can print PASS and still be")
    print("abandoning later regimes or controls. It is not proof of a dead")
    print("gate; it is the shape that let three dead gates hide this session.")
    if show:
        print()
        for r, early, _ln in flagged:
            why = "; ".join(w for w, _ in early[:2])
            print(f"  {r}: {why}")


if __name__ == "__main__":
    main()

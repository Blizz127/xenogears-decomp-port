#!/usr/bin/env python3
"""Compare two XENO_PRESENT_HASH_LOG files (run_hd2d_off_identity_test.sh).

Each line: "<index> <S|V> <w>x<h> <frame-crc32> <input-key> <schedule-frame>".
The input key is a CRC of what the present was rendered from, so presents of
two wall-clock-paced runs can be paired even when their indices drift.

PASS when every present key seen (unambiguously) in both runs has the same
frame CRC in both, and at least MIN_S / MIN_V percent of each run's scene (S)
and VRAM (V) presents are paired -- or when the two logs are identical index
for index. VRAM presents are mostly STR movie frames, which the wall-clock CD
pacing samples differently run to run, so their floor is lower. A key with
two different frame CRCs inside one run is ambiguous (the output depended on history the key does not cover); it is reported and
left out of pairing.

usage: hd2d_present_hash_compare.py A.txt B.txt MIN_S MIN_V LABEL
"""
import sys
from collections import defaultdict


def load(path):
    rows = []
    with open(path) as f:
        for line in f:
            p = line.split()
            if len(p) >= 5:
                rows.append((p[1], p[2], p[3], p[4]))  # path, size, crc, key
    return rows


def main():
    fa, fb, label = sys.argv[1], sys.argv[2], sys.argv[5]
    minp = {"S": float(sys.argv[3]), "V": float(sys.argv[4])}
    a, b = load(fa), load(fb)

    def keymap(rows):
        m = defaultdict(set)
        for path, size, crc, key in rows:
            m[(path, size, key)].add(crc)
        return m

    ma, mb = keymap(a), keymap(b)
    ambiguous = {k for k in set(ma) | set(mb)
                 if len(ma.get(k, ())) > 1 or len(mb.get(k, ())) > 1}
    common = (set(ma) & set(mb)) - ambiguous
    bad = sorted(k for k in common if ma[k] != mb[k])
    ok = not bad and bool(a) and bool(b)
    cov = []
    for name, rows in (("A", a), ("B", b)):
        for path in ("S", "V"):
            tot = sum(1 for r in rows if r[0] == path)
            paired = sum(1 for r in rows if r[0] == path and (r[0], r[1], r[3]) in common)
            pct = 100.0 * paired / tot if tot else 100.0
            cov.append(f"{name}.{path}={paired}/{tot}({pct:.0f}%)")
            if tot and pct < minp[path]:
                ok = False
    identical = bool(a) and [r[:3] for r in a] == [r[:3] for r in b]
    verdict = ok or identical
    print(f"HD2D OFF-IDENTITY {label} index_identical={'yes' if identical else 'no'} "
          f"presents={len(a)}/{len(b)} paired_keys={len(common)} mismatched_keys={len(bad)} "
          f"ambiguous_keys={len(ambiguous)} coverage {' '.join(cov)} -> "
          f"{'PASS' if verdict else 'FAIL'}")
    for k in bad[:8]:
        print(f"HD2D OFF-IDENTITY {label} mismatch path={k[0]} key={k[2]} "
              f"A={sorted(ma[k])} B={sorted(mb[k])}")
    return 0 if verdict else 1


if __name__ == "__main__":
    sys.exit(main())

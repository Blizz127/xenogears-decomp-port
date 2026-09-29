#!/usr/bin/env python3
"""Self-tests for the port policy guards; no retail data needed.

The retail-data guard is exercised against a synthetic reference blob (random
bytes standing in for a disc file): a C table copying a slice of it must be
caught in every encoding the guard claims to handle, and unrelated numbers,
counters and fill must not be.  The Psy-Q guard is checked for a sane
classification and for catching a Psy-Q TU / func_ in synthetic sources.
"""
from __future__ import annotations

import random
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import psyq_classify  # noqa: E402
import psyq_port_guard  # noqa: E402
import retail_data_guard as rdg  # noqa: E402

failures = 0


def check(cond, what):
    global failures
    print(("ok   " if cond else "FAIL ") + what)
    if not cond:
        failures += 1


def hits(text: bytes, ref) -> list:
    return [h for h in rdg.scan_bytes("x.c", text, ref, set()) if h[4] == "data"]


rng = random.Random(1234)
blob = bytes(rng.randrange(256) for _ in range(4096))
ref = rdg.Reference()
ref.add_blob("FAKE.BIN", blob, 0x80100000)

sl = blob[0x200:0x280]
u8 = ", ".join(f"0x{b:02x}" for b in sl).encode()
u16 = ", ".join(str(int.from_bytes(sl[i:i + 2], "little")) for i in range(0, len(sl), 2)).encode()
u32 = ", ".join(f"0x{int.from_bytes(sl[i:i + 4], 'little'):08X}u" for i in range(0, len(sl), 4)).encode()
check(bool(hits(b"unsigned char t[] = {" + u8 + b"};", ref)), "u8 hex table copied from reference is caught")
check(bool(hits(b"u16 t[] = {" + u16 + b"};", ref)), "u16 decimal table is caught")
check(bool(hits(b"u32 t[] = {" + u32 + b"};", ref)), "u32 table is caught")
pairs = []
for i in range(0, len(sl), 4):
    pairs.append(f"{{{i}, 0x{int.from_bytes(sl[i:i + 4], 'little'):08X}}}")
check(bool(hits(("r t[] = {" + ", ".join(pairs) + "};").encode(), ref)), "{index, value} pair table is caught")
esc = "".join(f"\\x{b:02x}" for b in sl).encode()
check(bool(hits(b'const char* s = "' + esc + b'";', ref)), "escaped string literal is caught")
check(bool(hits(bytes(sl) + b"\xff\xfe", ref)) or bool(rdg.scan_bytes("x.bin", b"\x00\xff" + sl, ref, set())),
      "binary file carrying the bytes is caught")
other = ", ".join(f"0x{rng.randrange(256):02x}" for _ in range(256)).encode()
check(not hits(b"unsigned char t[] = {" + other + b"};", ref), "unrelated random table passes")
check(not hits(b"int t[] = {" + b", ".join(str(i).encode() for i in range(200)) + b"};", ref),
      "counter/identity table passes")
check(not hits(b"u8 z[64] = {" + b"0, " * 64 + b"};", ref), "zero fill passes")
check(rdg.address_only((0x80010000).to_bytes(4, "little") * 4 + bytes(16)), "NULL-terminated pointer table is an address table")

# signatures: hashes of windows, never bytes
with tempfile.TemporaryDirectory() as t:
    sig = Path(t) / "sig.txt"
    w = blob[0x200:0x220]
    sig.write_text(f"{rdg.zlib.crc32(w):08x} {rdg.blake8(w)} FAKE.BIN@0x80100200\n")
    sref = rdg.Reference()
    check(sref.load_signatures(sig) == 1, "signature file loads")
    check(bool(hits(b"unsigned char t[] = {" + u8 + b"};", sref)), "signature-only mode catches the table")
    check(w.hex() not in sig.read_text() and bytes(w) not in sig.read_bytes(), "signature file holds no bytes")

# Psy-Q classification and guard
cls = psyq_classify.Classification()
check(len(cls.ranges) > 40, f"Psy-Q text ranges parsed ({len(cls.ranges)})")
check(cls.names.get("SpuSetNoiseClock", (0, ""))[1] == "libspu", "SpuSetNoiseClock classified libspu")
check(cls.library(0x80019560) is None, "game code (ClearMemory) is not Psy-Q")
with tempfile.TemporaryDirectory() as t:
    tree = Path(t) / "pc_port"
    (tree / "src").mkdir(parents=True)
    a = next(iter(sorted(a for a, _ in cls.names.values())))
    (tree / "src" / "x.c").write_text(
        f'#include "../../src/slus_006.64/psyq/libgte.c"\nint func_{a:08X}(void) {{ return 0; }}\n'
        "long SpuSetNoiseClock(long n) { return n; }\n")
    kinds = {f[0] for f in psyq_port_guard.check_sources(tree, cls)}
    check({"include", "func"} <= kinds, f"Psy-Q include and func_ in a port source are caught ({sorted(kinds)})")

print("port guard self-test:", "FAIL" if failures else "PASS")
sys.exit(1 if failures else 0)

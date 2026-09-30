#!/usr/bin/env python3
"""Apply this project's PC-port adjustments to the upstream Psy-Q headers.

The Psy-Q headers (include/psyq/*.h) are Sony SDK files. They are not
redistributed here; tools/fetch_upstream_toolchain.sh copies them from the
upstream decompilation (ladysilverberg/xenogears-decomp).  This project's tree
adds a few port-only blocks to two of them.  This script re-applies those
blocks to the fetched copies, anchored on macro and type names, and then
checks each result against the sha256 of the header this project builds with.

Usage: adjust_upstream_psyq_headers.py [include/psyq]
Exit status: 0 when both headers end up byte-identical to the expected ones
(or already were), 1 otherwise.  The script never writes a file it could not
fully adjust.
"""
import hashlib
import re
import sys
from pathlib import Path

EXPECTED = {
    "inline_c.h": "9e179fc547018c2c2926c7e19388d9e582dc0795f93c4d32ccdd49a9510859d8",
    "libspu.h": "2f1b906132daa737211243cb65c1c3f90282d1d10b236753c14eeccff44899b5",
}
UPSTREAM = {
    "inline_c.h": "2f1261e534cc811d6f337f21e1a854d69c3c92c7ab93d38f2444f25ff23223cd",
    "libspu.h": "15b05961db2abb0ea18e45a6035eebd80520e5bffd8407c149f1ea9cba5092b4",
}

GTEREG = "#ifdef XENO_PC_PORT\n#include <psx/gtereg.h>\n#endif\n\n"

BACK_COLOR = (
    "#ifdef XENO_PC_PORT\n"
    "/* PsyCross owns the host GTE register file. The retail inline sequence uses\n"
    " * MIPS temporaries $12/$13/$14 before ctc2; the host API preserves its\n"
    " * component << 4 writes without requiring MIPS register names. */\n"
    "#define gte_SetBackColor( r0, r1, r2 ) SetBackColor((r0), (r1), (r2))\n"
    "#else\n"
)
FAR_COLOR = (
    "#ifdef XENO_PC_PORT\n"
    "/* Same MIPS-temporary sequence as gte_SetBackColor; delegate to PsyCross. */\n"
    "#define gte_SetFarColor( r0, r1, r2 ) SetFarColor((r0), (r1), (r2))\n"
    "#else\n"
)
STORE_TAIL = (
    "\n"
    "#ifdef XENO_PC_PORT\n"
    "/* Host equivalents for the swc2 macros used by func_80030EE8.  PsyCross\n"
    " * exposes the emulated CP2 data registers through gtereg.h, so preserve the\n"
    " * retail register-to-memory transfer without emitting MIPS instructions. */\n"
    "#undef gte_stsxy0\n"
    "#undef gte_stsxy1\n"
    "#undef gte_stsxy2\n"
    "#undef gte_stsz3\n"
    "#define gte_stsxy0(r0) (*(unsigned int *)(r0) = C2_SXY0)\n"
    "#define gte_stsxy1(r0) (*(unsigned int *)(r0) = C2_SXY1)\n"
    "#define gte_stsxy2(r0) (*(unsigned int *)(r0) = C2_SXY2)\n"
    "#define gte_stsz3(r0, r1, r2) \\\n"
    "\tdo { *(unsigned int *)(r0) = C2_SZ1; \\\n"
    "\t     *(unsigned int *)(r1) = C2_SZ2; \\\n"
    "\t     *(unsigned int *)(r2) = C2_SZ3; } while (0)\n"
    "#endif\n"
)
# Retail layout checks for the three SPU attribute structs (port build only).
LAYOUT = {
    "SpuReverbAttr": (0x14, [("mask", 0x0), ("mode", 0x4), ("depth", 0x8),
                             ("delay", 0xC), ("feedback", 0x10)]),
    "SpuExtAttr": (0xC, [("volume", 0x0), ("reverb", 0x4), ("mix", 0x8)]),
    "SpuCommonAttr": (0x28, [("mask", 0x0), ("mvol", 0x4), ("mvolmode", 0x8),
                             ("mvolx", 0xC), ("cd", 0x10), ("ext", 0x1C)]),
}
PAD = " " * len("_Static_assert(")


def layout_block() -> str:
    groups = []
    for struct, (size, fields) in LAYOUT.items():
        g = [f"_Static_assert(sizeof({struct}) == 0x{size:X},\n"
             f'{PAD}"{struct} must retain its retail size");\n']
        for name, off in fields:
            g.append(f"_Static_assert(__builtin_offsetof({struct}, {name}) == 0x{off:X},\n"
                     f'{PAD}"{struct} {name} must retain its retail offset");\n')
        groups.append("".join(g))
    return "#ifdef XENO_PC_PORT\n" + "\n".join(groups) + "#endif\n\n"


def macro_end(text: str, start: int) -> int:
    """Offset just past the last line of the #define beginning at start."""
    pos = start
    while True:
        nl = text.index("\n", pos)
        if not text[pos:nl].rstrip().endswith("\\"):
            return nl + 1
        pos = nl + 1


def one(text: str, needle: str) -> int:
    i = text.find(needle)
    if i < 0 or text.find(needle, i + 1) >= 0:
        raise ValueError(f"anchor not found exactly once: {needle!r}")
    return i


def wrap_macro(text: str, name: str, prefix: str) -> str:
    i = one(text, f"#define {name}(")
    j = macro_end(text, i)
    return text[:i] + prefix + text[i:j] + "#endif\n" + text[j:]


def adjust_inline_c(text: str) -> str:
    i = one(text, "#define gte_ldv0(")
    text = text[:i] + GTEREG + text[i:]
    text = wrap_macro(text, "gte_SetBackColor", BACK_COLOR)
    text = wrap_macro(text, "gte_SetFarColor", FAR_COLOR)
    for name, word in (("gte_rtps", "0x0000007f"), ("gte_rtpt", "0x000000bf")):
        m = re.search(r"^ ?#define " + name + r"\(\)", text, re.M)
        if not m:
            raise ValueError(f"{name} not found")
        j = macro_end(text, m.start())
        body = text[m.start():j].lstrip(" ")
        body, n = re.subn(r'"c2\s+0x[0-9A-Fa-f]+;"', f'".word {word}"', body)
        if n != 1:
            raise ValueError(f"{name}: expected one cop2 word")
        text = text[:m.start()] + body + text[j:]
    return text + STORE_TAIL


def adjust_libspu(text: str) -> str:
    a = one(text, "} SpuVoiceAttr;")
    b = one(text, "} SpuCommonAttr;")
    region = text[a:b]
    subs = [
        (r"^(\s+)unsigned long(\tmask;)", r"\1unsigned int\2", 2),
        (r"^(\s+)long(\t\tmode;)", r"\1int\2", 1),
        (r"^    long {16}(delay|feedback)", "    int" + " " * 17 + r"\1", 2),
        (r"^    long\t(reverb|mix);", r"    int\t\t\1;", 2),
    ]
    for pat, rep, want in subs:
        region, n = re.subn(pat, rep, region, flags=re.M)
        if n != want:
            raise ValueError(f"libspu.h: {pat!r} matched {n}, expected {want}")
    text = text[:a] + region + text[b:]
    i = one(text, "#ifndef __SPU_IRQCALLBACK_PROC")
    return text[:i] + layout_block() + text[i:]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    d = Path(sys.argv[1] if len(sys.argv) > 1 else "include/psyq")
    ok = True
    for name, fn in (("inline_c.h", adjust_inline_c), ("libspu.h", adjust_libspu)):
        p = d / name
        if not p.is_file():
            print(f"[psyq-adjust] {p}: missing", file=sys.stderr)
            ok = False
            continue
        cur = p.read_bytes()
        if sha(cur) == EXPECTED[name]:
            print(f"[psyq-adjust] {name}: already adjusted")
            continue
        if sha(cur) != UPSTREAM[name]:
            print(f"[psyq-adjust] {name}: not the pinned upstream file; left as is", file=sys.stderr)
            ok = False
            continue
        try:
            new = fn(cur.decode()).encode()
        except ValueError as e:
            print(f"[psyq-adjust] {name}: {e}; left as is", file=sys.stderr)
            ok = False
            continue
        if sha(new) != EXPECTED[name]:
            print(f"[psyq-adjust] {name}: result does not match the expected header; left as is",
                  file=sys.stderr)
            ok = False
            continue
        p.write_bytes(new)
        print(f"[psyq-adjust] {name}: adjusted (sha256 verified)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

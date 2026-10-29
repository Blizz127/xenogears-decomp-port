#!/usr/bin/env python3
"""Emit deliberately-wrong copies of one world-map port body (negative controls).

  wm_mutant.py NAME OUTDIR [--max N]

Finds the definition of NAME in the world-map TUs that pc_port/build_port.sh
links, then writes up to N mutated copies of that TU to OUTDIR/NAME.<k>.c, each
with exactly one edit inside NAME's body: an integer literal changed by +1
(0 -> 1, 0x10 -> 0x11 ...). Comments, string/char literals, preprocessor lines
and array declarators are skipped. OUTDIR/NAME.list gets one line per mutant:
  <k> <tu> <line> <old> -> <new>
The differential must report FAIL for the mutant it keeps; the runner records
which candidate (by k) was the first rejected one, so nothing is selected away.
"""
import re, sys, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]


def linked_world_tus():
    bp = (ROOT / "pc_port/build_port.sh").read_text()
    out = []
    for m in re.finditer(r'^\s*(pc_port/src/world_map_[\w]+\.c)', bp, re.M):
        if m.group(1) not in out:
            out.append(m.group(1))
    return out


def mask_noncode(s):
    """Same-length copy with comments, strings, chars and # lines blanked."""
    o, i, n = list(s), 0, len(s)
    bol = True
    while i < n:
        c = s[i]
        if bol and c == "#":
            j = i
            while j < n and not (s[j] == "\n" and s[j - 1] != "\\"):
                o[j] = " " if s[j] != "\n" else "\n"
                j += 1
            i = j
            continue
        if c in " \t":
            i += 1
            continue
        bol = c == "\n"
        if s.startswith("/*", i):
            j = s.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if o[k] != "\n":
                    o[k] = " "
            i = j
            continue
        if s.startswith("//", i):
            j = s.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                o[k] = " "
            i = j
            continue
        if c in "\"'":
            j = i + 1
            while j < n and s[j] != c:
                j += 2 if s[j] == "\\" else 1
            for k in range(i, min(j + 1, n)):
                if o[k] != "\n":
                    o[k] = " "
            i = j + 1
            continue
        i += 1
    return "".join(o)


def find_body(code, name):
    for m in re.finditer(r'\b%s\s*\(' % re.escape(name), code):
        # definition: at line scope, and the next non-space after ')' is '{'
        depth, j = 0, m.end() - 1
        while j < len(code):
            if code[j] == "(":
                depth += 1
            elif code[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        k = j + 1
        while k < len(code) and code[k] in " \t\r\n":
            k += 1
        if k >= len(code) or code[k] != "{":
            continue
        line_start = code.rfind("\n", 0, m.start()) + 1
        if code[line_start:m.start()].strip().endswith(("=", ",", "return")):
            continue
        depth, e = 0, k
        while e < len(code):
            if code[e] == "{":
                depth += 1
            elif code[e] == "}":
                depth -= 1
                if depth == 0:
                    return k, e + 1
            e += 1
    return None


LIT = re.compile(r'(?<![\w.])(0[xX][0-9A-Fa-f]+|\d+)([uUlL]*)(?![\w.])')


def main():
    name, outdir = sys.argv[1], pathlib.Path(sys.argv[2])
    mx = int(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 8
    outdir.mkdir(parents=True, exist_ok=True)
    for tu in linked_world_tus():
        src = (ROOT / tu).read_text(errors="ignore")
        code = mask_noncode(src)
        span = find_body(code, name)
        if not span:
            continue
        a, b = span
        rows = []
        for m in LIT.finditer(code, a, b):
            # skip array declarators / subscripts of declarations: "[N]" right after an identifier type
            pre = code[max(a, m.start() - 1):m.start()]
            post = code[m.end(2):m.end(2) + 1]
            if pre == "[" and post == "]" and re.search(r'\b(u8|s8|u16|s16|u32|s32|int|char|short|long)\b[^;=]*\w\s*$',
                                                      code[code.rfind("\n", 0, m.start()):m.start() - 1]):
                continue
            lit, suf = m.group(1), m.group(2)
            v = int(lit, 0) if not (len(lit) > 1 and lit[0] == "0" and lit[1] not in "xX") else int(lit, 8)
            new = (("0x%X" % (v + 1)) if lit[:2].lower() == "0x" else str(v + 1)) + suf
            rows.append((m.start(), m.end(), lit + suf, new))
        lst = []
        for k, (s0, s1, old, new) in enumerate(rows[:mx]):
            mut = src[:s0] + new + src[s1:]
            (outdir / f"{name}.{k}.c").write_text(mut)
            line = src.count("\n", 0, s0) + 1
            lst.append(f"{k} {tu} {line} {old} -> {new}")
        (outdir / f"{name}.list").write_text("\n".join(lst) + ("\n" if lst else ""))
        print(f"wm_mutant: {name} in {tu}: {len(lst)} candidates")
        return 0
    print(f"wm_mutant: {name}: definition not found", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())

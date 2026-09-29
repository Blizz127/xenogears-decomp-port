#!/usr/bin/env python3
"""Generate the world-map retail-vs-port differential harness inputs.

Compiles every world-map port TU that pc_port/build_port.sh links (the bodies
under test are the bodies the port ships), then emits into OUT:

  objs.txt       object files to link
  wmd_stubs.c    one recording stub per symbol the TUs reference but do not
                 define (slus/psyq/port-runtime functions), plus storage for
                 undefined data symbols
  wmd_tables.inc stub table (guest address, arity, arg widths, pointer mask),
                 native-data mirror table, and the case table (every exported
                 wm_XXXXXXXX body whose address is a retail function start)

The retail side of the harness treats exactly the stubbed guest addresses as
its call boundary, so both sides stop at the same functions.
"""
import argparse, os, re, subprocess, sys, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
WM_LO, WM_HI = 0x80070CFC, 0x80099E8C

GFLAGS = ("-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM "
          "-D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -w -g -m64 "
          "-fno-builtin").split()
PSX = "pc_port/extern/PsyCross"
INC = ["-Ipc_port/src", "-Ipc_port/include_shim", "-Iinclude", "-Ipc_port/build_native",
       f"-I{PSX}/include", f"-I{PSX}/include/psx"]

WIDTH = {
    "u8": 8, "s8": 8, "char": 8, "unsigned char": 8, "signed char": 8, "uint8_t": 8,
    "int8_t": 8, "u_char": 8, "_Bool": 8, "bool": 8,
    "u16": 16, "s16": 16, "short": 16, "unsigned short": 16, "short int": 16,
    "short unsigned int": 16, "uint16_t": 16, "int16_t": 16, "u_short": 16,
}


def sh(cmd, **kw):
    return subprocess.run(cmd, check=True, capture_output=True, text=True, **kw).stdout


def linked_world_tus():
    bp = (ROOT / "pc_port/build_port.sh").read_text()
    tus = []
    for m in re.finditer(r'^\s*(pc_port/src/world_map_[\w]+\.c)', bp, re.M):
        if m.group(1) not in tus:
            tus.append(m.group(1))
    return tus


def retail_starts():
    starts = {}
    legacy = ROOT / "asm/world_map/120C.s"
    if legacy.exists():
        text = legacy.read_text()
    else:  # world_map has a C TU (b3fb9a39): splat writes per-function .s files
        text = "\n".join(f.read_text() for f in sorted((ROOT / "asm/world_map").rglob("*.s"))
                         if "/data/" not in str(f))
    for ln in text.splitlines():
        m = re.match(r'glabel (\w+)', ln)
        if m:
            cur = m.group(1)
            continue
        m = re.search(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ', ln)
        if m and cur:
            starts[int(m.group(1), 16)] = cur
            cur = None
    return starts


def slus_symbols():
    out = {}
    for p in ("config/symbol_addrs.slus_006.64.txt", "linker/undefined_funcs_auto.world_map.txt",
              "linker/undefined_syms_auto.world_map.txt"):
        f = ROOT / p
        if not f.exists():
            continue
        for ln in f.read_text().splitlines():
            m = re.match(r'\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;', ln)
            if m:
                out.setdefault(m.group(1), int(m.group(2), 16))
    return out


def split_params(s):
    depth, cur, out = 0, "", []
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def classify(t):
    """(width_bits, is_pointer) for a parameter/return type string."""
    t = re.sub(r'\b(const|volatile|register|struct|union|enum)\b', ' ', t)
    t = re.sub(r'\s+', ' ', t).strip()
    if "*" in t or "(" in t or "[" in t:
        return 64, True
    base = t.strip()
    return WIDTH.get(base, 32), False


aux_re = re.compile(r'^/\* ([^:]+):\d+:N[CF] \*/ (?:extern |static )?(.*?)\b(\w+) \((.*)\);')


def parse_aux(path, protos):
    for ln in open(path, errors="ignore"):
        m = aux_re.match(ln)
        if not m:
            continue
        ret, name, params = m.group(2).strip(), m.group(3), m.group(4)
        params = params.split("); /*")[0]
        ps = [] if params.strip() in ("void", "") else split_params(params)
        variadic = "..." in ps
        ps = [p for p in ps if p != "..."]
        if name not in protos or "NF" in ln:
            protos[name] = (ret, ps, variadic)


def extern_data_types(tus):
    """name -> type string from `extern T NAME;` / `extern T NAME[N];` in sources."""
    out = {}
    rx = re.compile(r'^\s*extern\s+([^;()]*?)\b(\w+)\s*(\[[^\]]*\])?\s*;', re.M)
    for tu in tus:
        for m in rx.finditer((ROOT / tu).read_text(errors="ignore")):
            out.setdefault(m.group(2), (m.group(1).strip(), m.group(3)))
    return out


def libc_names():
    names = set()
    libs = set()
    for so in ("libc.so.6", "libm.so.6"):
        p = sh(["gcc", f"-print-file-name={so}"]).strip()
        if os.path.exists(p):
            libs.add(os.path.realpath(p))
        for d in ("/lib64", "/usr/lib64", "/lib/x86_64-linux-gnu", "/usr/lib/x86_64-linux-gnu"):
            if os.path.exists(f"{d}/{so}"):
                libs.add(os.path.realpath(f"{d}/{so}"))
    if not libs:
        sys.exit("wm_diff_gen: cannot locate libc.so.6 to exclude libc symbols from stubbing")
    for lib in sorted(libs):
        if os.path.exists(lib):
            for ln in sh(["nm", "-D", "--defined-only", lib]).splitlines():
                parts = ln.split()
                if parts:
                    names.add(parts[-1].split("@")[0])
    return names


def addr_from_name(name, slus):
    m = re.fullmatch(r'(?:wm_|func_|D_)([0-9A-Fa-f]{8})', name)
    if m:
        return int(m.group(1), 16)
    if name in slus:
        return slus[name]
    alias = {"VSync": "Vsync", "RotMatrixZYX_gte": "RotMatrixZYX"}
    if alias.get(name) in slus:
        return slus[alias[name]]
    m = re.match(r'wm_([0-9A-Fa-f]{8})_', name)
    if m:
        return int(m.group(1), 16)
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--opt", default="-O0")
    ap.add_argument("--extra", nargs="*", default=[], help="extra -D flags for the TUs")
    a = ap.parse_args()
    out = pathlib.Path(a.out)
    (out / "obj").mkdir(parents=True, exist_ok=True)
    os.chdir(ROOT)
    tus = linked_world_tus()
    protos, objs = {}, []
    for tu in tus:
        base = pathlib.Path(tu).stem
        o, aux = out / "obj" / (base + ".o"), out / "obj" / (base + ".aux")
        cmd = ["gcc", "-c", tu, *GFLAGS, a.opt, *a.extra, *INC, "-aux-info", str(aux), "-o", str(o)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            sys.stderr.write(f"compile failed: {tu}\n{r.stderr}\n")
            sys.exit(1)
        parse_aux(aux, protos)
        objs.append(str(o))
    defined, undefined = set(), set()
    for o in objs:
        for ln in sh(["nm", o]).splitlines():
            parts = ln.split()
            if len(parts) == 2 and parts[0] == "U":
                undefined.add(parts[1])
            elif len(parts) == 3 and parts[1] in "TDBRCGSVW":
                defined.add(parts[2])
    libc = libc_names()
    # The GTE emulator (PsyCross INLINE_C.C + PsyX_GTE.cpp) is linked for real:
    # the retail side's cop2 instructions and the host bodies' gte calls share it.
    harness = {"g_PsxRam", "g_PsxScratchpad", "_GLOBAL_OFFSET_TABLE_", "gteRegs",
               "MFC2", "MTC2", "CFC2", "CTC2", "MFC2_S", "MTC2_S", "CFC2_S", "CTC2_S", "doCOP2"}
    need = sorted(n for n in undefined - defined - harness if n not in libc and not n.startswith("__"))
    slus = slus_symbols()
    dtypes = extern_data_types(tus)
    starts = retail_starts()

    stub_rows, data_rows, mirror_rows, body = [], [], [], []
    body.append('#include <stdint.h>\n#include <stddef.h>\n'
                'extern uint64_t wmd_host_call(unsigned idx, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);\n')
    for n in need:
        if n in protos and n not in dtypes:
            ret, ps, variadic = protos[n]
            widths, pmask = [], 0
            for i, p in enumerate(ps[:4]):
                w, isp = classify(p)
                widths.append(w)
                if isp:
                    pmask |= 1 << i
            rw, rp = classify(ret)
            if ret.strip() == "void":
                rw = 0
            idx = len(stub_rows)
            stub_rows.append((n, addr_from_name(n, slus), min(len(ps), 4), widths + [0] * (4 - len(widths)),
                              pmask, rw, int(rp), len(ps) > 4 or variadic))
            body.append(f"uint64_t {n}(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)"
                        f" {{ return wmd_host_call({idx}u, a0, a1, a2, a3); }}\n")
        else:
            t, arr = dtypes.get(n, ("", None))
            size = 0x1000
            kind, w = 0, 0
            if arr:
                mm = re.match(r'\[\s*(0x[0-9A-Fa-f]+|\d+)\s*\]', arr)
                size = max(int(mm.group(1), 0) if mm else 0x1000, 8) * 4
            elif t:
                w, isp = classify(t)
                kind = 2 if isp else 1
            addr = addr_from_name(n, slus)
            body.append(f"_Alignas(16) unsigned char {n}[{size}];\n")
            data_rows.append(n)
            if addr and kind:
                mirror_rows.append((n, addr, kind, w // 8 if kind == 1 else 4))
    # cases: exported bodies at a retail function start
    cases = []
    for n in sorted(defined):
        m = re.fullmatch(r'(?:wm_|func_)([0-9A-Fa-f]{8})', n)
        if not m:
            continue
        addr = int(m.group(1), 16)
        if addr not in starts or n not in protos:
            continue
        ret, ps, variadic = protos[n]
        if variadic or len(ps) > 4:
            continue
        widths, pmask = [], 0
        for i, p in enumerate(ps):
            w, isp = classify(p)
            widths.append(w)
            if isp:
                pmask |= 1 << i
        rw, rp = classify(ret)
        if ret.strip() == "void":
            rw = 0
        cases.append((n, addr, len(ps), widths + [0] * (4 - len(widths)), pmask, rw, int(rp)))
    (out / "wmd_stubs.c").write_text("".join(body))
    T = []
    T.append("static const WmdStub kStubs[] = {\n")
    for n, ad, na, ws, pm, rw, rp, trunc in stub_rows:
        T.append(f'  {{"{n}", 0x{ad:08X}u, {na}, {{{ws[0]},{ws[1]},{ws[2]},{ws[3]}}}, 0x{pm:x}, {rw}, {rp}, {int(trunc)}}},\n')
    T.append('  {0}\n};\n#define STUB_COUNT (sizeof(kStubs)/sizeof(kStubs[0]) - 1)\n')
    T.append("".join(f"extern unsigned char {n}[];\n" for n, *_ in mirror_rows))
    T.append("static const WmdMirror kMirrors[] = {\n")
    for n, ad, kind, w in mirror_rows:
        T.append(f'  {{"{n}", {n}, 0x{ad:08X}u, {kind}, {w}}},\n')
    T.append('  {0}\n};\n#define MIRROR_COUNT (sizeof(kMirrors)/sizeof(kMirrors[0]) - 1)\n')
    T.append("".join(f"extern uint64_t {n}();\n" for n, *_ in cases))
    T.append("static const WmdCase kCases[] = {\n")
    for n, ad, na, ws, pm, rw, rp in cases:
        T.append(f'  {{"{n}", (WmdFn){n}, 0x{ad:08X}u, {na}, {{{ws[0]},{ws[1]},{ws[2]},{ws[3]}}}, 0x{pm:x}, {rw}, {rp}}},\n')
    T.append('  {0}\n};\n#define CASE_COUNT (sizeof(kCases)/sizeof(kCases[0]) - 1)\n')
    (out / "wmd_tables.inc").write_text("".join(T))
    (out / "objs.txt").write_text("\n".join(objs) + "\n")
    unmapped = [r[0] for r in stub_rows if r[1] == 0]
    print(f"wm_diff_gen: tus={len(tus)} stubs={len(stub_rows)} (unmapped={len(unmapped)}) "
          f"data={len(data_rows)} mirrors={len(mirror_rows)} cases={len(cases)}")
    (out / "unmapped_stubs.txt").write_text("\n".join(unmapped) + "\n")


if __name__ == "__main__":
    main()

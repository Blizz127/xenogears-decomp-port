#!/usr/bin/env python3
"""Static scan: every retail call site of func_80079ED8 / func_8007A280 in the battle asm.
For each site: a3 setup before the call (store path = a3 low byte 0), and whether v0 is read
after the call before being redefined. Follows straight-line code; at a branch both the
fall-through and the target are followed (bounded), at jal/jr the result is decided by
whether v0 was already read/redefined."""
import re, sys, glob, json
ROOT = sys.argv[1]
TARGETS = {"func_80079ED8": 0x80079ED8, "func_8007A280": 0x8007A280}
LINE = re.compile(r'/\* [0-9A-F]+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s+(\S+)\s*(.*)')
LABEL = re.compile(r'^\s*(?:glabel|jlabel|dlabel)?\s*\.?(L[0-9A-F]{8}|func_[0-9A-F]{8})\s*:?\s*$')
ins = {}
for f in glob.glob(ROOT + "/asm/battle/**/*.s", recursive=True):
    for raw in open(f, errors="replace"):
        m = LINE.search(raw)
        if m:
            ins[int(m.group(1), 16)] = (m.group(2), m.group(3).split("#")[0].strip(), f)
REG = re.compile(r'\$(\w+)')
def regs(op, args):
    rs = REG.findall(args)
    if not rs: return None, []
    no_dest = op.startswith(("s", "b", "j", "mt", "mult", "div")) and op not in ("sll","srl","sra","sllv","srlv","srav","slt","slti","sltiu","sltu","sub","subu")
    if op in ("jal",): return "ra", []
    if no_dest: return None, rs
    return rs[0], rs[1:]
def branch_target(args):
    m = re.search(r'\.?(L[0-9A-F]{8})|0x([0-9a-fA-F]{8})|(func_[0-9A-F]{8})', args)
    if not m: return None
    s = m.group(1) or m.group(2) or m.group(3)
    return int(s.lstrip("L").replace("func_", ""), 16)
def follow(pc, depth=0, seen=None):
    """'read' if v0 read before redefinition, 'dead' if redefined/abandoned safely."""
    seen = seen or set()
    steps = 0
    while steps < 200:
        if pc in seen: return "dead"
        seen.add(pc)
        if pc not in ins: return "unknown@%08X" % pc
        op, args = ins[pc][:2]
        dest, srcs = regs(op, args)
        if op in ("jr",):
            return "read" if "v0" in srcs or args.strip() == "$ra" and False else "return"
        if "v0" in srcs: return "read"
        if op.startswith("b") or op in ("j", "jal"):
            ds_op, ds_args = ins.get(pc + 4, ("?", "", ""))[:2]
            dd, dsrcs = regs(ds_op, ds_args)
            if "v0" in dsrcs: return "read"
            if dd == "v0": return "dead"
            if op == "jal": return "dead"   # callee result replaces v0
            tgt = branch_target(args)
            if op == "j":
                pc = tgt; steps += 1; continue
            if depth > 6: return "unknown-depth"
            r1 = follow(tgt, depth + 1, set(seen)) if tgt else "unknown"
            r2 = follow(pc + 8, depth + 1, set(seen))
            for r in (r1, r2):
                if r != "dead": return r
            return "dead"
        if dest == "v0": return "dead"
        pc += 4; steps += 1
    return "unknown-long"
def a3_setup(pc):
    for back in range(1, 40):
        p = pc - 4 * back
        if p not in ins: break
        op, args = ins[p][:2]
        if op.startswith(("j", "b")) and back > 1: return "unknown(control flow)"
        d, s = regs(op, args)
        if d == "a3":
            return f"{op} {args}"
    return "unknown"
out = []
for pc in sorted(ins):
    op, args = ins[pc][:2]
    if op == "jal":
        for name in TARGETS:
            if name in args or ("0x%08x" % TARGETS[name]) in args.lower():
                ds = ins.get(pc + 4, ("?", "", ""))
                a3 = a3_setup(pc + 8)  # include delay slot
                res = follow(pc + 8)
                out.append(dict(site="%08X" % pc, callee=name, a3=a3, delay=f"{ds[0]} {ds[1]}", v0_after=res, file=ins[pc][2].split("/asm/")[-1]))
print(json.dumps(out, indent=1))

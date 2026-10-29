#!/usr/bin/env python3
"""Fixpoint over pass-through v0: seeds = store paths of 80079ED8/8007A280 (a3==0).
A function that tail-returns a pass-through v0 becomes a pass-through returner itself
(every call of it is then 'store-path' for this purpose).  Reports every site that READS a
pass-through v0, and every non-jal (data/.word/jalr-table) reference to a returner."""
import re, sys, glob, json, importlib.util
spec = importlib.util.spec_from_file_location("s", sys.argv[2]); 
ROOT = sys.argv[1]
src = open(sys.argv[2]).read().split("out = []")[0]
g = {"__name__": "lib"}; sys.argv = [sys.argv[0], ROOT]; exec(src, g)
ins, follow, a3_setup = g["ins"], g["follow"], g["a3_setup"]
# function starts
starts = sorted(set(int(m, 16) for f in glob.glob(ROOT + "/asm/battle/**/*.s", recursive=True)
                    for m in re.findall(r'glabel func_([0-9A-F]{8})', open(f, errors="replace").read())))
import bisect
def func_of(pc):
    i = bisect.bisect_right(starts, pc) - 1
    return starts[i] if i >= 0 else None
seeds = {0x80079ED8: "store-if-a3==0", 0x8007A280: "store-if-a3==0"}
returners = {}   # func -> reason
reads, notes = [], []
work = [(0x80079ED8, True), (0x8007A280, True)]
done = set()
while work:
    tgt, conditional = work.pop()
    if (tgt, conditional) in done: continue
    done.add((tgt, conditional))
    name = "func_%08X" % tgt
    for pc in sorted(ins):
        op, args = ins[pc][:2]
        if op != "jal" or name not in args: continue
        a3 = a3_setup(pc + 8)
        if conditional and re.search(r'ori \$a3, \$zero, 0x[1-9a-fA-F]', a3 or ""):
            continue   # load path: v0 is the real result
        res = follow(pc + 8)
        rec = dict(site="%08X" % pc, callee=name, in_func="func_%08X" % func_of(pc), a3=a3, v0_after=res)
        if res == "read": reads.append(rec)
        elif res == "return":
            f = func_of(pc)
            if f not in returners:
                returners[f] = rec; work.append((f, False))
        elif res != "dead": notes.append(rec)
# indirect references to any returner or seed
allf = set(returners) | set(seeds)
indirect = []
for f in glob.glob(ROOT + "/asm/battle/**/*.s", recursive=True):
    for i, line in enumerate(open(f, errors="replace")):
        for fn in allf:
            n = "func_%08X" % fn
            if n in line and "jal " not in line and "glabel" not in line and "endlabel" not in line:
                indirect.append(dict(func=n, file=f.split("/asm/")[-1], line=i + 1, text=line.strip()[:120]))
print(json.dumps(dict(pass_through_returners={"func_%08X" % k: v for k, v in returners.items()},
                      reads_of_pass_through=reads, undecided=notes, indirect_refs=indirect), indent=1))

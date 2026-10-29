#!/usr/bin/env python3
"""Translate one retail battle function from its splat listing into an
instruction-faithful C port body (include/battle_lift.h has the runtime
contract).

    tools/scripts/battle_lift_asm.py asm/battle/nonmatchings/main65/func_800A0838.s \
        --args 4 [--name func_800A0838] > body.c

(--args N is the function's argument count; --args auto takes 4, or more
when the listing reads incoming stack-argument words.)

One C statement per MIPS instruction.  o32 registers are u32 locals; a branch
evaluates its condition, then runs its delay slot, then jumps (R3000 order);
loads and stores go through bl_lw/bl_sh/... (the battle guest resolver),
LWL/LWR/SWL/SWR through bl_lwl/bl_lwr/bl_swl/bl_swr (the interpreter's
aligned-word merge); COP2 moves and commands call MFC2/MTC2/CFC2/CTC2/doCOP2
with the retail instruction word, as the interpreter's COP2 bus does; a JR
through a register other than $ra dispatches over the function's jump-table
targets (jlabels, or the .word .L entries of the jump tables splat puts in
the listing's own .rodata section).  Calls to overlay functions go through
bl_call_guest (typed native thunk or the interpreter); calls to
main-executable functions use the per-callee wrappers in MAIN_EXE below, each
reproducing the interpreter bridge's contract for that callee; a JALR goes
through bl_jal (the interpreter's own call path, decided at run time for the
register's target, as an interpreted jalr is).  A function without a frame
adjustment (a frameless leaf) gets frame 0.  %hi/%lo operands may name any
symbol of the battle link or the main executable, with a +/- offset; every
immediate, offset, shift amount and branch/jump/call target is checked
against the retail instruction word, so a misresolved symbol cannot slip
through.  (Mnemonics and register operands are taken from the listing text,
which splat's disassembler derives from the same word.)

The script refuses (exit 1) on anything it cannot translate exactly: an
unknown mnemonic, an unknown or ambiguous symbol, an immediate/offset/shift
amount/target that disagrees with the instruction word, a call to a name
with no listed address, a load whose result the next instruction reads (an
R3000 load-delay hazard; an LWL/LWR merging into the register its partner
just loaded is the architected exception), a read of v1 after a call, a jal
to a callee with no wrapper, a branch in a delay slot, a write to $sp in a
frameless function, or a register JR with no jump-table targets.  It prints,
on stderr, the overlay callees it bridged and the argument registers read
before written.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REGS = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]
CVAR = {r: ("r_" + r) for r in REGS}

LINE_RE = re.compile(r"^\s*/\*\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})\s*\*/\s*(\S+)\s*(.*)$")
LABEL_RE = re.compile(r"^\s*(?:jlabel\s+|glabel\s+)?(\.?L[0-9A-Fa-f_]+|func_[0-9A-Fa-f]+)\s*:?\s*$")
OVERLAY_LO = 0x8006FAF0
ROOT = Path(__file__).resolve().parents[2]


# The battle link's symbols (splat's undefined_*_auto lists are what the
# battle overlay references; config/ holds the named ones) and the main
# executable's.  A name defined with two different addresses is ambiguous:
# sym_addr() refuses it rather than pick one.
SYMBOL_FILES = ("config/symbol_addrs.slus_006.64.txt", "linker/undefined_funcs_auto.battle.txt",
                "linker/undefined_syms_auto.battle.txt", "config/symbol_addrs.battle.txt")


def _symbols() -> tuple[dict[str, int], set[str]]:
    out: dict[str, int] = {}
    ambiguous: set[str] = set()
    for rel in SYMBOL_FILES:
        path = ROOT / rel
        if not path.exists():
            continue
        for line in path.read_text().splitlines():
            m = re.match(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", line)
            if m:
                v = int(m.group(2), 16)
                if out.setdefault(m.group(1), v) != v:
                    ambiguous.add(m.group(1))
    return out, ambiguous


SYMBOLS, AMBIGUOUS = _symbols()

# Main-executable callees: C text with {a0}..{a3}, {sp} (the incoming $sp
# expression) substituted.  Each reproduces runtime_bridge_call for that
# address: pointer arguments resolved like translate_argument (bg_arg), the
# generic return in v0/v1 (low/high halves of the host return register) for
# value-returning natives.
# name -> (return kind, parameter C types).  Every argument is passed the way
# runtime_bridge_call's generic path passes it: translate_argument (bg_arg)
# of the raw register word, converted to the declared parameter type (an
# integer parameter therefore gets the low 32 bits, which for a translated
# guest address is its -no-pie host address, exactly as the bridge hands it).
# Return kinds: "v" void (v0 untouched), "i" 32-bit value (v0), "p" pointer
# or 64-bit value (v0 = low half, v1 = high half, the bridge's split).
MAIN_EXE = {
    "ApplyMatrix": ("p", ["MATRIX*", "SVECTOR*", "VECTOR*"]),
    "ratan2": ("i", ["int", "int"]),
    "SquareRoot0": ("i", ["int"]),
    "SetRotMatrix": ("v", ["MATRIX*"]),
    "SetTransMatrix": ("v", ["MATRIX*"]),
    "SetLightMatrix": ("v", ["MATRIX*"]),
    "MulMatrix": ("p", ["MATRIX*", "MATRIX*"]),
    "MulMatrix0": ("p", ["MATRIX*", "MATRIX*", "MATRIX*"]),
    "RotMatrix": ("p", ["SVECTOR*", "MATRIX*"]),
    "RotMatrixYXZ": ("p", ["SVECTOR*", "MATRIX*"]),
    "func_8002C700": ("i", ["u8*", "u8*", "u32*", "s32"]),
    "func_80039E60": ("v", ["s32"]),
    "func_8003A2E4": ("v", ["s32", "s32"]),
}
# Adopted legacy overlay leaves without a typed thunk: the interpreter's
# bridge calls their native body through the same generic path, so the
# lifted caller does too (direct call, translate_argument arguments).
LEGACY_LEAF = {
    "func_800A3484": ("v", ["s16*"]),
    "func_800AF400": ("i", []),
    "func_800B12D0": ("i", ["u32", "u32"]),
}
# Declarations the generated block needs (PsyQ GTE calls come from libgte.h).
DECLS = {
    "func_8002C700": "extern s32 func_8002C700(u8* a0, u8* a1, u32* a2, s32 a3);",
    "func_80039E60": "extern void func_80039E60(s32 packedId);",
    "func_8003A2E4": "extern void func_8003A2E4(s32 pData, s32 volume);",
    "func_800A3484": "extern void func_800A3484(s16* p);",
    "func_800AF400": "extern s32 func_800AF400(void);",
    "func_800B12D0": "extern u32 func_800B12D0(u32 index, u32 mask);",
}
# Callees the bridge special-cases: the C text of the same contract.
MAIN_EXE_SPECIAL = {
    "CompMatrix": "r_v0 = bl_CompMatrix(FN, r_a0, r_a1, r_a2);",
    "rsin": "r_v0 = (u32)bg_rsin((int)r_a0);",
    "rcos": "r_v0 = (u32)bg_rcos((int)r_a0);",
}


def die(msg: str) -> None:
    print(f"battle_lift_asm: {msg}", file=sys.stderr)
    sys.exit(1)


class Insn:
    def __init__(self, vaddr: int, word: int, op: str, args: list[str]):
        self.vaddr = vaddr
        self.word = word
        self.op = op
        self.args = args


def split_args(text: str) -> list[str]:
    text = re.sub(r"/\*.*?\*/", "", text).strip()
    if not text:
        return []
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    out.append(cur.strip())
    return out


DATA_WORD_RE = re.compile(r"^\s*/\*\s*[0-9A-Fa-f]+\s+([0-9A-Fa-f]{8})\s+[0-9A-Fa-f]{8}\s*\*/\s*\.word\s+(\S+)\s*$")


def parse(path: Path):
    """Instructions, labels and jump-table targets of one listing.  A target
    is an address marked `jlabel`, or a `.word .Lxxxxxxxx` entry of a jump
    table splat emitted into the listing's own .rodata section (the table
    itself stays in guest RAM: the lifted JR reads it from there)."""
    insns: list[Insn] = []
    labels: dict[int, str] = {}
    jlabels: set[int] = set()
    pending: list[tuple[str, bool]] = []
    text = True
    for line in path.read_text().splitlines():
        s = line.strip()
        if (not s or s.startswith("/* Handwritten") or s.startswith("/* Generated by")
                or s.startswith(".size") or s.startswith("endlabel")):
            continue
        if s.startswith(".section"):
            parts = s.split()
            text = len(parts) > 1 and parts[1].rstrip(",") == ".text"
            continue
        if not text:
            if s.startswith(".align") or re.fullmatch(r"dlabel\s+jtbl_[0-9A-Fa-f]{8}", s):
                continue
            m = DATA_WORD_RE.match(line)
            if m:
                t = re.fullmatch(r"\.L([0-9A-Fa-f]{8})", m.group(2))
                if t:
                    jlabels.add(int(t.group(1), 16))
                elif not re.fullmatch(r"0x0+", m.group(2)):  # alignment padding only
                    die(f"unparsed jump-table word: {s}")
                continue
            die(f"unparsed data line: {s}")
        m = LINE_RE.match(line)
        if m:
            vaddr = int(m.group(2), 16)
            b = bytes.fromhex(m.group(3))
            word = int.from_bytes(b, "little")
            for name, isj in pending:
                labels[vaddr] = name
                if isj:
                    jlabels.add(vaddr)
            pending = []
            insns.append(Insn(vaddr, word, m.group(4), split_args(m.group(5))))
            continue
        m = re.match(r"^\s*(jlabel|glabel)?\s*(\.L[0-9A-Fa-f]+|func_\w+)\s*:?\s*$", line)
        if m:
            pending.append((m.group(2), m.group(1) == "jlabel"))
            continue
        if s.startswith(".align") or s.startswith(".section") or s.startswith(".set"):
            continue
        die(f"unparsed line: {line}")
    return insns, labels, jlabels


def sym_addr(name: str) -> int:
    """Address of `symbol`, `symbol + off` or `symbol - off`."""
    m = re.fullmatch(r"\s*(\w+)\s*(?:([+-])\s*(0x[0-9A-Fa-f]+|\d+))?\s*", name)
    if not m:
        die(f"unknown symbol {name}")
    base = m.group(1)
    h = re.fullmatch(r"(?:D|jtbl|func)_([0-9A-Fa-f]{8})", base)
    if h:
        addr = int(h.group(1), 16)
    elif base in AMBIGUOUS:
        die(f"ambiguous symbol {base} (two addresses in {', '.join(SYMBOL_FILES)})")
    elif base in SYMBOLS:
        addr = SYMBOLS[base]
    else:
        die(f"unknown symbol {name}")
    if m.group(2):
        off = int(m.group(3), 0)
        addr += off if m.group(2) == "+" else -off
    return addr & 0xFFFFFFFF


def callee_addr(name: str) -> int | None:
    """Address a jal operand names (func_XXXXXXXX or a listed symbol); None
    when the name is not listed.  An ambiguous name is refused."""
    m = re.fullmatch(r"func_([0-9A-Fa-f]{8})", name)
    if m:
        return int(m.group(1), 16)
    if name in AMBIGUOUS:
        die(f"ambiguous symbol {name} (two addresses in {', '.join(SYMBOL_FILES)})")
    return SYMBOLS.get(name)


def imm(text: str) -> int:
    """Signed or unsigned integer value of an operand expression."""
    t = text.strip()
    t = re.sub(r"%hi\(([^)]+)\)", lambda m: str(((sym_addr(m.group(1).strip()) + 0x8000) >> 16) & 0xFFFF), t)
    def lo(m):
        v = sym_addr(m.group(1).strip()) & 0xFFFF
        return str(v - 0x10000 if v & 0x8000 else v)
    t = re.sub(r"%lo\(([^)]+)\)", lo, t)
    if not re.fullmatch(r"[0-9A-Fa-fxX()+\-<>&| ~*]+", t):
        die(f"bad immediate {text}")
    return int(eval(t, {"__builtins__": {}}, {}))


def reg(text: str) -> str:
    t = text.strip()
    if not t.startswith("$"):
        die(f"expected register, got {text}")
    name = t[1:]
    if name == "s8":
        name = "fp"
    if name not in CVAR:
        die(f"unknown register {text}")
    return name


def cop2reg(text: str) -> int:
    t = text.strip()
    if not re.fullmatch(r"\$\d+", t):
        die(f"bad cop2 register {text}")
    return int(t[1:])


def mem(text: str) -> tuple[int, str]:
    m = re.fullmatch(r"(.*)\((\$\w+)\)", text.strip())
    if not m:
        die(f"bad memory operand {text}")
    off = imm(m.group(1)) if m.group(1).strip() else 0
    return off, reg(m.group(2))


def R(r: str) -> str:
    return "0u" if r == "zero" else CVAR[r]


def addr_expr(off: int, base: str) -> str:
    if off == 0:
        return R(base)
    return f"{R(base)} + 0x{off & 0xFFFFFFFF:X}u"


BRANCH1 = {"beqz": "== 0", "bnez": "!= 0"}
BRANCHS = {"bgez": ">= 0", "bltz": "< 0", "blez": "<= 0", "bgtz": "> 0"}
LOADS = {"lw": "bl_lw", "lh": "bl_lh", "lhu": "bl_lhu", "lb": "bl_lb", "lbu": "bl_lbu"}
STORES = {"sw": "bl_sw", "sh": "bl_sh", "sb": "bl_sb"}
# Unaligned word halves: the load merges into the register's current value.
MERGE_LOADS = {"lwl": "bl_lwl", "lwr": "bl_lwr"}
MERGE_STORES = {"swl": "bl_swl", "swr": "bl_swr"}
MERGE_PARTNER = {"lwl": "lwr", "lwr": "lwl"}


def is_load(i: Insn) -> bool:
    """Delayed-load instructions (the R3000 load-delay slot applies)."""
    return i.op in LOADS or i.op in MERGE_LOADS or i.op in ("mfc2", "cfc2")


def reads_writes(i: Insn) -> tuple[set[str], set[str]]:
    """Integer registers read / written (for the hazard and argument checks)."""
    op, a = i.op, i.args
    rd: set[str] = set()
    wr: set[str] = set()
    if op in LOADS:
        off, b = mem(a[1]); rd.add(b); wr.add(reg(a[0]))
    elif op in MERGE_LOADS:
        off, b = mem(a[1]); rd |= {b, reg(a[0])}; wr.add(reg(a[0]))
    elif op in STORES or op in MERGE_STORES:
        off, b = mem(a[1]); rd |= {b, reg(a[0])}
    elif op in ("lwc2",):
        rd.add(mem(a[1])[1])
    elif op in ("swc2",):
        rd.add(mem(a[1])[1])
    elif op in ("mfc2", "cfc2"):
        wr.add(reg(a[0]))
    elif op in ("mtc2", "ctc2"):
        rd.add(reg(a[0]))
    elif op in ("addu", "subu", "and", "or", "xor", "nor", "slt", "sltu", "sllv", "srlv", "srav"):
        wr.add(reg(a[0])); rd |= {reg(a[1]), reg(a[2])}
    elif op in ("addiu", "andi", "ori", "xori", "slti", "sltiu", "sll", "srl", "sra"):
        wr.add(reg(a[0])); rd.add(reg(a[1]))
    elif op in ("negu", "move", "not"):
        wr.add(reg(a[0])); rd.add(reg(a[1]))
    elif op == "lui" or op == "li":
        wr.add(reg(a[0]))
    elif op in ("mult", "multu", "div", "divu"):
        regs = [reg(x) for x in a]
        rd |= set(regs[-2:])
    elif op in ("mflo", "mfhi"):
        wr.add(reg(a[0]))
    elif op in ("mtlo", "mthi"):
        rd.add(reg(a[0]))
    elif op in BRANCH1 or op in BRANCHS:
        rd.add(reg(a[0]))
    elif op in ("beq", "bne"):
        rd |= {reg(a[0]), reg(a[1])}
    elif op == "jr":
        rd.add(reg(a[0]))
    elif op == "jalr":
        rd.add(reg(a[-1])); wr.add(reg(a[0]) if len(a) == 2 else "ra")
    rd.discard("zero")
    wr.discard("zero")
    return rd, wr


def is_branch(i: Insn) -> bool:
    return (i.op in BRANCH1 or i.op in BRANCHS or i.op in ("beq", "bne", "b", "j", "jal", "jr", "jalr", "bal"))


def frame_size(insns) -> int:
    """The first `addiu $sp, $sp, -N` (0 for a frameless leaf)."""
    for i in insns:
        if i.op == "addiu" and reg(i.args[0]) == "sp" and reg(i.args[1]) == "sp" and imm(i.args[2]) < 0:
            return -imm(i.args[2])
    return 0


def stack_arg_reads(insns, frame: int) -> list[tuple[int, int]]:
    """(vaddr, argument index) of every load from an incoming stack-argument
    word: $sp offsets at or above frame + 0x10 (the caller's o32 home area
    holds a0..a3)."""
    out = []
    for i in insns:
        if i.op in LOADS or i.op in MERGE_LOADS or i.op == "lwc2":
            off, b = mem(i.args[1])
            if b == "sp" and off >= frame + 0x10:
                out.append((i.vaddr, 4 + (off - frame - 0x10) // 4))
    return out


# Control transfers whose last operand is a local label.
BRANCH_OPS = set(BRANCH1) | set(BRANCHS) | {"beq", "bne", "b", "j"}


def label_target(text: str) -> int | None:
    m = re.fullmatch(r"\.L([0-9A-Fa-f]{8})", text.strip())
    return int(m.group(1), 16) if m else None


class Lifter:
    def __init__(self, name: str, insns, labels, jlabels, nargs: int):
        self.name = name
        self.insns = insns
        self.labels = labels
        self.jlabels = jlabels
        self.nargs = nargs
        self.addr_index = {ins.vaddr: k for k, ins in enumerate(insns)}
        self.frame = None
        self.bridged: dict[str, int] = {}
        self.mainexe: dict[str, int] = {}
        self.legacy: dict[str, int] = {}
        self.viajal: dict[str, int] = {}
        self.jal_service = False
        self.void = False
        self.decls: set[str] = set()
        self.indirect = 0
        for x in sorted(jlabels):
            if x not in self.addr_index:
                die(f"jump-table target {x:08X} is not an instruction of {name}")

    def label_name(self, vaddr: int) -> str:
        return f"L_{vaddr:08X}"

    def target(self, text: str) -> int:
        t = text.strip()
        m = re.fullmatch(r"\.L([0-9A-Fa-f]{8})", t)
        if m:
            return int(m.group(1), 16)
        die(f"bad branch target {text}")

    def stmt(self, i: Insn) -> str:
        op, a = i.op, i.args
        if op == "nop":
            return ";"
        if op in LOADS:
            off, b = mem(a[1]); d = reg(a[0])
            e = f"{LOADS[op]}({addr_expr(off, b)})"
            return f"{e};" if d == "zero" else f"{R(d)} = {e};"
        if op in STORES:
            off, b = mem(a[1])
            return f"{STORES[op]}({addr_expr(off, b)}, {R(reg(a[0]))});"
        if op in MERGE_LOADS:
            off, b = mem(a[1]); d = reg(a[0])
            e = f"{MERGE_LOADS[op]}({addr_expr(off, b)}, {R(d)})"
            return f"{e};" if d == "zero" else f"{R(d)} = {e};"
        if op in MERGE_STORES:
            off, b = mem(a[1])
            return f"{MERGE_STORES[op]}({addr_expr(off, b)}, {R(reg(a[0]))});"
        if op == "lwc2":
            off, b = mem(a[1])
            return f"MTC2(bl_lw({addr_expr(off, b)}), {cop2reg(a[0])});"
        if op == "swc2":
            off, b = mem(a[1])
            return f"bl_sw({addr_expr(off, b)}, (u32)MFC2({cop2reg(a[0])}));"
        if op == "mfc2":
            return f"{R(reg(a[0]))} = (u32)MFC2({cop2reg(a[1])});"
        if op == "cfc2":
            return f"{R(reg(a[0]))} = (u32)CFC2({cop2reg(a[1])});"
        if op == "mtc2":
            return f"MTC2({R(reg(a[0]))}, {cop2reg(a[1])});"
        if op == "ctc2":
            return f"CTC2({R(reg(a[0]))}, {cop2reg(a[1])});"
        if op[0].isupper() and (i.word >> 25) == 0x25:  # COP2 command (0x4A......)
            return f"doCOP2((int)0x{i.word:08X}u); /* {op} {', '.join(a)} */"
        def w(d, e):
            return ";" if d == "zero" else f"{R(d)} = {e};"
        if op == "addu":
            return w(reg(a[0]), f"{R(reg(a[1]))} + {R(reg(a[2]))}")
        if op == "subu":
            return w(reg(a[0]), f"{R(reg(a[1]))} - {R(reg(a[2]))}")
        if op == "negu":
            return w(reg(a[0]), f"0u - {R(reg(a[1]))}")
        if op == "move":
            return w(reg(a[0]), R(reg(a[1])))
        if op == "and":
            return w(reg(a[0]), f"{R(reg(a[1]))} & {R(reg(a[2]))}")
        if op == "or":
            return w(reg(a[0]), f"{R(reg(a[1]))} | {R(reg(a[2]))}")
        if op == "xor":
            return w(reg(a[0]), f"{R(reg(a[1]))} ^ {R(reg(a[2]))}")
        if op == "nor":
            return w(reg(a[0]), f"~({R(reg(a[1]))} | {R(reg(a[2]))})")
        if op == "slt":
            return w(reg(a[0]), f"(u32)((s32){R(reg(a[1]))} < (s32){R(reg(a[2]))})")
        if op == "sltu":
            return w(reg(a[0]), f"(u32)({R(reg(a[1]))} < {R(reg(a[2]))})")
        if op == "sllv":
            return w(reg(a[0]), f"{R(reg(a[1]))} << ({R(reg(a[2]))} & 31)")
        if op == "srlv":
            return w(reg(a[0]), f"{R(reg(a[1]))} >> ({R(reg(a[2]))} & 31)")
        if op == "srav":
            return w(reg(a[0]), f"BL_SRA({R(reg(a[1]))}, {R(reg(a[2]))})")
        if op in ("sll", "srl", "sra"):
            n = imm(a[2]) & 31
            s = R(reg(a[1]))
            e = {"sll": f"{s} << {n}", "srl": f"{s} >> {n}", "sra": f"BL_SRA({s}, {n})"}[op]
            return w(reg(a[0]), e)
        if op == "addiu":
            v = imm(a[2])
            if not -0x8000 <= v <= 0x7FFF:
                die(f"addiu immediate out of range at {i.vaddr:08X}")
            d, s = reg(a[0]), reg(a[1])
            if d == "sp" and s == "sp" and v < 0:
                if self.frame is not None and i.vaddr != self.insns[0].vaddr and self.frame != -v:
                    die("second frame adjustment")
                self.frame = -v
            return w(d, f"{R(s)} + 0x{v & 0xFFFFFFFF:X}u")
        if op in ("andi", "ori", "xori"):
            v = imm(a[2]) & 0xFFFF
            c = {"andi": "&", "ori": "|", "xori": "^"}[op]
            return w(reg(a[0]), f"{R(reg(a[1]))} {c} 0x{v:X}u")
        if op == "slti":
            v = imm(a[2])
            return w(reg(a[0]), f"(u32)((s32){R(reg(a[1]))} < {v})")
        if op == "sltiu":
            v = imm(a[2]) & 0xFFFFFFFF
            if imm(a[2]) < 0:
                v = (imm(a[2]) + (1 << 32)) & 0xFFFFFFFF
            return w(reg(a[0]), f"(u32)({R(reg(a[1]))} < 0x{v:X}u)")
        if op == "lui":
            v = imm(a[1]) & 0xFFFF
            return w(reg(a[0]), f"0x{v << 16:X}u")
        if op in ("mult", "multu", "div", "divu"):
            regs = [reg(x) for x in a]
            if op in ("div", "divu") and len(regs) == 3:
                if regs[0] != "zero":
                    die(f"three-operand {op} at {i.vaddr:08X}")
                regs = regs[1:]
            m = {"mult": "BL_MULT", "multu": "BL_MULTU", "div": "BL_DIV", "divu": "BL_DIVU"}[op]
            return f"{m}({R(regs[0])}, {R(regs[1])});"
        if op == "mflo":
            return w(reg(a[0]), "lo")
        if op == "mfhi":
            return w(reg(a[0]), "hi")
        if op == "mtlo":
            return f"lo = {R(reg(a[0]))};"
        if op == "mthi":
            return f"hi = {R(reg(a[0]))};"
        if op == "break":
            code = imm(a[0]) if a else 0
            return f"bl_fail(FN, \"BREAK\", {code}u);"
        die(f"unknown mnemonic {op} at {i.vaddr:08X}")

    def call(self, i: Insn) -> str:
        name = i.args[0].strip()
        m = re.fullmatch(r"func_([0-9A-Fa-f]{8})", name)
        addr = int(m.group(1), 16) if m else None
        if self.jal_service:
            if addr is None:
                addr = callee_addr(name)  # agrees with the word (check_encodings)
                if addr is None:
                    die(f"no address for {name}")
            self.viajal[name] = self.viajal.get(name, 0) + 1
            return (f"bl_jal(FN, 0x{addr:08X}u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* {name} */")
        if name in LEGACY_LEAF:
            self.legacy[name] = self.legacy.get(name, 0) + 1
            return self.direct(name, LEGACY_LEAF[name])
        if addr is not None and addr >= OVERLAY_LO:
            self.bridged[name] = self.bridged.get(name, 0) + 1
            return (f"r_v0 = bl_call_guest(FN, 0x{addr:08X}u, r_a0, r_a1, r_a2, r_a3, r_sp); /* {name} */")
        self.mainexe[name] = self.mainexe.get(name, 0) + 1
        if name in MAIN_EXE_SPECIAL:
            return MAIN_EXE_SPECIAL[name] + f" /* {name} */"
        if name not in MAIN_EXE:
            die(f"no main-executable wrapper for {name}")
        return self.direct(name, MAIN_EXE[name])

    def direct(self, name, spec):
        kind, types = spec
        if name in DECLS:
            self.decls.add(DECLS[name])
        if len(types) > 4:
            die(f"{name}: stack arguments not supported")
        argv = ", ".join(f"({t})(uintptr_t)bg_arg(r_a{k})" for k, t in enumerate(types))
        if kind == "v":
            return f"{name}({argv}); /* {name} */"
        if kind == "i":
            return f"r_v0 = (u32){name}({argv}); /* {name} */"
        return (f"{{ uint64_t r_ = (uint64_t)(uintptr_t){name}({argv}); "
                f"r_v0 = (u32)r_; r_v1 = (u32)(r_ >> 32); }} /* {name} */")

    def check_hazards(self):
        for k in range(len(self.insns) - 1):
            i, n = self.insns[k], self.insns[k + 1]
            if is_load(i):
                _, w = reads_writes(i)
                r, _ = reads_writes(n)
                # LWL/LWR into the register its partner just loaded merges
                # with the pending value (the R3000 forwards it; the
                # interpreter retires the two masked merges in order), so
                # the sequential C gives the same register.
                # (The partner must not address through that register: the
                # interpreter computes its address before the first half
                # retires.)
                pair = (i.op in MERGE_LOADS and n.op == MERGE_PARTNER[i.op]
                        and reg(n.args[0]) == reg(i.args[0]))
                if pair and mem(n.args[1])[1] == reg(i.args[0]):
                    die(f"{i.op}/{n.op} pair at {i.vaddr:08X} addresses through the register it merges into")
                if w & r and not pair:
                    die(f"load-delay hazard at {i.vaddr:08X}")
            if is_branch(i) and k + 1 < len(self.insns) and is_branch(n):
                die(f"branch in delay slot at {n.vaddr:08X}")
            if i.op in ("jal", "jalr", "jr") and is_load(n):
                # Its value would retire only after the first instruction at
                # the target (the callee's, or the caller's after a return),
                # and an interpreted call's bridge would see the old register.
                die(f"load in the delay slot of {i.op} at {n.vaddr:08X}")
            if is_branch(i) and is_load(n) and i.op not in ("jal", "jr", "jalr"):
                _, w = reads_writes(n)
                t = self.target(i.args[-1])
                r, _ = reads_writes(self.insns[self.addr_index[t]])
                if w & r:
                    die(f"load-delay hazard across branch at {n.vaddr:08X}")
        # v1 read after a call before being written (callee v1 is not modelled
        # for overlay callees).
        for k, i in enumerate(self.insns):
            if i.op != "jal":
                continue
            j = k + 2
            while j < len(self.insns):
                ins = self.insns[j]
                r, w = reads_writes(ins)
                if "v1" in r:
                    die(f"v1 read after call at {ins.vaddr:08X}")
                if "v1" in w or is_branch(ins) or ins.vaddr in self.labels:
                    break
                j += 1

    def check_encodings(self):
        """Every immediate, offset, shift amount and branch/jump/call target
        the C uses must agree with the retail instruction word.  This is what
        guards the symbol resolution (%hi/%lo of a name or name+offset, a jal
        to a named function).  Mnemonics and register fields are not
        re-decoded here: they come from the listing text."""
        for i in self.insns:
            op, a, w = i.op, i.args, i.word
            lo16 = w & 0xFFFF
            simm = lo16 - 0x10000 if lo16 & 0x8000 else lo16

            def bad(what: str) -> None:
                die(f"{what} disagrees with the instruction word {w:08X} at "
                    f"{i.vaddr:08X} ({op} {', '.join(a)})")

            if (op in LOADS or op in STORES or op in MERGE_LOADS or op in MERGE_STORES
                    or op in ("lwc2", "swc2")):
                if mem(a[1])[0] != simm:
                    bad("offset")
            elif op in ("addiu", "slti", "sltiu"):
                if imm(a[-1]) != simm:
                    bad("signed immediate")
            elif op in ("andi", "ori", "xori", "lui"):
                if imm(a[-1]) & 0xFFFF != lo16:
                    bad("immediate")
            elif op in ("sll", "srl", "sra"):
                if imm(a[2]) & 31 != (w >> 6) & 31:
                    bad("shift amount")
            elif op in BRANCH_OPS or op == "jal":
                if op in ("j", "jal"):
                    expect = ((i.vaddr + 4) & 0xF0000000) | ((w & 0x03FFFFFF) << 2)
                else:
                    expect = (i.vaddr + 4 + (simm << 2)) & 0xFFFFFFFF
                t = label_target(a[-1])
                if t is None:
                    t = callee_addr(a[-1].strip())
                    if t is None:
                        die(f"{op} to {a[-1].strip()} at {i.vaddr:08X}: no listed address to "
                            f"check against the instruction word {w:08X}")
                if t != expect:
                    bad("branch/jump target")

    def arg_reads(self) -> set[str]:
        """Argument registers read before written on the straight-line entry
        path (diagnostic only)."""
        seen_w: set[str] = set()
        out: set[str] = set()
        for ins in self.insns:
            r, w = reads_writes(ins)
            out |= {x for x in r if x in ("a0", "a1", "a2", "a3") and x not in seen_w}
            seen_w |= w
            if is_branch(ins):
                break
        return out

    def emit(self) -> str:
        self.check_hazards()
        self.check_encodings()
        out: list[str] = []
        body: list[str] = []
        ins = self.insns
        k = 0
        n = len(ins)
        # The entry gets a label only when something branches back to it.
        entry_target = any(x.op in BRANCH_OPS and label_target(x.args[-1]) == ins[0].vaddr
                           for x in ins) or ins[0].vaddr in self.jlabels
        while k < n:
            i = ins[k]
            if i.vaddr in self.labels and (k != 0 or entry_target):
                body.append(f"{self.label_name(i.vaddr)}:")
            if is_branch(i):
                if k + 1 >= n:
                    die("branch without delay slot")
                ds = ins[k + 1]
                dstext = self.stmt(ds)
                op, a = i.op, i.args
                pre = f"    /* {i.vaddr:08X} {op} {', '.join(a)} */"
                body.append(pre)
                if op in BRANCH1:
                    cond = f"{R(reg(a[0]))} {BRANCH1[op]}"
                    t = self.target(a[1])
                    body.append(f"    {{ int c_ = ({cond}); {dstext} if (c_) goto {self.label_name(t)}; }}")
                elif op in BRANCHS:
                    cond = f"(s32){R(reg(a[0]))} {BRANCHS[op]}"
                    t = self.target(a[1])
                    body.append(f"    {{ int c_ = ({cond}); {dstext} if (c_) goto {self.label_name(t)}; }}")
                elif op in ("beq", "bne"):
                    c = "==" if op == "beq" else "!="
                    cond = f"{R(reg(a[0]))} {c} {R(reg(a[1]))}"
                    t = self.target(a[2])
                    body.append(f"    {{ int c_ = ({cond}); {dstext} if (c_) goto {self.label_name(t)}; }}")
                elif op in ("b", "j"):
                    t = self.target(a[0])
                    body.append(f"    {dstext} goto {self.label_name(t)};")
                elif op == "jal":
                    body.append(f"    {dstext}")
                    body.append(f"    {self.call(i)}")
                elif op == "jr":
                    r = reg(a[0])
                    if r == "ra":
                        body.append(f"    {dstext} goto bl_exit;")
                    else:
                        if not self.jlabels:
                            die(f"jr through ${r} at {i.vaddr:08X} with no jump-table targets")
                        cases = " ".join(f"case 0x{x:08X}u: goto {self.label_name(x)};" for x in sorted(self.jlabels))
                        body.append(f"    {{ u32 t_ = {R(r)}; {dstext} switch (t_) {{ {cases} default: bl_fail(FN, \"jump-table target\", t_); }} }}")
                elif op == "jalr":
                    # Target read before the delay slot runs (R3000 order); the
                    # interpreter's own call path then decides native vs
                    # interpreted for whatever address the register holds.
                    rs = reg(a[-1])
                    link = reg(a[0]) if len(a) == 2 else "ra"
                    setlink = "" if link in ("ra", "zero") else f"{R(link)} = 0x{i.vaddr + 8:08X}u; "
                    self.indirect += 1
                    body.append(f"    {{ u32 t_ = {R(rs)}; {setlink}{dstext} "
                                f"bl_jal(FN, t_, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); }} /* indirect call */")
                else:
                    die(f"unsupported control transfer {op} at {i.vaddr:08X}")
                if ds.vaddr in self.labels:
                    body.append(f"    if (0) {{ {self.label_name(ds.vaddr)}: {dstext} }}")
                k += 2
                continue
            body.append(f"    {self.stmt(i)} /* {i.vaddr:08X} {i.op} {', '.join(i.args)} */")
            k += 1
        if self.frame is None:
            # A frameless leaf: frame 0, so $sp stays the one bl_enter
            # returns (a fresh frame whose home area and stack-argument words
            # above $sp hold arguments 4.. of --args); it must never move it.
            for x in ins:
                if "sp" in reads_writes(x)[1]:
                    die(f"$sp written at {x.vaddr:08X} without a frame adjustment")
            self.frame = 0
        for vaddr, idx in stack_arg_reads(ins, self.frame):
            if idx >= max(self.nargs, 4):
                die(f"reads incoming stack argument {idx} at {vaddr:08X} but --args {self.nargs} "
                    f"(the lifted frame holds only the arguments --args passes)")
        params = ", ".join(f"u32 arg{x}" for x in range(self.nargs)) or "void"
        out.extend(sorted(self.decls))
        out.append(f"{'void' if self.void else 'u32'} {self.name}({params}) {{")
        out.append(f"    static const char FN[] = \"{self.name}\";")
        regs = [r for r in REGS if r not in ("zero",)]
        out.append("    u32 " + ", ".join(f"{CVAR[r]} = 0" for r in regs) + ";")
        out.append("    u32 hi = 0, lo = 0, bl_saved;")
        out.append(f"    r_sp = bl_enter(FN, 0x{self.frame:X}u, &bl_saved);")
        for x in range(min(self.nargs, 4)):
            out.append(f"    r_a{x} = arg{x};")
        for x in range(4, self.nargs):
            out.append(f"    bl_sw(r_sp + 0x{0x10 + (x - 4) * 4:X}u, arg{x});")
        out.extend(body)
        out.append("bl_exit:")
        out.append("    bl_leave(bl_saved);")
        out.append("    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;")
        out.append("    (void)r_v0;" if self.void else "    return r_v0;")
        out.append("}")
        return "\n".join(out) + "\n"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("asm")
    p.add_argument("--args", required=True,
                   help="argument count, or `auto`: 4, or more when the listing reads "
                        "incoming stack-argument words")
    p.add_argument("--name")
    p.add_argument("--void", action="store_true",
                   help="the function is void (its v0 at return is not part of its contract)")
    p.add_argument("--jal-service", action="store_true",
                   help="route every call through PcPort_BattleMipsCallJal (bl_jal): "
                        "the interpreter's own jal semantics, hooks included")
    ns = p.parse_args()
    path = Path(ns.asm)
    insns, labels, jlabels = parse(path)
    name = ns.name or path.stem
    if ns.args == "auto":
        nargs = max([4] + [idx + 1 for _, idx in stack_arg_reads(insns, frame_size(insns))])
    elif ns.args.isdigit():
        nargs = int(ns.args)
    else:
        die(f"--args takes a count or auto, not {ns.args}")
    lf = Lifter(name, insns, labels, jlabels, nargs)
    lf.jal_service = ns.jal_service
    lf.void = ns.void
    text = lf.emit()
    print(f"{name}: {len(insns)} instructions, frame 0x{lf.frame:X}, args read at entry {sorted(lf.arg_reads())}", file=sys.stderr)
    print("  overlay callees (bl_call_guest): " + ", ".join(f"{k} x{v}" for k, v in sorted(lf.bridged.items())), file=sys.stderr)
    print("  legacy native leaves (direct): " + ", ".join(f"{k} x{v}" for k, v in sorted(lf.legacy.items())), file=sys.stderr)
    if lf.viajal:
        print("  via the jal service: " + ", ".join(f"{k} x{v}" for k, v in sorted(lf.viajal.items())), file=sys.stderr)
    if lf.indirect:
        print(f"  indirect calls (jalr, via the jal service): {lf.indirect}", file=sys.stderr)
    print("  main-exe callees: " + ", ".join(f"{k} x{v}" for k, v in sorted(lf.mainexe.items())), file=sys.stderr)
    sys.stdout.write(text)


if __name__ == "__main__":
    main()

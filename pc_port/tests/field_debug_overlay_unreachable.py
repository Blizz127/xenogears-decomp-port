#!/usr/bin/env python3
"""Prove the field overlay's 0x8028xxxx callees are unreachable in retail.

field.bin calls ten functions in [0x80280000, 0x80290000): func_802811EC,
func_80281204, func_8028125C, func_802812A4, func_80281400, func_80281450,
func_802815B0, func_80281678, func_80281B00, func_80284EA4. No asm exists for
them in asm/ because they are not part of field.bin. They live in a developer
debug overlay -- archive directory 4 entry 0xAD (25032 bytes on the USA disc 1
image) -- that FieldMain loads to the raw address 0x80280000 only when
g_FieldSystemMode == SYSTEM_MODE_PC_HDD (0). The overlay's first 0x11EC bytes
are debug-HUD format strings ("CPU=%04d GPU=%04d", "---------- Player Info",
...); func_80281B00 is the CPU profiler section mark (field.bin passes it the
labels "MATRIX", "MODEL", "FntPrint", "Clear OTAG", ...), func_80281204 counts
encounters per formation. 0x80280000 is past the retail console's 2 MB of RAM:
the module targets the 8 MB development kit.

This script checks, from the shipped bytes:
  1. SLUS_006.64's D_80010000 (the value FieldMain tests) is 0xFFFFFFFF.
  2. FieldMain derives g_FieldSystemMode from it: -1 -> 1 (CD-ROM), else 0.
  3. field.bin and SLUS_006.64 contain no other store to g_FieldSystemMode.
  4. Every jal into [0x80200000, 0x80800000) in field.bin, and the one
     `lui $a1, 0x8028` load destination, is guarded by
     `lw rX, g_FieldSystemMode; bnez rX, past-the-call` with no branch target
     landing between the guard and the call (so it cannot be entered around).
  5. The debug entry points are exactly the function starts of archive entry
     0xAD loaded at 0x80280000 (each preceded by `jr $ra; nop`), when the disc
     image is available.
  6. Every C call site the port compiles (src/field/**) is inside an
     `if (g_FieldSystemMode == 0 / SYSTEM_MODE_PC_HDD)` block.
Then it re-runs checks 1-4 against deliberately broken copies and requires
each to be rejected (negative controls).
"""
import hashlib
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIELD_SHA = '38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
FIELD_VRAM = 0x8006FAF0
SLUS_VRAM = 0x80010000
SLUS_HDR = 0x800
MODE_ADDR = 0x800C268C          # g_FieldSystemMode (config/symbol_addrs.field.txt)
MODE_HI, MODE_LO = 0x800C, 0x268C
FIELD_MAIN = 0x80077E88
DEBUG_BASE = 0x80280000
DEBUG_ENTRIES = {0x802811EC, 0x80281204, 0x8028125C, 0x802812A4, 0x80281400,
                 0x80281450, 0x802815B0, 0x80281678, 0x80281B00, 0x80284EA4}


def words(raw, vram):
    for off in range(0, len(raw) - 3, 4):
        yield vram + off, struct.unpack_from('<I', raw, off)[0]


def op(w): return w >> 26
def rs(w): return (w >> 21) & 31
def rt(w): return (w >> 16) & 31
def imm(w): return w & 0xFFFF
def simm(w): return imm(w) - 0x10000 if imm(w) & 0x8000 else imm(w)


def branch_target(addr, w):
    o = op(w)
    if o in (4, 5, 6, 7) or (o == 1 and rt(w) in (0, 1, 16, 17)):
        return addr + 4 + (simm(w) << 2)
    if o == 2:  # j (jal is a call, not a label)
        return (addr & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
    return None


def writes(w):
    """Register written by instruction w (None if none / not tracked)."""
    o = op(w)
    if o == 0:
        return (w >> 11) & 31 if (w & 0x3F) not in (8, 0x18, 0x19, 0x1A, 0x1B, 0x11, 0x13) else None
    if o == 3:
        return 31
    if 8 <= o <= 15 or 0x20 <= o <= 0x26:
        return rt(w)
    if o == 0x12 and rs(w) in (0, 2):      # mfc2 / cfc2
        return rt(w)
    return None


def last_writer(at, before, reg):
    """Address of the closest instruction before `before` that writes reg."""
    for b in range(before - 4, before - 4 * 256, -4):
        w = at.get(b)
        if w is None:
            return None
        if writes(w) == reg:
            return b
    return None


def mode_refs(code, opcode):
    """(addr, rt) of `lw`/`sw` rt, lo(MODE)(rY) where rY still holds `lui hi(MODE)`.

    The lui tracking is linear over the image and dropped when rY is
    overwritten, so it only over-approximates across a label -- acceptable for
    a check that must find *every* store (extra hits fail loudly)."""
    out = []
    hi_reg = {}
    for addr, w in code:
        if op(w) == opcode and imm(w) == MODE_LO and hi_reg.get(rs(w)) == MODE_HI:
            out.append((addr, rt(w)))
        d = writes(w)
        if d is not None:
            hi_reg.pop(d, None)
        if op(w) == 0x0F:
            hi_reg[rt(w)] = imm(w)
    return out


def mode_loads(code):
    return dict(mode_refs(code, 0x23))


def mode_stores(code):
    return mode_refs(code, 0x2B)


def analyze(field, slus):
    """Return (errors, facts) for checks 1-4."""
    errors, facts = [], {}
    code = list(words(field, FIELD_VRAM))
    at = {a: w for a, w in code}

    # 1. D_80010000
    d = struct.unpack_from('<I', slus, SLUS_HDR + (0x80010000 - SLUS_VRAM))[0]
    facts['D_80010000'] = d
    if d != 0xFFFFFFFF:
        errors.append(f'D_80010000 = {d:#010x}, not 0xFFFFFFFF')

    # 2/3. stores to g_FieldSystemMode
    stores = mode_stores(code)
    facts['field_stores'] = [hex(a) for a, _ in stores]
    fm_end = FIELD_MAIN + 0x60
    if sorted(r for _, r in stores) != [0, 2] or \
            not all(FIELD_MAIN <= a < fm_end for a, _ in stores):
        errors.append(f'unexpected g_FieldSystemMode stores {facts["field_stores"]}')
    else:
        # lw v1,D_80010000; addiu v0,$zero,-1; beq v1,v0 -> the `ori v0,1` store
        seq = [at[FIELD_MAIN + 4], at[FIELD_MAIN + 8], at[FIELD_MAIN + 12]]
        beq = next(((a, w) for a, w in code
                    if FIELD_MAIN <= a < fm_end and op(w) == 4), None)
        ok = (seq[0] == 0x3C038001 and seq[1] == 0x8C630000 and
              seq[2] == 0x2402FFFF and beq is not None and
              {rs(beq[1]), rt(beq[1])} == {2, 3})
        if ok:
            tgt = branch_target(*beq)
            store_one = [a for a, r in stores if r == 2][0]
            ok = (tgt <= store_one and at[tgt] == 0x34020001 and
                  [r for a, r in stores if a < tgt] == [0])
        if not ok:
            errors.append('FieldMain no longer maps D_80010000 == -1 to mode 1')
    slus_code = list(words(slus[SLUS_HDR:], SLUS_VRAM))
    s_stores = mode_stores(slus_code)
    if s_stores:
        errors.append(f'SLUS stores g_FieldSystemMode at {[hex(a) for a, _ in s_stores]}')

    # 4. guards
    targets = {}
    for a, w in code:
        t = branch_target(a, w)
        if t is not None:
            targets.setdefault(t, []).append(a)
    loads = mode_loads(code)
    sites = []
    for a, w in code:
        if op(w) == 3:
            tgt = 0x80000000 | ((w & 0x3FFFFFF) << 2)
            if 0x80200000 <= tgt < 0x80800000:
                sites.append((a, tgt))
        if op(w) == 0x0F and imm(w) == (DEBUG_BASE >> 16):
            sites.append((a, DEBUG_BASE))
    facts['sites'] = len(sites)
    facts['callees'] = sorted({t for _, t in sites if t != DEBUG_BASE})
    for a, tgt in sites:
        if tgt != DEBUG_BASE and tgt not in DEBUG_ENTRIES:
            errors.append(f'{a:#x}: jal {tgt:#x} is not a known debug entry')
        guard = None
        for b in range(a - 4, a - 4 * 64, -4):
            w = at.get(b)
            if w is None or b < FIELD_VRAM:
                break
            if op(w) == 5 and rt(w) == 0 and branch_target(b, w) > a:
                src = last_writer(at, b, rs(w))
                if src in loads and loads[src] == rs(w):
                    guard = b
                    break
            if op(w) == 2 or (op(w) == 0 and (w & 0x3F) == 8):   # j / jr end the path
                break
        if guard is None:
            errors.append(f'{a:#x}: call to {tgt:#x} has no g_FieldSystemMode guard')
            continue
        entered = [t for t in targets if guard + 8 <= t <= a]
        if entered:
            errors.append(f'{a:#x}: label(s) {[hex(t) for t in entered]} bypass the guard at {guard:#x}')
    return errors, facts


def read_debug_overlay(slus, disc):
    exe = slus
    def at(v): return v - SLUS_VRAM + SLUS_HDR
    hdr4 = struct.unpack_from('<H', exe, at(0x80018004) + 2 * 4)[0]
    e = exe[at(0x80010004) + (0xAD + hdr4 - 1 - 1) * 7:][:7]
    sector = e[0] | e[1] << 8 | e[2] << 16
    size = struct.unpack('<i', e[3:7])[0]
    out = b''
    with open(disc, 'rb') as f:
        for i in range((size + 2047) // 2048):
            f.seek((sector + i) * 2352)
            s = f.read(2352)
            out += s[24:24 + 2048]
    return sector, size, out[:size]


def check_c_sources(sources=None):
    errors, n = [], 0
    if sources is None:
        sources = {p.relative_to(ROOT): p.read_text(errors='replace')
                   for p in sorted((ROOT / 'src/field').rglob('*.c'))}
    for p, text in sources.items():
        lines = text.splitlines()
        for i, line in enumerate(lines):
            m = re.search(r'\b(func_8028[0-9A-F]{4})\s*\(', line)
            if not m or re.match(r'\s*(extern|void|s32|int)\b', line) or \
                    line.strip().startswith(('/*', '*')):
                continue
            n += 1
            ctx = '\n'.join(lines[max(0, i - 4):i])
            if not re.search(r'if\s*\(\s*g_FieldSystemMode\s*==\s*'
                             r'(0|SYSTEM_MODE_PC_HDD|SYSTEM_PC_HARDDRIVE)\s*\)\s*\{?\s*$',
                             ctx, re.M):
                errors.append(f'{p}:{i + 1}: unguarded {m.group(1)}')
    return errors, n


def main():
    disc_dir = Path(os.environ.get('XENO_DISC_DIR', ROOT / 'disc'))
    field = (disc_dir / 'field.bin').read_bytes()
    slus = (disc_dir / 'SLUS_006.64').read_bytes()
    if hashlib.sha256(field).hexdigest() != FIELD_SHA:
        print('FAIL: disc/field.bin is not the retail USA field overlay')
        return 1

    errors, facts = analyze(field, slus)
    print(f'D_80010000={facts["D_80010000"]:#010x}  mode stores={facts["field_stores"]}')
    print(f'guarded sites={facts["sites"]}  callees='
          f'{" ".join(f"func_{c:08X}" for c in facts["callees"])}')
    if set(facts['callees']) != DEBUG_ENTRIES:
        errors.append(f'callee set changed: {sorted(hex(c) for c in facts["callees"])}')

    c_errors, n_c = check_c_sources()
    errors += c_errors
    print(f'C call sites in src/field: {n_c}, all inside a PC-HDD guard' if not c_errors
          else f'C call sites in src/field: {n_c}')

    disc = disc_dir / 'disc1.bin'
    if disc.exists():
        sector, size, blob = read_debug_overlay(slus, disc)
        if size != 25032 or b'CPU=%04d GPU=%04d' not in blob:
            errors.append(f'archive dir 4 entry 0xAD is not the debug overlay (size {size})')
        else:
            # The lowest entry directly follows the overlay's rodata (format
            # strings + a pointer table); every later one follows `jr $ra; nop`.
            if not blob.startswith(b'REC %07d'):
                errors.append('entry 0xAD does not start with the debug format strings')
            for ent in sorted(DEBUG_ENTRIES)[1:]:
                off = ent - DEBUG_BASE
                prev = struct.unpack_from('<II', blob, off - 8)
                if off >= size or prev != (0x03E00008, 0):
                    errors.append(f'func_{ent:08X} is not a function start in entry 0xAD')
            print(f'debug overlay: archive dir 4 entry 0xAD, sector {sector}, {size} bytes, '
                  f'all {len(DEBUG_ENTRIES)} entries follow `jr $ra; nop`')
    else:
        print('NOTE: disc/disc1.bin absent; overlay-identity check (5) not run')
        errors.append('disc/disc1.bin absent: check 5 NOT RUN')

    # Negative controls: each broken copy must be rejected.
    def patched(buf, vaddr, vram, word, hdr=0):
        b = bytearray(buf)
        struct.pack_into('<I', b, hdr + vaddr - vram, word)
        return bytes(b)
    controls = {
        'D_80010000 zeroed': (field, patched(slus, 0x80010000, SLUS_VRAM, 0, SLUS_HDR)),
        # FieldProfilerMark guard in func_8007554C (bnez v1 at 0x80075590 -> nop)
        'profiler guard removed': (patched(field, 0x80075590, FIELD_VRAM, 0), slus),
        # extra `lui $at,0x800C; sw $zero, g_FieldSystemMode($at)` in func_80079288's epilogue
        'stray mode store': (patched(patched(field, 0x80079538, FIELD_VRAM, 0x3C01800C),
                                     0x8007953C, FIELD_VRAM, 0xAC20268C), slus),
        # FieldMain stores 1 on both paths
        'mode forced to CD-ROM ignored': (patched(field, 0x80077EBC, FIELD_VRAM, 0xAC22268C), slus),
    }
    unguarded = {'control.c': 'void f(void) {\n    if (g_FieldSystemMode != 0) {\n'
                              '        g();\n    }\n    func_80281B00(&D_8006FB34);\n}\n'}
    if not check_c_sources(unguarded)[0]:
        errors.append('negative control "unguarded C call" was NOT rejected')
    else:
        print('control "unguarded C call" rejected')
    at = {a: w for a, w in words(field, FIELD_VRAM)}
    assert op(at[0x80075590]) == 5, hex(at[0x80075590])  # the guard really is a bnez
    for name, (f, s) in controls.items():
        errs, _ = analyze(f, s)
        if not errs:
            errors.append(f'negative control "{name}" was NOT rejected')
        else:
            print(f'control "{name}" rejected: {errs[0]}')

    if errors:
        for e in errors:
            print('FAIL:', e)
        return 1
    print('PASS field debug overlay (0x8028xxxx) unreachable in retail CD-ROM mode '
          f'({len(controls) + 1} negative controls rejected)')
    return 0


if __name__ == '__main__':
    sys.exit(main())

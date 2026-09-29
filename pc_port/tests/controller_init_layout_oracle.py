#!/usr/bin/env python3
"""Independent oracle for retail ControllerInit (0x80036288) + func_8003611C.

Executes the retail bytes from disc/SLUS_006.64 with a tiny MIPS subset
interpreter (only the opcodes these two leaf-level routines use; anything else
aborts). Calls are recorded, not followed, except func_8003611C, whose body is
interpreted too because it mutates game state. Memory starts from the retail
.sdata bytes plus the seeded values given on the command line, and the final
state of every byte the routines wrote is emitted as a C header for the
production-body test (controller_init_layout_test.c).

usage: controller_init_layout_oracle.py SLUS_006.64 out.h
"""
import struct
import sys

SLUS_VRAM, HDR = 0x80010000, 0x800
CONTROLLER_INIT, RESET_ACTUATORS = 0x80036288, 0x8003611C
NAMES = {0x80040828: 'InitPAD', 0x800408C4: 'StartPAD', 0x800405D4: 'ChangeClearPAD',
         0x80035DB0: 'ControllerResetState', 0x8003611C: 'func_8003611C',
         0x80040C3C: 'func_80040C3C'}
# Seeds: bytes the test pre-loads identically before calling the C body.
SEED_ACT = bytes(range(0xA0, 0xB0))            # D_8005A1BC[16]
SEEDS = {0x8005938C: b'\x55', 0x80059390: b'\x78\x56\x34\x12'}


def run(exe, mem, entry, calls):
    def rd(a, n):
        out = bytearray()
        for i in range(n):
            b = a + i
            if b in mem:
                out.append(mem[b])
            elif SLUS_VRAM <= b < SLUS_VRAM + len(exe) - HDR:
                out.append(exe[HDR + b - SLUS_VRAM])
            else:
                raise RuntimeError(f'read of unseeded {b:#x}')
        return int.from_bytes(out, 'little')
    def wr(a, n, v):
        for i in range(n):
            mem[a + i] = (v >> (8 * i)) & 0xFF
    reg = [0] * 32
    reg[29] = 0x801FF000
    reg[31] = 0xDEAD0000
    pc, npc = entry, entry + 4
    pending = None
    for _ in range(10000):
        pending_now = None
        if pc == 0xDEAD0000:
            return
        w = struct.unpack_from('<I', exe, HDR + pc - SLUS_VRAM)[0]
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        nxt = npc + 4
        target = None
        def setr(r, v):
            if r:
                reg[r] = v & 0xFFFFFFFF
        if w == 0:
            pass
        elif op == 0 and (w & 0x3F) == 0x21:                 # addu
            setr((w >> 11) & 31, reg[rs] + reg[rt])
        elif op == 0 and (w & 0x3F) == 0x08:                 # jr
            target = reg[rs]
        elif op == 3:                                        # jal
            dest = (pc & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            pending_now = dest
            if dest == RESET_ACTUATORS:
                target = dest
                setr(31, pc + 8)
            # other callees: recorded only (their effects are out of scope)
        elif op == 1 and rt == 1:                            # bgez
            if reg[rs] < 0x80000000:
                target = npc + (simm << 2)
        elif op == 5:                                        # bne
            if reg[rs] != reg[rt]:
                target = npc + (simm << 2)
        elif op == 9:
            setr(rt, reg[rs] + simm)                         # addiu
        elif op == 13:
            setr(rt, reg[rs] | imm)                          # ori
        elif op == 15:
            setr(rt, imm << 16)                              # lui
        elif op in (0x20, 0x24, 0x21, 0x25, 0x23):           # lb lbu lh lhu lw
            n = {0x20: 1, 0x24: 1, 0x21: 2, 0x25: 2, 0x23: 4}[op]
            v = rd((reg[rs] + simm) & 0xFFFFFFFF, n)
            if op in (0x20, 0x21) and v & (1 << (8 * n - 1)):
                v -= 1 << (8 * n)
            setr(rt, v)
        elif op in (0x22, 0x26):                             # lwl / lwr
            a = (reg[rs] + simm) & 0xFFFFFFFF
            base = a & ~3
            word = rd(base, 4)
            k = a & 3
            if op == 0x22:
                sh = 8 * (3 - k)
                mask = (0xFFFFFFFF << sh) & 0xFFFFFFFF
                setr(rt, (reg[rt] & ~mask) | ((word << sh) & mask))
            else:
                sh = 8 * k
                mask = 0xFFFFFFFF >> sh
                setr(rt, (reg[rt] & ~mask) | (word >> sh))
        elif op in (0x2A, 0x2E):                             # swl / swr
            a = (reg[rs] + simm) & 0xFFFFFFFF
            base, k = a & ~3, a & 3
            word = rd(base, 4)
            if op == 0x2A:
                sh = 8 * (3 - k)
                mask = 0xFFFFFFFF >> sh
                word = (word & ~mask) | (reg[rt] >> sh)
            else:
                sh = 8 * k
                mask = (0xFFFFFFFF << sh) & 0xFFFFFFFF
                word = (word & ~mask) | ((reg[rt] << sh) & mask)
            wr(base, 4, word & 0xFFFFFFFF)
        elif op in (0x28, 0x29, 0x2B):                       # sb sh sw
            n = {0x28: 1, 0x29: 2, 0x2B: 4}[op]
            wr((reg[rs] + simm) & 0xFFFFFFFF, n, reg[rt])
        else:
            raise RuntimeError(f'unsupported instruction {w:#010x} at {pc:#x}')
        if pending is not None:          # this instruction was the jal delay slot
            calls.append((NAMES.get(pending, hex(pending)), reg[4], reg[5], reg[6], reg[7]))
        pending = pending_now
        pc, npc = npc, (target if target is not None else nxt)
    raise RuntimeError('did not return')


def main():
    exe = open(sys.argv[1], 'rb').read()
    mem = {}
    for i, b in enumerate(SEED_ACT):
        mem[0x8005A1BC + i] = b
    for a, bs in SEEDS.items():
        for i, b in enumerate(bs):
            mem[a + i] = b
    seeded = dict(mem)
    calls = []
    run(exe, mem, CONTROLLER_INIT, calls)
    maps = [mem.get(0x80050238 + i, exe[HDR + 0x80050238 + i - SLUS_VRAM]) for i in range(8)]
    masks = [struct.unpack_from('<H', exe, HDR + 0x800501E8 + 2 * i - SLUS_VRAM)[0] for i in range(8)]
    act = [mem[0x8005A1BC + i] for i in range(16)]
    written = sorted(a for a in mem if mem[a] != seeded.get(a) or a not in seeded)
    names = [c[0] for c in calls]
    print('retail calls:', ' '.join(names))
    print('retail mappings:', maps, ' D_8005938C=%d D_80059390=%#x D_80050204=%#x' % (
        mem[0x8005938C], int.from_bytes(bytes(mem[0x80059390 + i] for i in range(4)), 'little'),
        int.from_bytes(bytes(mem.get(0x80050204 + i, 0xEE) for i in range(4)), 'little')))
    expected_calls = ['InitPAD', 'StartPAD', 'ChangeClearPAD', 'ControllerResetState',
                      'func_8003611C', 'func_80040C3C']
    if names != expected_calls:
        print('FAIL: retail call sequence changed', names)
        return 1
    initpad = calls[0]
    if initpad[1:] != (0x800625FC, 0x22, 0x800625FC + 0x22, 0x22):
        print('FAIL: InitPAD arguments', [hex(x) for x in initpad[1:]])
        return 1
    with open(sys.argv[2], 'w') as f:
        f.write('/* Generated by controller_init_layout_oracle.py from disc/SLUS_006.64. */\n')
        f.write('static const unsigned char kSeedAct[16] = {%s};\n' % ','.join(map(str, SEED_ACT)))
        f.write('static const unsigned char kSdataMappings[8] = {%s};\n' % ','.join(
            str(exe[HDR + 0x80050238 + i - SLUS_VRAM]) for i in range(8)))
        f.write('static const unsigned short kSdataMasks[8] = {%s};\n' % ','.join(map(str, masks)))
        f.write('static const unsigned char kRetailMappings[8] = {%s};\n' % ','.join(map(str, maps)))
        f.write('static const unsigned char kRetailAct[16] = {%s};\n' % ','.join(map(str, act)))
        f.write('#define RETAIL_8005938C %du\n' % mem[0x8005938C])
        f.write('#define RETAIL_80059390 %du\n' % int.from_bytes(
            bytes(mem[0x80059390 + i] for i in range(4)), 'little'))
        f.write('#define SEED_8005938C %du\n#define SEED_80059390 0x12345678u\n' % SEEDS[0x8005938C][0])
    print(f'oracle: {len(written)} bytes written by retail ControllerInit/func_8003611C')
    return 0


if __name__ == '__main__':
    sys.exit(main())

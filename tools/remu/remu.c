/* remu: tiny MIPS-I (R3000 integer subset) emulator for retail-oracle testing.
 *
 * Purpose: execute RETAIL bytes (parsed from splat .s disassembly comments)
 * so a transcribed C function can be diffed against real retail behavior
 * without needing the retail cc1 toolchain.
 *
 * Supported: all integer ALU/branch/jump/load/store ops incl. lwl/lwr/swl/swr,
 * mult/div (single-cycle result), mfc0/mtc0 (stubbed: status reads 0).
 * NOT supported: FPU (cop1), GTE (cop2), cache ops, real BIOS. A jal/j to an
 * address with no loaded code is treated as an external stub call: v0 is
 * forced to 0, the call is logged, execution continues. Tests must seed any
 * global state the function reads (uninit reads are tracked and reported).
 *
 * Memory model: 2MB PSX RAM; KSEG0/KSEG1/uncached mirrors normalized.
 * Delay slots are emulated. No load-delay (retail code carries its nops).
 */
#include "remu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RAM_SIZE (2u * 1024u * 1024u)
#define SCRATCH_SIZE 1024u
#define MEM_SIZE (RAM_SIZE + SCRATCH_SIZE)
/* scratchpad lives just past RAM in the same allocation */
#define SCRATCH_OFF RAM_SIZE

struct remu {
    uint8_t *ram;
    uint8_t *touched; /* 1 per byte once written (or seeded) */
    uint32_t regs[32];
    uint32_t hi, lo;
    uint32_t pc;
    int steps;
    int max_steps;
    int halted;
    int stub_calls;
    char stub_log[4096];
    /* loaded code ranges (vram,words) for stub detection */
    uint32_t code_lo[512];
    uint32_t code_hi[512];
    int nranges;
};

static uint32_t norm(uint32_t a) {
    /* KSEG0 0x80000000, KSEG1 0xA0000000, KUSEG 0x00000000 mirrors of RAM */
    if ((a & 0xFFE00000u) == 0x80000000u || (a & 0xFFE00000u) == 0xA0000000u ||
        (a & 0xFFE00000u) == 0x00000000u)
        return a & 0x1FFFFFu;
    /* PSX scratchpad 0x1F800000-0x1F8003FF (fully tracked, zero-init) */
    if (a >= 0x1F800000u && a < 0x1F800000u + SCRATCH_SIZE)
        return SCRATCH_OFF + (a - 0x1F800000u);
    /* HW regs / BIOS / anything else: loud fault (no silent wrong answer) */
    return 0xFFFFFFFFu;
}

remu_t *remu_create(void) {
    remu_t *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->ram = calloc(1, RAM_SIZE + SCRATCH_SIZE);
    m->touched = calloc(1, RAM_SIZE + SCRATCH_SIZE);
    if (!m->ram || !m->touched) { remu_destroy(m); return NULL; }
    m->max_steps = 1000000;
    m->regs[29] = 0x801FFF00u; /* sp */
    m->regs[28] = 0x80059170u; /* gp (per overlay yaml) */
    return m;
}

void remu_destroy(remu_t *m) {
    if (!m) return;
    free(m->ram); free(m->touched); free(m);
}

void remu_set_reg(remu_t *m, int r, uint32_t v) { if (r) m->regs[r] = v; }
uint32_t remu_get_reg(remu_t *m, int r) { return m->regs[r]; }

int remu_poke(remu_t *m, uint32_t addr, const void *data, uint32_t len) {
    uint32_t o = norm(addr);
    if (o == 0xFFFFFFFFu || o + len > MEM_SIZE) return -1;
    memcpy(m->ram + o, data, len);
    memset(m->touched + o, 1, len);
    return 0;
}

int remu_peek(remu_t *m, uint32_t addr, void *data, uint32_t len) {
    uint32_t o = norm(addr);
    if (o == 0xFFFFFFFFu || o + len > MEM_SIZE) return -1;
    memcpy(data, m->ram + o, len);
    return 0;
}

/* Parse one splat .s function file: lines look like
 *     / * 486CC 800B81BC E0FFBD27 * /  addiu $sp, $sp, -0x20
 * (comment = romoff vram opcode). Loads words at vram. Returns entry vram
 * (address of `glabel name`) or 0 on parse failure. Extra dep files can be
 * appended with remu_load_s again; ranges accumulate for stub detection. */
uint32_t remu_load_s(remu_t *m, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char line[1024];
    uint32_t entry = 0, first = 0xFFFFFFFFu, last = 0;
    int words = 0;
    while (fgets(line, sizeof line, f)) {
        unsigned rom = 0, vram = 0, op = 0;
        if (sscanf(line, " /* %x %x %x", &rom, &vram, &op) == 3) {
            /* op is the ROM byte sequence in display order (big-endian
             * print of little-endian bytes): byte i = (op >> 24-8i). */
            uint32_t o = norm(vram);
            if (o == 0xFFFFFFFFu || o + 4 > MEM_SIZE) continue;
            m->ram[o] = (uint8_t)((op >> 24) & 0xFF);
            m->ram[o+1] = (uint8_t)((op >> 16) & 0xFF);
            m->ram[o+2] = (uint8_t)((op >> 8) & 0xFF);
            m->ram[o+3] = (uint8_t)(op & 0xFF);
            if (vram < first) first = vram;
            if (vram > last) last = vram;
            words++;
            if (!entry) {
                /* entry = vram of first opcode line */
                entry = vram;
            }
        } else if (!entry && (strstr(line, "glabel ") || strstr(line, "jlabel "))) {
            /* label seen before first opcode; entry resolved at first word */
        }
    }
    fclose(f);
    if (!words) return 0;
    if (m->nranges < 512) {
        m->code_lo[m->nranges] = first;
        m->code_hi[m->nranges] = last + 4;
        m->nranges++;
    }
    return entry;
}

static int in_code(remu_t *m, uint32_t pc) {
    for (int i = 0; i < m->nranges; i++)
        if (pc >= m->code_lo[i] && pc < m->code_hi[i]) return 1;
    return 0;
}

static uint32_t rd32(remu_t *m, uint32_t addr, int *uninit) {
    uint32_t o = norm(addr);
    if (o == 0xFFFFFFFFu || o + 4 > MEM_SIZE || (addr & 3)) { m->halted = -2; return 0; }
    if (!m->touched[o] || !m->touched[o+1] || !m->touched[o+2] || !m->touched[o+3])
        *uninit = 1;
    uint32_t v;
    memcpy(&v, m->ram + o, 4);
    return v;
}

static void wr32(remu_t *m, uint32_t addr, uint32_t v) {
    uint32_t o = norm(addr);
    if (o == 0xFFFFFFFFu || o + 4 > MEM_SIZE || (addr & 3)) { m->halted = -2; return; }
    memcpy(m->ram + o, &v, 4);
    memset(m->touched + o, 1, 4);
}

static void exec_one(remu_t *m, uint32_t pc, uint32_t w,
                     uint32_t *branch_to, int *is_branch, int *is_jal_link,
                     uint32_t *link_reg) {
    uint32_t op = (w >> 26) & 63, rs = (w >> 21) & 31, rt = (w >> 16) & 31;
    uint32_t rd = (w >> 11) & 31, sh = (w >> 6) & 31, fn = w & 63;
    int16_t imm = (int16_t)(w & 0xFFFF);
    uint32_t uimm = w & 0xFFFF;
    uint32_t *R = m->regs;
    *is_branch = 0; *is_jal_link = 0;
#define W(r, v) do { if (r) R[r] = (v); } while (0)
    switch (op) {
    case 0: /* SPECIAL */
        switch (fn) {
        case 0x00: W(rd, R[rt] << sh); break;                    /* SLL */
        case 0x02: W(rd, R[rt] >> sh); break;                    /* SRL */
        case 0x03: W(rd, (uint32_t)((int32_t)R[rt] >> sh)); break; /* SRA */
        case 0x04: W(rd, R[rt] << (R[rs] & 31)); break;          /* SLLV */
        case 0x06: W(rd, R[rt] >> (R[rs] & 31)); break;          /* SRLV */
        case 0x07: W(rd, (uint32_t)((int32_t)R[rt] >> (R[rs] & 31))); break; /* SRAV */
        case 0x08: /* JR */ *is_branch = 1; *branch_to = R[rs]; break;
        case 0x09: /* JALR */ W(rd ? rd : 31, pc + 8); *is_branch = 1; *branch_to = R[rs]; break;
        case 0x0C: m->halted = 1; break;                         /* SYSCALL */
        case 0x0D: m->halted = 2; break;                         /* BREAK */
        case 0x10: W(rd, m->hi); break;                          /* MFHI */
        case 0x11: W(rd, m->hi = R[rs]); break;                  /* MTHI */
        case 0x12: W(rd, m->lo); break;                          /* MFLO */
        case 0x13: W(rd, m->lo = R[rs]); break;                  /* MTLO */
        case 0x18: { int64_t p = (int64_t)(int32_t)R[rs] * (int64_t)(int32_t)R[rt]; /* MULT */
                     m->lo = (uint32_t)p; m->hi = (uint32_t)(p >> 32); break; }
        case 0x19: { uint64_t p = (uint64_t)R[rs] * (uint64_t)R[rt]; /* MULTU */
                     m->lo = (uint32_t)p; m->hi = (uint32_t)(p >> 32); break; }
        case 0x1A: if ((int32_t)R[rt] != 0) { /* DIV */
                     m->lo = (uint32_t)((int32_t)R[rs] / (int32_t)R[rt]);
                     m->hi = (uint32_t)((int32_t)R[rs] % (int32_t)R[rt]); } break;
        case 0x1B: if (R[rt] != 0) { m->lo = R[rs] / R[rt]; m->hi = R[rs] % R[rt]; } break; /* DIVU */
        case 0x20: W(rd, R[rs] + R[rt]); break;                  /* ADD (no trap) */
        case 0x21: W(rd, R[rs] + R[rt]); break;                  /* ADDU */
        case 0x22: W(rd, R[rs] - R[rt]); break;                  /* SUB */
        case 0x23: W(rd, R[rs] - R[rt]); break;                  /* SUBU */
        case 0x24: W(rd, R[rs] & R[rt]); break;                  /* AND */
        case 0x25: W(rd, R[rs] | R[rt]); break;                  /* OR */
        case 0x26: W(rd, R[rs] ^ R[rt]); break;                  /* XOR */
        case 0x27: W(rd, ~(R[rs] | R[rt])); break;               /* NOR */
        case 0x2A: W(rd, (int32_t)R[rs] < (int32_t)R[rt]); break; /* SLT */
        case 0x2B: W(rd, R[rs] < R[rt]); break;                  /* SLTU */
        default: m->halted = -3; break;
        }
        break;
    case 0x01: /* REGIMM */
        switch (rt) {
        case 0x00: if ((int32_t)R[rs] < 0) { *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BLTZ */
        case 0x01: if ((int32_t)R[rs] >= 0) { *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BGEZ */
        case 0x10: if ((int32_t)R[rs] < 0) { W(31, pc + 8); *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BLTZAL */
        case 0x11: if ((int32_t)R[rs] >= 0) { W(31, pc + 8); *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BGEZAL */
        default: m->halted = -3; break;
        }
        break;
    case 0x02: *is_branch = 1; *branch_to = (pc & 0xF0000000u) | ((w & 0x3FFFFFFu) << 2); break; /* J */
    case 0x03: /* JAL */ W(31, pc + 8); *is_branch = 1; *branch_to = (pc & 0xF0000000u) | ((w & 0x3FFFFFFu) << 2); break;
    case 0x04: if (R[rs] == R[rt]) { *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BEQ */
    case 0x05: if (R[rs] != R[rt]) { *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BNE */
    case 0x06: if ((int32_t)R[rs] <= 0) { *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BLEZ */
    case 0x07: if ((int32_t)R[rs] > 0) { *is_branch = 1; *branch_to = pc + 4 + ((int32_t)imm * 4); } break; /* BGTZ */
    case 0x08: W(rt, R[rs] + (int32_t)imm); break;               /* ADDI */
    case 0x09: W(rt, R[rs] + (int32_t)imm); break;               /* ADDIU */
    case 0x0A: W(rt, (int32_t)R[rs] < imm); break;               /* SLTI */
    case 0x0B: W(rt, R[rs] < (uint32_t)(int32_t)imm); break;     /* SLTIU */
    case 0x0C: W(rt, R[rs] & uimm); break;                       /* ANDI */
    case 0x0D: W(rt, R[rs] | uimm); break;                       /* ORI */
    case 0x0E: W(rt, R[rs] ^ uimm); break;                       /* XORI */
    case 0x0F: W(rt, uimm << 16); break;                         /* LUI */
    case 0x10: /* COP0 */ {
        unsigned co = (w >> 25) & 1, copop = (w >> 21) & 31;
        if (co) { /* CO: tlbr/tlbwi/rfe/eret: ignore */ }
        else if (copop == 0) W(rt, 0);        /* MFC0 -> 0 */
        else if (copop == 4) { }              /* MTC0: ignore */
        else m->halted = -3;
        break;
    }
    case 0x11: case 0x12: case 0x13: m->halted = -4; break;      /* COP1/2/3 */
    case 0x20: case 0x21: case 0x22: { /* LB/LH/LWL */
        uint32_t a = R[rs] + (int32_t)imm;
        if (op == 0x20) { uint32_t o = norm(a); if (o == 0xFFFFFFFFu || o >= MEM_SIZE) { m->halted = -2; break; }
            if (!m->touched[o]) m->halted = 3; W(rt, (uint32_t)(int8_t)m->ram[o]); }
        else if (op == 0x21) { if (a & 1) { m->halted = -2; break; } uint32_t o = norm(a);
            if (o == 0xFFFFFFFFu || o + 2 > MEM_SIZE) { m->halted = -2; break; }
            { uint16_t hv = (uint16_t)(m->ram[o] | (m->ram[o+1] << 8));
              if (!m->touched[o] || !m->touched[o+1]) m->halted = 3;
              else W(rt, (uint32_t)(int16_t)hv); } }
        else { /* LWL */ uint32_t al = a & ~3u; int un = 0; uint32_t mv = rd32(m, al, &un);
            if (un) m->halted = 3; if (m->halted < 0) break;
            unsigned shift = ((3u - (a & 3)) * 8); uint32_t mask = 0xFFFFFFFFu << shift;
            W(rt, (R[rt] & ~mask) | ((mv << shift) & mask)); }
        break;
    }
    case 0x23: { int un = 0; uint32_t v = rd32(m, R[rs] + (int32_t)imm, &un); /* LW */
        if (un) m->halted = 3; else W(rt, v); break; }
    case 0x24: { uint32_t a = R[rs] + (int32_t)imm; uint32_t o = norm(a); /* LBU */
        if (o == 0xFFFFFFFFu || o >= MEM_SIZE) { m->halted = -2; break; }
        if (!m->touched[o]) m->halted = 3; W(rt, m->ram[o]); break; }
    case 0x25: { uint32_t a = R[rs] + (int32_t)imm; /* LHU */
        if (a & 1) { m->halted = -2; break; } uint32_t o = norm(a);
        if (o == 0xFFFFFFFFu || o + 2 > MEM_SIZE) { m->halted = -2; break; }
        if (!m->touched[o] || !m->touched[o+1]) m->halted = 3;
        W(rt, (uint32_t)(m->ram[o] | (m->ram[o+1] << 8))); break; }
    case 0x26: { /* LWR */ uint32_t a = R[rs] + (int32_t)imm; uint32_t al = a & ~3u; int un = 0;
        uint32_t mv = rd32(m, al, &un); if (un) m->halted = 3; if (m->halted < 0) break;
        unsigned shift = ((a & 3) * 8); uint32_t mask = 0xFFFFFFFFu >> shift;
        W(rt, (R[rt] & ~mask) | ((mv >> shift) & mask)); break; }
    case 0x28: { uint32_t a = R[rs] + (int32_t)imm; uint32_t o = norm(a); /* SB */
        if (o == 0xFFFFFFFFu || o >= MEM_SIZE) { m->halted = -2; break; }
        m->ram[o] = R[rt] & 0xFF; m->touched[o] = 1; break; }
    case 0x29: { uint32_t a = R[rs] + (int32_t)imm; /* SH */
        if (a & 1) { m->halted = -2; break; } uint32_t o = norm(a);
        if (o == 0xFFFFFFFFu || o + 2 > MEM_SIZE) { m->halted = -2; break; }
        m->ram[o] = R[rt] & 0xFF; m->ram[o+1] = (R[rt] >> 8) & 0xFF;
        m->touched[o] = m->touched[o+1] = 1; break; }
    case 0x2A: case 0x2B: { /* SWL/SWR/SW */
        uint32_t a = R[rs] + (int32_t)imm;
        if (op == 0x2B) { wr32(m, a, R[rt]); break; }
        uint32_t al = a & ~3u; uint32_t o = norm(al);
        if (o == 0xFFFFFFFFu || o + 4 > MEM_SIZE) { m->halted = -2; break; }
        uint32_t mv; memcpy(&mv, m->ram + o, 4);
        if (op == 0x2A) { /* SWL: LE bytes 0..k take rt's high bytes */
            unsigned s = (3u - (a & 3)) * 8; uint32_t mask = 0xFFFFFFFFu >> s;
            mv = (mv & ~mask) | ((R[rt] >> s) & mask); }
        else { /* SWR: LE bytes k..3 take rt's low bytes */
            unsigned s = (a & 3) * 8; uint32_t mask = 0xFFFFFFFFu << s;
            mv = (mv & ~mask) | ((R[rt] << s) & mask); }
        memcpy(m->ram + o, &mv, 4);
        /* mark touched conservatively: touched bytes among the 4 depend on lane;
         * tests reseed, so mark the full word */
        memset(m->touched + o, 1, 4);
        break;
    }
    default: m->halted = -3; break;
    }
#undef W
    (void)link_reg;
}

/* fetch word at pc (must be loaded code or mapped RAM) */
static void dbg(remu_t *m, const char *what, uint32_t pc, uint32_t w) {
    if (getenv("REMU_DEBUG"))
        fprintf(stderr, "remu %s pc=%08X w=%08X steps=%d\n", what, pc, w, m->steps);
}

static int fetch(remu_t *m, uint32_t pc, uint32_t *w) {
    uint32_t o = norm(pc);
    if (o == 0xFFFFFFFFu || o + 4 > MEM_SIZE || (pc & 3)) return -1;
    memcpy(w, m->ram + o, 4);
    return 0;
}

/* Run from entry until jr-ra-to-sentinel / syscall / error / step budget.
 * Returns: 0 ok (returned), 1 syscall-stop, 2 break-stop, 3 uninit-read,
 *          -1 step budget, -2 mem fault, -3 unknown opcode, -4 cop1/2/3. */
int remu_call(remu_t *m, uint32_t entry, uint32_t a0, uint32_t a1,
              uint32_t a2, uint32_t a3) {
    m->regs[4] = a0; m->regs[5] = a1; m->regs[6] = a2; m->regs[7] = a3;
    m->regs[31] = 0xDEADBEEFu;
    m->pc = entry;
    m->halted = 0; m->steps = 0; m->stub_calls = 0;
    m->stub_log[0] = '\0';
    int trace = getenv("REMU_TRACE") != NULL;
    for (;;) {
        uint32_t w;
        if (m->steps++ > m->max_steps) return -1;
        if (trace)
            fprintf(stderr, "T pc=%08X v0=%08X v1=%08X a0=%08X ra=%08X\n",
                    m->pc, m->regs[2], m->regs[3], m->regs[4], m->regs[31]);
        if (m->pc == 0xDEADBEEFu) return 0;
        if (!in_code(m, m->pc)) {
            /* external/stub call target: log, force v0=0, return to ra */
            m->stub_calls++;
            size_t n = strlen(m->stub_log);
            snprintf(m->stub_log + n, sizeof(m->stub_log) - n, " %08X", m->pc);
            m->regs[2] = 0;
            m->pc = m->regs[31];
            continue;
        }
        if (fetch(m, m->pc, &w)) { dbg(m, "fetch-fault", m->pc, 0); return -2; }
        uint32_t bto = 0; int br = 0, jl = 0; uint32_t lr = 0;
        exec_one(m, m->pc, w, &bto, &br, &jl, &lr);
        int h = m->halted;
        if (h) {
            if (h < 0) dbg(m, "halt", m->pc, w);
            if (h == 3) return 3;
            if (h == 1) return 1;
            if (h == 2) return 2;
            return h;
        }
        if (!br) {
            /* not a branch: the next word is a normal instruction */
            m->pc += 4;
        } else {
            /* branch: execute the delay slot (its own control effect is
             * undefined on R3000 and ignored), then take the branch */
            uint32_t ds = m->pc + 4, w2;
            if (fetch(m, ds, &w2)) return -2;
            uint32_t bto2 = 0; int br2 = 0; uint32_t lr2 = 0;
            exec_one(m, ds, w2, &bto2, &br2, &jl, &lr2);
            (void)bto2; (void)br2;
            h = m->halted;
            if (h) {
                if (h < 0) dbg(m, "halt-ds", ds, w2);
                if (h == 3) return 3;
                if (h == 1) return 1;
                if (h == 2) return 2;
                return h;
            }
            m->pc = bto;
        }
        m->regs[0] = 0;
    }
}

int remu_stub_calls(remu_t *m) { return m->stub_calls; }
const char *remu_stub_log(remu_t *m) { return m->stub_log; }

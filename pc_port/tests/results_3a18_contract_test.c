/* Contract oracle for RESULTS caller 0x801E3A18 (skill/flag promotion over
 * the three result slots).  Not an adoption and not runtime evidence.
 *
 * Retail side: the real module bytes (extracted from the disc at test time,
 * never in the tree) run in the production MIPS interpreter
 * (battle_mips_adapter.c).  Its six helper calls (0x801E3BE0, 3D54, 3E14,
 * 3F28, 3EA4, 3FB0) are taken by a scripted bridge that records
 * (target, a0), clobbers every caller-saved register a helper may write and
 * sometimes mutates the slot kind, the current-slot word 0x801E44EC or the
 * slot base 0x801E44C8.
 *
 * Model side: model_3A18() below, a register-exact C transcription of
 * 0x801E3A18 with an explicit guest frame, calling the same scripted helpers.
 *
 * Both start from identical RAM and registers; the oracle requires identical
 * call logs, all 2 MB of RAM (guest frame, 0x101C/0x101F result bytes,
 * 0x801E44EC) and all 32 GPRs at return (0x801E3A18 has no mult/div).  This is the guest-frame and
 * call-sequence contract a native 0x801E3A18 must meet; it proves the model,
 * not a production body.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

#define RAM_SIZE 0x200000u
#define MODULE_ADDR 0x801DE000u
#define MODULE_SIZE 0x6500u
#define ENTRY 0x801E3A18u
#define HALT 0x80001000u
#define BASE_WORD 0x801E44C8u
#define SLOT_WORD 0x801E44ECu
#define BUF_A 0x80100000u
#define BUF_B 0x80120000u
#define STACK_TOP 0x801FF000u

enum { R_AT = 1, R_V0 = 2, R_V1 = 3, R_A0 = 4, R_A1 = 5, R_A2 = 6, R_A3 = 7,
       R_T0 = 8, R_S0 = 16, R_S1 = 17, R_S2 = 18, R_T8 = 24, R_T9 = 25,
       R_SP = 29, R_RA = 31 };

static uint8_t g_module[MODULE_SIZE];

typedef struct World {
    uint8_t ram[RAM_SIZE];
    uint32_t log[64][2];
    unsigned calls;
    uint32_t rng;      /* scripted-helper stream, identical on both sides */
} World;

static World g_retail, g_model;
static unsigned g_cover[6], g_mutations[3], g_dead, g_kind_hist[16];

static uint32_t next(uint32_t *s)
{
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5;
    return *s;
}

static int in_ram(uint32_t address, unsigned width)
{
    uint32_t off = address & 0x1FFFFFFFu;
    return off + width <= RAM_SIZE && (address >> 29) == 4u;
}

static uint32_t rd(World *w, uint32_t a, unsigned n)
{
    uint32_t v = 0, off = a & 0x1FFFFFFFu;
    if (!in_ram(a, n)) { fprintf(stderr, "RESULTS 3A18 FAIL model read %08x\n", a); exit(1); }
    for (unsigned i = 0; i < n; i++) v |= (uint32_t)w->ram[off + i] << (8 * i);
    return v;
}

static void wr(World *w, uint32_t a, unsigned n, uint32_t v)
{
    uint32_t off = a & 0x1FFFFFFFu;
    if (!in_ram(a, n)) { fprintf(stderr, "RESULTS 3A18 FAIL model write %08x\n", a); exit(1); }
    for (unsigned i = 0; i < n; i++) w->ram[off + i] = (uint8_t)(v >> (8 * i));
}

static int bus_read(void *o, uint32_t a, unsigned n, uint32_t *v)
{
    if (!in_ram(a, n)) return -1;
    *v = rd(o, a, n);
    return 0;
}

static int bus_write(void *o, uint32_t a, unsigned n, uint32_t v)
{
    if (!in_ram(a, n)) return -1;
    wr(o, a, n, v);
    return 0;
}

static int helper_index(uint32_t target)
{
    static const uint32_t helpers[6] = {0x801E3BE0u, 0x801E3D54u, 0x801E3E14u,
                                        0x801E3F28u, 0x801E3EA4u, 0x801E3FB0u};
    for (int i = 0; i < 6; i++)
        if (helpers[i] == target) return i;
    return -1;
}

/* One scripted helper: log, maybe mutate, clobber caller-saved registers.
 * Shared verbatim by both sides; the stream lives in the World so the
 * draws match call for call. */
static void scripted_helper(World *w, uint32_t *gpr, uint32_t target, int count)
{
    uint32_t r, slot;
    static const int clobber[] = {R_AT, R_V0, R_V1, R_A0, R_A1, R_A2, R_A3,
                                  8, 9, 10, 11, 12, 13, 14, 15, R_T8, R_T9};

    if (w->calls < 64) { w->log[w->calls][0] = target; w->log[w->calls][1] = gpr[R_A0]; }
    w->calls++;
    if (count) g_cover[helper_index(target)]++;
    r = next(&w->rng);
    switch (r % 10u) {
    case 0: case 1:   /* the current slot's kind changes under the caller */
        slot = rd(w, SLOT_WORD, 4);
        wr(w, slot + 0x56u, 1, next(&w->rng) % 13u);
        if (count) g_mutations[0]++;
        break;
    case 2:           /* the current-slot word moves to another slot */
        wr(w, SLOT_WORD, 4, rd(w, BASE_WORD, 4) + 0x170u * (next(&w->rng) % 3u));
        if (count) g_mutations[1]++;
        break;
    case 3:           /* the slot base switches buffers */
        wr(w, BASE_WORD, 4, rd(w, BASE_WORD, 4) == BUF_A ? BUF_B : BUF_A);
        if (count) g_mutations[2]++;
        break;
    default:
        break;
    }
    for (unsigned i = 0; i < sizeof(clobber) / sizeof(clobber[0]); i++)
        gpr[clobber[i]] = next(&w->rng);
    /* v0's low byte is what the caller stores; make zero common. */
    if ((next(&w->rng) & 3u) == 0) gpr[R_V0] &= ~0xFFu;
}

static int bridge(void *o, PcPortMipsCpu *cpu, uint32_t target)
{
    if (helper_index(target) < 0)
        return 0;
    scripted_helper(o, cpu->gpr, target, 1);
    return 1;
}

/* ---- Model: register-exact transcription of 0x801E3A18 ---------------- */
static void call(World *w, uint32_t *r, uint32_t target, uint32_t ret_addr)
{
    r[R_RA] = ret_addr;
    scripted_helper(w, r, target, 0);
}

static void model_3A18(World *w, uint32_t *r)
{
    uint32_t sp;

    sp = r[R_SP] = r[R_SP] - 32u;
#ifndef MUTANT_NO_FRAME
    wr(w, sp + 20u, 4, r[R_S1]);
    wr(w, sp + 24u, 4, r[R_S2]);
    wr(w, sp + 28u, 4, r[R_RA]);
    wr(w, sp + 16u, 4, r[R_S0]);
#endif
    r[R_S1] = 0;
    r[R_S2] = 10;
    r[R_S0] = r[R_S1] & 0xFFu;
    for (;;) {
        uint32_t s0 = r[R_S0];
        uint32_t kind;

        r[R_V1] = rd(w, BASE_WORD, 4);
        r[R_V0] = ((s0 * 3u) * 8u - s0) << 4;       /* s0 * 0x170 */
        r[R_A0] = r[R_V0] + r[R_V1];
        r[R_V0] = rd(w, r[R_A0] + 0x7Cu, 2);
        r[R_AT] = 0x801E0000u;
        wr(w, SLOT_WORD, 4, r[R_A0]);
        r[R_V0] &= 0x8000u;
        if (r[R_V0] != 0)
            goto next_slot;

        /* 3A70: skill-learn scan for kinds outside 7..8 */
        kind = r[R_A0] = rd(w, r[R_A0] + 0x56u, 1);
        r[R_V0] = (int32_t)kind < 9;
        if (r[R_V0] == 0) {
            r[R_V0] = (int32_t)kind < 7;
        } else {
            r[R_V0] = (int32_t)kind < 7;
            if (r[R_V0] == 0)
                goto after_3BE0;
        }
        call(w, r, 0x801E3BE0u, 0x801E3A94u);
        r[R_A0] = r[R_V0];
        r[R_V0] = r[R_A0] & 0xFFu;
        if (r[R_V0] != 0) {
            r[R_V0] = rd(w, BASE_WORD, 4) + r[R_S0];
            wr(w, r[R_V0] + 0x101Cu, 1, r[R_A0]);
        }
after_3BE0:
        /* 3AB8: everything but kind 10 runs 3D54 */
#ifdef MUTANT_CACHE_KIND
        r[R_V0] = rd(w, SLOT_WORD, 4);
        r[R_A0] = kind;
#else
        r[R_V0] = rd(w, SLOT_WORD, 4);
        r[R_A0] = rd(w, r[R_V0] + 0x56u, 1);
#endif
        if (r[R_A0] == r[R_S2]) {
            r[R_V0] = (int32_t)r[R_A0] < 7;
            goto check_3E14;
        }
        r[R_V0] = (int32_t)r[R_A0] < 7;
        call(w, r, 0x801E3D54u, 0x801E3ADCu);
        r[R_A0] = r[R_V0];
        r[R_V0] = r[R_A0] & 0xFFu;
        r[R_V1] = r[R_S1] & 0xFFu;
        if (r[R_V0] != 0) {
            r[R_V0] = rd(w, BASE_WORD, 4) + r[R_V1];
            wr(w, r[R_V0] + 0x101Fu, 1, r[R_A0]);
        }
        r[R_V0] = rd(w, SLOT_WORD, 4);
        r[R_A0] = rd(w, r[R_V0] + 0x56u, 1);
        r[R_V0] = (int32_t)r[R_A0] < 7;
check_3E14:
        /* 3B18: 3E14 for kinds < 7, 9 and >= 11 */
        if (r[R_V0] != 0) {
            r[R_V0] = (int32_t)r[R_A0] < 9;
        } else {
            r[R_V0] = (int32_t)r[R_A0] < 9;
            if (r[R_V0] != 0 || r[R_A0] == r[R_S2])
                goto check_3F28;
        }
        call(w, r, 0x801E3E14u, 0x801E3B38u);
check_3F28:
        /* 3B38: 3F28 for kinds < 8 and >= 11 */
        r[R_V0] = rd(w, SLOT_WORD, 4);
        r[R_A0] = rd(w, r[R_V0] + 0x56u, 1);
        r[R_V0] = (int32_t)r[R_A0] < 11;
        if (r[R_V0] == 0) {
            r[R_V0] = (int32_t)r[R_A0] < 8;
        } else {
            r[R_V0] = (int32_t)r[R_A0] < 8;
            if (r[R_V0] == 0)
                goto check_3EA4;
        }
        call(w, r, 0x801E3F28u, 0x801E3B68u);
check_3EA4:
        /* 3B68: kind 8 -> 3EA4 (a0 is whatever is live) */
        r[R_V0] = rd(w, SLOT_WORD, 4);
        r[R_V1] = rd(w, r[R_V0] + 0x56u, 1);
        r[R_V0] = 8;
        if (r[R_V1] == r[R_V0])
            call(w, r, 0x801E3EA4u, 0x801E3B8Cu);
        /* 3B8C: kind 7 -> 3FB0 */
        r[R_V0] = rd(w, SLOT_WORD, 4);
        r[R_V1] = rd(w, r[R_V0] + 0x56u, 1);
        r[R_V0] = 7;
        if (r[R_V1] == r[R_V0])
            call(w, r, 0x801E3FB0u, 0x801E3BB0u);
next_slot:
        r[R_S1] += 1;
        r[R_V0] = r[R_S1] & 0xFFu;
        r[R_V0] = r[R_V0] < 3u;
        r[R_S0] = r[R_S1] & 0xFFu;
        if (r[R_V0] == 0)
            break;
    }
    r[R_RA] = rd(w, sp + 28u, 4);
    r[R_S2] = rd(w, sp + 24u, 4);
    r[R_S1] = rd(w, sp + 20u, 4);
    r[R_S0] = rd(w, sp + 16u, 4);
    r[R_SP] = sp + 32u;
}

/* ---- Cases ---------------------------------------------------------- */
static void shape(World *w, uint32_t *gpr, uint32_t seed)
{
    uint32_t s = seed * 2654435761u + 1u;

    memset(w->ram, 0, RAM_SIZE);
    memcpy(w->ram + (MODULE_ADDR & 0x1FFFFFFFu), g_module, MODULE_SIZE);
    for (uint32_t i = 0; i < 0x1200u; i += 4) {
        wr(w, BUF_A + i, 4, next(&s));
        wr(w, BUF_B + i, 4, next(&s));
    }
    for (unsigned k = 0; k < 3; k++) {
        uint32_t kind = (next(&s) & 1u) ? next(&s) % 13u : next(&s) & 0xFFu;
        wr(w, BUF_A + 0x170u * k + 0x56u, 1, kind);
        wr(w, BUF_B + 0x170u * k + 0x56u, 1, next(&s) % 13u);
        if (next(&s) % 4u != 0)   /* not dead */
            wr(w, BUF_A + 0x170u * k + 0x7Cu, 2, rd(w, BUF_A + 0x170u * k + 0x7Cu, 2) & 0x7FFFu);
        wr(w, BUF_B + 0x170u * k + 0x7Cu, 2, next(&s) & 0x7FFFu);
    }
    wr(w, BASE_WORD, 4, BUF_A);
    wr(w, SLOT_WORD, 4, next(&s));
    for (unsigned i = 0; i < 0x40u; i += 4)   /* stack contents below sp */
        wr(w, STACK_TOP - 0x40u + i, 4, next(&s));
    for (unsigned i = 0; i < 32; i++) gpr[i] = next(&s);
    gpr[0] = 0;
    gpr[R_SP] = STACK_TOP;
    gpr[R_RA] = HALT;
    w->calls = 0;
    w->rng = seed ^ 0xA5A5A5A5u;
    if (w->rng == 0) w->rng = 1;
}

int main(int argc, char **argv)
{
    unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 0) : 20000u;
    unsigned failures = 0;
    FILE *f;

    if (argc < 2) { fprintf(stderr, "usage: %s results_module.bin [cases]\n", argv[0]); return 2; }
    f = fopen(argv[1], "rb");
    if (f == NULL || fread(g_module, 1, MODULE_SIZE, f) != MODULE_SIZE) {
        fprintf(stderr, "RESULTS 3A18 FAIL cannot read %s\n", argv[1]);
        return 1;
    }
    fclose(f);

    for (uint32_t seed = 1; seed <= cases; seed++) {
        PcPortMipsBus bus = {&g_retail, bus_read, bus_write, bridge, NULL, NULL, NULL};
        PcPortMipsCpu cpu;
        uint32_t model_gpr[32];
        int rc;

        shape(&g_retail, model_gpr, seed);
        shape(&g_model, model_gpr, seed);
        for (unsigned k = 0; k < 3; k++) {
            uint32_t slot = BUF_A + 0x170u * k;
            if (rd(&g_retail, slot + 0x7Cu, 2) & 0x8000u) g_dead++;
            else g_kind_hist[rd(&g_retail, slot + 0x56u, 1) < 13 ? rd(&g_retail, slot + 0x56u, 1) : 15]++;
        }
        PcPortMipsCpuInit(&cpu, &bus);
        memcpy(cpu.gpr, model_gpr, sizeof(cpu.gpr));
        rc = PcPortMipsRun(&cpu, ENTRY, HALT, 100000);
        model_3A18(&g_model, model_gpr);
        model_gpr[0] = 0;

        if (rc != PC_PORT_MIPS_HALTED || g_retail.calls != g_model.calls ||
            memcmp(g_retail.log, g_model.log, sizeof(g_retail.log)) != 0 ||
            memcmp(cpu.gpr, model_gpr, sizeof(model_gpr)) != 0 ||
            memcmp(g_retail.ram, g_model.ram, RAM_SIZE) != 0) {
            if (failures++ < 8) {
                fprintf(stderr, "RESULTS 3A18 FAIL seed=%u rc=%d calls=%u/%u %s\n", seed, rc,
                        g_retail.calls, g_model.calls, rc ? cpu.error : "");
                for (unsigned i = 0; i < 32; i++)
                    if (cpu.gpr[i] != model_gpr[i])
                        fprintf(stderr, "  gpr%u retail=%08x model=%08x\n", i, cpu.gpr[i], model_gpr[i]);
                for (uint32_t i = 0; i < RAM_SIZE; i++)
                    if (g_retail.ram[i] != g_model.ram[i]) {
                        fprintf(stderr, "  first RAM diff at %08x\n", 0x80000000u + i);
                        break;
                    }
            }
            continue;
        }
        /* The guest frame, stated explicitly: s0,s1,s2,ra at sp-16..sp-4,
         * the 16-byte argument area below them untouched. */
        {
            uint32_t sp = STACK_TOP - 32u, check[32];
            shape(&g_model, check, seed);
            if (rd(&g_retail, sp + 16u, 4) != check[R_S0] || rd(&g_retail, sp + 20u, 4) != check[R_S1] ||
                rd(&g_retail, sp + 24u, 4) != check[R_S2] || rd(&g_retail, sp + 28u, 4) != HALT ||
                memcmp(g_retail.ram + ((sp) & 0x1FFFFFFFu), g_model.ram + ((sp) & 0x1FFFFFFFu), 16) != 0) {
                fprintf(stderr, "RESULTS 3A18 FAIL frame seed=%u\n", seed);
                failures++;
            }
        }
    }
    printf("RESULTS 3A18 coverage: calls 3BE0=%u 3D54=%u 3E14=%u 3F28=%u 3EA4=%u 3FB0=%u; "
           "mutations kind=%u slot=%u base=%u; dead slots=%u\n",
           g_cover[0], g_cover[1], g_cover[2], g_cover[3], g_cover[4], g_cover[5],
           g_mutations[0], g_mutations[1], g_mutations[2], g_dead);
    for (unsigned i = 0; i < 6; i++)
        if (g_cover[i] == 0) { fprintf(stderr, "RESULTS 3A18 FAIL helper %u never called\n", i); failures++; }
    if (failures) {
        fprintf(stderr, "RESULTS 3A18 FAIL %u/%u cases\n", failures, cases);
        return 1;
    }
    printf("RESULTS 3A18 CONTRACT PASS %u cases: call log, all GPRs, 2 MB RAM and guest frame "
           "identical to retail under scripted helpers\n", cases);
    return 0;
}

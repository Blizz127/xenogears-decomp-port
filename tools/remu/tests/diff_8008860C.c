/* Differential test: host-compiled src/battle/main40.c func_8008860C
 * vs the retail bytes (asm/battle/nonmatchings/main40/func_8008860C.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8008860C(): two-phase board setup off the D_800D2DB4 base. Phase 1
 * accumulates func_80076A10() results into base[0x5D74] (selectors
 * D_800C33B0[0..1], +0x1720), copies D_800CCB34 to base[0x5D83], stores a
 * func_80076A10() result to base[0x5D70] (+0x0, selector 0xA8) and
 * D_800CCB34 to base[0x5D92], then runs func_80076B68() over
 * base[0x5D74]/base[0x5D70] entries (+0x1720). Phase 2 repeats the shape
 * for base[0x5D7E] (selectors D_800C33B0[2..3], +0x2530, D_800CCB34 to
 * base[0x5D8D]) with func_80076BF0() over base[0x5D7E] entries (+0x2530).
 *
 * Callee control: func_80076A10/80076B68/80076BF0 live in other battle
 * TUs (80076A10 is still asm; 80076B68/80076BF0 write through their
 * pointer arg), so all three are stubbed on BOTH sides (remu forces
 * v0=0 for unloaded jal targets; the host stubs return 0 / ignore args).
 * The test asserts per-callee stub counts match host-vs-retail exactly
 * and every stub target is one of the three known callees. No
 * GTE/cop1/HW regs.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8008860C src/battle/main40.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main40.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8008860C(void);

/* TU externs the test must provide (only what func_8008860C touches). */
u8 c33[8];
__asm__(".globl D_800C33B0\n.set D_800C33B0, c33");
u8 ccb[4];
__asm__(".globl D_800CCB34\n.set D_800CCB34, ccb");
#define MEM_LEN 0xC000u
static u8 mem[MEM_LEN];
u8 *D_800D2DB4;

/* Dual-side stubs (always 0 / no effect), with host call counters. */
static int h_6A10, h_6B68, h_6BF0;
u32 func_80076A10(u8 sel, u8 *p, u32 m, u32 n) {
    (void)sel; (void)p; (void)m; (void)n;
    h_6A10++;
    return 0;
}
void func_80076B68(u8 *p) { (void)p; h_6B68++; }
void func_80076BF0(u8 *p) { (void)p; h_6BF0++; }

#define RETAIL_S "asm/battle/nonmatchings/main40/func_8008860C.s"
#define C33_BASE 0x800C33B0u
#define CCB_BASE 0x800CCB34u
#define D2DB4_ADDR 0x800D2DB4u
#define MEM_BASE 0x801F0000u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, int id) {
    uint32_t x = seed ^ 0x88600860u;
    u8 h_c33[4], h_ccb;
    static u8 h_mem[MEM_LEN];
    uint32_t rbase = MEM_BASE;

    for (int k = 0; k < 4; k++) h_c33[k] = (u8)(lcg(&x) >> 16);
    h_ccb = (u8)(lcg(&x) >> 16);
    for (uint32_t k = 0; k < MEM_LEN; k++) h_mem[k] = (u8)(lcg(&x) >> 16);
    /* pin the loop counts (id-selected small values, occasional wide). */
    h_mem[0x5D74] = (u8)((id % 11 == 10) ? (lcg(&x) & 0xFFu) : (id % 6));
    h_mem[0x5D70] = (u8)((id % 13 == 12) ? (lcg(&x) & 0xFFu) : (id % 8));
    h_mem[0x5D7E] = (u8)((id % 7 == 6) ? (lcg(&x) & 0xFFu) : (id % 5));

    /* host side */
    memcpy(c33, h_c33, 4);
    ccb[0] = h_ccb;
    memcpy(mem, h_mem, MEM_LEN);
    D_800D2DB4 = mem;
    h_6A10 = h_6B68 = h_6BF0 = 0;
    func_8008860C();
    int w_6A10 = h_6A10, w_6B68 = h_6B68, w_6BF0 = h_6BF0;

    /* retail side */
    if (remu_poke(m, C33_BASE, h_c33, 4)) { printf("FAIL [%d] poke\n", id); return 0; }
    if (remu_poke(m, CCB_BASE, &h_ccb, 1)) { printf("FAIL [%d] poke\n", id); return 0; }
    if (remu_poke(m, D2DB4_ADDR, &rbase, 4)) { printf("FAIL [%d] poke\n", id); return 0; }
    if (remu_poke(m, MEM_BASE, h_mem, MEM_LEN)) { printf("FAIL [%d] poke\n", id); return 0; }
    int rc = remu_call(m, entry, 0, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL [%d] seed=%08X remu rc=%d stubs=%d%s\n", id, seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    /* stub discipline: same per-callee counts, known targets only */
    int r_6A10 = 0, r_6B68 = 0, r_6BF0 = 0;
    const char *log = remu_stub_log(m);
    {
        char tmp[4096];
        snprintf(tmp, sizeof(tmp), "%s", log);
        for (char *t = strtok(tmp, " "); t; t = strtok(NULL, " ")) {
            if (strcmp(t, "80076A10") == 0) r_6A10++;
            else if (strcmp(t, "80076B68") == 0) r_6B68++;
            else if (strcmp(t, "80076BF0") == 0) r_6BF0++;
            else {
                printf("FAIL [%d] unknown stub target %s\n", id, t);
                return 0;
            }
        }
    }
    if (r_6A10 != w_6A10 || r_6B68 != w_6B68 || r_6BF0 != w_6BF0) {
        printf("FAIL [%d] stub count retail=(%d,%d,%d) host=(%d,%d,%d)%s\n",
               id, r_6A10, r_6B68, r_6BF0, w_6A10, w_6B68, w_6BF0, log);
        return 0;
    }
    /* memory effects */
    static u8 r_mem[MEM_LEN];
    u8 r_c33[4], r_ccb;
    uint32_t r_ptr;
    if (remu_peek(m, MEM_BASE, r_mem, MEM_LEN)) { printf("FAIL [%d] peek\n", id); return 0; }
    if (remu_peek(m, C33_BASE, r_c33, 4)) { printf("FAIL [%d] peek\n", id); return 0; }
    if (remu_peek(m, CCB_BASE, &r_ccb, 1)) { printf("FAIL [%d] peek\n", id); return 0; }
    if (remu_peek(m, D2DB4_ADDR, &r_ptr, 4)) { printf("FAIL [%d] peek\n", id); return 0; }
    if (memcmp(r_mem, mem, MEM_LEN) != 0) {
        printf("FAIL [%d] seed=%08X mem mismatch\n", id, seed);
        for (uint32_t k = 0; k < MEM_LEN; k++)
            if (r_mem[k] != mem[k]) {
                printf("  off %05X retail=%02X host=%02X\n", k, r_mem[k], mem[k]);
                break;
            }
        return 0;
    }
    if (memcmp(r_c33, c33, 4) != 0 || r_ccb != ccb[0] || r_ptr != rbase) {
        printf("FAIL [%d] scalar global mismatch\n", id);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8008860Cu) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu,
                         0x80000000u, 0xDEADBEEFu };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i], n)) return 1;
        n++;
    }
    uint32_t s = 0x88600860u;
    for (int i = 0; i < 120; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s, n)) return 1;
        n++;
    }
    printf("DIFF 8008860C OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

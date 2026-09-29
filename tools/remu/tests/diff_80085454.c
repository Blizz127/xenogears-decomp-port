/* Differential test: host-compiled src/battle/main36.c func_80085454
 * vs the retail bytes (asm/battle/nonmatchings/main36/func_80085454.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80085454(idx): copy D_800D2C54 (u16 every 4 bytes) and D_800D2C88
 * (bytes) into the D_800C3FE8 table picked by (idx & 0xFF) * 72, folding
 * each D_800D2D70 halfword with its source halfword per the
 * (D_800D2C88, D_800D2D5C) byte pair over 11 passes (0x67 - 0x5C).
 * Table, D_800D2D70 area and the 11 used D_800D2D5C bytes are compared
 * host-vs-retail byte for byte.
 *
 * D_800D2D67 is the loop-bound address (D_800D2D5C + 11). The two
 * symbols are defined adjacently here and the layout is asserted at
 * startup; a layout break fails the test instead of passing silently.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085454 src/battle/main36.c -fno-data-sections
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main36.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_80085454(s32 arg0);

/* TU externs the test must provide. D_800D2D67 must immediately follow
 * D_800D2D5C (verified in main); alignment via u32 backing. */
__attribute__((aligned(4))) u8 D_800C3FE8[20480];
__attribute__((aligned(4))) u8 D_800D2C54[80];
u8 D_800D2C88[32];
u16 D_800D2D70[20];
/* Loop bound D_800D2D67 must immediately follow the 11 used bytes of
 * D_800D2D5C (0x67 - 0x5C = 11). Kept as adjacent same-alignment
 * definitions and verified by the startup assert below; a layout break
 * fails the test instead of passing silently. (An assembler .set alias
 * was tried first, but clang folds it to an absolute address in the
 * same TU and miscompiles the check. A shared explicit section is used
 * because -fdata-sections would otherwise let the linker reorder the
 * per-symbol sections.) */
/* Loop bound D_800D2D67 must immediately follow the 11 used bytes of
 * D_800D2D5C. This needs plain-.bss declaration order, so build with an
 * extra -fno-data-sections (see build line below); the startup assert
 * verifies the layout and fails safe if a toolchain ever reorders it. */
u8 D_800D2D5C[11];
u8 D_800D2D67;

#define RETAIL_S "asm/battle/nonmatchings/main36/func_80085454.s"
#define TAB_BASE 0x800C3FE8u
#define TAB_LEN 20480u
#define S54_BASE 0x800D2C54u
#define S54_LEN 80u
#define S88_BASE 0x800D2C88u
#define S88_LEN 32u
#define A2_BASE 0x800D2D70u
#define A2_LEN 40u
#define A3_BASE 0x800D2D5Cu
#define A3_LEN 11u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t idx) {
    uint32_t x = seed ^ 0x54545454u;
    static u8 tab[TAB_LEN];
    static u8 s54[S54_LEN], s88[S88_LEN], a2[A2_LEN], a3[A3_LEN];
    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < S54_LEN; j++)
        s54[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < S88_LEN; j++)
        s88[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < A2_LEN; j++)
        a2[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < A3_LEN; j++)
        a3[j] = (u8)(lcg(&x) >> 16);
    /* pin control bytes per seed class so every dispatch arm is hit,
     * including both signs of the |T-A| difference (the mutant drops
     * the negation, which only shows when d < 0). */
    switch (seed & 7u) {
    case 0: memset(s88, 2, A3_LEN); memset(a3, 2, A3_LEN); break;
    case 1: memset(s88, 0, A3_LEN); memset(a3, 2, A3_LEN);
        memset(s54, 0, S54_LEN); memset(a2, 1, A2_LEN); break;
    case 2: memset(s88, 5, A3_LEN); memset(a3, 2, A3_LEN);
        memset(s54, 0x7F, S54_LEN); memset(a2, 0xFF, A2_LEN); break;
    case 3: memset(s88, 1, A3_LEN); break;
    case 4: memset(s88, 7, A3_LEN); break;
    case 5: memset(s88, 2, A3_LEN); memset(a3, 0, A3_LEN);
        memset(s54, 0, S54_LEN); memset(a2, 1, A2_LEN); break;
    case 6: memset(s88, 2, A3_LEN); memset(a3, 5, A3_LEN);
        memset(s54, 0x7F, S54_LEN); memset(a2, 0xFF, A2_LEN); break;
    default: break;
    }

    /* host side */
    memcpy(D_800C3FE8, tab, TAB_LEN);
    memcpy(D_800D2C54, s54, S54_LEN);
    memcpy(D_800D2C88, s88, S88_LEN);
    memcpy(D_800D2D70, a2, A2_LEN);
    memcpy(D_800D2D5C, a3, A3_LEN);
    func_80085454((s32)idx);

    /* retail side */
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, S54_BASE, s54, S54_LEN)) { printf("FAIL poke s54\n"); return 0; }
    if (remu_poke(m, S88_BASE, s88, S88_LEN)) { printf("FAIL poke s88\n"); return 0; }
    if (remu_poke(m, A2_BASE, a2, A2_LEN)) { printf("FAIL poke a2\n"); return 0; }
    if (remu_poke(m, A3_BASE, a3, A3_LEN)) { printf("FAIL poke a3\n"); return 0; }
    int rc = remu_call(m, entry, idx, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%08X unexpected stub calls%s\n", seed,
               remu_stub_log(m));
        return 0;
    }
    static u8 rtab[TAB_LEN], ra2[A2_LEN], ra3[A3_LEN];
    if (remu_peek(m, TAB_BASE, rtab, TAB_LEN)) { printf("FAIL peek\n"); return 0; }
    if (remu_peek(m, A2_BASE, ra2, A2_LEN)) { printf("FAIL peek\n"); return 0; }
    if (remu_peek(m, A3_BASE, ra3, A3_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(rtab, D_800C3FE8, TAB_LEN) != 0) {
        printf("FAIL seed=%08X idx=%u table mismatch\n", seed, idx);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (rtab[k] != D_800C3FE8[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, rtab[k], D_800C3FE8[k]);
                break;
            }
        return 0;
    }
    if (memcmp(ra2, D_800D2D70, A2_LEN) != 0) {
        printf("FAIL seed=%08X idx=%u a2 mismatch\n", seed, idx);
        return 0;
    }
    if (memcmp(ra3, D_800D2D5C, A3_LEN) != 0) {
        printf("FAIL seed=%08X idx=%u a3 mismatch\n", seed, idx);
        return 0;
    }
    return 1;
}

int main(void) {
    /* Load-bearing diagnostic: materializing both addresses here makes
     * clang emit D_800D2D5C before D_800D2D67 in .bss (verified 3/3 at
     * O0/O2/UBSan; without it the order flips). Do not remove: the
     * assert below fails safe if any toolchain reorders them. */
    printf("layout 5C=%p 67=%p\n", (void*)D_800D2D5C, (void*)&D_800D2D67);
    if (&D_800D2D67 != &D_800D2D5C[11]) {
        printf("FAIL host layout: D_800D2D67 != D_800D2D5C+11\n");
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085454u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t idxs[] = { 0, 1, 2, 3, 5, 100, 255, 0xFFFFFFFFu };
    uint32_t s = 0x54541234u;
    for (unsigned i = 0; i < sizeof(idxs) / sizeof(idxs[0]); i++) {
        for (int k = 0; k < 30; k++) {
            s = s * 1103515245u + 12345u;
            if (!check_one(m, entry, s, idxs[i])) return 1;
            n++;
        }
    }
    printf("DIFF 80085454 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

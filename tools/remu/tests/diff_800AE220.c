/* Differential test: host-compiled src/battle/main72.c func_800AE220
 * vs the retail bytes (asm/battle/nonmatchings/main72/func_800AE220.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800AE220(a0, a1): select a node by a1 (0 -> D_8005919C,
 * 1 -> *(a0+0xB0)+8 chain, 2 -> *(a0+0xB4)+8 chain, 3 -> D_800C4924)
 * and return *(u16*)(node+0x14) << 16; any other a1 returns 3.
 * Pointer slots hold native-width pointers on each side (4 bytes
 * retail, 8 bytes host) and are never compared; only the u16-derived
 * return value is checked, with identical graph contents on both sides.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800AE220 src/battle/main72.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main72.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u32 func_800AE220(u8 *a0, u32 a1);

/* TU externs the test must provide (D_ = retail address convention). */
u8 *D_8005919C;
u8 *D_800C4924;

#define RETAIL_S "asm/battle/nonmatchings/main72/func_800AE220.s"
#define A0 0x801F0200u
#define N1 0x801F0300u
#define L1 0x801F0340u
#define N2 0x801F0380u
#define L2 0x801F03C0u
#define G0 0x801F0400u
#define G3 0x801F0440u
#define P_G0 0x8005919Cu
#define P_G3 0x800C4924u

/* 8-aligned so native-width pointer slots are defined under UBSan. */
static uint64_t h_a0[32];
static uint64_t h_n1[8];
static uint64_t h_l1[8];
static uint64_t h_n2[8];
static uint64_t h_l2[8];
static uint64_t h_g0[8];
static uint64_t h_g3[8];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a1) {
    uint32_t x = seed ^ 0xAE22A1u;
    uint32_t h3 = lcg(&x), h4 = lcg(&x);
    uint16_t hN1 = (uint16_t)(h3 & 0xFFFFu);
    uint16_t hN2 = (uint16_t)((h3 >> 16) & 0xFFFFu);
    uint16_t hG0 = (uint16_t)(h4 & 0xFFFFu);
    uint16_t hG3 = (uint16_t)((h4 >> 16) & 0xFFFFu);
    uint32_t want;
    switch (a1) {
    case 0: want = (uint32_t)hG0 << 16; break;
    case 1: want = (uint32_t)hN1 << 16; break;
    case 2: want = (uint32_t)hN2 << 16; break;
    case 3: want = (uint32_t)hG3 << 16; break;
    default: want = 3u; break;
    }

    /* host graph: the body loads node slots as u32 (retail 4-byte
     * pointers), so store the low 32 bits, matching what retail reads.
     * Statics in this non-PIE binary live below 4GB, so zero-extension
     * in the body recovers the full host address. */
    memset(h_a0, 0, sizeof(h_a0));
    *(uint32_t *)((u8 *)h_a0 + 0xB0) = (uint32_t)(uintptr_t)h_n1;
    *(uint32_t *)((u8 *)h_a0 + 0xB4) = (uint32_t)(uintptr_t)h_n2;
    *(uint32_t *)((u8 *)h_n1 + 8) = (uint32_t)(uintptr_t)h_l1;
    *(uint32_t *)((u8 *)h_n2 + 8) = (uint32_t)(uintptr_t)h_l2;
    *(u8 **)((u8 *)h_n1 + 8) = (u8 *)h_l1;
    *(u8 **)((u8 *)h_n2 + 8) = (u8 *)h_l2;
    *(uint16_t *)((u8 *)h_l1 + 0x14) = hN1;
    *(uint16_t *)((u8 *)h_l2 + 0x14) = hN2;
    *(uint16_t *)((u8 *)h_g0 + 0x14) = hG0;
    *(uint16_t *)((u8 *)h_g3 + 0x14) = hG3;
    D_8005919C = (u8 *)h_g0;
    D_800C4924 = (u8 *)h_g3;
    u32 got = func_800AE220((u8 *)h_a0, a1);
    if (got != want) {
        printf("FAIL seed=%08X a1=%u host=%08X want=%08X\n", seed, a1, got, want);
        return 0;
    }

    /* retail graph (4-byte pointers) */
    uint32_t v;
    u8 zero[32];
    memset(zero, 0, sizeof(zero));
    if (remu_poke(m, A0, zero, 32)) { printf("FAIL poke a0\n"); return 0; }
    v = N1; if (remu_poke(m, A0 + 0xB0, &v, 4)) { printf("FAIL poke\n"); return 0; }
    v = N2; if (remu_poke(m, A0 + 0xB4, &v, 4)) { printf("FAIL poke\n"); return 0; }
    v = L1; if (remu_poke(m, N1 + 8, &v, 4)) { printf("FAIL poke\n"); return 0; }
    v = L2; if (remu_poke(m, N2 + 8, &v, 4)) { printf("FAIL poke\n"); return 0; }
    v = hN1; if (remu_poke(m, L1 + 0x14, &v, 2)) { printf("FAIL poke\n"); return 0; }
    v = hN2; if (remu_poke(m, L2 + 0x14, &v, 2)) { printf("FAIL poke\n"); return 0; }
    v = hG0; if (remu_poke(m, G0 + 0x14, &v, 2)) { printf("FAIL poke\n"); return 0; }
    v = hG3; if (remu_poke(m, G3 + 0x14, &v, 2)) { printf("FAIL poke\n"); return 0; }
    v = G0; if (remu_poke(m, P_G0, &v, 4)) { printf("FAIL poke P_G0\n"); return 0; }
    v = G3; if (remu_poke(m, P_G3, &v, 4)) { printf("FAIL poke P_G3\n"); return 0; }

    int rc = remu_call(m, entry, A0, a1, 0, 0);
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
    if (remu_get_reg(m, 2) != want) {
        printf("FAIL seed=%08X a1=%u retail=%08X want=%08X\n", seed, a1,
               remu_get_reg(m, 2), want);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800AE220u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t a1s[] = { 0, 1, 2, 3, 4, 5, 0xFFFFFFFFu, 0x80000000u, 6, 100 };
    uint32_t s = 0xE2201234u;
    for (unsigned i = 0; i < sizeof(a1s) / sizeof(a1s[0]); i++) {
        for (int k = 0; k < 25; k++) {
            s = s * 1103515245u + 12345u;
            if (!check_one(m, entry, s, a1s[i])) return 1;
            n++;
        }
    }
    printf("DIFF 800AE220 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

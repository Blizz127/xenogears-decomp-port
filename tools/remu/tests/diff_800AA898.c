/* Differential test: host-compiled src/battle/main72.c func_800AA898
 * vs the retail bytes (asm/battle/nonmatchings/main72/func_800AA898.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800AA898(p, a1, a2, a3): init struct at p (a1 unused, words from
 * a2/a3, marker bytes, zeroed tables, -1 slots); retail v0 is 1.
 * The whole 256-byte struct area is compared host-vs-retail byte for
 * byte, plus v0. Buffers are word-aligned so host u32/u16 stores are
 * defined under UBSan.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800AA898 src/battle/main72.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main72.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u32 func_800AA898(u8 *p, u32 a1, u32 a2, u32 a3);

#define RETAIL_S "asm/battle/nonmatchings/main72/func_800AA898.s"
#define BASE 0x801F0200u
#define LEN 256u

/* Word-aligned on both sides (BASE is word-aligned too). */
static u32 h_buf[LEN / 4];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    uint32_t x = seed ^ 0xA898A898u;
    u8 init[LEN];
    for (uint32_t j = 0; j < LEN; j++)
        init[j] = (u8)(lcg(&x) >> 16);
    uint32_t a1 = lcg(&x), a2 = lcg(&x), a3 = lcg(&x);

    /* host side */
    memcpy(h_buf, init, LEN);
    u32 got = func_800AA898((u8 *)h_buf, a1, a2, a3);

    /* retail side */
    if (remu_poke(m, BASE, init, LEN)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, BASE, a1, a2, a3);
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
    if (remu_get_reg(m, 2) != 1u) {
        printf("FAIL seed=%08X retail v0=%08X want=1\n", seed, remu_get_reg(m, 2));
        return 0;
    }
    if (got != 1u) {
        printf("FAIL seed=%08X host ret=%08X want=1\n", seed, got);
        return 0;
    }
    static u8 rbuf[LEN];
    if (remu_peek(m, BASE, rbuf, LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(rbuf, h_buf, LEN) != 0) {
        printf("FAIL seed=%08X struct mismatch\n", seed);
        for (uint32_t k = 0; k < LEN; k++)
            if (rbuf[k] != ((u8 *)h_buf)[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, rbuf[k], ((u8 *)h_buf)[k]);
                break;
            }
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800AA898u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu,
                         0x80000000u, 0x12345678u };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, 1u << b)) return 1;
        n++;
    }
    uint32_t s = 0xA8981234u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 800AA898 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

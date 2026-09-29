/* Differential test: host-compiled src/battle/main14.c func_8007A7BC
 * vs the retail bytes (asm/battle/nonmatchings/main14/func_8007A7BC.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8007A7BC(ppSrc, dst, a2, a3): d = dst + 8*(a3 & 0xFF);
 * d[0] = 0x80, d[1..4] = (*ppSrc)[0..3] (src pointer is word-loaded);
 * returns (a3 + 1) & 0xFF. a2 is unused.
 *
 * Retail scratch lives inside the D_800CCD64/D_800CCD6C spans (already
 * proven pokable by diff_8007A280): the pointer cell + src bytes in the
 * CCD64 span, the dst window in the CCD6C span.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007A7BC src/battle/main14.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
u8 func_8007A7BC(u8** a0, u8* a1, u8 a2, u8 a3);

/* TU externs the test must provide (only linked if reachable; the tested
 * body uses none, scratch is addressed numerically). */

#define RETAIL_S "asm/battle/nonmatchings/main14/func_8007A7BC.s"
#define CELL_BASE 0x800CCD64u
#define CELL_OFF 0x100u
#define SRC_OFF 0x200u
#define MIRR_LEN 0x300u
#define DST_BASE 0x800CCD6Cu
#define DST_OFF 0x1000u
#define DST_LEN 2110u /* 8*255 + 5 max touch, plus slack */

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 hmirr[MIRR_LEN];
static u8 hdst[DST_LEN];

static int check_one(remu_t *m, uint32_t entry, uint32_t seed,
                     uint32_t a2, uint32_t a3) {
    uint32_t x = seed ^ 0x7A7BC79u;
    static u8 mirr[MIRR_LEN];
    static u8 dst[DST_LEN];
    static u8 expect[DST_LEN];
    static u8 back[DST_LEN];
    for (uint32_t j = 0; j < MIRR_LEN; j++)
        mirr[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < DST_LEN; j++)
        dst[j] = (u8)(lcg(&x) >> 16);

    /* pointer cell -> SRC (little-endian word load on retail) */
    uint32_t src_addr = CELL_BASE + SRC_OFF;
    mirr[CELL_OFF] = (u8)(src_addr & 0xFFu);
    mirr[CELL_OFF + 1] = (u8)((src_addr >> 8) & 0xFFu);
    mirr[CELL_OFF + 2] = (u8)((src_addr >> 16) & 0xFFu);
    mirr[CELL_OFF + 3] = (u8)((src_addr >> 24) & 0xFFu);

    memcpy(hmirr, mirr, MIRR_LEN);
    memcpy(hdst, dst, DST_LEN);
    if (remu_poke(m, CELL_BASE, mirr, MIRR_LEN)) { printf("FAIL poke mirr\n"); return 0; }
    if (remu_poke(m, DST_BASE + DST_OFF, dst, DST_LEN)) { printf("FAIL poke dst\n"); return 0; }

    uint32_t d = ((a3 & 0xFFu) << 3);
    memcpy(expect, dst, DST_LEN);
    expect[d] = 0x80;
    expect[d + 1] = mirr[SRC_OFF];
    expect[d + 2] = mirr[SRC_OFF + 1];
    expect[d + 3] = mirr[SRC_OFF + 2];
    expect[d + 4] = mirr[SRC_OFF + 3];
    uint32_t want_ret = (a3 + 1u) & 0xFFu;

    /* host side: hdst mirrors the retail dst window, so the same
     * window-relative offset d applies */
    u8 *hptr = hmirr + SRC_OFF;
    u8 hret = func_8007A7BC(&hptr, hdst, (u8)a2, (u8)a3);
    if (hret != (u8)want_ret) {
        printf("FAIL seed=%08X host ret=%02X want=%08X\n", seed, hret, want_ret);
        return 0;
    }

    int rc = remu_call(m, entry, CELL_BASE + CELL_OFF, DST_BASE + DST_OFF, a2, a3);
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
    uint32_t rv0 = remu_get_reg(m, 2);
    /* host return is compared via its own dst write + explicit re-call below;
     * retail v0 must equal (a3+1)&0xFF */
    if ((rv0 & 0xFFu) != want_ret) {
        printf("FAIL seed=%08X retail ret=%08X want=%08X\n", seed, rv0, want_ret);
        return 0;
    }
    if (remu_peek(m, DST_BASE + DST_OFF, back, DST_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(back, expect, DST_LEN) != 0) {
        printf("FAIL seed=%08X retail dst mismatch\n", seed);
        for (uint32_t k = 0; k < DST_LEN; k++)
            if (back[k] != expect[k]) {
                printf("  off %u retail=%02X want=%02X\n", k, back[k], expect[k]);
                break;
            }
        return 0;
    }
    /* host dst mirror must show the identical 5-byte change */
    if (memcmp(hdst, expect, DST_LEN) != 0) {
        printf("FAIL seed=%08X host dst mismatch\n", seed);
        for (uint32_t k = 0; k < DST_LEN; k++)
            if (hdst[k] != expect[k]) {
                printf("  off %u host=%02X want=%02X\n", k, hdst[k], expect[k]);
                break;
            }
        return 0;
    }
    /* src bytes are load-only on both sides */
    if (memcmp(hmirr + SRC_OFF, mirr + SRC_OFF, 4) != 0) {
        printf("FAIL seed=%08X host src store\n", seed);
        return 0;
    }
    static u8 srcback[8];
    if (remu_peek(m, CELL_BASE + SRC_OFF, srcback, 4)) { printf("FAIL peek src\n"); return 0; }
    if (memcmp(srcback, mirr + SRC_OFF, 4) != 0) {
        printf("FAIL seed=%08X retail src store\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8007A7BCu) { printf("FAIL load entry=%08X\n", entry); return 1; }
    uint32_t s = 0x7A7BC234u;
    for (int i = 0; i < 256; i++) {
        s = s * 1103515245u + 12345u;
        uint32_t seed = s;
        s = s * 1103515245u + 12345u;
        /* full-word a3 exercises the &0xFF mask; a2 is ignored */
        uint32_t a3 = (i < 128) ? (s & 0xFFu) : s;
        uint32_t a2 = s >> 16;
        if (!check_one(m, entry, seed, a2, a3))
            return 1;
        /* a2-independence: same seed with flipped a2 must behave identically */
        if ((i & 7) == 0 && !check_one(m, entry, seed ^ 0x5A5A5A5Au, a2 ^ 0xFFFFu, a3))
            return 1;
    }
    printf("DIFF 8007A7BC OK (256 seeds, retail-exact)\n");
    return 0;
}

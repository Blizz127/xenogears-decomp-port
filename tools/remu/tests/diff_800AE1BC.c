/* Differential test: host-compiled src/battle/main72.c func_800AE1BC
 * vs the retail bytes (asm/battle/nonmatchings/main72/func_800AE1BC.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800AE1BC(a0, a1, a2): if *(u16*)(a1+0x12)==0, store -1 at
 * a0+0x98 and return -1; else init a0+0x98..0xA4 (with a1+*(a1+0x14)
 * stored at +0xA0/+0xA4) and return that sum.
 * The returned/stored sum embeds the a1 base, which differs between
 * host and retail: each side's sum is checked against its own base +
 * the loaded offset word, and the struct area is compared with the two
 * embedded-pointer slots masked out.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800AE1BC src/battle/main72.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main72.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u32 func_800AE1BC(u8 *a0, u8 *a1, u32 a2);

#define RETAIL_S "asm/battle/nonmatchings/main72/func_800AE1BC.s"
#define DST 0x801F0200u
#define SRC 0x801F0400u
#define DST_LEN 256u
#define SRC_LEN 64u

/* Word-aligned on both sides (DST/SRC are word-aligned too). */
static u32 h_dst[DST_LEN / 4];
static u32 h_src[SRC_LEN / 4];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    uint32_t x = seed ^ 0xAE1BC1BCu;
    u8 dst[DST_LEN], src[SRC_LEN];
    for (uint32_t j = 0; j < DST_LEN; j++)
        dst[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < SRC_LEN; j++)
        src[j] = (u8)(lcg(&x) >> 16);
    /* force all four control combinations over the sweep */
    switch (seed & 3u) {
    case 0: src[0x12] = 0; src[0x13] = 0; break;
    case 1: src[0x12] = 1; break;
    default: break;
    }
    uint32_t a2 = (seed & 4u) ? lcg(&x) | 1u : 0u;
    uint32_t off = ((uint32_t)src[0x14]) | ((uint32_t)src[0x15] << 8) |
                   ((uint32_t)src[0x16] << 16) | ((uint32_t)src[0x17] << 24);

    /* host side */
    memcpy(h_dst, dst, DST_LEN);
    memcpy(h_src, src, SRC_LEN);
    u32 hret = func_800AE1BC((u8 *)h_dst, (u8 *)h_src, a2);
    uint32_t hbase = (uint32_t)(uintptr_t)h_src;

    /* retail side */
    if (remu_poke(m, DST, dst, DST_LEN)) { printf("FAIL poke dst\n"); return 0; }
    if (remu_poke(m, SRC, src, SRC_LEN)) { printf("FAIL poke src\n"); return 0; }
    int rc = remu_call(m, entry, DST, SRC, a2, 0);
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
    uint32_t rret = remu_get_reg(m, 2);

    /* return-value check, base-normalized */
    if ((src[0x12] | src[0x13]) == 0) {
        if (hret != 0xFFFFFFFFu || rret != 0xFFFFFFFFu) {
            printf("FAIL seed=%08X null-path ret host=%08X retail=%08X\n",
                   seed, hret, rret);
            return 0;
        }
    } else {
        if (hret != hbase + off) {
            printf("FAIL seed=%08X host ret=%08X want=%08X\n", seed, hret,
                   hbase + off);
            return 0;
        }
        if (rret != SRC + off) {
            printf("FAIL seed=%08X retail ret=%08X want=%08X\n", seed, rret,
                   SRC + off);
            return 0;
        }
    }

    /* struct area: mask the two embedded-pointer slots, then compare */
    static u8 rdst[DST_LEN];
    if (remu_peek(m, DST, rdst, DST_LEN)) { printf("FAIL peek\n"); return 0; }
    u8 hbytes[DST_LEN];
    memcpy(hbytes, h_dst, DST_LEN);
    if ((src[0x12] | src[0x13]) != 0) {
        /* +0xA0/+0xA4 hold base+off on each side; verify then mask */
        uint32_t hw0, hw1, rw0, rw1;
        memcpy(&hw0, hbytes + 0xA0, 4);
        memcpy(&hw1, hbytes + 0xA4, 4);
        memcpy(&rw0, rdst + 0xA0, 4);
        memcpy(&rw1, rdst + 0xA4, 4);
        if (hw0 != hbase + off || hw1 != hbase + off ||
            rw0 != SRC + off || rw1 != SRC + off) {
            printf("FAIL seed=%08X ptr slots host=%08X/%08X retail=%08X/%08X\n",
                   seed, hw0, hw1, rw0, rw1);
            return 0;
        }
        memset(hbytes + 0xA0, 0, 8);
        memset(rdst + 0xA0, 0, 8);
    }
    if (memcmp(rdst, hbytes, DST_LEN) != 0) {
        printf("FAIL seed=%08X struct mismatch\n", seed);
        for (uint32_t k = 0; k < DST_LEN; k++)
            if (rdst[k] != hbytes[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, rdst[k], hbytes[k]);
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
    if (entry != 0x800AE1BCu) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0x00000002u, 0x00000003u,
                         0x00000004u, 0x00000005u, 0x00000006u, 0x00000007u,
                         0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u, 0x12345678u };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, 1u << b)) return 1;
        n++;
    }
    uint32_t s = 0xE1BC1234u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 800AE1BC OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

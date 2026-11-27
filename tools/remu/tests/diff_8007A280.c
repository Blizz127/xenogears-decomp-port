/* Differential test: host-compiled src/battle/main14.c func_8007A280
 * vs the retail bytes (read from the caller's own battle.bin)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8007A280(a0, a1, a2, a3): jump-table dispatch on (a1 & 0xFF)
 * over 24 D_800CCxx bases; the halfword at base + 368*(a0 & 0xFF) is
 * returned when (a3 & 0xFF) != 0, else overwritten with (u16)a2.
 *
 * Coverage: every idx 0..23 per seed (the table order is identity, read
 * from asm/battle/data/0.rodata.s jtbl_8006FBDC). idx >= 0x18 is
 * SKIPPED: retail jumps through a wild v1 there. On the store path
 * (a3 == 0) retail returns the entry residue of $t0, which no C can
 * express, so only the store effect is compared (call discarded).
 * Retail bases overlap (1KB apart, 94KB reach each), so one random span
 * covers all of them on both sides; each host base mirrors its retail
 * slice per call.
 *
 * Build (from repo root):
 *   # Supply disc/battle.bin or set XENO_BATTLE_FILE to your own extraction.
 *   tools/remu/run_diff.sh diff_8007A280 src/battle/main14.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main14.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u16 func_8007A280(u8 a0, u8 a1, u16 a2, u8 a3);

#define JTBL_BASE 0x8006FBDCu
#define JTBL_LEN 96u
#define MIN_BASE 0x800CCD1Cu
#define SPAN_OFF 0xF6u /* max base - min base: 0x800CCE12 - 0x800CCD1C */
#define REACH (368u * 255u + 2u) /* +1 tail byte: the u16 load at row 255
 * reads off+1, which must stay inside the host mirror array */
#define SPAN_LEN (SPAN_OFF + REACH)

/* Per-base retail addresses in jtbl order (identity). */
static const uint32_t rbase[24] = {
    0x800CCD36u, 0x800CCD34u, 0x800CCD64u, 0x800CCD66u,
    0x800CCD68u, 0x800CCD6Au, 0x800CCD6Cu, 0x800CCD6Eu,
    0x800CCD70u, 0x800CCD72u, 0x800CCD74u, 0x800CCD76u,
    0x800CCDF8u, 0x800CCDFCu, 0x800CCDFEu, 0x800CCE08u,
    0x800CCE0Au, 0x800CCE0Cu, 0x800CCE0Eu, 0x800CCE10u,
    0x800CCE12u, 0x800CCD1Cu, 0x800CCD1Eu, 0x800CCD20u,
};

/* Backing storage used by the production battle guest aliases. */
__attribute__((aligned(16))) u8 g_PsxRam[PSX_RAM_SIZE];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed,
                     uint32_t a0, uint32_t idx, uint32_t a2, uint32_t a3) {
    uint32_t x = seed ^ 0x7A28079u;
    static u8 span[SPAN_LEN];
    for (uint32_t j = 0; j < SPAN_LEN; j++)
        span[j] = (u8)(lcg(&x) >> 16);
    uint32_t row = a0 & 0xFFu;
    uint32_t off = row * 368u;

    u8 *host_base = PSX_ADDR(rbase[idx]);
    /* host side: mirror the touched base's slice */
    memcpy(host_base, span + (rbase[idx] - MIN_BASE), REACH);
    /* retail side: the whole overlapping span at once */
    if (remu_poke(m, MIN_BASE, span, SPAN_LEN)) { printf("FAIL poke\n"); return 0; }

    if ((a3 & 0xFFu) != 0) {
        u16 want;
        /* expected value first (bytes are identical pre-call) */
        want = (u16)(span[(rbase[idx] - MIN_BASE) + off] | ((u16)span[(rbase[idx] - MIN_BASE) + off + 1] << 8));
        u16 got = func_8007A280((u8)a0, (u8)idx, (u16)a2, (u8)a3);
        int rc = remu_call(m, entry, a0, idx, a2, a3);
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
        if (got != want || rv0 != want) {
            printf("FAIL seed=%08X load host=%04X retail=%04X want=%04X\n",
                   seed, got, rv0 & 0xFFFFu, want);
            return 0;
        }
        /* load path stores nothing: span must be untouched on both */
        static u8 rspan[SPAN_LEN];
        if (remu_peek(m, MIN_BASE, rspan, SPAN_LEN)) { printf("FAIL peek\n"); return 0; }
        if (memcmp(rspan, span, SPAN_LEN) != 0) {
            printf("FAIL seed=%08X load-path store\n", seed);
            return 0;
        }
    } else {
        /* store path: v0 is entry-$t0 residue, compare memory only */
        (void)func_8007A280((u8)a0, (u8)idx, (u16)a2, (u8)a3);
        int rc = remu_call(m, entry, a0, idx, a2, a3);
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
        u16 hwant = (u16)a2;
        uint32_t roff = (rbase[idx] - MIN_BASE) + off;
        /* expected full span: init with exactly the stored byte applied */
        static u8 expect[SPAN_LEN];
        memcpy(expect, span, SPAN_LEN);
        expect[roff] = (u8)(hwant & 0xFFu);
        expect[roff + 1] = (u8)((hwant >> 8) & 0xFFu);
        static u8 rspan[SPAN_LEN];
        if (remu_peek(m, MIN_BASE, rspan, SPAN_LEN)) { printf("FAIL peek\n"); return 0; }
        if (memcmp(rspan, expect, SPAN_LEN) != 0) {
            printf("FAIL seed=%08X retail span mismatch\n", seed);
            for (uint32_t k = 0; k < SPAN_LEN; k++)
                if (rspan[k] != expect[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, rspan[k], expect[k]);
                    break;
                }
            return 0;
        }
        /* host mirror slice must show the same single-byte change */
        for (uint32_t k = 0; k < REACH; k++) {
            u8 wantk = (k == off) ? (u8)(hwant & 0xFFu) : (k == off + 1) ? (u8)((hwant >> 8) & 0xFFu) : span[(rbase[idx] - MIN_BASE) + k];
            if (host_base[k] != wantk) {
                printf("FAIL seed=%08X host slice @%u got=%02X want=%02X\n",
                       seed, k, host_base[k], wantk);
                return 0;
            }
        }
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    const uint32_t entry = 0x8007A280u;
    if (remu_load_battle_slice(m, entry, 936u, 1) ||
        remu_load_battle_slice(m, JTBL_BASE, JTBL_LEN, 0)) {
        fputs("FAIL loading owned battle function or dispatch table\n", stderr);
        return 1;
    }

    int n = 0;
    uint32_t s = 0x7A281234u;
    uint32_t rows[] = { 0, 1, 2, 127, 254, 255, 0x1FFu, 0x100u };
    for (unsigned ri = 0; ri < sizeof(rows) / sizeof(rows[0]); ri++) {
        for (uint32_t idx = 0; idx < 24; idx++) {
            for (int k = 0; k < 4; k++) {
                s = s * 1103515245u + 12345u;
                uint32_t a2 = s >> 16;
                s = s * 1103515245u + 12345u;
                /* alternate load/store paths, plus full-word args */
                uint32_t a3 = (k % 2) ? ((s >> 8) | 1u) : 0u;
                uint32_t a0 = rows[ri] | ((k == 3) ? 0xFF00u : 0u);
                if (!check_one(m, entry, s, a0, idx, a2, a3)) return 1;
                n++;
            }
        }
    }
    printf("DIFF 8007A280 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

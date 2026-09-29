/* Differential test: host-compiled src/battle/main27.c func_8007D610
 * vs the retail bytes (asm/battle/nonmatchings/main27/func_8007D610.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8007D610(ppBoard, index): count i in 3..10 with D_800D2DCC[i] != 0,
 *   (*(u16 *)(D_800CCD64 + 0x450 + (i - 3) * 0x170) & 0xC000) == 0 and
 *   D_800C3EB7[0x54 + (i - 3) * 0x1C] == 0; store the count to
 *   D_800D3430[((index & 0xFF) << 6) + p[1]] (p = *ppBoard).
 * Retail v0 on return is the store address; the host body is void, so v0 is
 * compared against the independently computed address while the table is
 * compared host-vs-retail byte for byte.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007D610 src/battle/main27.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main27.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8007D610(u8 **ppBoard, u8 index);

/* TU externs the test must provide (sizes generous; D_800D2DCC/D_800CCD64/
 * D_800C3EB7/D_800D3430 are used by the tested function, the rest keep the
 * link whole). */
u8 D_800CCD64[8192];
u8 D_800D3430[17408];
u8 D_800D2DCC[64];
u8 D_800C3EB7[512];
u8 D_800D32A1[64];
u8 D_800CCDEC[8192];
u8 D_800CCD34[8192];
u8 D_800D3420[18432];
u32 func_8007A628(u32 a0, u32 a1) { (void)a0; (void)a1; return 0; }
u32 func_80089C08(u8 idx) { (void)idx; return 0; }

#define RETAIL_S "asm/battle/nonmatchings/main27/func_8007D610.s"
#define CCD_BASE 0x800CCD64u
#define CCD_LEN 8192u
#define E3_BASE 0x800D2DCCu
#define E3_LEN 64u
#define B7_BASE 0x800C3EB7u
#define B7_LEN 512u
#define TAB_BASE 0x800D3430u
/* Retail indexes row[p[1]] with p[1] up to 0xFF,
 * so the max touch is (0xFF << 6) + 0xFF = 16575. */
#define TAB_LEN 17408u
#define PP 0x801F0000u
#define BOARD 0x801F0010u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    int idx = (int)seed;
    u8 p1 = (u8)(seed >> 8);
    uint32_t row = (uint32_t)(((uint32_t)idx & 0xFFu) << 6);
    static u8 ccd[CCD_LEN];
    static u8 e3[E3_LEN];
    static u8 b7[B7_LEN];
    static u8 tab[TAB_LEN];
    u8 board[8];
    uint32_t x = seed ^ 0xD610D610u;

    for (uint32_t j = 0; j < CCD_LEN; j++)
        ccd[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < E3_LEN; j++)
        e3[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < B7_LEN; j++)
        b7[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    memset(board, 0, sizeof(board));
    board[1] = p1;

    /* host side */
    memcpy(D_800CCD64, ccd, CCD_LEN);
    memcpy(D_800D2DCC, e3, E3_LEN);
    memcpy(D_800C3EB7, b7, B7_LEN);
    memcpy(D_800D3430, tab, TAB_LEN);
    memcpy(h_board, board, sizeof(h_board));
    u8 *h_pp = h_board;
    u8 **hpp = &h_pp;
    func_8007D610(hpp, (u8)idx);

    /* retail side */
    uint32_t pc = BOARD;
    if (remu_poke(m, CCD_BASE, ccd, CCD_LEN)) { printf("FAIL poke ccd\n"); return 0; }
    if (remu_poke(m, E3_BASE, e3, E3_LEN)) { printf("FAIL poke e3\n"); return 0; }
    if (remu_poke(m, B7_BASE, b7, B7_LEN)) { printf("FAIL poke b7\n"); return 0; }
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, BOARD, board, sizeof(board))) { printf("FAIL poke board\n"); return 0; }
    if (remu_poke(m, PP, &pc, 4)) { printf("FAIL poke pp\n"); return 0; }
    int rc = remu_call(m, entry, PP, (uint32_t)idx, 0, 0);
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
    /* v0 holds the store address (sb in the delay slot of jr) */
    uint32_t want_v0 = TAB_BASE + row + p1;
    uint32_t got_v0 = remu_get_reg(m, 2);
    if (got_v0 != want_v0) {
        printf("FAIL seed=%08X retail v0=%08X want=%08X\n", seed, got_v0, want_v0);
        return 0;
    }
    /* memory effects: exactly the stored count byte; table otherwise intact */
    static u8 rtab[TAB_LEN];
    if (remu_peek(m, TAB_BASE, rtab, TAB_LEN)) { printf("FAIL peek tab\n"); return 0; }
    if (memcmp(rtab, D_800D3430, TAB_LEN) != 0) {
        printf("FAIL seed=%08X table mismatch\n", seed);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (rtab[k] != D_800D3430[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, rtab[k], D_800D3430[k]);
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
    if (entry != 0x8007D610u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    /* edge seeds */
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu,
                         0x80000000u, 0xFFFFFFFFu };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    /* all single bits */
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, 1u << b)) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep */
    uint32_t s = 0x12345678u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 8007D610 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

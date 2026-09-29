/* Differential test: host-compiled src/battle/main16.c func_8007A92C
 * vs the retail bytes (asm/battle/nonmatchings/main16/func_8007A92C.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * Retail leaves the destination address in v0; that address lives in the
 * emulated address space, so v0 is compared relative to the table base.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007A92C src/battle/main16.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main16.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8007A92C(u8 **ppBoard, u8 index);

/* TU externs the test must provide (sizes generous; D_800D3420 is used). */
__attribute__((aligned(16))) u8 D_800D3420[17408];
__attribute__((aligned(16))) u8 D_800D3410[17408];
u32 D_800D2D40, D_800D2D48;
u8 D_800C3BCC[64];
u16 D_8005A3A0[8];
u8 D_800D2E62[8];
u16 D_800D39E0;
u8 *D_800D3278;
void LoadImage(void *a, void *b) { (void)a; (void)b; }
void DrawSync(s32 mode) { (void)mode; }

#define RETAIL_S "asm/battle/nonmatchings/main16/func_8007A92C.s"
#define TAB_BASE 0x800D3420u
/* 256x64 rows plus slack: retail stores at row + (p[1] << 1) with p[1] up
 * to 0xFF, so the max touch is (0xFF << 6) + 0x1FE + 2 = 16832. */
#define TAB_LEN 17408u
#define PP 0x801F0000u
#define BOARD 0x801F0010u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    u8 b1 = (u8)seed, b2 = (u8)(seed >> 8);
    u8 b3 = (u8)(seed >> 16), idx = (u8)(seed >> 24);
    uint32_t x = seed;
    static u8 tab[TAB_LEN];
    u8 board[8];

    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    board[0] = (u8)(seed >> 8); board[1] = b1;
    board[2] = b2; board[3] = b3;
    board[4] = 0; board[5] = 0; board[6] = 0; board[7] = 0;

    /* host side */
    memcpy(D_800D3420, tab, TAB_LEN);
    memcpy(h_board, board, sizeof(board));
    u8 *h_pp = h_board;
    func_8007A92C(&h_pp, idx);

    /* retail side */
    uint32_t pc = BOARD;
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, BOARD, board, sizeof(board))) { printf("FAIL poke board\n"); return 0; }
    if (remu_poke(m, PP, &pc, 4)) { printf("FAIL poke pp\n"); return 0; }
    int rc = remu_call(m, entry, PP, idx, 0, 0);
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
    /* v0 is the store address: compare relative to the table base */
    uint32_t want_off = ((uint32_t)idx << 6) + ((uint32_t)b1 << 1);
    uint32_t got_off = remu_get_reg(m, 2) - TAB_BASE;
    if (got_off != want_off) {
        printf("FAIL seed=%08X retail v0 off=%08X host off=%08X\n",
               seed, got_off, want_off);
        return 0;
    }
    /* memory effects: the halfword store */
    static u8 rtab[TAB_LEN];
    if (remu_peek(m, TAB_BASE, rtab, TAB_LEN)) { printf("FAIL peek tab\n"); return 0; }
    if (memcmp(rtab, D_800D3420, TAB_LEN) != 0) {
        printf("FAIL seed=%08X table mismatch\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8007A92Cu) { printf("FAIL load entry=%08X\n", entry); return 1; }

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
    printf("DIFF 8007A92C OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

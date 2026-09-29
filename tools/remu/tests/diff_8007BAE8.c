/* Differential test: host-compiled src/battle/mainasm_BFF8.c func_8007BAE8
 * vs the retail bytes (asm/battle/matchings/mainasm_BFF8/func_8007BAE8.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8007BAE8(ppBoard, a1): slot = (a1 & 0xFF) + 3;
 * *(u32 *)(D_800CCE34 + slot * 368) = p[1] | (p[2] << 8), p = *ppBoard.
 * void: retail leaves the offset in v0 but the only caller ignores it,
 * so only the memory effect is compared (plus zero stub calls).
 *
 * Retail scratch (pp cell + board bytes) lives in the never-stored prefix
 * of the span (stores start at offset 368*3); the host side uses mirrors.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007BAE8 src/battle/main23.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_8007BAE8(u8** a0, s32 a1);

#define RETAIL_S "asm/battle/matchings/mainasm_BFF8/func_8007BAE8.s"
#define TAB_BASE 0x800CCE34u
#define TAB_LEN (368u * 258u + 4u) /* max slot 0xFF+3, plus the stored word */
/* TU extern the test must provide. */
u8 D_800CCE34[TAB_LEN];
#define CELL_OFF 0x100u /* pp cell: 4-byte LE pointer to BOARD_OFF */
#define BOARD_OFF 0x200u /* board bytes: p[1], p[2] read from here */

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a1) {
    uint32_t x = seed ^ 0x7BAE879u;
    static u8 tab[TAB_LEN];
    static u8 expect[TAB_LEN];
    static u8 back[TAB_LEN];
    u8 board[8];
    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < sizeof(board); j++)
        board[j] = (u8)(lcg(&x) >> 16);

    /* scratch cell -> board (stores never reach below 368*3) */
    uint32_t baddr = TAB_BASE + BOARD_OFF;
    tab[CELL_OFF] = (u8)(baddr & 0xFFu);
    tab[CELL_OFF + 1] = (u8)((baddr >> 8) & 0xFFu);
    tab[CELL_OFF + 2] = (u8)((baddr >> 16) & 0xFFu);
    tab[CELL_OFF + 3] = (u8)((baddr >> 24) & 0xFFu);
    memcpy(tab + BOARD_OFF, board, sizeof(board));

    /* independent expectation: direct slot multiply */
    uint32_t off = (((a1 & 0xFFu) + 3u) * 368u);
    uint32_t v = (uint32_t)board[1] | ((uint32_t)board[2] << 8);
    memcpy(expect, tab, TAB_LEN);
    expect[off] = (u8)(v & 0xFFu);
    expect[off + 1] = (u8)((v >> 8) & 0xFFu);
    expect[off + 2] = (u8)((v >> 16) & 0xFFu);
    expect[off + 3] = (u8)((v >> 24) & 0xFFu);

    /* host side */
    memcpy(D_800CCE34, tab, TAB_LEN);
    memcpy(h_board, board, sizeof(h_board));
    u8 *h_pp = h_board;
    u8 **hpp = &h_pp;
    func_8007BAE8(hpp, (u8)a1);
    if (memcmp(D_800CCE34, expect, TAB_LEN) != 0) {
        printf("FAIL seed=%08X host tab mismatch\n", seed);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (D_800CCE34[k] != expect[k]) {
                printf("  off %u host=%02X want=%02X\n", k, D_800CCE34[k], expect[k]);
                break;
            }
        return 0;
    }

    /* retail side */
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, TAB_BASE + CELL_OFF, a1, 0, 0);
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
    if (remu_peek(m, TAB_BASE, back, TAB_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(back, expect, TAB_LEN) != 0) {
        printf("FAIL seed=%08X a1=%08X retail tab mismatch\n", seed, a1);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (back[k] != expect[k]) {
                printf("  off %u retail=%02X want=%02X\n", k, back[k], expect[k]);
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
    if (entry != 0x8007BAE8u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    static const uint32_t a1s[] = { 0, 1, 2, 3, 0x7Fu, 0x80u, 0xFCu, 0xFDu,
                                    0xFEu, 0xFFu, 0x100u, 0x1FFu, 0x200u, 0x1234u };
    uint32_t s = 0x7BAE1234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(a1s) / sizeof(a1s[0]); i++) {
        for (int r = 0; r < 16; r++) {
            s = s * 1103515245u + 12345u;
            if (!check_one(m, entry, s, a1s[i]))
                return 1;
            n++;
        }
    }
    for (int r = 0; r < 256; r++) {
        s = s * 1103515245u + 12345u;
        uint32_t a1 = s; /* full-word a1 exercises the &0xFF mask */
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s, a1))
            return 1;
        n++;
    }
    printf("DIFF 8007BAE8 OK (%d seeds, retail-exact)\n", n);
    return 0;
}

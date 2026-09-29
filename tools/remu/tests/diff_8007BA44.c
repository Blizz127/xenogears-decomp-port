/* Differential test: host-compiled src/battle/mainasm_BF14.c func_8007BA44
 * vs the retail bytes (asm/battle/matchings/mainasm_BF14/func_8007BA44.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8007BA44(pp, idx): T-row = D_800D3410 + ((idx & 0xFF) << 6), p = *pp;
 *   ((u32 *)T-row)[p[2]] = *(u16 *)(T-row + 0x10 + (p[1] << 1)).
 * Retail v0 on return is the loaded halfword value; the host body is void,
 * so v0 is compared against the independently computed halfword while the
 * table is compared host-vs-retail byte for byte (full-table compare
 * catches any width/scale error on either side).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007BA44 src/battle/mainasm_BF14.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (mainasm_BF14.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8007BA44(u8 **pp, s32 idx);

/* TU externs the test must provide (sizes generous; D_800D3410 is used). */
u8 D_800D3420[18432];
u8 D_800D3410[18432];
u8 D_800D343F;
u8 D_800D342E;

#define RETAIL_S "asm/battle/matchings/mainasm_BF14/func_8007BA44.s"
#define B10 0x800D3410u
#define PBOX 0x80181000u
#define PBYTES (PBOX + 0x10u)
#define REGION 18432u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    int idx = (int)seed;
    u8 b1 = (u8)(seed >> 8), b2 = (u8)(seed >> 17);
    uint32_t row = (uint32_t)(((uint32_t)idx & 0xFFu) << 6);
    static u8 tab[REGION];
    u8 board[8];
    uint32_t x = seed ^ 0xBA440BA4u;

    for (uint32_t j = 0; j < REGION; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    memset(board, 0, sizeof(board));
    board[1] = b1; board[2] = b2;

    /* host side */
    memcpy(D_800D3410, tab, REGION);
    memcpy(h_board, board, sizeof(h_board));
    u8 *h_pp = h_board;
    u8 **hpp = &h_pp;
    func_8007BA44(hpp, idx);

    /* retail side */
    uint32_t pptr = PBYTES;
    if (remu_poke(m, B10, tab, REGION)) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, PBOX, &pptr, 4)) { printf("FAIL poke p\n"); return 0; }
    if (remu_poke(m, PBYTES, board, sizeof(board))) { printf("FAIL poke pb\n"); return 0; }
    int rc = remu_call(m, entry, PBOX, (uint32_t)idx, 0, 0);
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
    /* v0 holds the loaded halfword (lhu before the sw in the jr delay slot) */
    uint32_t off16 = 0x10u + row + (uint32_t)b1 * 2u;
    uint32_t want_v0 = (uint32_t)tab[off16] | ((uint32_t)tab[off16 + 1u] << 8);
    uint32_t got_v0 = remu_get_reg(m, 2);
    if (got_v0 != want_v0) {
        printf("FAIL seed=%08X retail v0=%08X want=%08X\n", seed, got_v0, want_v0);
        return 0;
    }
    /* memory effects: whole table must match (only the target word changes) */
    static u8 rtab[REGION];
    if (remu_peek(m, B10, rtab, REGION)) { printf("FAIL peek tab\n"); return 0; }
    if (memcmp(rtab, D_800D3410, REGION) != 0) {
        printf("FAIL seed=%08X idx=%d b=(%u,%u) table mismatch\n",
               seed, idx, b1, b2);
        for (uint32_t k = 0; k < REGION; k++)
            if (rtab[k] != D_800D3410[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, rtab[k], D_800D3410[k]);
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
    if (entry != 0x8007BA44u) { printf("FAIL load entry=%08X\n", entry); return 1; }

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
    printf("DIFF 8007BA44 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

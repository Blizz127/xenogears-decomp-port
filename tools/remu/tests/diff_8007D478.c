/* Multi-load differential test: host-compiled src/battle/main27.c
 * func_8007D478 vs the retail bytes executed by remu, with callee bodies
 * ALSO loaded as retail code:
 *   asm/battle/nonmatchings/main27/func_8007D478.s   (entry)
 *   tools/remu/tests/func_8001BD40_randsq.s  (8001BD40 with its two
 *     `jal rand` words retargeted from unmapped libc 0x8003FA38 to the
 *     programmed counter below; all other words byte-identical)
 *   asm/battle/matchings/main41/func_80089C08.s
 *   tools/remu/tests/rand_seq.s  (programmed `rand`, see below)
 * Fails on any behavioral mismatch.
 *
 * func_8007D478(ppBoard, index): clear the D_800D3420 slot
 * ((index & 0xFF) << 6) + p[1]*2, then loop: r = func_8001BD40(0, 2);
 * if flag[r] is clear, read D_800CCD64[r*368]: store func_80089C08(r)
 * to the slot and return when bit15 is set with bits 0x4002 clear,
 * else set flag[r]. Return with the slot still zero once all three
 * flags are set.
 *
 * Programmed rand: the real libc rand (0x8003FA38) has no .s to load;
 * a constant-0 stub (remu's default) would pin r = 0 forever, hanging
 * every seed whose row 0 does not store. Instead the test loads
 * rand_seq.s, a counter returning (n++ & 0xFF) from retail scratch
 * 0x801F0200, retargets 8001BD40's two `jal rand` words at it (patched
 * copy above), and mirrors the counter on the host; both sides then
 * cycle r = 0,1,2 and every seed terminates. The residual (real rand
 * distribution) is environmental and documented, not verified.
 *
 * Row plans per seed force the three row halves to STORE (0x8000),
 * FLAG-clear (0x0000) or FLAG-set (0xC402) patterns, covering the
 * first-store, late-store, revisit-skip and exhaustion paths.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007D478 src/battle/main27.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "retail_oracle.h"
#include "common.h"
void func_8007D478(u8** ppBoard, u8 index);

/* host rand mirror: (n++ & 0xFF), counter reset per check like retail */
static uint32_t rand_ctr;
int rand(void) {
    uint32_t v = rand_ctr;
    rand_ctr = v + 1;
    return (int)(v & 0xFFu);
}

/* ---- host replicas (frozen copies of transcribed bodies) ---- */
u8 func_8001BD40(u8 min, u8 max) {
    u8 range;
    if (min == 0xFF) return 0xFF;
    if (max == 0) return 0;
    if (min == max) return min;
    range = (u8)(max - min);
    if (range < 0xFF) {
        return (u8)(min + (rand() & 0xFF) % (range + 1));
    }
    return (u8)(rand() & 0xFF);
}

__attribute__((aligned(2))) u16 H48[256];
u32 func_80089C08(u8 idx) { return H48[idx]; }

/* ---- TU externs the test must provide (disjoint regions) ---- */
#define TAB_BASE 0x800D3420u
#define TAB_LEN 18432u
#define C64_BASE 0x800CCD64u
#define C64_LEN (368u * 2u + 2u) /* rows 0..2 halves, tail byte included */
u8 D_800D3420[TAB_LEN];
__attribute__((aligned(2))) u8 D_800CCD64[C64_LEN];

#define RETAIL_D478 "asm/battle/nonmatchings/main27/func_8007D478.s"
#define RETAIL_1BD40 "tools/remu/tests/func_8001BD40_randsq.s"
#define RETAIL_89C08 "asm/battle/matchings/main41/func_80089C08.s"
#define RETAIL_RAND "tools/remu/tests/rand_seq.s"
#define T48_BASE 0x800C3448u
#define T48_LEN 512u
#define PP 0x801F0000u
#define BOARD 0x801F0010u
#define RCTR 0x801F0200u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];

/* programmed-rand model: one step of the shared (n++ & 0xFF) sequence */
static uint32_t seq_next(uint32_t *c) {
    uint32_t v = *c;
    *c = v + 1;
    return (v & 0xFFu) % 3u;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t idx,
                     uint32_t board1, int plan) {
    uint32_t x = seed ^ 0x7D47879u;
    static u8 tab[TAB_LEN];
    static u8 expect[TAB_LEN];
    static u8 back[TAB_LEN];
    static u8 c64[C64_LEN];
    static u8 c64back[C64_LEN];
    static u16 tab48[256];
    u8 board[8];
    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < C64_LEN; j++)
        c64[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < 256; j++)
        tab48[j] = (u16)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < sizeof(board); j++)
        board[j] = (u8)(lcg(&x) >> 16);
    board[1] = (u8)board1;

    /* force the three row halves per plan (high nibble pattern per row) */
    static const uint16_t plans[] = {
        0x8000u, 0x8000u, 0x8000u, /* all store */
        0x0000u, 0x0000u, 0x0000u, /* all flag-clear */
        0xC402u, 0xC402u, 0xC402u, /* all flag-set */
        0x0000u, 0x0000u, 0x8000u, /* late store */
        0x0000u, 0x8000u, 0x0000u,
        0x8000u, 0x0000u, 0x0000u, /* first store */
        0xC402u, 0x0000u, 0x8000u, /* mixed */
    };
    if (plan >= 0) {
        for (int k = 0; k < 3; k++) {
            uint16_t h = plans[(uint32_t)plan * 3u + (uint32_t)k];
            c64[(uint32_t)k * 368u] = (u8)(h & 0xFFu);
            c64[(uint32_t)k * 368u + 1] = (u8)((h >> 8) & 0xFFu);
        }
    }
    /* else: fully random halves */

    memcpy(D_800D3420, tab, TAB_LEN);
    memcpy(D_800CCD64, c64, C64_LEN);
    memcpy(H48, tab48, sizeof(tab48));
    memcpy(h_board, board, sizeof(h_board));
    u8 p48[T48_LEN];
    for (uint32_t j = 0; j < 256; j++) {
        p48[2 * j] = (u8)(tab48[j] & 0xFFu);
        p48[2 * j + 1] = (u8)((tab48[j] >> 8) & 0xFFu);
    }
    uint32_t baddr = BOARD;
    u8 cell[4] = { (u8)(baddr & 0xFFu), (u8)((baddr >> 8) & 0xFFu),
                   (u8)((baddr >> 16) & 0xFFu), (u8)((baddr >> 24) & 0xFFu) };
    u8 zero[4] = { 0, 0, 0, 0 };
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, C64_BASE, c64, C64_LEN)) { printf("FAIL poke c64\n"); return 0; }
    if (remu_poke(m, T48_BASE, p48, T48_LEN)) { printf("FAIL poke t48\n"); return 0; }
    if (remu_poke(m, PP, cell, 4)) { printf("FAIL poke pp\n"); return 0; }
    if (remu_poke(m, BOARD, board, sizeof(board))) { printf("FAIL poke board\n"); return 0; }
    if (remu_poke(m, RCTR, zero, 4)) { printf("FAIL poke rctr\n"); return 0; }

    /* independent expectation (direct formulation, capped model loop) */
    uint32_t slot = ((idx & 0xFFu) << 6) + ((uint32_t)board[1] * 2u);
    memcpy(expect, tab, TAB_LEN);
    expect[slot] = 0;
    expect[slot + 1] = 0;
    uint32_t f[3] = { 0, 0, 0 };
    uint32_t mc = 0;
    int done = 0;
    for (int it = 0; it < 64 && !done; it++) {
        uint32_t a0 = seq_next(&mc);
        if (f[a0] == 0) {
            uint32_t h = (uint32_t)c64[a0 * 368u] |
                         ((uint32_t)c64[a0 * 368u + 1] << 8);
            if ((h & 0x8000u) != 0 && (h & 0x4002u) == 0) {
                uint32_t w = tab48[a0];
                expect[slot] = (u8)(w & 0xFFu);
                expect[slot + 1] = (u8)((w >> 8) & 0xFFu);
                done = 1;
            } else {
                f[a0] = 1;
            }
        }
        if (!done && (f[0] & f[1] & f[2]) != 0)
            done = 1;
    }
    if (!done) {
        printf("FAIL seed=%08X model loop cap\n", seed);
        return 0;
    }

    /* host side (counter reset like retail) */
    rand_ctr = 0;
    u8 *h_pp = h_board;
    u8 **hpp = &h_pp;
    func_8007D478(hpp, (u8)idx);
    if (memcmp(D_800D3420, expect, TAB_LEN) != 0) {
        printf("FAIL seed=%08X plan=%d host tab mismatch\n", seed, plan);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (D_800D3420[k] != expect[k]) {
                printf("  off %u host=%02X want=%02X\n", k, D_800D3420[k], expect[k]);
                break;
            }
        return 0;
    }
    if (memcmp(D_800CCD64, c64, C64_LEN) != 0) {
        printf("FAIL seed=%08X host c64 store\n", seed);
        return 0;
    }

    /* retail side: every callee is loaded, zero stubs expected */
    int rc = remu_call(m, entry, PP, idx, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%08X unexpected stubs%s\n", seed,
               remu_stub_log(m));
        return 0;
    }
    if (remu_peek(m, TAB_BASE, back, TAB_LEN)) { printf("FAIL peek tab\n"); return 0; }
    if (memcmp(back, expect, TAB_LEN) != 0) {
        printf("FAIL seed=%08X plan=%d idx=%08X retail tab mismatch\n", seed, plan, idx);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (back[k] != expect[k]) {
                printf("  off %u retail=%02X want=%02X\n", k, back[k], expect[k]);
                break;
            }
        return 0;
    }
    if (remu_peek(m, C64_BASE, c64back, C64_LEN)) { printf("FAIL peek c64\n"); return 0; }
    if (memcmp(c64back, c64, C64_LEN) != 0) {
        printf("FAIL seed=%08X retail c64 store\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_oracle(m, RETAIL_D478);
    if (entry != 0x8007D478u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_oracle(m, RETAIL_1BD40) != 0x8001BD40u) { printf("FAIL load 1bd40\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_89C08) != 0x80089C08u) { printf("FAIL load 89c08\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_RAND) == 0) { printf("FAIL load rand\n"); return 1; }
    static const uint32_t idxs[] = { 0, 1, 0x3Fu, 0x80u, 0xFFu, 0x100u, 0x1FFu, 0x1234u };
    uint32_t s = 0x7D478234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(idxs) / sizeof(idxs[0]); i++) {
        for (int plan = -1; plan < 7; plan++) {
            for (int r = 0; r < 3; r++) {
                s = s * 1103515245u + 12345u;
                uint32_t seed = s;
                s = s * 1103515245u + 12345u;
                uint32_t b1 = (s >> 16) & 0xFFu;
                if (!check_one(m, entry, seed, idxs[i], b1, plan))
                    return 1;
                n++;
            }
        }
    }
    printf("DIFF 8007D478 OK (%d seeds, retail-exact)\n", n);
    return 0;
}

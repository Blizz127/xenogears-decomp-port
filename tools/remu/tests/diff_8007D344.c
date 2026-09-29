/* Multi-load differential test: host-compiled src/battle/main27.c
 * func_8007D344 vs the retail bytes executed by remu, with the three
 * callee bodies ALSO loaded as retail code:
 *   asm/battle/nonmatchings/main27/func_8007D344.s   (entry)
 *   asm/battle/nonmatchings/main14/func_8007A6C8.s
 *   asm/slus_006.64/matchings/system/temp3/func_8001BD40.s
 *   asm/battle/matchings/main41/func_80089C08.s
 * Fails on any behavioral mismatch.
 *
 * func_8007D344(ppBoard, index): clear the D_800D3420 slot
 * ((index & 0xFF) << 6) + p[1]*2, then for s1 in 3..10 call
 * func_8007A6C8(s1, p[2]); when that is nonzero and
 * D_800C3EB7[0x54 + k*0x1C] & 0x80, collect s1. If any collected, store
 * func_80089C08(list[func_8001BD40(0, count-1)]) back to the slot.
 *
 * Host replicas of the three callees are frozen copies of their
 * independently verified/transcribed bodies (diff_8007A6C8 for the first,
 * temp3.c/main41.c transcriptions for the other two).
 *
 * RNG pinning: retail `rand` has no .s to load, so remu stubs it to v0=0;
 * the host pins it the same way (rand() below returns 0). The pin only
 * fixes VALUES, never paths: 8001BD40's branches do not depend on the
 * rand result, and the test documents which slice is covered.
 *
 * Seed patterns (C3EB7 bit7 at 0x54+k*0x1C forced per pattern):
 *   A: all clear  -> count stays 0, tail skipped, zero stubs expected
 *   B: all set    -> count is 8 (board[2] forced nonzero, gates forced
 *      open), tail runs, exactly one stub (the pinned rand call)
 *   C: random pattern -> count in 0..8, tail iff count > 0
 *
 * The D_800D3420/D_800D2DCC regions overlap in retail (E3 lies inside the
 * TAB span) and one call stores through TAB while the loop reads E3, so
 * the host mirrors alias one blob (H2) exactly like retail.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007D344 src/battle/main27.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_8007D344(u8** ppBoard, u8 index);

/* pinned RNG: remu forces unloaded `rand` to v0=0; host matches it */
int rand(void) { return 0; }

/* ---- TU externs: unified host blob + table ---- */
#define SPAN2_BASE 0x800C3EB7u
#define SPAN2_LEN (0x800E3BFEu - 0x800C3EB7u)
#define B7_OFF 0u
#define C64_OFF (0x800CCD64u - SPAN2_BASE)
#define C6C_OFF (0x800CCD6Cu - SPAN2_BASE)
#define TAB_OFF (0x800D3420u - SPAN2_BASE)
#define E3_OFF (0x800D2DCCu - SPAN2_BASE)
__asm__(
".pushsection .bss,\"aw\",@nobits\n"
".p2align 1\n"
".globl H2_span\nH2_span:\n.space 130375\n"
".globl D_800C3EB7\nD_800C3EB7 = H2_span + 0\n"
".globl D_800CCD64\nD_800CCD64 = H2_span + 36525\n"
".globl D_800CCD6C\nD_800CCD6C = H2_span + 36533\n"
".globl D_800D3420\nD_800D3420 = H2_span + 62825\n"
".globl D_800D2DCC\nD_800D2DCC = H2_span + 61205\n"
".popsection\n"
);
extern u8 H2_span[];
extern u8 D_800C3EB7[];
extern u8 D_800CCD64[];
extern u8 D_800CCD6C[];
extern u8 D_800D3420[];
extern u8 D_800D2DCC[];

#define RETAIL_D344 "asm/battle/nonmatchings/main27/func_8007D344.s"
#define RETAIL_A6C8 "asm/battle/nonmatchings/main14/func_8007A6C8.s"
#define RETAIL_1BD40 "asm/slus_006.64/matchings/system/temp3/func_8001BD40.s"
#define RETAIL_89C08 "asm/battle/matchings/main41/func_80089C08.s"
#define T48_BASE 0x800C3448u
#define T48_LEN 512u
#define PP 0x801F0000u
#define BOARD 0x801F0010u

_Static_assert(SPAN2_LEN == 130375, "SPAN2_LEN baked into H2 asm");
_Static_assert(C64_OFF == 36525 && C6C_OFF == 36533 && TAB_OFF == 62825 &&
               E3_OFF == 61205, "alias offsets baked into H2 asm");

/* ---- host replicas (frozen copies of verified/transcribed bodies) ---- */
u32 func_8007A6C8(u8 a0, u8 a1) {
    u32 t = (u32)a0 & 0xFFu;
    u32 ret = 0;
    u32 off = ((t << 1) + t) << 3;
    off -= t;
    off <<= 4;
    if (D_800D2DCC[t] == 0) {
        return 0;
    }
    if ((*(u16*)(D_800CCD64 + off) & 0xC002u) != 0) {
        return 0;
    }
    if ((a1 & 0xFFu) != 0) {
        return 1;
    }
    ret = (*(u16*)(D_800CCD6C + off) & 0x20u) == 0;
    return ret;
}

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

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];

/* independent loop model: direct multiplies, not the shift idiom */
static uint32_t model_a6c8(const u8 *sp, uint32_t s1, uint32_t b2) {
    uint32_t t = s1 & 0xFFu;
    uint32_t h64 = (uint32_t)sp[C64_OFF + t * 368u] |
                   ((uint32_t)sp[C64_OFF + t * 368u + 1] << 8);
    if (sp[E3_OFF + t] == 0)
        return 0;
    if ((h64 & 0xC002u) != 0)
        return 0;
    if ((b2 & 0xFFu) != 0)
        return 1;
    uint32_t h6c = (uint32_t)sp[C6C_OFF + t * 368u] |
                   ((uint32_t)sp[C6C_OFF + t * 368u + 1] << 8);
    return (h6c & 0x20u) == 0;
}

/* count stub-call target addresses in the remu log; returns distinct count,
 * stores total in *total */
static int stub_targets(const char *log, int *total) {
    uint32_t addrs[64];
    int nd = 0;
    *total = 0;
    while (*log == ' ') {
        unsigned a;
        log++;
        if (sscanf(log, "%08X", &a) != 1)
            break;
        (*total)++;
        int i;
        for (i = 0; i < nd; i++)
            if (addrs[i] == a)
                break;
        if (i == nd && nd < 64)
            addrs[nd++] = a;
        while (*log && *log != ' ')
            log++;
    }
    return nd;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t idx,
                     uint32_t board1, uint32_t board2, int pattern) {
    uint32_t x = seed ^ 0x7D34479u;
    static u8 span2[SPAN2_LEN];
    static u8 expect[SPAN2_LEN];
    static u8 back[SPAN2_LEN];
    static u16 tab48[256];
    u8 board[8];
    for (uint32_t j = 0; j < SPAN2_LEN; j++)
        span2[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < 256; j++)
        tab48[j] = (u16)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < sizeof(board); j++)
        board[j] = (u8)(lcg(&x) >> 16);
    board[1] = (u8)board1;

    if (pattern == 1) {
        /* B: force all 8 collects */
        board[2] = (u8)(board2 | 1u);
        if (board[2] == 0)
            board[2] = 1;
        for (int k = 0; k < 8; k++) {
            span2[B7_OFF + 0x54u + (uint32_t)k * 0x1Cu] |= 0x80u;
            span2[E3_OFF + 3u + (uint32_t)k] |= 0x01u;
            uint32_t o = C64_OFF + (3u + (uint32_t)k) * 368u;
            uint32_t h = (uint32_t)span2[o] | ((uint32_t)span2[o + 1] << 8);
            h &= ~0xC002u;
            span2[o] = (u8)(h & 0xFFu);
            span2[o + 1] = (u8)((h >> 8) & 0xFFu);
        }
    } else if (pattern == 0) {
        /* A: force zero collects */
        for (int k = 0; k < 8; k++)
            span2[B7_OFF + 0x54u + (uint32_t)k * 0x1Cu] &= ~0x80u;
    } else {
        /* C: random 0x80 pattern */
        for (int k = 0; k < 8; k++)
            if ((lcg(&x) >> 16) & 1u)
                span2[B7_OFF + 0x54u + (uint32_t)k * 0x1Cu] |= 0x80u;
            else
                span2[B7_OFF + 0x54u + (uint32_t)k * 0x1Cu] &= ~0x80u;
    }

    memcpy(H2_span, span2, SPAN2_LEN);
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
    if (remu_poke(m, SPAN2_BASE, span2, SPAN2_LEN)) { printf("FAIL poke span2\n"); return 0; }
    if (remu_poke(m, T48_BASE, p48, T48_LEN)) { printf("FAIL poke t48\n"); return 0; }
    if (remu_poke(m, PP, cell, 4)) { printf("FAIL poke pp\n"); return 0; }
    if (remu_poke(m, BOARD, board, sizeof(board))) { printf("FAIL poke board\n"); return 0; }

    /* independent expectation over the span image. The TAB slot overlaps
     * the C64/C6C windows in the single retail image, so the initial
     * clear is applied to a working copy FIRST and the loop reads the
     * post-clear bytes, exactly like both executors do. */
    uint32_t slot = TAB_OFF + ((idx & 0xFFu) << 6) + ((uint32_t)board[1] * 2u);
    memcpy(expect, span2, SPAN2_LEN);
    expect[slot] = 0;
    expect[slot + 1] = 0;
    uint32_t s1 = 3, s4 = 0, s3 = 0x54, n = 0, collected[8];
    for (int k = 0; k < 8; k++) {
        uint32_t r = model_a6c8(expect, s1, board[2]);
        s1++;
        if (((r & 0xFFu) != 0) && ((expect[B7_OFF + s3] & 0x80u) != 0)) {
            collected[n++] = s1 - 1;
            s4++;
        }
        s3 += 0x1Cu;
    }
    if (s4 != 0) {
        /* rand pinned to 0 -> 8001BD40(0, s4-1) returns 0 -> collected[0] */
        uint32_t w = tab48[collected[0]];
        expect[slot] = (u8)(w & 0xFFu);
        expect[slot + 1] = (u8)((w >> 8) & 0xFFu);
    }
    if (pattern == 1 && s4 != 8) {
        printf("FAIL seed=%08X pat=B forcing lost s4=%u\n", seed, s4);
        return 0;
    }
    /* tail calls rand exactly once, EXCEPT when s4 == 1: then
     * 8001BD40(0, 0) returns through its max==0 path with no rand call */
    int want_stubs = (s4 > 1) ? 1 : 0;

    /* host side */
    u8 *h_pp = h_board;
    u8 **hpp = &h_pp;
    func_8007D344(hpp, (u8)idx);
    if (memcmp(H2_span, expect, SPAN2_LEN) != 0) {
        printf("FAIL seed=%08X pat=%d host span2 mismatch\n", seed, pattern);
        for (uint32_t k = 0; k < SPAN2_LEN; k++)
            if (H2_span[k] != expect[k]) {
                printf("  off %u host=%02X want=%02X\n", k, H2_span[k], expect[k]);
                break;
            }
        return 0;
    }

    /* retail side */
    int rc = remu_call(m, entry, PP, idx, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    int total = 0;
    int nd = stub_targets(remu_stub_log(m), &total);
    /* tail taken iff s4 > 0; its only external call is the pinned rand */
    if (total != want_stubs || (want_stubs && nd != 1)) {
        printf("FAIL seed=%08X pat=%d s4 stub mismatch total=%d distinct=%d%s\n",
               seed, pattern, total, nd, remu_stub_log(m));
        return 0;
    }
    if (remu_peek(m, SPAN2_BASE, back, SPAN2_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(back, expect, SPAN2_LEN) != 0) {
        printf("FAIL seed=%08X pat=%d idx=%08X retail span2 mismatch\n", seed, pattern, idx);
        for (uint32_t k = 0; k < SPAN2_LEN; k++)
            if (back[k] != expect[k]) {
                printf("  off %u retail=%02X want=%02X\n", k, back[k], expect[k]);
                break;
            }
        return 0;
    }
    return 1;
}

int main(void) {
    if (((uintptr_t)H2_span & 1u) != 0 ||
        D_800C3EB7 != H2_span + B7_OFF ||
        D_800CCD64 != H2_span + C64_OFF ||
        D_800CCD6C != H2_span + C6C_OFF ||
        D_800D3420 != H2_span + TAB_OFF ||
        D_800D2DCC != H2_span + E3_OFF) {
        printf("FAIL host layout\n");
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_D344);
    if (entry != 0x8007D344u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_s(m, RETAIL_A6C8) != 0x8007A6C8u) { printf("FAIL load a6c8\n"); return 1; }
    if (remu_load_s(m, RETAIL_1BD40) != 0x8001BD40u) { printf("FAIL load 1bd40\n"); return 1; }
    if (remu_load_s(m, RETAIL_89C08) != 0x80089C08u) { printf("FAIL load 89c08\n"); return 1; }
    static const uint32_t idxs[] = { 0, 1, 0x3Fu, 0x80u, 0xFFu, 0x100u, 0x1FFu, 0x1234u };
    uint32_t s = 0x7D344234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(idxs) / sizeof(idxs[0]); i++) {
        for (int pat = 0; pat < 3; pat++) {
            for (int r = 0; r < 4; r++) {
                s = s * 1103515245u + 12345u;
                uint32_t seed = s;
                s = s * 1103515245u + 12345u;
                uint32_t b1 = (s >> 16) & 0xFFu;
                s = s * 1103515245u + 12345u;
                uint32_t b2 = (s >> 16) & 0xFFu;
                if (!check_one(m, entry, seed, idxs[i], b1, b2, pat))
                    return 1;
                n++;
            }
        }
    }
    printf("DIFF 8007D344 OK (%d seeds, retail-exact)\n", n);
    return 0;
}

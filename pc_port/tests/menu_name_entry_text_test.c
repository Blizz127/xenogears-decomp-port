/* Name entry (menu mode 3, archive dir 0x10 file 8): retail MIPS from the
 * user's disc versus the port's native bodies of the two text helpers that
 * keep their retail asm in the matching build (NON_MATCHING port bodies):
 *   NameEntryMenu_801CB2F0  length, in 2-byte glyphs, of an edit buffer
 *   NameEntryMenu_801CB25C  start an edit (blank buffer + copy of name id)
 * The retail functions run in the test-only MIPS interpreter
 * (battle_mips_adapter.c) over the overlay image read from
 * disc/name_entry_menu.bin; the native ones come from the production TU
 * src/name_entry_menu/main/misc.c compiled in port mode. */
#include "common.h"
#include "main/game.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GameState g_GameState;
u8 D_80059171;
extern u8 NameEntryMenu_801CB2F0(u8* glyphs);
extern void NameEntryMenu_801CB25C(u8* glyphs, u8* name);

#define OVL_BASE 0x801C5000u
#define GS_BASE 0x8006D634u /* retail g_GameState */
#define ID_ADDR 0x80059171u /* retail D_80059171 */
#define BUF_A 0x80100000u
#define BUF_B 0x80100100u
#define STACK_LO 0x801FE000u
#define STACK_HI 0x801FF000u
#define RET 0xFFFFFFFCu

static u8 ovl[0x7800];
static u8 bufA[0x100], bufB[0x100];
static u8 stack[STACK_HI - STACK_LO];

static int memory(u32 a, unsigned w, u32* v, int write) {
    u8* p = NULL;
    if (a >= OVL_BASE && (uint64_t)a + w <= OVL_BASE + sizeof ovl && !write)
        p = ovl + (a - OVL_BASE);
    else if (a >= GS_BASE && (uint64_t)a + w <= GS_BASE + sizeof g_GameState)
        p = (u8*)&g_GameState + (a - GS_BASE);
    else if (a == ID_ADDR && w == 1)
        p = &D_80059171;
    else if (a >= BUF_A && (uint64_t)a + w <= BUF_A + sizeof bufA)
        p = bufA + (a - BUF_A);
    else if (a >= BUF_B && (uint64_t)a + w <= BUF_B + sizeof bufB)
        p = bufB + (a - BUF_B);
    else if (a >= STACK_LO && (uint64_t)a + w <= STACK_HI)
        p = stack + (a - STACK_LO);
    if (!p)
        return -1;
    if (write) {
        for (unsigned i = 0; i < w; i++)
            p[i] = (u8)(*v >> (8 * i));
    } else {
        *v = 0;
        for (unsigned i = 0; i < w; i++)
            *v |= (u32)p[i] << (8 * i);
    }
    return 0;
}
static int rd(void* u, u32 a, unsigned w, u32* v) { (void)u; return memory(a, w, v, 0); }
static int wr(void* u, u32 a, unsigned w, u32 v) { (void)u; return memory(a, w, &v, 1); }

static int retail(u32 entry, u32 a0, u32 a1, u32* v0) {
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu c;
    PcPortMipsCpuInit(&c, &bus);
    c.gpr[4] = a0;
    c.gpr[5] = a1;
    c.gpr[29] = STACK_HI - 0x100;
    c.gpr[31] = RET;
    if (PcPortMipsRun(&c, entry, RET, 5000)) {
        fprintf(stderr, "FAIL retail %08x: %s\n", entry, c.error);
        return 1;
    }
    *v0 = c.gpr[2];
    return 0;
}

static unsigned rng = 1;
static u8 rnd8(void) {
    rng = rng * 1664525u + 1013904223u;
    return (u8)(rng >> 24);
}

/* A 10-glyph name: glyphs 0x20.. with, at pair t (0-9), the end marker
 * 0x1F,0x00 (variant 0), the near-marker 0x1F,0x01 (variant 1), or a
 * 0x1F,0x00 straddling pairs t and t+1 (variant 2); t = 10 is a full-length
 * name with no marker, t = 0 variant 0 the empty name. */
static void text_pattern(u8* g, int t, int variant) {
    int k;
    for (k = 0; k < 10; k++) {
        g[k * 2] = (u8)(0x20 + k);
        g[k * 2 + 1] = 0;
    }
    if (t >= 10)
        return;
    if (variant == 0) {
        g[t * 2] = 0x1F;
        g[t * 2 + 1] = 0x00;
    } else if (variant == 1) {
        g[t * 2] = 0x1F;
        g[t * 2 + 1] = 0x01;
    } else if (t < 9) {
        g[t * 2 + 1] = 0x1F;
        g[t * 2 + 2] = 0x00;
    }
}

/* One length case: the 20-byte buffer, non-marker filler, then a far
 * 0x1F,0x00 guard at byte 0x40, so a scan that ignored the 10-glyph bound
 * would return 32 instead of 10.  Retail and native see the same bytes. */
static int length_case(const u8 g[20], unsigned* cases) {
    u32 v0;
    static u8 host[sizeof bufA];
    memset(bufA, 0xA5, sizeof bufA);
    memcpy(bufA, g, 20);
    bufA[0x40] = 0x1F;
    bufA[0x41] = 0x00;
    memcpy(host, bufA, sizeof host);
    if (retail(0x801CB2F0, BUF_A, 0, &v0))
        return 1;
    u8 n = NameEntryMenu_801CB2F0(host);
    if (v0 != n) {
        fprintf(stderr, "FAIL name entry length retail=%u native=%u buf=", (unsigned)v0, n);
        for (int i = 0; i < 20; i++)
            fprintf(stderr, "%02x", g[i]);
        fprintf(stderr, "\n");
        return 1;
    }
    (*cases)++;
    return 0;
}

static int test_length(void) {
    unsigned cases = 0;
    u8 g[20];
    int t, k, variant;

    /* End marker, near-marker and straddling marker at every pair position;
     * empty (marker first) through full length (no marker). */
    for (t = 0; t <= 10; t++) {
        for (variant = 0; variant < 3; variant++) {
            text_pattern(g, t, variant);
            if (length_case(g, &cases))
                return 1;
        }
    }
    /* 0x00 high byte after a non-0x1F low byte at every position. */
    for (t = 0; t < 10; t++) {
        memset(g, 0x30, sizeof g);
        g[t * 2] = 0x1E;
        g[t * 2 + 1] = 0x00;
        if (length_case(g, &cases))
            return 1;
    }
    /* Biased random buffers: each byte 0x1F, 0x00 or anything. */
    for (k = 0; k < 100000; k++) {
        for (t = 0; t < 20; t++) {
            u8 r = rnd8();
            g[t] = r < 80 ? 0x1F : r < 160 ? 0x00 : rnd8();
        }
        if (length_case(g, &cases))
            return 1;
    }
    printf("NAME ENTRY LENGTH PASS cases=%u\n", cases);
    return 0;
}

/* Start an edit for every name id 0-30: the whole 31-entry name table
 * (g_GameState.field_0x0 is 31 * 0x14 bytes) and a superset of the FE58
 * arguments found on both discs (0-7, 11-17, 19, 20, 22-26).  Names: random,
 * all 0x00, all 0xFF, then every text_pattern (empty, each marker variant at
 * each position, full length).  Both buffers start full of a sentinel so a
 * stray or missing write shows up; g_GameState must come back unchanged. */
static int test_edit_start(void) {
    unsigned cases = 0, id, pass;
    static u8 hostA[sizeof bufA], hostB[sizeof bufB];
    static GameState saved;

    for (pass = 0; pass < 3 + 11 * 3; pass++) {
        for (id = 0; id < 31; id++) {
            u32 v0;
            size_t i;
            for (i = 0; i < sizeof g_GameState; i++)
                ((u8*)&g_GameState)[i] = pass == 1 ? 0x00 : pass == 2 ? 0xFF : rnd8();
            if (pass >= 3)
                text_pattern((u8*)g_GameState.field_0x0 + id * 0x14, (pass - 3) / 3, (pass - 3) % 3);
            memcpy(&saved, &g_GameState, sizeof saved);
            D_80059171 = (u8)id;
            memset(bufA, 0x5A, sizeof bufA);
            memset(bufB, 0xC3, sizeof bufB);
            memcpy(hostA, bufA, sizeof hostA);
            memcpy(hostB, bufB, sizeof hostB);
            if (retail(0x801CB25C, BUF_A, BUF_B, &v0))
                return 1;
            NameEntryMenu_801CB25C(hostA, hostB);
            if (memcmp(bufA, hostA, sizeof bufA) || memcmp(bufB, hostB, sizeof bufB) ||
                memcmp(&saved, &g_GameState, sizeof saved)) {
                fprintf(stderr, "FAIL name entry edit start id=%u pass=%u\n", id, pass);
                return 1;
            }
            cases++;
        }
    }
    printf("NAME ENTRY EDIT START PASS cases=%u\n", cases);
    return 0;
}

int main(void) {
    FILE* f = fopen("disc/name_entry_menu.bin", "rb");
    if (!f)
        return 2;
    size_t n = fread(ovl, 1, sizeof ovl, f);
    fclose(f);
    if (n != sizeof ovl)
        return 2;
    if (test_length() || test_edit_start())
        return 1;
    printf("NAME ENTRY TEXT HELPERS PASS\n");
    return 0;
}

/* Differential test: host-compiled src/battle/main37.c func_80085D34
 * vs the retail bytes (asm/battle/nonmatchings/main37/func_80085D34.s)
 * executed by remu, with the callee stubbed on both sides:
 *   tools/remu/tests/76a10_log.s (retail logging stub: records count +
 *   a0..a3 of 3 calls at scratch 0x80070000, returns table-driven
 *   0x55/0xAA/0xFF from the word table at 0x80070060).
 * Fails on any behavioral mismatch.
 *
 * func_80085D34(): zero D_800D2D28[0x7B], then three rounds calling
 * func_80076A10(a0, D_800D2D28[0x7B]*80 + 0x9C8 + (u32)D_800C3EA4,
 * a2, 0xD0) with (a0, a2) = (C3EAC[0x2D4]+0xF, 0x2A), (0x19, 0x32),
 * (C3EAC[0x2D5]+0xF, 0x3A), adding each return into D_800D2D28[0x7B]
 * (mod 256); finally D_800D2D28[0xA4] = D_800CCB34. Takes no args.
 *
 * Proof shape: host recorder + retail logging stub compared against an
 * independent model (call args per round, final 0x7B/0xA4 bytes), plus
 * direct host-vs-retail span comparison of the D28 blob. The nonzero
 * stub returns walk b through 0 -> 0x55 -> 0xFF -> 0xFE (wrap covered).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085D34 src/battle/main37.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085D34(void);

/* TU externs the test must provide. */
u8 *D_800D2D28;
u8 *D_800C3EAC;
u8 *D_800C3EA4;
u8 D_800CCB34;

/* 76A10 host recorder: table-driven returns matching the retail stub. */
static u32 rec_a0[3], rec_a1[3], rec_a2[3], rec_a3[3];
static int rec_n;
static const u32 rec_ret[3] = { 0x55u, 0xAAu, 0xFFu };
u32 func_80076A10(u32 a0, u32 a1, u32 a2, u32 a3) {
    if (rec_n < 3) {
        rec_a0[rec_n] = a0;
        rec_a1[rec_n] = a1;
        rec_a2[rec_n] = a2;
        rec_a3[rec_n] = a3;
    }
    u32 r = rec_ret[rec_n < 3 ? rec_n : 2];
    rec_n++;
    return r;
}

#define RETAIL_S "asm/battle/nonmatchings/main37/func_80085D34.s"
#define RETAIL_STUB "tools/remu/tests/76a10_log.s"
#define STUB_VRAM 0x80076A10u
#define D28_BASE 0x801F0800u
#define D28_LEN 0x200u
#define D28_PTRW 0x800D2D28u
#define SCR_BASE 0x801F0400u
#define SCR_LEN 0x300u
#define PTRW_BASE 0x800C3EACu
#define CCB34_BASE 0x800CCB34u
#define EA4_PTRW 0x800C3EA4u
#define EA4_GUEST 0x801F0C00u
#define LOG_BASE 0x80070000u
#define LOG_LEN 0x70u
__attribute__((aligned(2))) static u8 h_d28[D28_LEN];
__attribute__((aligned(2))) static u8 h_scr[SCR_LEN];
__attribute__((aligned(2))) static u8 h_ea4[64];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

/* stub image check (encoding slips halt here, not in call records) */
static const u8 stub_want[60] = {
    0x07, 0x80, 0x01, 0x3C, 0x20, 0x00, 0x28, 0x8C,
    0x01, 0x00, 0x03, 0x25, 0x20, 0x00, 0x23, 0xAC,
    0x80, 0x18, 0x08, 0x00, 0x21, 0x18, 0x61, 0x00,
    0x60, 0x00, 0x62, 0x8C, 0x00, 0x41, 0x08, 0x00,
    0x21, 0x08, 0x28, 0x00, 0x24, 0x00, 0x24, 0xAC,
    0x28, 0x00, 0x25, 0xAC, 0x2C, 0x00, 0x26, 0xAC,
    0x30, 0x00, 0x27, 0xAC, 0x08, 0x00, 0xE0, 0x03,
    0x00, 0x00, 0x00, 0x00,
};

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, u8 ccb34) {
    uint32_t x = seed ^ 0x85D3479u;
    for (uint32_t j = 0; j < D28_LEN; j++)
        h_d28[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < SCR_LEN; j++)
        h_scr[j] = (u8)(lcg(&x) >> 16);

    /* model over the seed images (EA4 value differs per side). */
    u8 b0 = h_scr[0x2D4];
    u8 b2 = h_scr[0x2D5];
    u32 ma0[3] = { (u32)b0 + 15u, 0x19u, (u32)b2 + 15u };
    u32 ma2[3] = { 0x2Au, 0x32u, 0x3Au };
    /* b walk under stub returns 0x55/0xAA/0xFF */
    u32 bb[4] = { 0u, 0x55u, 0xFFu, 0xFEu };

    /* host side (EA4 = truncated host address, same value the TU adds) */
    D_800D2D28 = h_d28;
    D_800C3EAC = h_scr;
    D_800C3EA4 = h_ea4;
    D_800CCB34 = ccb34;
    u32 hea4 = (u32)(uintptr_t)h_ea4;
    /* seed bytes the host run overwrites: restored after the host
     * checks, so the retail poke sees the seed image */
    u8 save7b = h_d28[0x7B];
    u8 savea4 = h_d28[0xA4];
    rec_n = 0;
    func_80085D34();
    if (rec_n != 3) {
        printf("FAIL seed=%08X host call count=%d\n", seed, rec_n);
        return 0;
    }
    for (int i = 0; i < 3; i++) {
        u32 want_a1 = bb[i] * 80u + 0x9C8u + hea4;
        if (rec_a0[i] != ma0[i] || rec_a1[i] != want_a1 ||
            rec_a2[i] != ma2[i] || rec_a3[i] != 0xD0u) {
            printf("FAIL seed=%08X host call %d args=%08X %08X %08X %08X want=%08X %08X %08X %08X\n",
                   seed, i, rec_a0[i], rec_a1[i], rec_a2[i], rec_a3[i],
                   ma0[i], want_a1, ma2[i], 0xD0u);
            return 0;
        }
    }
    if (h_d28[0x7B] != 0xFEu || h_d28[0xA4] != ccb34) {
        printf("FAIL seed=%08X host d28[7B]=%02X d28[A4]=%02X\n", seed,
               h_d28[0x7B], h_d28[0xA4]);
        return 0;
    }

    /* retail pokes (seed image restored; host-verified 0x7B/0xA4
     * re-applied after, so the final comparison uses the post image) */
    h_d28[0x7B] = save7b;
    h_d28[0xA4] = savea4;
    if (remu_poke(m, D28_BASE, h_d28, D28_LEN)) { printf("FAIL poke d28\n"); return 0; }
    h_d28[0x7B] = 0xFEu;
    h_d28[0xA4] = ccb34;
    if (remu_poke(m, SCR_BASE, h_scr, SCR_LEN)) { printf("FAIL poke scr\n"); return 0; }
    if (remu_poke(m, CCB34_BASE, &ccb34, 1)) { printf("FAIL poke ccb\n"); return 0; }
    {
        /* guest EA4 value: its word is loaded and added into a1 */
        u8 pw[4] = { (u8)(EA4_GUEST & 0xFFu), (u8)((EA4_GUEST >> 8) & 0xFFu),
                     (u8)((EA4_GUEST >> 16) & 0xFFu),
                     (u8)((EA4_GUEST >> 24) & 0xFFu) };
        if (remu_poke(m, EA4_PTRW, pw, 4)) { printf("FAIL poke ea4\n"); return 0; }
    }
    {
        u8 pw[4] = { (u8)(D28_BASE & 0xFFu), (u8)((D28_BASE >> 8) & 0xFFu),
                     (u8)((D28_BASE >> 16) & 0xFFu),
                     (u8)((D28_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, D28_PTRW, pw, 4)) { printf("FAIL poke d28ptr\n"); return 0; }
    }
    {
        u8 pw[4] = { (u8)(SCR_BASE & 0xFFu), (u8)((SCR_BASE >> 8) & 0xFFu),
                     (u8)((SCR_BASE >> 16) & 0xFFu),
                     (u8)((SCR_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, PTRW_BASE, pw, 4)) { printf("FAIL poke ptrw\n"); return 0; }
    }
    {
        u8 z[LOG_LEN];
        memset(z, 0, sizeof z);
        /* return table at 0x80070060: words 0x55/0xAA/0xFF */
        z[0x60] = 0x55; z[0x64] = 0xAA; z[0x68] = 0xFF;
        if (remu_poke(m, LOG_BASE, z, sizeof z)) { printf("FAIL poke log\n"); return 0; }
    }

    /* retail side */
    int rc = remu_call(m, entry, 0, 0, 0, 0);
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
    /* retail-vs-model: stub log count + args (EA4 = guest value) */
    {
        u8 lg[LOG_LEN];
        if (remu_peek(m, LOG_BASE, lg, sizeof lg)) { printf("FAIL peek log\n"); return 0; }
        u32 n = (u32)lg[0x20] | ((u32)lg[0x21] << 8) |
                ((u32)lg[0x22] << 16) | ((u32)lg[0x23] << 24);
        if (n != 3) {
            printf("FAIL seed=%08X retail call count=%u\n", seed, n);
            return 0;
        }
        for (int i = 0; i < 3; i++) {
            u32 g0 = (u32)lg[0x24 + 16 * i] | ((u32)lg[0x25 + 16 * i] << 8) |
                     ((u32)lg[0x26 + 16 * i] << 16) | ((u32)lg[0x27 + 16 * i] << 24);
            u32 g1 = (u32)lg[0x28 + 16 * i] | ((u32)lg[0x29 + 16 * i] << 8) |
                     ((u32)lg[0x2A + 16 * i] << 16) | ((u32)lg[0x2B + 16 * i] << 24);
            u32 g2 = (u32)lg[0x2C + 16 * i] | ((u32)lg[0x2D + 16 * i] << 8) |
                     ((u32)lg[0x2E + 16 * i] << 16) | ((u32)lg[0x2F + 16 * i] << 24);
            u32 g3 = (u32)lg[0x30 + 16 * i] | ((u32)lg[0x31 + 16 * i] << 8) |
                     ((u32)lg[0x32 + 16 * i] << 16) | ((u32)lg[0x33 + 16 * i] << 24);
            u32 want_a1 = bb[i] * 80u + 0x9C8u + EA4_GUEST;
            if (g0 != ma0[i] || g1 != want_a1 || g2 != ma2[i] || g3 != 0xD0u) {
                printf("FAIL seed=%08X retail call %d args=%08X %08X %08X %08X want=%08X %08X %08X %08X\n",
                       seed, i, g0, g1, g2, g3,
                       ma0[i], want_a1, ma2[i], 0xD0u);
                return 0;
            }
        }
    }
    /* direct host-vs-retail D28 comparison */
    {
        static u8 d28back[D28_LEN];
        if (remu_peek(m, D28_BASE, d28back, D28_LEN)) { printf("FAIL peek d28\n"); return 0; }
        if (memcmp(d28back, h_d28, D28_LEN) != 0) {
            printf("FAIL seed=%08X retail d28 mismatch\n", seed);
            for (uint32_t k = 0; k < D28_LEN; k++)
                if (d28back[k] != h_d28[k]) {
                    printf("  off %u retail=%02X host=%02X\n", k, d28back[k], h_d28[k]);
                    break;
                }
            return 0;
        }
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085D34u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_s(m, RETAIL_STUB) != 0x80076A10u) { printf("FAIL load stub\n"); return 1; }
    {
        u8 got[60];
        if (remu_peek(m, STUB_VRAM, got, sizeof got) ||
            memcmp(got, stub_want, sizeof got) != 0) {
            printf("FAIL stub image\n");
            return 1;
        }
    }
    uint32_t s = 0x85D41234u;
    int n = 0;
    for (int k = 0; k < 64; k++) {
        s = s * 1103515245u + 12345u;
        /* ccb34 edges on some rounds (0xA4 store must track it) */
        u8 ccb = (k & 7) == 0 ? (u8)(k * 37u) : (u8)(s >> 24);
        if (!check_one(m, entry, s, ccb))
            return 1;
        n++;
    }
    printf("DIFF 80085D34 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

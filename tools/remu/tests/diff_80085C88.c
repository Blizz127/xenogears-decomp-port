/* Composition differential test: host-compiled src/battle/main36.c
 * func_80085C88 vs the retail bytes executed by remu, with callee bodies
 * ALSO loaded as retail code:
 *   asm/battle/matchings/main36/func_80085C88.s   (entry)
 *   asm/battle/nonmatchings/main36/func_80085454.s
 *   asm/battle/nonmatchings/main36/func_80085618.s
 *   asm/battle/matchings/main41/func_80089C08.s
 *   asm/battle/nonmatchings/main39/func_800883AC.s
 *   asm/battle/matchings/main41/func_80089C48.s
 * Fails on any behavioral mismatch.
 *
 * func_80085C88(index): s0 = index & 0xFF; call 85454(s0); call
 * 85618(s0); D_800D2D28[0xAD] = 0. Both callees are individually proven
 * retail-exact (diff_80085454 / diff_80085618); this test proves the
 * composition: call order, s0 arg passing, and the trailing store.
 * Proof shape is direct host-vs-retail span comparison (straight-line
 * orchestration: any divergence perturbs a span).
 *
 * Memory model: the diff_80085618 S1/CCD/E3/B301/SCR2/T48/jtbl setup,
 * plus diff_80085454's S54/S88/A2/A3 spans. D_800C3FE8 is the H_S1+308
 * alias (guest-true: one address; 85454 row writes land inside S1-live).
 * D_800D2D28 is a pointer at a 0x200 blob (guest 0x801F0800).
 * D_800D2D5C/67 need adjacent layout: build with -fno-data-sections
 * (startup assert fails safe on reorder).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085C88 src/battle/main36.c -fno-data-sections
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085C88(u8 index);

/* ---- spans ---- */
#define S1_BASE 0x800C3EB4u
#define S1_LEN (0x800D07C0u - 0x800C3EB4u)
#define S1_LIVE 0x8E80u
#define S1_C3FE8 308u
#define S1_C4000 332u
#define C48E8_OFF (0x800C48E8u - 0x800C3EB4u)
#define CCD_BASE 0x800CCD34u
#define CCD_LEN (0x800CDC54u - 0x800CCD34u)
#define CCE08_BASE 0x800CCE08u
#define CCE08_LEN (368u * 10u + 2u)
#define E08_IN_CCD 212u
#define UNI_LEN (212u + 3682u)
#define E3_BASE 0x800D2DCCu
#define E3_LEN 256u
#define T48_BASE 0x800C3448u
#define T48_LEN 512u
#define SCR2_BASE 0x801F0400u
#define SCR2_LEN 0x300u
#define C2050_BASE 0x800C2050u
#define B301_BASE 0x800D301Cu
#define B301_LEN (0x800D3A9Au - 0x800D301Cu)
#define PTRW_BASE 0x800C3EACu
#define JTBL_BASE 0x80070250u
#define S54_BASE 0x800D2C54u
#define S54_LEN 80u
#define S88_BASE 0x800D2C88u
#define S88_LEN 32u
#define A2_BASE 0x800D2D70u
#define A2_LEN 40u
#define A3_BASE 0x800D2D5Cu
#define A3_LEN 11u
#define D28_BASE 0x801F0800u
#define D28_LEN 0x200u
#define D28_PTRW 0x800D2D28u
_Static_assert(0x800CCE08u - CCD_BASE == E08_IN_CCD, "e08 overlap offset");
_Static_assert(CCD_LEN < UNI_LEN &&
               CCE08_BASE + CCE08_LEN - CCD_BASE == UNI_LEN, "union extent");
_Static_assert(S1_LIVE + UNI_LEN <= S1_LEN, "CCD alias fits in H_S1");
__asm__(
".pushsection .bss,\"aw\",@nobits\n"
".p2align 2\n"
".globl H_S1\nH_S1:\n.space 51468\n"
".globl D_800C3EB4\nD_800C3EB4 = H_S1 + 0\n"
".globl D_800C3EB5\nD_800C3EB5 = H_S1 + 1\n"
".globl D_800C3FE8\nD_800C3FE8 = H_S1 + 308\n"
".globl D_800C4000\nD_800C4000 = H_S1 + 332\n"
".globl H_CCD\nH_CCD = H_S1 + 0x8E80\n"
".globl D_800CCD34\nD_800CCD34 = H_CCD + 0\n"
".globl D_800CCD36\nD_800CCD36 = H_CCD + 2\n"
".globl D_800CCD38\nD_800CCD38 = H_CCD + 4\n"
".globl D_800CCD3A\nD_800CCD3A = H_CCD + 6\n"
".globl D_800CCD64\nD_800CCD64 = H_CCD + 48\n"
".globl D_800CCDC4\nD_800CCDC4 = H_CCD + 144\n"
".globl D_800CCDC6\nD_800CCDC6 = H_CCD + 146\n"
".globl D_800CCDEC\nD_800CCDEC = H_CCD + 184\n"
".globl D_800CCDF0\nD_800CCDF0 = H_CCD + 188\n"
".globl D_800CCE08\nD_800CCE08 = H_CCD + 212\n"
".globl H_B301\nH_B301:\n.space 2686\n"
".globl D_800D301C\nD_800D301C = H_B301 + 0\n"
".globl D_800D301D\nD_800D301D = H_B301 + 1\n"
".globl D_800D32A1\nD_800D32A1 = H_B301 + 645\n"
".popsection\n"
);
extern u8 H_S1[];
extern u8 D_800C3EB4[];
extern u8 D_800C3EB5[];
extern u8 D_800C3FE8[];
extern u8 D_800C4000[];
extern u8 H_CCD[];
extern u8 D_800CCD34[];
extern u8 D_800CCD36[];
extern u8 D_800CCD38[];
extern u8 D_800CCD3A[];
extern u8 D_800CCD64[];
extern u8 D_800CCDC4[];
extern u8 D_800CCDC6[];
extern u8 D_800CCDEC[];
extern u8 D_800CCDF0[];
extern u8 D_800CCE08[];
extern u8 H_B301[];
extern u8 D_800D301C[];
extern u8 D_800D301D[];
extern u8 D_800D32A1[];
u8 D_800D2DCC[256];
u8 *D_800C3EAC;
u16 D_800C48E8;
u8 D_800C2050;
__attribute__((aligned(2))) u8 H_SCR2[SCR2_LEN];
__attribute__((aligned(4))) u8 D_800D2C54[80];
u8 D_800D2C88[32];
u16 D_800D2D70[20];
u8 D_800D2D5C[11];
u8 D_800D2D67;
__attribute__((aligned(2))) u8 H_D2D28[D28_LEN];
u8 *D_800D2D28;
_Static_assert(S1_LEN == 51468, "S1 size baked into H_S1 asm");
_Static_assert(B301_LEN == 2686, "B301 size baked in");
_Static_assert(C48E8_OFF == 2612, "C48E8 S1 offset");

/* ---- host replicas (frozen copies of transcribed bodies) ---- */
__attribute__((aligned(2))) u16 H48[256];
u16 func_80089C08(u8 idx) { return H48[idx]; }

u32 func_80089C48(u8 a0) { return 0xFFFFu ^ (u32)H48[a0]; }

void func_800883AC(u8 a0) {
    u32 t = (u32)a0 & 0xFFu;
    u32 s1 = ((t >= 3u) ? 8u : 0u) | (D_800D32A1[t * 8u] ? 0x10u : 0u);
    u32 o28 = t * 28u;
    u32 b4 = D_800C3EB4[o28];
    u32 ix = (b4 + (s1 & 0xFFu)) << 2;
    D_800D301C[ix] = (u8)(D_800D301C[ix] - 1u);
    u32 m = 0xFFFFu ^ (u32)H48[D_800C3EB5[o28]];
    D_800D301D[ix] = (u8)(D_800D301D[ix] & m);
}

#define RETAIL_85C88 "asm/battle/matchings/main36/func_80085C88.s"
#define RETAIL_85454 "asm/battle/nonmatchings/main36/func_80085454.s"
#define RETAIL_85618 "asm/battle/nonmatchings/main36/func_80085618.s"
#define RETAIL_89C08 "asm/battle/matchings/main41/func_80089C08.s"
#define RETAIL_883AC "asm/battle/nonmatchings/main39/func_800883AC.s"
#define RETAIL_89C48 "asm/battle/matchings/main41/func_80089C48.s"

/* jtbl_80070250 contents (asm/battle/data/0.rodata.s). */
static const uint32_t jtblw[12] = {
    0x8008570Cu, 0x80085908u, 0x8008583Cu, 0x80085954u,
    0x80085A70u, 0x8008570Cu, 0x80085A70u, 0x8008570Cu,
    0x8008570Cu, 0x80085908u, 0x800859CCu, 0x80085A18u,
};

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}
static u16 g16(const u8 *p) { return (u16)(p[0] | ((u16)p[1] << 8)); }

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t idxw,
                     int plan) {
    uint32_t x = seed ^ 0x85C8879u;
    static u8 s1[S1_LEN];
    static u8 s1back[S1_LEN];
    static u8 ccd[CCD_LEN];
    static u8 ccdback[CCD_LEN];
    static u8 e08[CCE08_LEN];
    static u8 e08back[CCE08_LEN];
    static u8 e3[E3_LEN];
    static u16 tab48[256];
    static u8 b301[B301_LEN];
    static u8 b301back[B301_LEN];
    static u8 scr2[SCR2_LEN];
    static u8 scr2back[SCR2_LEN];
    static u8 s54[S54_LEN], s88[S88_LEN], a2[A2_LEN], a3[A3_LEN];
    static u8 s54back[S54_LEN], s88back[S88_LEN], a2back[A2_LEN], a3back[A3_LEN];
    static u8 d28[D28_LEN];
    static u8 d28back[D28_LEN];
    for (uint32_t j = 0; j < S1_LEN; j++)
        s1[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < CCD_LEN; j++)
        ccd[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < CCE08_LEN; j++)
        e08[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < E3_LEN; j++)
        e3[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < 256; j++)
        tab48[j] = (u16)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < B301_LEN; j++)
        b301[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < SCR2_LEN; j++)
        scr2[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < S54_LEN; j++)
        s54[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < S88_LEN; j++)
        s88[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < A2_LEN; j++)
        a2[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < A3_LEN; j++)
        a3[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < D28_LEN; j++)
        d28[j] = (u8)(lcg(&x) >> 16);
    u8 c2050 = (u8)(lcg(&x) >> 16);

    /* 85454 dispatch pins (subset of diff_80085454 classes). */
    switch ((uint32_t)plan & 7u) {
    case 0: memset(s88, 2, A3_LEN); memset(a3, 2, A3_LEN); break;
    case 1: memset(s88, 0, A3_LEN); memset(a3, 2, A3_LEN);
        memset(s54, 0, S54_LEN); memset(a2, 1, A2_LEN); break;
    case 2: memset(s88, 5, A3_LEN); memset(a3, 2, A3_LEN);
        memset(s54, 0x7F, S54_LEN); memset(a2, 0xFF, A2_LEN); break;
    case 3: memset(s88, 1, A3_LEN); break;
    case 4: memset(s88, 7, A3_LEN); break;
    default: break;
    }
    /* 85618 gates + dispatch bytes at A = idx & 0xFF (best effort: the
     * 85454 pass overwrites row A's C3FE8 columns first). */
    uint32_t A = idxw & 0xFFu;
    if ((plan & 8) == 0) {
        for (uint32_t k = 0; k < 11; k++)
            e3[k] |= 0x01u;
    }
    {
        int cp = (plan >> 4) & 15;
        for (uint32_t k = 0; k < 11; k++) {
            uint32_t o = S1_C4000 + 72u * A + k;
            if (cp < 12)
                s1[o] = (u8)cp;
            else if (cp == 12)
                s1[o] = 0xFFu;
        }
        if ((plan & 256) == 0)
            c2050 = 0;
        else
            c2050 |= 0x01u;
    }
    /* the trailing store must be observable: prefill 0xAD nonzero */
    d28[0xAD] = (u8)(d28[0xAD] | 0x01u);

    /* host setup */
    memcpy(H_S1, s1, S1_LEN);
    memcpy(H_CCD, ccd, CCD_LEN);
    memcpy(D_800CCE08, e08, CCE08_LEN);
    memcpy(D_800D2DCC, e3, E3_LEN);
    memcpy(H48, tab48, sizeof(tab48));
    memcpy(H_B301, b301, B301_LEN);
    memcpy(H_SCR2, scr2, SCR2_LEN);
    memcpy(D_800D2C54, s54, S54_LEN);
    memcpy(D_800D2C88, s88, S88_LEN);
    memcpy(D_800D2D70, a2, A2_LEN);
    memcpy(D_800D2D5C, a3, A3_LEN);
    memcpy(H_D2D28, d28, D28_LEN);
    D_800C3EAC = H_SCR2;
    D_800D2D28 = H_D2D28;
    D_800C48E8 = g16(s1 + C48E8_OFF);
    D_800C2050 = c2050;
    /* retail pokes (TAB rides inside S1; pointer words overlay after) */
    if (remu_poke(m, S1_BASE, s1, S1_LEN)) { printf("FAIL poke s1\n"); return 0; }
    if (remu_poke(m, CCD_BASE, ccd, CCD_LEN)) { printf("FAIL poke ccd\n"); return 0; }
    if (remu_poke(m, CCE08_BASE, e08, CCE08_LEN)) { printf("FAIL poke e08\n"); return 0; }
    if (remu_poke(m, E3_BASE, e3, E3_LEN)) { printf("FAIL poke e3\n"); return 0; }
    {
        u8 p48[T48_LEN];
        for (uint32_t j = 0; j < 256; j++) {
            p48[2 * j] = (u8)(tab48[j] & 0xFFu);
            p48[2 * j + 1] = (u8)((tab48[j] >> 8) & 0xFFu);
        }
        if (remu_poke(m, T48_BASE, p48, T48_LEN)) { printf("FAIL poke t48\n"); return 0; }
    }
    {
        u8 jw[48];
        for (int j = 0; j < 12; j++) {
            jw[4 * j] = (u8)(jtblw[j] & 0xFFu);
            jw[4 * j + 1] = (u8)((jtblw[j] >> 8) & 0xFFu);
            jw[4 * j + 2] = (u8)((jtblw[j] >> 16) & 0xFFu);
            jw[4 * j + 3] = (u8)((jtblw[j] >> 24) & 0xFFu);
        }
        if (remu_poke(m, JTBL_BASE, jw, sizeof(jw))) { printf("FAIL poke jtbl\n"); return 0; }
    }
    if (remu_poke(m, S54_BASE, s54, S54_LEN)) { printf("FAIL poke s54\n"); return 0; }
    if (remu_poke(m, S88_BASE, s88, S88_LEN)) { printf("FAIL poke s88\n"); return 0; }
    if (remu_poke(m, A2_BASE, a2, A2_LEN)) { printf("FAIL poke a2\n"); return 0; }
    if (remu_poke(m, A3_BASE, a3, A3_LEN)) { printf("FAIL poke a3\n"); return 0; }
    if (remu_poke(m, SCR2_BASE, scr2, SCR2_LEN)) { printf("FAIL poke scr2\n"); return 0; }
    if (remu_poke(m, C2050_BASE, &c2050, 1)) { printf("FAIL poke c2050\n"); return 0; }
    if (remu_poke(m, B301_BASE, b301, B301_LEN)) { printf("FAIL poke b301\n"); return 0; }
    if (remu_poke(m, D28_BASE, d28, D28_LEN)) { printf("FAIL poke d28\n"); return 0; }
    {
        u8 pw[4] = { (u8)(SCR2_BASE & 0xFFu), (u8)((SCR2_BASE >> 8) & 0xFFu),
                     (u8)((SCR2_BASE >> 16) & 0xFFu),
                     (u8)((SCR2_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, PTRW_BASE, pw, 4)) { printf("FAIL poke ptrw\n"); return 0; }
    }
    {
        u8 pw[4] = { (u8)(D28_BASE & 0xFFu), (u8)((D28_BASE >> 8) & 0xFFu),
                     (u8)((D28_BASE >> 16) & 0xFFu),
                     (u8)((D28_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, D28_PTRW, pw, 4)) { printf("FAIL poke d28ptr\n"); return 0; }
    }

    /* host side */
    func_80085C88((u8)idxw);
    /* mirror the accum scalar into the S1 image: the TU uses the
     * D_800C48E8 scalar while retail accumulates at guest S1[0xA34]. */
    H_S1[C48E8_OFF] = (u8)(D_800C48E8 & 0xFFu);
    H_S1[C48E8_OFF + 1] = (u8)((D_800C48E8 >> 8) & 0xFFu);
    if (H_D2D28[0xAD] != 0) {
        printf("FAIL seed=%08X host d28[AD]=%02X\n", seed, H_D2D28[0xAD]);
        return 0;
    }

    /* retail side: every callee loaded, zero stubs expected */
    int rc = remu_call(m, entry, idxw, 0, 0, 0);
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

    /* direct host-vs-retail span comparison */
    if (remu_peek(m, S1_BASE, s1back, S1_LEN) ||
        memcmp(s1back, H_S1, S1_LIVE) != 0) {
        printf("FAIL seed=%08X retail s1 mismatch\n", seed);
        if (!remu_peek(m, S1_BASE, s1back, S1_LEN))
            for (uint32_t k = 0; k < S1_LIVE; k++)
                if (s1back[k] != H_S1[k]) {
                    printf("  off %u retail=%02X host=%02X\n", k, s1back[k], H_S1[k]);
                    break;
                }
        return 0;
    }
    if (remu_peek(m, CCD_BASE, ccdback, CCD_LEN) ||
        memcmp(ccdback, H_CCD, CCD_LEN) != 0) {
        printf("FAIL seed=%08X retail ccd mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, CCE08_BASE, e08back, CCE08_LEN) ||
        memcmp(e08back, D_800CCE08, CCE08_LEN) != 0) {
        printf("FAIL seed=%08X retail e08 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, E3_BASE, s88back, E3_LEN) ||
        memcmp(s88back, D_800D2DCC, E3_LEN) != 0) {
        printf("FAIL seed=%08X retail e3 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, B301_BASE, b301back, B301_LEN) ||
        memcmp(b301back, H_B301, B301_LEN) != 0) {
        printf("FAIL seed=%08X retail b301 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, SCR2_BASE, scr2back, SCR2_LEN) ||
        memcmp(scr2back, H_SCR2, SCR2_LEN) != 0) {
        printf("FAIL seed=%08X retail scr2 mismatch\n", seed);
        return 0;
    }
    /* S54[52,80) aliases S88[0,28) in guest (0x800D2C54+52 ==
     * 0x800D2C88): both are read-only inputs, so both sides are checked
     * against the poke images (no-write check), not against each other. */
    if (memcmp(D_800D2C54, s54, S54_LEN) != 0) {
        printf("FAIL seed=%08X host wrote s54\n", seed);
        return 0;
    }
    /* guest tail [52,80) holds the later S88 poke, not the s54 image */
    if (remu_peek(m, S54_BASE, s54back, S54_LEN) ||
        memcmp(s54back, s54, 52) != 0 || memcmp(s54back + 52, s88, 28) != 0) {
        printf("FAIL seed=%08X retail wrote s54/s88\n", seed);
        return 0;
    }
    if (remu_peek(m, S88_BASE, s88back, S88_LEN) ||
        memcmp(s88back, D_800D2C88, S88_LEN) != 0) {
        printf("FAIL seed=%08X retail s88 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, A2_BASE, a2back, A2_LEN) ||
        memcmp(a2back, D_800D2D70, A2_LEN) != 0) {
        printf("FAIL seed=%08X retail a2 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, A3_BASE, a3back, A3_LEN) ||
        memcmp(a3back, D_800D2D5C, A3_LEN) != 0) {
        printf("FAIL seed=%08X retail a3 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, D28_BASE, d28back, D28_LEN) ||
        memcmp(d28back, H_D2D28, D28_LEN) != 0) {
        printf("FAIL seed=%08X retail d28 mismatch\n", seed);
        for (uint32_t k = 0; k < D28_LEN; k++)
            if (d28back[k] != H_D2D28[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, d28back[k], H_D2D28[k]);
                break;
            }
        return 0;
    }
    {
        u16 hac;
        if (remu_peek(m, 0x800C48E8u, (u8 *)&hac, 2) || hac != D_800C48E8) {
            printf("FAIL seed=%08X retail acc=%04X want=%04X\n", seed, hac, D_800C48E8);
            return 0;
        }
    }
    {
        u8 hc;
        if (remu_peek(m, C2050_BASE, &hc, 1) || hc != D_800C2050) {
            printf("FAIL seed=%08X retail c2050=%02X want=%02X\n", seed, hc, D_800C2050);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    printf("layout 5C=%p 67=%p\n", (void*)D_800D2D5C, (void*)&D_800D2D67);
    if (&D_800D2D67 != &D_800D2D5C[11]) {
        printf("FAIL host layout: D_800D2D67 != D_800D2D5C+11\n");
        return 1;
    }
    if (((uintptr_t)H_S1 & 3u) != 0 || ((uintptr_t)H_CCD & 3u) != 0 ||
        H_CCD != H_S1 + S1_LIVE ||
        D_800C3EB4 != H_S1 + 0 || D_800C3FE8 != H_S1 + S1_C3FE8 ||
        D_800C4000 != H_S1 + S1_C4000 ||
        D_800CCD34 != H_CCD + 0 || D_800CCD64 != H_CCD + 48 ||
        D_800CCDC4 != H_CCD + 144 || D_800CCDEC != H_CCD + 184 ||
        D_800CCE08 != H_CCD + 212 ||
        D_800D301C != H_B301 + 0 || D_800D32A1 != H_B301 + 645) {
        printf("FAIL host layout\n");
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_85C88);
    if (entry != 0x80085C88u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_s(m, RETAIL_85454) != 0x80085454u) { printf("FAIL load 85454\n"); return 1; }
    if (remu_load_s(m, RETAIL_85618) != 0x80085618u) { printf("FAIL load 85618\n"); return 1; }
    if (remu_load_s(m, RETAIL_89C08) != 0x80089C08u) { printf("FAIL load 89c08\n"); return 1; }
    if (remu_load_s(m, RETAIL_883AC) != 0x800883ACu) { printf("FAIL load 883ac\n"); return 1; }
    if (remu_load_s(m, RETAIL_89C48) != 0x80089C48u) { printf("FAIL load 89c48\n"); return 1; }

    static const uint32_t idxs[] = { 0, 1, 2, 3, 5, 100, 255, 0xFFFFFFFFu };
    uint32_t s = 0x85C81234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(idxs) / sizeof(idxs[0]); i++) {
        for (int mem = 0; mem < 2; mem++) {
            for (int k = 0; k < 8; k++) {
                s = s * 1103515245u + 12345u;
                int plan = (int)((i * 16u + (uint32_t)mem * 8u + (uint32_t)k) % 560u);
                uint32_t idxw = idxs[i] | ((mem & 1) ? 0u : 0xCD00u);
                if (!check_one(m, entry, s, idxw, plan))
                    return 1;
                n++;
            }
        }
    }
    printf("DIFF 80085C88 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

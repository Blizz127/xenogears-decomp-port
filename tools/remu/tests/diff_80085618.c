/* Multi-load differential test: host-compiled src/battle/main36.c
 * func_80085618 vs the retail bytes executed by remu, with callee bodies
 * ALSO loaded as retail code:
 *   asm/battle/nonmatchings/main36/func_80085618.s   (entry)
 *   asm/battle/matchings/main41/func_80089C08.s
 *   asm/battle/nonmatchings/main39/func_800883AC.s
 *   asm/battle/matchings/main41/func_80089C48.s
 * Fails on any behavioral mismatch.
 *
 * func_80085618(a0): for s2 in 0..10, gate on D_800D2DCC[s2] and bit15
 * of D_800CCD64[s2*0x170] (bit15 set: OR func_80089C08(s2) into
 * D_800C48E8); else dispatch on D_800C4000[72*(a0&0xFF)+s2] (< 12)
 * through six row cases (jump table jtbl_80070250, poked below), with
 * a D_800C3EAC[s2+0x2EB] = 1 mark on the fall-through epilogue.
 *
 * Memory model notes (guest-accurate aliasing):
 * - S1 blob [0x800C3EB4, +51468): s5/s3/s7 traffic, C4000 dispatch
 *   bytes, C48E8 accum word, C3EB4/5 tables. S1-live is [0, 0x8E80);
 *   above that the CCD span overlaps (separate pokes, live-checked).
 * - D_800C3EAC is a POINTER variable in the TU: the test points it at
 *   SCR2 scratch and pokes the pointer word at 0x800C3EAC after S1.
 *   D_800C2050/D_800C48E8 are u8/u16 scalars in the TU: host scalars +
 *   ordered pokes; the model tracks them separately.
 * - CCD span [0x800CCD34, +3872): CCD34/36/38/3A/64 + CCDC4/6 + CCDEC/F0
 *   + CCE08 rows overlap in guest, so host backing aliases exactly the
 *   same way: H_CCD = H_S1 + 0x8E80, D_800CCE08 = H_CCD + 212, and the
 *   model keeps one union image where both views alias.
 * - s4 stores (s4+0x8F3C/0x8E84/0x8E88/0x8F14) land at CCD-coords
 *   184/0/4/144 (+s0) in the CCD span, NOT in any R9 span; the model
 *   writes them into the union image, and the host TU writes them
 *   through the aliased backing.
 * - B301 blob [0x800D301C, 0x800D3A9A): 883AC's D301C/D + D32A1 overlap.
 * - jtbl_80070250 (12 words) is retail rodata: remu starts zeroed, so
 *   the test pokes the true table contents (taken from 0.rodata.s).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085618 src/battle/main36.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085618(s32 a0);

/* ---- TU externs: blobs, aliases, scalars, pointer ---- */
#define CCE08_BASE 0x800CCE08u
#define CCE08_LEN (368u * 10u + 2u)
#define E3_BASE 0x800D2DCCu
#define E3_LEN 256u
#define T48_BASE 0x800C3448u
#define T48_LEN 512u
#define SCR2_BASE 0x801F0400u
#define SCR2_LEN 0x300u
#define S1_BASE 0x800C3EB4u
#define S1_LEN (0x800D07C0u - 0x800C3EB4u)
#define S1_LIVE 0x8E80u /* S1-meaning accesses stay below this; at and
 * above lies the CCD span (H_CCD alias), verified via the union image */
#define CCD_BASE 0x800CCD34u
#define CCD_LEN (0x800CDC54u - 0x800CCD34u)
/* guest-unified CCD/e08 image: e08 (off 212, len 3682) is poked after CCD
 * and aliases it, so the model keeps one array where both views alias,
 * exactly like guest memory. Union covers to e08 end. */
#define E08_IN_CCD 212u
#define UNI_LEN (212u + 3682u)
_Static_assert(0x800CCE08u - CCD_BASE == E08_IN_CCD, "e08 overlap offset");
_Static_assert(CCD_LEN < UNI_LEN &&
               CCE08_BASE + CCE08_LEN - CCD_BASE == UNI_LEN, "union extent");
__asm__(
".pushsection .bss,\"aw\",@nobits\n"
".p2align 2\n"
".globl H_S1\nH_S1:\n.space 51468\n"
".globl D_800C3EB4\nD_800C3EB4 = H_S1 + 0\n"
".globl D_800C3EB5\nD_800C3EB5 = H_S1 + 1\n"
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
extern u8 H_B301[];
extern u8 D_800D301C[];
extern u8 D_800D301D[];
extern u8 D_800D32A1[];
extern u8 D_800CCE08[];
extern u8 H_CCD[];
u8 D_800D2DCC[256];
u8 *D_800C3EAC;
u16 D_800C48E8;
u8 D_800C2050;
__attribute__((aligned(2))) u8 H_SCR2[SCR2_LEN];

#define C48E8_OFF (0x800C48E8u - 0x800C3EB4u)
#define C2050_BASE 0x800C2050u
#define C3EB4_OFF 0u
#define C3FE8_OFF (0x800C3FE8u - 0x800C3EB4u)
#define C4000_OFF (0x800C4000u - 0x800C3EB4u)
#define B301_LEN (0x800D3A9Au - 0x800D301Cu)
_Static_assert(S1_LEN == 51468, "S1 size baked into H_S1 asm");
_Static_assert(CCD_LEN == 3872, "CCD span size");
_Static_assert(UNI_LEN == 3894, "union covers CCD + e08 tail");
_Static_assert(S1_LIVE == 0x8E80u, "CCD overlap starts at S1-live end");
_Static_assert(B301_LEN == 2686, "B301 size baked in");
_Static_assert(C3FE8_OFF == 308 && C4000_OFF == 332, "S1 alias offsets");
_Static_assert((0x800CCDC4u - 0x800CCD34u) == 144 &&
               (0x800CCDEC - 0x800CCD34u) == 184 &&
               (0x800CCDF0 - 0x800CCD34u) == 188, "CCD alias offsets");
_Static_assert((0x800D32A1u - 0x800D301Cu) == 645, "B301 alias offset");
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

#define RETAIL_85618 "asm/battle/nonmatchings/main36/func_80085618.s"
#define RETAIL_89C08 "asm/battle/matchings/main41/func_80089C08.s"
#define RETAIL_883AC "asm/battle/nonmatchings/main39/func_800883AC.s"
#define RETAIL_89C48 "asm/battle/matchings/main41/func_80089C48.s"
#define JTBL_BASE 0x80070250u
#define PTRW_BASE 0x800C3EACu
#define SCR2_BASE 0x801F0400u
#define SCR2_LEN 0x300u
#define B301_BASE 0x800D301Cu
#define B301_LEN (0x800D3A9Au - 0x800D301Cu)
/* H_S1 spans S1-retail; H_CCD aliases H_S1 + S1_LIVE exactly like the
 * guest CCD span overlaps the S1 span tail. */
_Static_assert(S1_LIVE + UNI_LEN <= S1_LEN, "CCD alias fits in H_S1");

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

/* jtbl_80070250 contents (asm/battle/data/0.rodata.s), poked (remu starts
 * zeroed, and the dispatch reads the real table). */
static const uint32_t jtblw[12] = {
    0x8008570Cu, 0x80085908u, 0x8008583Cu, 0x80085954u,
    0x80085A70u, 0x8008570Cu, 0x80085A70u, 0x8008570Cu,
    0x8008570Cu, 0x80085908u, 0x800859CCu, 0x80085A18u,
};

static u16 g16(const u8 *p) { return (u16)(p[0] | ((u16)p[1] << 8)); }
static void s16f(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xFFu);
    p[1] = (u8)((v >> 8) & 0xFFu);
}
static u32 g32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}
static void s32f(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xFFu);
    p[1] = (u8)((v >> 8) & 0xFFu);
    p[2] = (u8)((v >> 16) & 0xFFu);
    p[3] = (u8)((v >> 24) & 0xFFu);
}

static u8 h_scr2[SCR2_LEN];

/* independent 883AC model (direct formulation; s1w is the S1 work image
 * whose base IS D_800C3EB4, so table reads are plain offsets) */
static void model_883AC(const u8 *s1w, u8 *b301, const u16 *t48, uint32_t a0) {
    uint32_t t = a0 & 0xFFu;
    uint32_t s1v = ((t >= 3u) ? 8u : 0u) | (b301[0x285u + t * 8u] ? 0x10u : 0u);
    uint32_t o28 = t * 28u;
    uint32_t b4 = s1w[o28];
    uint32_t jx = (b4 + (s1v & 0xFFu)) << 2;
    b301[jx] = (u8)(b301[jx] - 1u);
    uint32_t m = 0xFFFFu ^ (uint32_t)t48[s1w[o28 + 1]];
    b301[jx + 1] = (u8)(b301[jx + 1] & m);
}

/* S1-relative offsets used by the model (match the TU pointer setup) */
#define S1_C3FE8 308u
#define S1_C4000 332u

static u16 acc_init(const u8 *s1) { return g16(s1 + C48E8_OFF); }

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a0w,
                     int plan) {
    uint32_t x = seed ^ 0x8561879u;
    static u8 s1[S1_LEN];
    static u8 s1e[S1_LEN];
    static u8 s1back[S1_LEN];
    static u8 ccd[CCD_LEN];
    static u8 ccdback[CCD_LEN];
    static u8 e08[CCE08_LEN];
    static u8 e08back[CCE08_LEN];
    static u8 uni[UNI_LEN];
    u8 *ccde = uni;
    u8 *e08e = uni + E08_IN_CCD;
    static u8 e3[E3_LEN];
    static u16 tab48[256];
    static u8 b301[B301_LEN];
    static u8 b301e[B301_LEN];
    static u8 b301back[B301_LEN];
    static u8 scr2[SCR2_LEN];
    static u8 scr2e[SCR2_LEN];
    static u8 scr2back[SCR2_LEN];
    /* NOTE: no R9 span exists for this function; the s4 stores land in
     * the CCD span (union image). R9_BASE belongs to sibling code. */
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
    u8 c2050 = (u8)(lcg(&x) >> 16);

    /* forcing: D2DCC gates + dispatch bytes + C2050 */
    if ((plan & 1) == 0) {
        for (uint32_t k = 0; k < 11; k++)
            e3[k] |= 0x01u;
    }
    {
        int cp = (plan >> 1) & 15;
        for (uint32_t k = 0; k < 11; k++) {
            uint32_t o = S1_C4000 + 72u * (a0w & 0xFFu) + k;
            if (cp < 12)
                s1[o] = (u8)cp;
            else if (cp == 12)
                s1[o] = 0xFFu;
        }
        if ((plan & 16) == 0)
            c2050 = 0;
        else
            c2050 |= 0x01u;
    }

    /* independent expectation over work images */
    uint32_t A = a0w & 0xFFu;
    uint32_t fp = 72u * A;
    memcpy(s1e, s1, S1_LEN);
    /* poke order CCD-then-e08: e08 fill wins the overlap, tail included */
    memcpy(ccde, ccd, CCD_LEN);
    memcpy(e08e, e08, CCE08_LEN);
    memcpy(b301e, b301, B301_LEN);
    memcpy(scr2e, scr2, SCR2_LEN);
    /* NOTE: the D_800C3EAC pointer word lives at guest 0x800C3EAC, 8 bytes
     * below S1_BASE: it is outside the S1 span, set only by the PTRW poke,
     * and retail only reads it. No s1e bytes correspond to it. */
    u16 acc = g16(s1 + C48E8_OFF);
    u8 c2050e = c2050;
    for (uint32_t s2 = 0; s2 < 11; s2++) {
        uint32_t s0 = 0x170u * s2;
        uint32_t s3 = S1_C3FE8 + fp + 2u * s2;
        uint32_t s5 = 4u + 0x1Cu * s2;
        uint32_t s6 = 0x170u * s2;
        int mark = 0;
        if (e3[s2] != 0) {
            u16 h = g16(ccde + 0x30u + s0);
            if ((h & 0x8000u) != 0) {
                /* OR-acc path jumps straight to the no-mark tail. */
                acc |= tab48[s2];
                s16f(s1e + C48E8_OFF, acc);
            } else {
                u8 d = s1e[S1_C4000 + fp + s2];
                if (d < 12) {
                    switch (d) {
                    case 0:
                    case 5:
                    case 7:
                    case 8:
                        if (s1e[s5] != 0) {
                            u16 y = g16(s1e + s3);
                            u32 xx = g32(ccde + 184u + s0);
                            u32 diff = xx - y;
                            if ((int32_t)diff > 0) {
                                /* MIN store jumps to the mark tail: no call. */
                                s32f(ccde + 184u + s0, diff);
                                mark = 1;
                                break;
                            } else {
                                s32f(ccde + 184u + s0, 0);
                                acc |= tab48[s2];
                                s16f(s1e + C48E8_OFF, acc);
                                s16f(e08e + s0, g16(e08e + s0) | 0x8000u);
                                s16f(ccde + 0x30u + s0, h | 0x8000u);
                            }
                        } else {
                            int16_t xx = (int16_t)g16(ccde + s0);
                            int16_t yy = (int16_t)g16(s1e + s3);
                            int32_t diff = (int32_t)xx - (int32_t)yy;
                            if (diff > 0) {
                                /* MIN store jumps to the mark tail: no call. */
                                s16f(ccde + s0, (u16)diff);
                                mark = 1;
                                break;
                            } else {
                                s16f(ccde + s0, 0);
                                acc |= tab48[s2];
                                s16f(s1e + C48E8_OFF, acc);
                                s16f(ccde + 0x30u + s0, h | 0x8000u);
                            }
                        }
                        if (s2 >= 3) {
                            model_883AC(s1e, b301e, tab48, s2);
                        }
                        mark = 1;
                        break;
                    case 1:
                    case 9: {
                        int16_t xx = (int16_t)g16(ccde + 4u + s0);
                        int16_t yy = (int16_t)g16(s1e + s3);
                        int32_t diff = (int32_t)xx - (int32_t)yy;
                        if (diff > 0) {
                            s16f(ccde + 4u + s0, (u16)diff);
                        } else {
                            s16f(ccde + 4u + s0, 0);
                        }
                        break;
                    }
                    case 2:
                        if (s1e[s5] != 0 && c2050e == 0) {
                            u16 y = g16(s1e + s3);
                            u32 xx = g32(ccde + 184u + s0);
                            u32 v = xx + y;
                            /* s4+0x8F3C lands at CCD-coords 184+s0 */
                            s32f(ccde + 184u + s0, v);
                            if (g32(ccde + 188u + s0) < v) {
                                s32f(ccde + 184u + s0,
                                     g32(ccde + 188u + s0));
                            }
                        } else {
                            u16 xx = g16(ccde + s0);
                            u16 y = g16(s1e + s3);
                            u16 v = (u16)(xx + y);
                            /* s4+0x8E84 lands at CCD-coords s0 */
                            s16f(ccde + s0, v);
                            if (g16(ccde + 2u + s0) < v) {
                                s16f(ccde + s0, g16(ccde + 2u + s0));
                            }
                        }
                        mark = 1;
                        break;
                    case 3:
                        if (s1e[s5] == 0 || c2050e != 0) {
                            u16 xx = g16(ccde + 4u + s0);
                            u16 y = g16(s1e + s3);
                            u16 v = (u16)(xx + y);
                            /* s4+0x8E88 lands at CCD-coords 4+s0 */
                            s16f(ccde + 4u + s0, v);
                            if (g16(ccde + 6u + s0) < v) {
                                s16f(ccde + 4u + s0, g16(ccde + 6u + s0));
                            }
                        }
                        break;
                    case 10: {
                        u16 y = g16(ccde + 144u + s6);
                        u16 xx = g16(s1e + s3);
                        u32 diff = (u32)y - (u32)xx;
                        if ((int32_t)diff > 0) {
                            s16f(ccde + 144u + s0, (u16)diff);
                        } else {
                            s16f(ccde + 144u + s0, 0);
                        }
                        mark = 1;
                        break;
                    }
                    case 11: {
                        u16 v = (u16)(g16(ccde + 144u + s6) + g16(s1e + s3));
                        /* s4+0x8F14 lands at CCD-coords 144+s0 */
                        s16f(ccde + 144u + s0, v);
                        if (g16(ccde + 146u + s0) < v) {
                            s16f(ccde + 144u + s0, g16(ccde + 146u + s0));
                        }
                        mark = 1;
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }
        if (mark) {
            scr2e[0x2EBu + s2] = 1;
        }
    }

    /* host setup: blobs, arrays, scalars, pointer, table */
    memcpy(H_S1, s1, S1_LEN);
    /* H_CCD aliases H_S1 + S1_LIVE; order mirrors guest poke order */
    memcpy(H_CCD, ccd, CCD_LEN);
    memcpy(D_800CCE08, e08, CCE08_LEN);
    memcpy(D_800D2DCC, e3, E3_LEN);
    memcpy(H48, tab48, sizeof(tab48));
    memcpy(H_B301, b301, B301_LEN);
    memcpy(h_scr2, scr2, SCR2_LEN);
    D_800C3EAC = h_scr2;
    D_800C48E8 = acc_init(s1);
    D_800C2050 = c2050;
    /* retail pokes (S1 first; the pointer word overlays it) */
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
    if (remu_poke(m, SCR2_BASE, scr2, SCR2_LEN)) { printf("FAIL poke scr2\n"); return 0; }
    if (remu_poke(m, C2050_BASE, &c2050, 1)) { printf("FAIL poke c2050\n"); return 0; }
    if (remu_poke(m, B301_BASE, b301, B301_LEN)) { printf("FAIL poke b301\n"); return 0; }
    {
        u8 pw[4] = { (u8)(SCR2_BASE & 0xFFu), (u8)((SCR2_BASE >> 8) & 0xFFu),
                     (u8)((SCR2_BASE >> 16) & 0xFFu),
                     (u8)((SCR2_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, PTRW_BASE, pw, 4)) { printf("FAIL poke ptrw\n"); return 0; }
    }

    /* host side. The [0xA34,0xA36) accum-scalar hole is dead on host:
     * the TU reads the D_800C48E8 scalar, never these bytes. */
    func_80085618((s32)a0w);
    {
        static const uint32_t lo[] = { 0, 0xA36u };
        static const uint32_t hi[] = { 0xA34u, S1_LIVE };
        for (int r = 0; r < 2; r++) {
            if (memcmp(H_S1 + lo[r], s1e + lo[r], hi[r] - lo[r]) != 0) {
                printf("FAIL seed=%08X host s1 mismatch range %u\n", seed, r);
                for (uint32_t k = lo[r]; k < hi[r]; k++)
                    if (H_S1[k] != s1e[k]) {
                        printf("  off %u host=%02X want=%02X\n", k, H_S1[k], s1e[k]);
                        break;
                    }
                return 0;
            }
        }
    }
    /* host backing aliases exactly like guest (D_800CCE08 = H_CCD + 212),
     * so the unified model image compares whole. */
    if (memcmp(H_CCD, ccde, UNI_LEN) != 0) {
        printf("FAIL seed=%08X host ccd mismatch\n", seed);
        for (uint32_t k = 0; k < UNI_LEN; k++)
            if (H_CCD[k] != ccde[k]) {
                printf("  off %u host=%02X want=%02X\n", k, H_CCD[k], ccde[k]);
                break;
            }
        return 0;
    }
    if (memcmp(D_800CCE08, e08e, CCE08_LEN) != 0) {
        printf("FAIL seed=%08X host e08 mismatch\n", seed);
        return 0;
    }
    if (memcmp(H_B301, b301e, B301_LEN) != 0) {
        printf("FAIL seed=%08X host b301 mismatch\n", seed);
        for (uint32_t k = 0; k < B301_LEN; k++)
            if (H_B301[k] != b301e[k]) {
                printf("  off %u host=%02X want=%02X\n", k, H_B301[k], b301e[k]);
                break;
            }
        return 0;
    }
    if (memcmp(h_scr2, scr2e, SCR2_LEN) != 0) {
        printf("FAIL seed=%08X host scr2 mismatch\n", seed);
        for (uint32_t k = 0; k < SCR2_LEN; k++)
            if (h_scr2[k] != scr2e[k]) {
                printf("  off %u host=%02X want=%02X\n", k, h_scr2[k], scr2e[k]);
                break;
            }
        return 0;
    }
    if (D_800C48E8 != acc) {
        printf("FAIL seed=%08X host acc=%04X want=%04X\n", seed, D_800C48E8, acc);
        return 0;
    }

    /* retail side: every callee loaded, zero stubs expected */
    int rc = remu_call(m, entry, a0w, 0, 0, 0);
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
    if (remu_peek(m, S1_BASE, s1back, S1_LEN) ||
        memcmp(s1back, s1e, S1_LIVE) != 0) {
        printf("FAIL seed=%08X retail s1 mismatch\n", seed);
        if (!remu_peek(m, S1_BASE, s1back, S1_LEN))
            for (uint32_t k = 0; k < S1_LIVE; k++)
                if (s1back[k] != s1e[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, s1back[k], s1e[k]);
                    break;
                }
        return 0;
    }
    {
        /* the pointer word is outside the S1 span; retail only reads it */
        u8 pwback[4];
        static const u8 pw[] = { (u8)(SCR2_BASE & 0xFFu),
                                 (u8)((SCR2_BASE >> 8) & 0xFFu),
                                 (u8)((SCR2_BASE >> 16) & 0xFFu),
                                 (u8)((SCR2_BASE >> 24) & 0xFFu) };
        if (remu_peek(m, PTRW_BASE, pwback, 4) ||
            memcmp(pwback, pw, 4) != 0) {
            printf("FAIL seed=%08X retail ptrw changed\n", seed);
            return 0;
        }
    }
    if (remu_peek(m, CCD_BASE, ccdback, CCD_LEN) ||
        memcmp(ccdback, ccde, CCD_LEN) != 0) {
        printf("FAIL seed=%08X retail ccd mismatch\n", seed);
        if (!remu_peek(m, CCD_BASE, ccdback, CCD_LEN)) {
            int n = 0;
            for (uint32_t k = 0; k < CCD_LEN && n < 12; k++)
                if (ccdback[k] != ccde[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, ccdback[k], ccde[k]);
                    n++;
                }
        }
        return 0;
    }
    if (remu_peek(m, CCE08_BASE, e08back, CCE08_LEN) ||
        memcmp(e08back, e08e, CCE08_LEN) != 0) {
        printf("FAIL seed=%08X retail e08 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, B301_BASE, b301back, B301_LEN) ||
        memcmp(b301back, b301e, B301_LEN) != 0) {
        printf("FAIL seed=%08X retail b301 mismatch\n", seed);
        if (!remu_peek(m, B301_BASE, b301back, B301_LEN))
            for (uint32_t k = 0; k < B301_LEN; k++)
                if (b301back[k] != b301e[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, b301back[k], b301e[k]);
                    break;
                }
        return 0;
    }
    if (remu_peek(m, SCR2_BASE, scr2back, SCR2_LEN) ||
        memcmp(scr2back, scr2e, SCR2_LEN) != 0) {
        printf("FAIL seed=%08X retail scr2 mismatch\n", seed);
        for (uint32_t k = 0; k < SCR2_LEN; k++)
            if (scr2back[k] != scr2e[k]) {
                printf("  off %u retail=%02X want=%02X\n", k, scr2back[k], scr2e[k]);
                break;
            }
        return 0;
    }
    return 1;
}

int main(void) {
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
    uint32_t entry = remu_load_s(m, RETAIL_85618);
    if (entry != 0x80085618u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_s(m, RETAIL_89C08) != 0x80089C08u) { printf("FAIL load 89c08\n"); return 1; }
    if (remu_load_s(m, RETAIL_883AC) != 0x800883ACu) { printf("FAIL load 883ac\n"); return 1; }
    if (remu_load_s(m, RETAIL_89C48) != 0x80089C48u) { printf("FAIL load 89c48\n"); return 1; }
    static const uint32_t avec[] = { 0, 1, 2, 5, 6, 7, 10, 64, 128, 200, 255 };
    uint32_t s = 0x85618234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(avec) / sizeof(avec[0]); i++) {
        for (int mem = 0; mem < 2; mem++) {
            s = s * 1103515245u + 12345u;
            uint32_t seed = s;
            int plan = (int)((i * 2u + (uint32_t)mem) % 28u);
            uint32_t a0w = avec[i] | ((mem & 1) ? 0u : 0xAB00u);
            if (!check_one(m, entry, seed, a0w, plan))
                return 1;
            n++;
        }
    }
    printf("DIFF 80085618 OK (%d seeds, retail-exact)\n", n);
    return 0;
}

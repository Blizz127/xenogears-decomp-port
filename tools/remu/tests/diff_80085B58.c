/* Multi-load differential test: host-compiled src/battle/main36.c
 * func_80085B58 vs the retail bytes executed by remu. Callees:
 * - func_80085388 / func_80085618: real TUs on host, retail .s + their
 *   callees (89C08/883AC/89C48) loaded on retail.
 * - func_8009ADA0: hand-written host replica + independent model (same
 *   logic, shared body) vs retail .s. NOT yet transcribed (main56.c);
 *   the replica is verified transitively by this test.
 * - func_800BE538: tail call with a deep subtree (polling loop): host
 *   recorder + retail logging stub (be538_log.s) + model prediction of
 *   presence (0/1) and args. Its real effects belong to mainc122.c.
 * Fails on any behavioral mismatch.
 *
 * func_80085B58(a0): w[3] zeroed on stack; r = 9ADA0(a0&0xFF, w) (only
 * a0/a1 set: a2 passes caller residue through, 0 under the test on both
 * sides); r == 0 returns; else for s1 in 0..2, when w[s1] != 0: zero
 * *(D_800C3EAC+0x2DA), call 85388, D_800C4000[s2] = s1+8, copy w[s1] low
 * half to D_800C3FE8+2*s2, call 85618 with the 0x2DA byte (jal delay
 * is nop); then tail
 * call BE538(s4&0xFF, w[0..2]). v0 residue ignored by the only caller.
 *
 * Proof shape: host blobs vs retail spans compared DIRECTLY (same TU
 * logic both sides; any orchestration divergence perturbs spans), plus
 * a small model predicting 9ADA0 return + w words + BE538 presence/args
 * (stack-local state invisible otherwise). No 85388/85618/883AC model:
 * their spans are covered by direct comparison.
 *
 * Guest-accurate aliasing (copied from diff_80085618): H_CCD aliases
 * H_S1 + S1_LIVE, D_800CCE08 aliases H_CCD + 212, s4 stores land in the
 * CCD span (no R9 span exists here). D_800C34B0 pointer word lives at
 * guest 0x800C34B0 inside the T48 range: poked after T48, and the test
 * forces 883AC mask bytes away from index 52 so neither side reads the
 * clobbered T48[52] (89C08 takes s2 <= 10 only).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085B58 src/battle/main36.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "retail_oracle.h"
#include "common.h"
void func_80085B58(s32 a0, u32 a2_residue);

/* ---- spans ---- */
#define S1_BASE 0x800C3EB4u
#define S1_LEN (0x800D07C0u - 0x800C3EB4u)
#define S1_LIVE 0x8E80u /* S1-meaning accesses stay below this; at and
 * above lies the CCD span (H_CCD alias), verified via the union image */
#define S1_C3FE8 308u
#define S1_C4000 332u
#define C48E8_OFF (0x800C48E8u - 0x800C3EB4u)
#define CCD_BASE 0x800CCD34u
#define CCD_LEN (0x800CDC54u - 0x800CCD34u)
/* guest-unified CCD/e08 image: e08 (off 212, len 3682) is poked after CCD
 * and aliases it: host backing aliases the same way (H_CCD = H_S1 +
 * S1_LIVE, D_800CCE08 = H_CCD + 212). Union covers to e08 end. */
#define CCE08_BASE 0x800CCE08u
#define CCE08_LEN (368u * 10u + 2u)
#define E08_IN_CCD 212u
#define UNI_LEN (212u + 3682u)
_Static_assert(0x800CCE08u - CCD_BASE == E08_IN_CCD, "e08 overlap offset");
_Static_assert(CCD_LEN < UNI_LEN &&
               CCE08_BASE + CCE08_LEN - CCD_BASE == UNI_LEN, "union extent");
_Static_assert(S1_LIVE + UNI_LEN <= S1_LEN, "CCD alias fits in H_S1");
_Static_assert(S1_LIVE == 0x8E80u, "CCD overlap starts at S1-live end");
#define E3_BASE 0x800D2DCCu
#define E3_LEN 256u
#define T48_BASE 0x800C3448u
#define T48_LEN 512u
#define T48_PTR_OFF 104u /* D_800C34B0 word inside T48 range, poked after */
#define SCR2_BASE 0x801F0400u
#define SCR2_LEN 0x300u
#define C2050_BASE 0x800C2050u
#define B301_BASE 0x800D301Cu
#define B301_LEN (0x800D3A9Au - 0x800D301Cu)
#define PTRW_BASE 0x800C3EACu
#define JTBL_BASE 0x80070250u
/* 9ADA0 row table: rows are 368 bytes, u8 index, max read row+0xDE */
#define T34_BASE 0x80100000u
#define T34_STRIDE 368u
#define T34_LEN (256u * 368u + 0xE0u)
#define T34_PTRW 0x800C34B0u
/* BE538 logging-stub scratch */
#define BES_BASE 0x80070000u
#define BES_LEN 64u
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
".globl H_T34\nH_T34:\n.space 94432\n"
".popsection\n"
);
extern u8 H_S1[];
extern u8 H_CCD[];
extern u8 H_B301[];
extern u8 H_T34[];
extern u8 D_800C3EB4[];
extern u8 D_800C3EB5[];
extern u8 D_800C3FE8[];
extern u8 D_800C4000[];
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
extern u8 D_800D301C[];
extern u8 D_800D301D[];
extern u8 D_800D32A1[];
u8 *D_800C34B0;
u8 D_800D2DCC[256];
u8 *D_800C3EAC;
u16 D_800C48E8;
u8 D_800C2050;
__attribute__((aligned(2))) u8 H_SCR2[SCR2_LEN];
__attribute__((aligned(2))) u16 H48[256];
_Static_assert(S1_LEN == 51468, "S1 size baked into H_S1 asm");
_Static_assert(T34_LEN == 94432, "T34 blob size baked in");
_Static_assert(B301_LEN == 2686, "B301 size baked in");
_Static_assert(C48E8_OFF == 2612, "C48E8 S1 offset");

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}
static u16 g16(const u8 *p) { return (u16)(p[0] | ((u16)p[1] << 8)); }
static void s16f(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xFFu);
    p[1] = (u8)((v >> 8) & 0xFFu);
}

/* ---- host replicas (frozen copies of transcribed bodies) ---- */
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

/* 9ADA0 shared body: table base + out words + row index + entry-a2 flag.
 * Replicates func_8009ADA0.s exactly (multu/mfhi/srl sequences are the
 * bit-exact u64 formula). The TU snapshots entry-rdx and passes it
 * explicitly (pinned 0 by the test's asm call); retail $a2 flows through
 * untouched (remu a2 = 0). */
static u32 r9a_body(const u8 *t34, u32 *w, u32 t, u32 a2in) {
    const u8 *row = t34 + t * T34_STRIDE;
    u16 h7c = g16(row + 0x7C);
    if (h7c & 0x8000u)
        return 0;
    u32 a2 = a2in;
    if (h7c & 0x0800u) {
        w[0] = (u32)(((u64)g16(row + 0x4E) * 0xCCCCCCCDull >> 32) >> 4) & 0xFFFFu;
        a2 = 1;
    }
    if (g16(row + 0x80) & 0x0200u) {
        w[1] = (u32)(((u64)g16(row + 0x52) * 0xCCCCCCCDull >> 32) >> 4) & 0xFFFFu;
        a2 = 1;
    }
    if (g16(row + 0x120) & 0x0200u) {
        w[1] = (u32)(((u64)g16(row + 0x52) * 0xCCCCCCCDull >> 32) >> 4) & 0xFFFFu;
        a2 = 1;
    }
    w[2] = 0;
    if (g16(row + 0x120) & 0x0080u) {
        w[2] = (u32)(((u64)g16(row + 0xDE) * 0x51EB851Full >> 32) >> 4) & 0xFFFFu;
        a2 = 1;
    }
    if (g16(row + 0x124) & 0x8000u) {
        w[2] = (w[2] + (u32)(((u64)g16(row + 0xDE) * 0x51EB851Full >> 32) >> 4)) & 0xFFFFu;
        a2 = 1;
    }
    return a2;
}

u32 func_8009ADA0(u8 idx, u32 *out, u32 a2in) {
    return r9a_body(H_T34, out, (u32)idx & 0xFFu, a2in);
}

/* BE538 host recorder (real effects belong to mainc122.c) */
static u32 be_r_a0, be_r_a1, be_r_a2, be_r_a3;
static int be_r_n;
void func_800BE538(u8 a0, u32 a1, u32 a2, u32 a3) {
    be_r_n++;
    be_r_a0 = a0;
    be_r_a1 = a1;
    be_r_a2 = a2;
    be_r_a3 = a3;
}

/* jtbl_80070250 contents (asm/battle/data/0.rodata.s), poked (remu starts
 * zeroed, and the dispatch reads the real table). */
static const uint32_t jtblw[12] = {
    0x8008570Cu, 0x80085908u, 0x8008583Cu, 0x80085954u,
    0x80085A70u, 0x8008570Cu, 0x80085A70u, 0x8008570Cu,
    0x8008570Cu, 0x80085908u, 0x800859CCu, 0x80085A18u,
};

#define RETAIL_85B58 "asm/battle/nonmatchings/main36/func_80085B58.s"
#define RETAIL_85618 "asm/battle/nonmatchings/main36/func_80085618.s"
#define RETAIL_85388 "asm/battle/nonmatchings/main36/func_80085388.s"
#define RETAIL_89C08 "asm/battle/matchings/main41/func_80089C08.s"
#define RETAIL_883AC "asm/battle/nonmatchings/main39/func_800883AC.s"
#define RETAIL_89C48 "asm/battle/matchings/main41/func_80089C48.s"
#define RETAIL_9ADA0 "asm/battle/nonmatchings/main56/func_8009ADA0.s"
#define RETAIL_BE538 "tools/remu/tests/be538_log.s"

static u16 acc_init(const u8 *s1) { return g16(s1 + C48E8_OFF); }

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a0w,
                     int plan) {
    uint32_t x = seed ^ 0x85B5879u;
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
    static u8 t34[T34_LEN];
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
    for (uint32_t j = 0; j < T34_LEN; j++)
        t34[j] = (u8)(lcg(&x) >> 16);
    u8 c2050 = (u8)(lcg(&x) >> 16);

    /* forcing: 85618 gates + dispatch bytes + C2050 + mask-index guard */
    if ((plan & 1) == 0) {
        for (uint32_t k = 0; k < 11; k++)
            e3[k] |= 0x01u;
    }
    {
        /* 85618 always runs with arg 0 here (0x2DA zeroed): fp = 0 */
        int cp = (plan >> 1) & 15;
        for (uint32_t k = 0; k < 11; k++) {
            uint32_t o = S1_C4000 + k;
            if (cp < 12)
                s1[o] = (u8)cp;
            else if (cp == 12)
                s1[o] = 0xFFu;
        }
        if ((plan & 16) == 0)
            c2050 = 0;
        else
            c2050 |= 0x01u;
        /* keep 883AC mask reads off the T48 pointer-word slot */
        for (uint32_t t = 0; t <= 10; t++)
            if (s1[28u * t + 1] == 52)
                s1[28u * t + 1] = 51;
    }
    /* forcing: 9ADA0 row for A = a0w & 0xFF */
    uint32_t A = a0w & 0xFFu;
    uint32_t q9 = (uint32_t)plan % 4u;
    {
        u8 *row = t34 + A * T34_STRIDE;
        u16 f7c = g16(row + 0x7C);
        u16 f80 = g16(row + 0x80);
        u16 f20 = g16(row + 0x120);
        u16 f24 = g16(row + 0x124);
        if (q9 == 0) {
            f7c |= 0x8000u;
        } else if (q9 == 1) {
            f7c = (u16)((f7c & ~0x8000u) | 0x0800u);
            f80 |= 0x0200u;
            f20 &= (u16)~(0x0200u | 0x0080u);
            f24 &= (u16)~0x8000u;
        } else if (q9 == 2) {
            f7c &= (u16)~(0x8000u | 0x0800u);
            f80 &= (u16)~0x0200u;
            f20 |= 0x0200u | 0x0080u;
            f24 |= 0x8000u;
        } else {
            f7c &= (u16)~(0x8000u | 0x0800u);
            f80 &= (u16)~0x0200u;
            f20 &= (u16)~(0x0200u | 0x0080u);
            f24 &= (u16)~0x8000u;
        }
        s16f(row + 0x7C, f7c);
        s16f(row + 0x80, f80);
        s16f(row + 0x120, f20);
        s16f(row + 0x124, f24);
    }

    /* model: 9ADA0 return + w words + BE538 presence/args */
    u32 mw[3] = { 0u, 0u, 0u };
    u32 mret = r9a_body(t34, mw, A, 0);
    int mbe = (mret != 0) ? 1 : 0;

    /* host setup: blobs, arrays, scalars, pointers, recorder */
    memcpy(H_S1, s1, S1_LEN);
    /* H_CCD aliases H_S1 + S1_LIVE; order mirrors guest poke order */
    memcpy(H_CCD, ccd, CCD_LEN);
    memcpy(D_800CCE08, e08, CCE08_LEN);
    memcpy(D_800D2DCC, e3, E3_LEN);
    memcpy(H48, tab48, sizeof(tab48));
    memcpy(H_B301, b301, B301_LEN);
    memcpy(H_SCR2, scr2, SCR2_LEN);
    memcpy(H_T34, t34, T34_LEN);
    D_800C3EAC = H_SCR2;
    D_800C34B0 = H_T34;
    D_800C48E8 = acc_init(s1);
    D_800C2050 = c2050;
    be_r_n = 0;
    be_r_a0 = be_r_a1 = be_r_a2 = be_r_a3 = 0xDDDDDDDDu;
    /* retail pokes (S1 first; pointer word + T48x + C2050 overlay it) */
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
    if (remu_poke(m, T34_BASE, t34, T34_LEN)) { printf("FAIL poke t34\n"); return 0; }
    {
        u8 pw[4] = { (u8)(T34_BASE & 0xFFu), (u8)((T34_BASE >> 8) & 0xFFu),
                     (u8)((T34_BASE >> 16) & 0xFFu),
                     (u8)((T34_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, T34_PTRW, pw, 4)) { printf("FAIL poke t34ptr\n"); return 0; }
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
    {
        u8 z[BES_LEN];
        memset(z, 0, sizeof z);
        if (remu_poke(m, BES_BASE, z, sizeof z)) { printf("FAIL poke bes\n"); return 0; }
    }

    /* Explicit adapter input mirrors remu's zero entry a2. */
    func_80085B58(a0w, 0);

    /* mirror the accum scalar into the S1-live expect image: the TU
     * reads/writes the D_800C48E8 scalar, never H_S1[0xA34], while
     * retail reads/writes guest S1[0xA34] in place. */
    H_S1[C48E8_OFF] = (u8)(D_800C48E8 & 0xFFu);
    H_S1[C48E8_OFF + 1] = (u8)((D_800C48E8 >> 8) & 0xFFu);

    /* host-vs-model control flow: BE538 presence + args */
    if (be_r_n != mbe) {
        printf("FAIL seed=%08X host be count=%d want=%d\n", seed, be_r_n, mbe);
        return 0;
    }
    if (mbe) {
        if (be_r_a0 != (A & 0xFFu) || be_r_a1 != mw[0] || be_r_a2 != mw[1] ||
            be_r_a3 != mw[2]) {
            printf("FAIL seed=%08X host be args=%02X %08X %08X %08X want=%02X %08X %08X %08X\n",
                   seed, be_r_a0, be_r_a1, be_r_a2, be_r_a3,
                   A & 0xFFu, mw[0], mw[1], mw[2]);
            return 0;
        }
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

    /* retail-vs-model control flow: BE538 presence + args */
    {
        u8 sb[BES_LEN];
        if (remu_peek(m, BES_BASE, sb, sizeof sb)) { printf("FAIL peek bes\n"); return 0; }
        u32 n = (u32)sb[0] | ((u32)sb[1] << 8) | ((u32)sb[2] << 16) | ((u32)sb[3] << 24);
        u32 g0 = (u32)sb[4] | ((u32)sb[5] << 8) | ((u32)sb[6] << 16) | ((u32)sb[7] << 24);
        u32 g1 = (u32)sb[8] | ((u32)sb[9] << 8) | ((u32)sb[10] << 16) | ((u32)sb[11] << 24);
        u32 g2 = (u32)sb[12] | ((u32)sb[13] << 8) | ((u32)sb[14] << 16) | ((u32)sb[15] << 24);
        u32 g3 = (u32)sb[16] | ((u32)sb[17] << 8) | ((u32)sb[18] << 16) | ((u32)sb[19] << 24);
        if ((int)n != mbe) {
            printf("FAIL seed=%08X retail be count=%u want=%d\n", seed, n, mbe);
            return 0;
        }
        if (mbe && (g0 != (A & 0xFFu) || g1 != mw[0] || g2 != mw[1] || g3 != mw[2])) {
            printf("FAIL seed=%08X retail be args=%02X %08X %08X %08X want=%02X %08X %08X %08X\n",
                   seed, g0, g1, g2, g3, A & 0xFFu, mw[0], mw[1], mw[2]);
            return 0;
        }
    }

    /* direct host-vs-retail span comparison */
    if (remu_peek(m, S1_BASE, s1back, S1_LEN) ||
        memcmp(s1back, H_S1, S1_LIVE) != 0) {
        printf("FAIL seed=%08X retail s1 mismatch\n", seed);
        if (!remu_peek(m, S1_BASE, s1back, S1_LEN))
            for (uint32_t k = 0; k < S1_LIVE; k++)
                if (s1back[k] != H_S1[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, s1back[k], H_S1[k]);
                    break;
                }
        return 0;
    }
    {
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
        memcmp(ccdback, H_CCD, CCD_LEN) != 0) {
        printf("FAIL seed=%08X retail ccd mismatch\n", seed);
        if (!remu_peek(m, CCD_BASE, ccdback, CCD_LEN)) {
            int n = 0;
            for (uint32_t k = 0; k < CCD_LEN && n < 12; k++)
                if (ccdback[k] != H_CCD[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, ccdback[k], H_CCD[k]);
                    n++;
                }
        }
        return 0;
    }
    if (remu_peek(m, CCE08_BASE, e08back, CCE08_LEN) ||
        memcmp(e08back, D_800CCE08, CCE08_LEN) != 0) {
        printf("FAIL seed=%08X retail e08 mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, B301_BASE, b301back, B301_LEN) ||
        memcmp(b301back, H_B301, B301_LEN) != 0) {
        printf("FAIL seed=%08X retail b301 mismatch\n", seed);
        if (!remu_peek(m, B301_BASE, b301back, B301_LEN))
            for (uint32_t k = 0; k < B301_LEN; k++)
                if (b301back[k] != H_B301[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, b301back[k], H_B301[k]);
                    break;
                }
        return 0;
    }
    if (remu_peek(m, SCR2_BASE, scr2back, SCR2_LEN) ||
        memcmp(scr2back, H_SCR2, SCR2_LEN) != 0) {
        printf("FAIL seed=%08X retail scr2 mismatch\n", seed);
        if (!remu_peek(m, SCR2_BASE, scr2back, SCR2_LEN))
            for (uint32_t k = 0; k < SCR2_LEN; k++)
                if (scr2back[k] != H_SCR2[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, scr2back[k], H_SCR2[k]);
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
    {
        /* T48 hole [104,108): the T34 pointer word overlays it on retail */
        static u8 t48back[T48_LEN];
        if (remu_peek(m, T48_BASE, t48back, T48_LEN)) { printf("FAIL peek t48\n"); return 0; }
        u8 p48[T48_LEN];
        for (uint32_t j = 0; j < 256; j++) {
            p48[2 * j] = (u8)(H48[j] & 0xFFu);
            p48[2 * j + 1] = (u8)((H48[j] >> 8) & 0xFFu);
        }
        for (uint32_t k = 0; k < T48_LEN; k++) {
            if (k >= T48_PTR_OFF && k < T48_PTR_OFF + 4)
                continue;
            if (t48back[k] != p48[k]) {
                printf("FAIL seed=%08X retail t48 mismatch off %u\n", seed, k);
                return 0;
            }
        }
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
    uint32_t entry = remu_load_oracle(m, RETAIL_85B58);
    if (entry != 0x80085B58u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_oracle(m, RETAIL_85618) != 0x80085618u) { printf("FAIL load 85618\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_85388) != 0x80085388u) { printf("FAIL load 85388\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_89C08) != 0x80089C08u) { printf("FAIL load 89c08\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_883AC) != 0x800883ACu) { printf("FAIL load 883ac\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_89C48) != 0x80089C48u) { printf("FAIL load 89c48\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_9ADA0) != 0x8009ADA0u) { printf("FAIL load 9ada0\n"); return 1; }
    if (remu_load_oracle(m, RETAIL_BE538) != 0x800BE538u) { printf("FAIL load be538\n"); return 1; }
    {
        /* dump-verify the logging stub words (encoding slips halt here) */
        static const u8 want[40] = {
            0x07, 0x80, 0x01, 0x3C, 0x00, 0x00, 0x22, 0x8C,
            0x01, 0x00, 0x42, 0x24, 0x00, 0x00, 0x22, 0xAC,
            0x04, 0x00, 0x24, 0xAC, 0x08, 0x00, 0x25, 0xAC,
            0x0C, 0x00, 0x26, 0xAC, 0x10, 0x00, 0x27, 0xAC,
            0x08, 0x00, 0xE0, 0x03, 0x00, 0x00, 0x00, 0x00,
        };
        u8 got[40];
        if (remu_peek(m, 0x800BE538u, got, sizeof got) ||
            memcmp(got, want, sizeof want) != 0) {
            printf("FAIL be538 stub image\n");
            return 1;
        }
    }
    static const uint32_t avec[] = { 0, 1, 2, 5, 6, 7, 10, 64, 128, 200, 255 };
    uint32_t s = 0x85B58234u;
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
    printf("DIFF 80085B58 OK (%d seeds, retail-exact)\n", n);
    return 0;
}

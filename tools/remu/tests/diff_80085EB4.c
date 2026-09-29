/* Differential test: host-compiled src/battle/main38.c func_80085EB4
 * vs the retail bytes (asm/battle/nonmatchings/main38/func_80085EB4.s)
 * executed by remu, with the callee loaded as retail code:
 *   asm/battle/matchings/main41/func_80089C6C.s
 * Fails on any behavioral mismatch.
 *
 * func_80085EB4(a0, a1): if *(D_800C3EAC+0x2D6) != 0 and (a0&0xFF)==4,
 * scan the 13 D_800C3160 pointer rows for the first whose 7 bytes match
 * D_800C3EAC[0x2CC..]; on a match at row a3, run the jtbl_80070280
 * fall-through chain (a0 = 12-a3), then call func_80089C6C(
 * *(g_GameState+0x16C0+D_800D2D24[(a1&0xFF)]*32),
 * *(u8*)(D_800C31AC[D_800CCD3E[(a1&0xFF)*368]]+0xC-(12-a3)));
 * return 1 iff its low half is nonzero, else 0. The function stores
 * nothing (stack spills aside): proof is the return value (host direct,
 * retail via remu_get_reg v0) plus a no-write check over every blob and
 * the 89C6C call args on both sides.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085EB4 src/battle/main38.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
u32 func_80085EB4(u8 a0, u8 a1);

/* TU externs the test must provide. */
u8 *D_800C3EAC;
u32 D_800C3160[13];
u8 D_800CCD3E[256u * 368u + 1u];
u8 D_800D2D24[256];
u32 D_800C31AC[256];
u8 g_GameState[0x16C0u + 256u * 32u + 2u];
u16 D_800C3468[16];

/* 89C6C host replica (frozen copy of the matched body) + recorder. */
static u32 rec_a0;
static u8 rec_a1;
static int rec_n;
u32 func_80089C6C(u32 a0, u8 a1) {
    rec_n++;
    rec_a0 = a0;
    rec_a1 = a1;
    u32 t = (u32)a1 & 0xFFu;
    if (t >= 0x10u)
        return 0;
    return (u32)D_800C3468[t] & a0;
}

#define RETAIL_S "asm/battle/nonmatchings/main38/func_80085EB4.s"
#define RETAIL_89C6C "asm/battle/matchings/main41/func_80089C6C.s"
#define SCR_BASE 0x801F0400u
#define SCR_LEN 0x300u
#define PTRW_BASE 0x800C3EACu
#define T3160_BASE 0x800C3160u
#define ROW_BASE 0x801F1000u
#define CCD3E_BASE 0x800CCD3Eu
#define CCD3E_LEN (256u * 368u + 1u)
#define D2D24_BASE 0x800D2D24u
#define C31AC_BASE 0x800C31ACu
#define B31_BASE 0x801F2000u
#define B31_PTRVAL 0x801F200Cu
#define GS_BASE 0x8006D634u
#define GS_LEN (0x16C0u + 256u * 32u + 2u)
#define T3468_BASE 0x800C3468u
#define JTBL_BASE 0x80070280u
/* D_800D2D24 aliases CCD3E+24550 in guest (later poke wins the overlap) */
#define D2D24_IN_CCD3E 24550u
_Static_assert(D2D24_BASE - CCD3E_BASE == D2D24_IN_CCD3E, "d2d24 overlap offset");
__attribute__((aligned(2))) static u8 h_scr[SCR_LEN];
__attribute__((aligned(2))) static u8 h_rows[13][8];
__attribute__((aligned(2))) static u8 h_b31[64];

/* jtbl_80070280 contents (asm/battle/data/0.rodata.s). */
static const uint32_t jtblw[13] = {
    0x80085F6Cu, 0x80085F70u, 0x80085F74u, 0x80085F78u,
    0x80085F7Cu, 0x80085F80u, 0x80085F84u, 0x80085F88u,
    0x80085F8Cu, 0x80085F90u, 0x80085F94u, 0x80085F98u,
    0x80085F9Cu,
};

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}
static u16 g16(const u8 *p) { return (u16)(p[0] | ((u16)p[1] << 8)); }

/* first fully-matching row of scr[0x2CC..] in rows, or 13 */
static int find_a3(const u8 *scr, const u8 rows[13][8]) {
    int a3 = 0;
    while (a3 < 13) {
        int k = 0;
        while (k < 7 && scr[0x2CC + k] == rows[a3][k])
            k++;
        if (k == 7)
            break;
        a3++;
    }
    return a3;
}
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

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a0w,
                     uint32_t a1w, int plan) {
    uint32_t x = seed ^ 0x85EB479u;
    static u8 scr[SCR_LEN];
    static u8 ccd3e[CCD3E_LEN];
    static u8 d2d24[256];
    static u16 t3468[16];
    static u8 gs[GS_LEN];
    for (uint32_t j = 0; j < SCR_LEN; j++)
        scr[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < CCD3E_LEN; j++)
        ccd3e[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < 256; j++)
        d2d24[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < 16; j++)
        t3468[j] = (u16)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < GS_LEN; j++)
        gs[j] = (u8)(lcg(&x) >> 16);
    static u8 rows[13][8];
    for (int i = 0; i < 13; i++)
        for (int k = 0; k < 8; k++)
            rows[i][k] = (u8)(lcg(&x) >> 16);
    /* guest identity: D_800D2D24 aliases CCD3E[24550..); the d2d24
     * image wins, so v1=67 reads the same byte on both sides */
    memcpy(ccd3e + D2D24_IN_CCD3E, d2d24, 256);

    /* forcing: gate + a0 + match row + 89C6C outcome */
    if ((plan & 1) == 0)
        scr[0x2D6] = 0;
    else if (scr[0x2D6] == 0)
        scr[0x2D6] = 0x41;
    uint32_t A0 = (plan & 2) ? 4u : (u32)(0xCDu + (uint32_t)plan);
    uint32_t V1 = a1w & 0xFFu;
    int match_row = -1;
    if (plan & 4) {
        match_row = (plan >> 3) % 13;
        for (int i = 0; i < 13; i++) {
            if (i < match_row)
                rows[i][0] = (u8)(scr[0x2CC] ^ (u8)(i + 1));
            else if (i == match_row)
                memcpy(rows[i], scr + 0x2CC, 7);
        }
    } else {
        for (int i = 0; i < 13; i++)
            rows[i][0] = (u8)(scr[0x2CC] ^ (u8)(i + 1));
    }
    {
        /* the 89C6C table index is the d byte (post[24-xx]); bias it */
        int fa3 = find_a3(scr, rows);
        if ((plan & 64) && fa3 < 13) {
            u8 fd = (u8)(((24u - (12u - (u32)fa3)) * 37u + 11u) & 0xFFu);
            if (fd < 16) {
                if (plan & 128)
                    t3468[fd] = 0xFFFFu;
                else
                    t3468[fd] = 0u;
            }
        }
    }

    /* independent model */
    u32 mret = 0;
    int mcall = 0;
    u32 ma0 = 0;
    u8 ma1 = 0;
    {
        int gate = scr[0x2D6] != 0;
        int a0ok = ((A0 & 0xFFu) == 4u);
        int a3 = 0;
        while (a3 < 13) {
            int k = 0;
            while (k < 7 && scr[0x2CC + k] == rows[a3][k])
                k++;
            if (k == 7)
                break;
            a3++;
        }
        if (gate && a0ok && a3 < 13) {
            u32 xx = 12u - (u32)a3;
            u8 post = (u8)(((24u - xx) * 37u + 11u) & 0xFFu);
            u8 c = d2d24[V1];
            u16 h = g16(gs + 0x16C0u + (u32)c * 32u);
            /* 89C6C indexes D_800C3468 by its own a1 (the d byte) */
            u32 r89 = (post < 16) ? ((u32)t3468[post] & (u32)h) : 0u;
            mcall = 1;
            ma0 = (u32)h;
            ma1 = post;
            if ((r89 & 0xFFFFu) != 0)
                mret = 1;
        }
    }

    /* host setup */
    memcpy(h_scr, scr, SCR_LEN);
    memcpy(h_rows, rows, sizeof h_rows);
    for (int i = 0; i < 13; i++)
        D_800C3160[i] = (u32)(uintptr_t)h_rows[i];
    memcpy(D_800CCD3E, ccd3e, CCD3E_LEN);
    memcpy(D_800D2D24, d2d24, 256);
    for (int i = 0; i < 256; i++)
        D_800C31AC[i] = (u32)(uintptr_t)(h_b31 + 12);
    for (int i = 0; i < 64; i++)
        h_b31[i] = (u8)((i * 37u + 11u) & 0xFFu);
    memcpy(g_GameState, gs, GS_LEN);
    memcpy(D_800C3468, t3468, sizeof t3468);
    D_800C3EAC = h_scr;
    rec_n = 0;
    u32 hret = func_80085EB4((u8)A0, (u8)V1);
    if (hret != mret) {
        printf("FAIL seed=%08X host ret=%u want=%u\n", seed, hret, mret);
        return 0;
    }
    if (rec_n != mcall) {
        printf("FAIL seed=%08X host calls=%d want=%d\n", seed, rec_n, mcall);
        return 0;
    }
    if (mcall && (rec_a0 != ma0 || rec_a1 != ma1)) {
        printf("FAIL seed=%08X host 89c6c args=%08X %02X want=%08X %02X\n",
               seed, rec_a0, rec_a1, ma0, ma1);
        return 0;
    }

    /* retail pokes */
    if (remu_poke(m, SCR_BASE, scr, SCR_LEN)) { printf("FAIL poke scr\n"); return 0; }
    {
        u8 pw[4] = { (u8)(SCR_BASE & 0xFFu), (u8)((SCR_BASE >> 8) & 0xFFu),
                     (u8)((SCR_BASE >> 16) & 0xFFu),
                     (u8)((SCR_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, PTRW_BASE, pw, 4)) { printf("FAIL poke ptrw\n"); return 0; }
    }
    for (int i = 0; i < 13; i++) {
        u8 w[4];
        uint32_t a = ROW_BASE + (uint32_t)i * 8u;
        s32f(w, a);
        if (remu_poke(m, T3160_BASE + (uint32_t)i * 4u, w, 4)) { printf("FAIL poke t3160\n"); return 0; }
        if (remu_poke(m, a, rows[i], 7)) { printf("FAIL poke row\n"); return 0; }
    }
    if (remu_poke(m, CCD3E_BASE, ccd3e, CCD3E_LEN)) { printf("FAIL poke ccd3e\n"); return 0; }
    if (remu_poke(m, D2D24_BASE, d2d24, 256)) { printf("FAIL poke d2d24\n"); return 0; }
    {
        u8 w[4];
        s32f(w, B31_PTRVAL);
        for (int i = 0; i < 256; i++)
            if (remu_poke(m, C31AC_BASE + (uint32_t)i * 4u, w, 4)) { printf("FAIL poke c31ac\n"); return 0; }
    }
    {
        static u8 b31[64];
        for (int i = 0; i < 64; i++)
            b31[i] = (u8)((i * 37u + 11u) & 0xFFu);
        if (remu_poke(m, B31_BASE, b31, 64)) { printf("FAIL poke b31\n"); return 0; }
    }
    if (remu_poke(m, GS_BASE, gs, GS_LEN)) { printf("FAIL poke gs\n"); return 0; }
    {
        u8 t48[32];
        for (int i = 0; i < 16; i++) {
            t48[2 * i] = (u8)(t3468[i] & 0xFFu);
            t48[2 * i + 1] = (u8)((t3468[i] >> 8) & 0xFFu);
        }
        if (remu_poke(m, T3468_BASE, t48, 32)) { printf("FAIL poke t3468\n"); return 0; }
    }
    {
        u8 jw[52];
        for (int i = 0; i < 13; i++) {
            jw[4 * i] = (u8)(jtblw[i] & 0xFFu);
            jw[4 * i + 1] = (u8)((jtblw[i] >> 8) & 0xFFu);
            jw[4 * i + 2] = (u8)((jtblw[i] >> 16) & 0xFFu);
            jw[4 * i + 3] = (u8)((jtblw[i] >> 24) & 0xFFu);
        }
        if (remu_poke(m, JTBL_BASE, jw, sizeof jw)) { printf("FAIL poke jtbl\n"); return 0; }
    }

    /* retail side: 89C6C loaded, zero stubs expected */
    int rc = remu_call(m, entry, A0, a1w, 0, 0);
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
    if (remu_get_reg(m, 2) != mret) {
        printf("FAIL seed=%08X retail ret=%08X want=%u\n", seed,
               remu_get_reg(m, 2), mret);
        return 0;
    }

    /* no-write check over every blob (ccd3e expectation carries the
     * d2d24 overlay: poke order ccd3e-then-d2d24, later wins) */
    {
        static u8 back[CCD3E_LEN];
        static u8 exp[CCD3E_LEN];
        if (remu_peek(m, SCR_BASE, back, SCR_LEN) || memcmp(back, scr, SCR_LEN)) {
            printf("FAIL seed=%08X retail wrote scr\n", seed);
            return 0;
        }
        memcpy(exp, ccd3e, CCD3E_LEN);
        memcpy(exp + D2D24_IN_CCD3E, d2d24, 256);
        if (remu_peek(m, CCD3E_BASE, back, CCD3E_LEN) || memcmp(back, exp, CCD3E_LEN)) {
            printf("FAIL seed=%08X retail wrote ccd3e\n", seed);
            if (!remu_peek(m, CCD3E_BASE, back, CCD3E_LEN))
                for (uint32_t k = 0; k < CCD3E_LEN; k++)
                    if (back[k] != exp[k]) {
                        printf("  off %u retail=%02X want=%02X\n", k, back[k], exp[k]);
                        break;
                    }
            return 0;
        }
    }
    return 1;
}

int main(void) {
    if (((uintptr_t)h_rows & 3u) != 0) {
        printf("FAIL host layout\n");
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085EB4u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    if (remu_load_s(m, RETAIL_89C6C) != 0x80089C6Cu) { printf("FAIL load 89c6c\n"); return 1; }
    static const uint32_t a1s[] = { 0, 1, 15, 16, 17, 100, 255, 0xFFFFFFFFu };
    uint32_t s = 0x85EB1234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(a1s) / sizeof(a1s[0]); i++) {
        for (int k = 0; k < 32; k++) {
            s = s * 1103515245u + 12345u;
            uint32_t a0w = ((k & 1) ? 4u : 0xCD00u | (s & 0xFFu)) ^ ((k & 2) ? 0x100u : 0u);
            if (!check_one(m, entry, s, a0w, a1s[i], (int)(i * 32u + (uint32_t)k)))
                return 1;
            n++;
        }
    }
    printf("DIFF 80085EB4 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

/* Differential test: host-compiled src/battle/main37.c func_80085CCC
 * vs the retail bytes (asm/battle/nonmatchings/main37/func_80085CCC.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80085CCC(a0, a1, a2): v = D_800D39DC; D_800C48E8 = 0;
 * D_800D2CA9 = a0; b = *(D_800C3EAC + 0x2DC); D_800D2C94 = a1;
 * D_800D2C98 = a2; D_800D2C96 = v; D_800D2CAA = b - 1 (mod 256);
 * call func_800941A4 (jal delay is nop). The callee is untranscribed,
 * so both sides stub it: host via a recorder, retail via remu's
 * external-jal stub (exactly 1 hit expected).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085CCC src/battle/main37.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085CCC(u8 a0, u16 a1, u16 a2);

/* TU externs the test must provide. */
u8 *D_800C3EAC;
u16 D_800D39DC;
u16 D_800C48E8;
u8 D_800D2CA9;
u16 D_800D2C94;
u16 D_800D2C98;
u16 D_800D2C96;
u8 D_800D2CAA;

/* 941A4 host recorder (real effects belong to main51.c). */
static int rec_n;
void func_800941A4(void) {
    rec_n++;
}

#define RETAIL_S "asm/battle/nonmatchings/main37/func_80085CCC.s"
#define PTRW_BASE 0x800C3EACu
#define D39DC_BASE 0x800D39DCu
#define C48E8_BASE 0x800C48E8u
#define D2CA9_BASE 0x800D2CA9u
#define D2C94_BASE 0x800D2C94u
#define D2C98_BASE 0x800D2C98u
#define D2C96_BASE 0x800D2C96u
#define D2CAA_BASE 0x800D2CAAu
#define SCR_BASE 0x801F0400u
#define SCR_LEN 0x300u
__attribute__((aligned(2))) static u8 h_scr[SCR_LEN];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a0w,
                     uint32_t a1w, uint32_t a2w, u16 v39, u8 b) {
    /* host side */
    for (uint32_t j = 0; j < SCR_LEN; j++)
        h_scr[j] = (u8)((j * 31u + (seed & 0xFFu)) & 0xFFu);
    h_scr[0x2DC] = b;
    D_800C3EAC = h_scr;
    D_800D39DC = v39;
    D_800C48E8 = 0xBEEFu;
    D_800D2CA9 = 0x11u;
    D_800D2C94 = 0x2222u;
    D_800D2C98 = 0x3333u;
    D_800D2C96 = 0x4444u;
    D_800D2CAA = 0x55u;
    rec_n = 0;
    func_80085CCC((u8)a0w, (u16)a1w, (u16)a2w);
    u8 want_aa = (u8)(b - 1u);
    if (D_800C48E8 != 0 || D_800D2CA9 != (u8)a0w ||
        D_800D2C94 != (u16)a1w || D_800D2C98 != (u16)a2w ||
        D_800D2C96 != v39 || D_800D2CAA != want_aa || rec_n != 1) {
        printf("FAIL seed=%08X host acc=%04X a9=%02X 94=%04X 98=%04X 96=%04X aa=%02X n=%d\n",
               seed, D_800C48E8, D_800D2CA9, D_800D2C94, D_800D2C98,
               D_800D2C96, D_800D2CAA, rec_n);
        return 0;
    }

    /* retail side */
    if (remu_poke(m, SCR_BASE, h_scr, SCR_LEN)) { printf("FAIL poke scr\n"); return 0; }
    {
        u8 pw[4] = { (u8)(SCR_BASE & 0xFFu), (u8)((SCR_BASE >> 8) & 0xFFu),
                     (u8)((SCR_BASE >> 16) & 0xFFu),
                     (u8)((SCR_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, PTRW_BASE, pw, 4)) { printf("FAIL poke ptrw\n"); return 0; }
    }
    {
        u8 v2[2] = { (u8)(v39 & 0xFFu), (u8)((v39 >> 8) & 0xFFu) };
        if (remu_poke(m, D39DC_BASE, v2, 2)) { printf("FAIL poke 39dc\n"); return 0; }
    }
    int rc = remu_call(m, entry, a0w, a1w, a2w, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 1) {
        printf("FAIL seed=%08X stub count=%d want=1%s\n", seed,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    u8 back[2];
    u8 hb;
    if (remu_peek(m, C48E8_BASE, back, 2) || back[0] != 0 || back[1] != 0) {
        printf("FAIL seed=%08X retail acc=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    if (remu_peek(m, D2CA9_BASE, &hb, 1) || hb != (u8)a0w) {
        printf("FAIL seed=%08X retail a9=%02X want=%02X\n", seed, hb, (u8)a0w);
        return 0;
    }
    u8 e1[2] = { (u8)(a1w & 0xFFu), (u8)((a1w >> 8) & 0xFFu) };
    if (remu_peek(m, D2C94_BASE, back, 2) || memcmp(back, e1, 2) != 0) {
        printf("FAIL seed=%08X retail 94=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    u8 e2[2] = { (u8)(a2w & 0xFFu), (u8)((a2w >> 8) & 0xFFu) };
    if (remu_peek(m, D2C98_BASE, back, 2) || memcmp(back, e2, 2) != 0) {
        printf("FAIL seed=%08X retail 98=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    u8 e3[2] = { (u8)(v39 & 0xFFu), (u8)((v39 >> 8) & 0xFFu) };
    if (remu_peek(m, D2C96_BASE, back, 2) || memcmp(back, e3, 2) != 0) {
        printf("FAIL seed=%08X retail 96=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    if (remu_peek(m, D2CAA_BASE, &hb, 1) || hb != want_aa) {
        printf("FAIL seed=%08X retail aa=%02X want=%02X\n", seed, hb, want_aa);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085CCCu) { printf("FAIL load entry=%08X\n", entry); return 1; }
    static const uint32_t edges[] = {
        0u, 1u, 0xFFu, 0x100u, 0xFFFFu, 0x10000u, 0xABCD1234u, 0xFFFFFFFFu,
    };
    static const u8 bs[] = { 0, 1, 2, 0x7F, 0x80, 0xFE, 0xFF };
    uint32_t s = 0x85CC1234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        for (unsigned b = 0; b < sizeof(bs) / sizeof(bs[0]); b++) {
            for (int k = 0; k < 4; k++) {
                s = s * 1103515245u + 12345u;
                uint32_t a0w = edges[i] ^ ((k & 1) ? 0xCD00u : 0u);
                uint32_t a1w = edges[(i + k) & 7] ^ (s & 0xFFFF0000u);
                uint32_t a2w = edges[(i + 2 * k) & 7] ^ ((s >> 9) | (s << 17));
                u16 v39 = (u16)(s >> 16);
                if (!check_one(m, entry, s, a0w, a1w, a2w, v39, bs[b]))
                    return 1;
                n++;
            }
        }
    }
    printf("DIFF 80085CCC OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

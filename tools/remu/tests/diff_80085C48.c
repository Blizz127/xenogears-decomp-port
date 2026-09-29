/* Differential test: host-compiled src/battle/main36.c func_80085C48
 * vs the retail bytes (asm/battle/matchings/main36/func_80085C48.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80085C48(a0, a1, a2): v = D_800D39DC; D_800C48E8 = 0;
 * D_800D2C94 = (u16)a1; D_800D2C96 = v; call func_80098C6C(a2 & 0xFFFF)
 * (a0 is ignored; the jal delay slot computes the masked arg). The
 * callee is untranscribed, so both sides stub it: host via a recorder,
 * retail via remu's external-jal stub (exactly 1 hit expected). The
 * recorder pins the call arg; the spans pin the stores.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085C48 src/battle/main36.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085C48(u32 a0, u32 a1, u32 a2);

/* TU externs the test must provide (u16 scalars, guest-absolute). */
u16 D_800D39DC;
u16 D_800C48E8;
u16 D_800D2C94;
u16 D_800D2C96;

/* 98C6C host recorder (real effects belong elsewhere). */
static u32 rec_arg;
static int rec_n;
void func_80098C6C(u32 v) {
    rec_n++;
    rec_arg = v;
}

#define RETAIL_S "asm/battle/matchings/main36/func_80085C48.s"
#define D39DC_BASE 0x800D39DCu
#define C48E8_BASE 0x800C48E8u
#define D2C94_BASE 0x800D2C94u
#define D2C96_BASE 0x800D2C96u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

/* Acc pre-fill: independent of the lcg stream used for args. */
static uint32_t acc_prefill(uint32_t seed) {
    return (seed ^ 0x85C4811u) * 1103515245u + 12345u;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a0,
                     uint32_t a1, uint32_t a2, u16 v39) {
    /* host side */
    D_800D39DC = v39;
    D_800C48E8 = (u16)(acc_prefill(seed) & 0xFFFFu);
    D_800D2C94 = (u16)0xAAAAu;
    D_800D2C96 = (u16)0x5555u;
    rec_n = 0;
    rec_arg = 0xDDDDDDDDu;
    func_80085C48(a0, a1, a2);
    if (D_800C48E8 != 0) {
        printf("FAIL seed=%08X host acc=%04X want=0\n", seed, D_800C48E8);
        return 0;
    }
    if (D_800D2C94 != (u16)a1) {
        printf("FAIL seed=%08X host 2c94=%04X want=%04X\n", seed,
               D_800D2C94, (u16)a1);
        return 0;
    }
    if (D_800D2C96 != v39) {
        printf("FAIL seed=%08X host 2c96=%04X want=%04X\n", seed,
               D_800D2C96, v39);
        return 0;
    }
    if (rec_n != 1 || rec_arg != (a2 & 0xFFFFu)) {
        printf("FAIL seed=%08X host call n=%d arg=%08X want=%08X\n", seed,
               rec_n, rec_arg, a2 & 0xFFFFu);
        return 0;
    }

    /* retail side: the 98C6C jal is an external stub (1 hit, v0 = 0). */
    u8 poke[2];
    poke[0] = (u8)(v39 & 0xFFu);
    poke[1] = (u8)((v39 >> 8) & 0xFFu);
    if (remu_poke(m, D39DC_BASE, poke, 2)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, a0, a1, a2, 0);
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
    if (remu_peek(m, C48E8_BASE, back, 2) || back[0] != 0 || back[1] != 0) {
        printf("FAIL seed=%08X retail acc=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    u8 e1[2] = { (u8)(a1 & 0xFFu), (u8)((a1 >> 8) & 0xFFu) };
    if (remu_peek(m, D2C94_BASE, back, 2) || memcmp(back, e1, 2) != 0) {
        printf("FAIL seed=%08X retail 2c94=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    if (remu_peek(m, D2C96_BASE, back, 2) || memcmp(back, poke, 2) != 0) {
        printf("FAIL seed=%08X retail 2c96=%02X%02X\n", seed, back[1], back[0]);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085C48u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    static const uint32_t edges[] = {
        0u, 1u, 0xFFu, 0x100u, 0xFFFFu, 0x10000u, 0x12345u, 0xFFFFFFFFu,
    };
    uint32_t s = 0x85C41234u;
    int n = 0;
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        for (unsigned j = 0; j < sizeof(edges) / sizeof(edges[0]); j++) {
            for (int k = 0; k < 4; k++) {
                s = s * 1103515245u + 12345u;
                uint32_t a0 = edges[(i + k) & 7];
                uint32_t a1 = edges[i] ^ (s & 0xFFFF0000u);
                uint32_t a2 = edges[j] ^ ((s >> 7) | (s << 21));
                u16 v39 = (u16)(s >> 16);
                if (!check_one(m, entry, s, a0, a1, a2, v39))
                    return 1;
                n++;
            }
        }
    }
    printf("DIFF 80085C48 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

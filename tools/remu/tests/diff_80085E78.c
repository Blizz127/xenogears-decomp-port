/* Differential test: host-compiled src/battle/main37.c func_80085E78
 * vs the retail bytes (asm/battle/matchings/main37/func_80085E78.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80085E78(): for i in 0..6: *(D_800C3EAC + 0x2CC + i) = 0xFF;
 * then *(D_800C3EAC + 0x2D6) = 0. Takes no args, no calls.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085E78 src/battle/main37.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085E78(void);

/* TU externs the test must provide. */
u8 *D_800C3EAC;

#define RETAIL_S "asm/battle/matchings/main37/func_80085E78.s"
#define PTRW_BASE 0x800C3EACu
#define SCR_BASE 0x801F0400u
#define SCR_LEN 0x300u
__attribute__((aligned(2))) static u8 h_scr[SCR_LEN];

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    uint32_t x = seed ^ 0x85E7879u;
    for (uint32_t j = 0; j < SCR_LEN; j++)
        h_scr[j] = (u8)(lcg(&x) >> 16);

    /* host side */
    D_800C3EAC = h_scr;
    func_80085E78();
    for (uint32_t k = 0; k < 7; k++) {
        if (h_scr[0x2CC + k] != 0xFFu) {
            printf("FAIL seed=%08X host scr[%X]=%02X\n", seed, 0x2CC + k,
                   h_scr[0x2CC + k]);
            return 0;
        }
    }
    if (h_scr[0x2D6] != 0) {
        printf("FAIL seed=%08X host scr[2D6]=%02X\n", seed, h_scr[0x2D6]);
        return 0;
    }

    /* retail side */
    if (remu_poke(m, SCR_BASE, h_scr, SCR_LEN)) {
        /* h_scr holds POST-host bytes; restore is unnecessary: the
         * stores are idempotent, so poking the post image is exact. */
        printf("FAIL poke scr\n");
        return 0;
    }
    {
        u8 pw[4] = { (u8)(SCR_BASE & 0xFFu), (u8)((SCR_BASE >> 8) & 0xFFu),
                     (u8)((SCR_BASE >> 16) & 0xFFu),
                     (u8)((SCR_BASE >> 24) & 0xFFu) };
        if (remu_poke(m, PTRW_BASE, pw, 4)) { printf("FAIL poke ptrw\n"); return 0; }
    }
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
    static u8 back[SCR_LEN];
    if (remu_peek(m, SCR_BASE, back, SCR_LEN) ||
        memcmp(back, h_scr, SCR_LEN) != 0) {
        printf("FAIL seed=%08X retail scr mismatch\n", seed);
        for (uint32_t k = 0; k < SCR_LEN; k++)
            if (back[k] != h_scr[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, back[k], h_scr[k]);
                break;
            }
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085E78u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    uint32_t s = 0x85E71234u;
    int n = 0;
    for (int k = 0; k < 64; k++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s))
            return 1;
        n++;
    }
    printf("DIFF 80085E78 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

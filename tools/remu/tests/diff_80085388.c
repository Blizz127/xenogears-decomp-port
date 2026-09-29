/* Differential test: host-compiled src/battle/main36.c func_80085388
 * vs the retail bytes (asm/battle/nonmatchings/main36/func_80085388.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80085388(): 11 passes (i = 0..10) clearing two u16 slots and
 * setting two byte slots to 0xFF around D_800C3FE8, strided by
 * *(D_800C3EAC + 0x2DA) * 72. The whole table is compared
 * host-vs-retail byte for byte (max touch 255*72 + 0x3C + 10).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085388 src/battle/main36.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main36.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_80085388(void);

/* TU externs the test must provide. */
u8 *D_800C3EAC;
u8 D_800C3FE8[20480];

#define RETAIL_S "asm/battle/nonmatchings/main36/func_80085388.s"
#define TAB_BASE 0x800C3FE8u
#define TAB_LEN 20480u
#define HDR_BASE 0x801F0000u
#define HDR_LEN 0x300u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    uint32_t x = seed ^ 0x85388853u;
    static u8 tab[TAB_LEN];
    static u8 hdr[HDR_LEN];
    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < HDR_LEN; j++)
        hdr[j] = (u8)(lcg(&x) >> 16);
    /* stride byte pinned per seed class so all v1 values get covered */
    u8 v = (u8)(seed >> 3);

    /* host side */
    static u8 h_hdr[HDR_LEN];
    memcpy(D_800C3FE8, tab, TAB_LEN);
    memcpy(h_hdr, hdr, HDR_LEN);
    h_hdr[0x2DA] = v;
    D_800C3EAC = h_hdr;
    func_80085388();

    /* retail side */
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke tab\n"); return 0; }
    hdr[0x2DA] = v;
    if (remu_poke(m, HDR_BASE, hdr, HDR_LEN)) { printf("FAIL poke hdr\n"); return 0; }
    uint32_t pv = HDR_BASE;
    if (remu_poke(m, 0x800C3EACu, &pv, 4)) { printf("FAIL poke ptr\n"); return 0; }
    int rc = remu_call(m, entry, 0, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%08X unexpected stub calls%s\n", seed,
               remu_stub_log(m));
        return 0;
    }
    static u8 rtab[TAB_LEN];
    if (remu_peek(m, TAB_BASE, rtab, TAB_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(rtab, D_800C3FE8, TAB_LEN) != 0) {
        printf("FAIL seed=%08X v=%u table mismatch\n", seed, v);
        for (uint32_t k = 0; k < TAB_LEN; k++)
            if (rtab[k] != D_800C3FE8[k]) {
                printf("  off %u retail=%02X host=%02X\n", k, rtab[k], D_800C3FE8[k]);
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
    if (entry != 0x80085388u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    /* every stride byte value (drives all table regions) */
    for (uint32_t v = 0; v < 256; v++) {
        if (!check_one(m, entry, (v << 3) | 0x5388u)) return 1;
        n++;
    }
    uint32_t s = 0x85381234u;
    for (int i = 0; i < 100; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 80085388 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

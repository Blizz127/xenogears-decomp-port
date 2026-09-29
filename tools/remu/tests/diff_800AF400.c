/* Differential test: host-compiled src/battle/main73.c func_800AF400
 * vs retail func_800AF400.s in either generated assembly directory
 * executed by remu. Fails on any behavioral mismatch.
 *
 * Build (from repo root):
 *   gcc -std=gnu17 -O2 -Iinclude -Isrc -Itools/remu \
 *       tools/remu/tests/diff_800AF400.c build-main73.o tools/remu/remu.c \
 *       -o /tmp/diff_800AF400 && /tmp/diff_800AF400
 * where build-main73.o is main73.c compiled with INCLUDE_ASM neutralized
 * (see tools/remu/tests/remu_shim.h, force-included).
 */
#include <stdio.h>
#include <stdint.h>

#include "remu.h"
#include "retail_oracle.h"

/* ---- host side: the real TU object (main73.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
s32 func_800AF400(void);

/* TU externs the test must provide (sizes generous; only D_800C3E30 is used). */
u16 D_800C3E30;
u8 D_800C3BCC[64];
u32 D_800D2D40, D_800D2D48;
u16 D_8005A3A0[8];
u8 D_800D2E62[8];
u16 D_800D39E0;
u8 *D_800D3278;
void LoadImage(void *a, void *b) { (void)a; (void)b; }
void DrawSync(s32 mode) { (void)mode; }

#define RETAIL_S "asm/battle/nonmatchings/main73/func_800AF400.s"
#define GVAR 0x800C3E30u

static int check_one(remu_t *m, uint32_t entry, uint16_t seed) {
    D_800C3E30 = seed;
    uint16_t be = seed;
    if (remu_poke(m, GVAR, &be, 2)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, 0, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%04X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%04X unexpected stub calls%s\n", seed,
               remu_stub_log(m));
        return 0;
    }
    s32 want = (s32)remu_get_reg(m, 2);
    s32 got = func_800AF400();
    if (got != want) {
        printf("FAIL seed=%04X retail=%d host=%d\n", seed, want, got);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_oracle(m, RETAIL_S);
    if (entry != 0x800AF400u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    /* edge seeds: zero, every single bit, full, bit-12/13 boundary */
    uint16_t edges[] = { 0x0000, 0xFFFF, 0x1FFF, 0x2000, 0xE000,
                         0x1000, 0x0FFF, 0x0001, 0x8000 };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    for (int b = 0; b < 16; b++) {
        if (!check_one(m, entry, (uint16_t)(1u << b))) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep incl. multi-bit patterns */
    uint32_t s = 0x12345678u;
    for (int i = 0; i < 300; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, (uint16_t)(s >> 16))) return 1;
        n++;
    }
    printf("DIFF 800AF400 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

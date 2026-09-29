/* Differential test: host-compiled src/battle/main17.c func_8007A9D0
 * vs the retail bytes (asm/battle/nonmatchings/main17/func_8007A9D0.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * Build (from repo root):
 *   gcc -std=gnu17 -O2 -Iinclude -Isrc -Itools/remu \
 *       tools/remu/tests/diff_8007A9D0.c build-main17.o tools/remu/remu.c \
 *       -o /tmp/diff_8007A9D0 && /tmp/diff_8007A9D0
 * where build-main17.o is main17.c compiled with INCLUDE_ASM neutralized
 * (see tools/remu/tests/remu_shim.h, force-included).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main17.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8007A9D0(u8 **ppBoard, u8 index);

/* TU externs the test must provide (rows of 64 bytes, index is u8;
 * p[1] can address past the row end, so keep 256 bytes of margin). */
u8 D_800D3430[64 * 256 + 256];

#define RETAIL_S "asm/battle/nonmatchings/main17/func_8007A9D0.s"
#define RETAIL_ENTRY 0x8007A9D0u
#define GVAR 0x800D3430u
#define PP_ADDR 0x800F0000u
#define BOARD_ADDR 0x800F1000u
#define D_SIZE (sizeof(D_800D3430))

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    u8 idx = (u8)(seed & 0xFFu);
    u8 off = (u8)((seed >> 8) & 0xFFu);
    u8 operand = (u8)((seed >> 16) & 0xFFu);
    u8 init = (u8)((seed >> 24) & 0xFFu);

    /* seed the background pattern on both sides */
    for (uint32_t i = 0; i < D_SIZE; i++)
        D_800D3430[i] = (u8)((i * 31u + (seed & 0xFFFFu) + (seed >> 16)) & 0xFFu);
    D_800D3430[(uint32_t)idx * 64u + off] = init;
    if (remu_poke(m, GVAR, D_800D3430, D_SIZE)) { printf("FAIL poke\n"); return 0; }

    u8 hboard[8] = { 0xAA, off, operand, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    if (remu_poke(m, BOARD_ADDR, hboard, sizeof(hboard))) { printf("FAIL poke\n"); return 0; }
    uint32_t pp = BOARD_ADDR;
    if (remu_poke(m, PP_ADDR, &pp, 4)) { printf("FAIL poke\n"); return 0; }

    /* expected v0 from the pre-state (retail leaves the slti flag in v0) */
    uint32_t pre_sum = (uint32_t)init + (uint32_t)operand;
    uint32_t want_v0 = (pre_sum < 0x100u) ? 1u : 0u;

    int rc = remu_call(m, entry, PP_ADDR, idx, 0, 0);
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
    uint32_t got_v0 = remu_get_reg(m, 2);
    if (got_v0 != want_v0) {
        printf("FAIL seed=%08X retail v0=%08X want=%08X\n", seed, got_v0, want_v0);
        return 0;
    }

    u8 *hpp = hboard;
    func_8007A9D0(&hpp, idx);

    static u8 rback[64 * 256 + 256];
    if (remu_peek(m, GVAR, rback, D_SIZE)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(rback, D_800D3430, D_SIZE) != 0) {
        printf("FAIL seed=%08X memory mismatch (retail vs host)\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != RETAIL_ENTRY) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    /* edge seeds: 0, 1, -1, 0x7FFFFFFF, 0x80000000 */
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, (uint32_t)(1u << b))) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep */
    uint32_t s = 0x12345678u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 8007A9D0 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}

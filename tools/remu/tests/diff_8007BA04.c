/* Differential for the matched row-byte to row-halfword conversion body. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_8007BA04(u8 **pp, s32 idx);

#define BASE 0x800D3420u
#define PBOX 0x80181000u
#define PBYTES (PBOX + 0x10u)
#define REGION 18432u

u8 D_800D3420[REGION];
u8 D_800D3410[REGION];

static void make_pattern(u8 *out, uint32_t seed) {
    uint32_t x = seed;
    for (uint32_t i = 0; i < REGION; i++) {
        x = x * 1103515245u + 12345u;
        out[i] = (u8)(x >> 16);
    }
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    const s32 idx = (s32)seed;
    const u8 b1 = (u8)(seed >> 8), b2 = (u8)(seed >> 17);
    static u8 initial[REGION], retail[REGION];
    u8 board[8] = {0};
    make_pattern(initial, seed ^ 0xBA040BA0u);
    board[1] = b1;
    board[2] = b2;

    memcpy(D_800D3420, initial, REGION);
    u8 *host_board = board;
    u8 **host_pp = &host_board;
    func_8007BA04(host_pp, idx);

    if (remu_poke(m, BASE, initial, REGION)) return 0;
    uint32_t board_addr = PBYTES;
    if (remu_poke(m, PBOX, &board_addr, 4) ||
        remu_poke(m, PBYTES, board, sizeof(board))) return 0;
    int rc = remu_call(m, entry, PBOX, (uint32_t)idx, 0, 0);
    if (rc || remu_stub_calls(m)) {
        printf("FAIL seed=%08X rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_peek(m, BASE, retail, REGION)) return 0;
    if (memcmp(retail, D_800D3420, REGION)) {
        printf("FAIL seed=%08X table mismatch\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) return 1;
    uint32_t entry = remu_load_s(m, "asm/battle/matchings/mainasm_BF14/func_8007BA04.s");
    if (entry != 0x8007BA04u) {
        printf("FAIL load entry=%08X\n", entry);
        return 1;
    }
    const uint32_t edges[] = {0, 1, 0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u};
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
        if (!check_one(m, entry, edges[i])) return 1;
    uint32_t state = 0xBA040BA0u;
    for (unsigned i = 0; i < 256; i++) {
        state = state * 1103515245u + 12345u;
        if (!check_one(m, entry, state)) return 1;
    }
    printf("DIFF 8007BA04 OK (261 seeds, retail-exact)\n");
    remu_destroy(m);
    return 0;
}

/* Differential test for the port translation of func_8007B8D4. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_8007B8D4(u8** ppBoard, u8 index);

#define TAB_BASE 0x800D3420u
#define PARTY_BASE 0x8005A3A0u
#define PP 0x801F0000u
#define BOARD 0x801F0010u
#define TAB_LEN 17408u

u8 D_800D3420[TAB_LEN];
u16 D_8005A3A0[0x100];

static uint32_t next_value(uint32_t* state)
{
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static int check_one(remu_t* m, uint32_t entry, uint32_t seed)
{
    u8 board[8];
    u8* hostBoard;
    u8* oracleBoard;
    u8 before[TAB_LEN];
    uint32_t state = seed;
    uint32_t guestPointer = BOARD;
    unsigned i;

    for (i = 0; i < TAB_LEN; ++i) {
        D_800D3420[i] = (u8)(next_value(&state) >> 24);
        before[i] = D_800D3420[i];
    }
    for (i = 0; i < 0x100; ++i) {
        D_8005A3A0[i] = (u16)next_value(&state);
    }
    board[0] = 0;
    board[1] = (u8)(seed >> 8);
    board[2] = (u8)(seed >> 16);
    board[3] = (u8)(seed >> 24);
    board[4] = board[5] = board[6] = board[7] = 0;
    hostBoard = board;
    func_8007B8D4(&hostBoard, (u8)seed);

    if (remu_poke(m, TAB_BASE, before, TAB_LEN) != 0 ||
        remu_poke(m, PARTY_BASE, D_8005A3A0, sizeof(D_8005A3A0)) != 0 ||
        remu_poke(m, BOARD, board, sizeof(board)) != 0 ||
        remu_poke(m, PP, &guestPointer, sizeof(guestPointer)) != 0) {
        fprintf(stderr, "B8D4 FAIL seed=%08x fixture setup\n", seed);
        return 0;
    }
    if (remu_call(m, entry, PP, (u8)seed, 0, 0) != 0 ||
        remu_stub_calls(m) != 0) {
        fprintf(stderr, "B8D4 FAIL seed=%08x rc/stub\n", seed);
        return 0;
    }

    oracleBoard = board;
    (void)oracleBoard;
    {
        u8 actual[TAB_LEN];
        if (remu_peek(m, TAB_BASE, actual, TAB_LEN) != 0 ||
            memcmp(actual, D_800D3420, TAB_LEN) != 0) {
            fprintf(stderr, "B8D4 FAIL seed=%08x table mismatch\n", seed);
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    remu_t* m = remu_create();
    uint32_t entry;
    uint32_t state = 0xB8D4C0DEu;
    int i;

    if (m == NULL) return 1;
    entry = remu_load_s(m, "asm/battle/nonmatchings/main21/func_8007B8D4.s");
    if (entry != 0x8007B8D4u) {
        fprintf(stderr, "B8D4 FAIL load entry=%08x\n", entry);
        return 1;
    }
    for (i = 0; i < 256; ++i) {
        uint32_t seed = (i < 8) ? (uint32_t)(i * 0x01010101u) : next_value(&state);
        if (!check_one(m, entry, seed)) return 1;
    }
    printf("DIFF 8007B8D4 OK (256 seeds, retail-exact)\n");
    remu_destroy(m);
    return 0;
}

/* Retail differential for the port translation of func_8007B578. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_8007B578(u8** ppBoard, u8 index);

#define TAB_BASE 0x800D3420u
#define PP 0x801F0000u
#define BOARD 0x801F0010u
#define TAB_LEN 17408u

u8 D_800D3420[TAB_LEN];
static unsigned s_helperCalls;

u32 func_80079E7C(u16 value)
{
    (void)value;
    s_helperCalls++;
    return 0;
}

u32 func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3)
{
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    s_helperCalls++;
    return 0;
}

static int check_one(remu_t* m, u32 entry, u32 seed)
{
    u8 board[8];
    u8* hostBoard;
    u8 expected[TAB_LEN];
    u8 before[TAB_LEN];
    u32 state = seed;
    u32 guestPointer = BOARD;
    unsigned i;

    for (i = 0; i < TAB_LEN; ++i) {
        state = state * 1664525u + 1013904223u;
        D_800D3420[i] = (u8)(state >> 24);
        before[i] = D_800D3420[i];
    }
    board[0] = 0;
    board[1] = (u8)(seed >> 8);
    board[2] = (u8)(seed >> 16);
    board[3] = (u8)(seed >> 24);
    board[4] = board[5] = board[6] = board[7] = 0;

    s_helperCalls = 0;
    hostBoard = board;
    func_8007B578(&hostBoard, (u8)seed);
    memcpy(expected, D_800D3420, TAB_LEN);
    if (s_helperCalls != 2) {
        fprintf(stderr, "B578 FAIL seed=%08x host helper count\n", seed);
        return 0;
    }

    if (remu_poke(m, TAB_BASE, before, TAB_LEN) != 0 ||
        remu_poke(m, BOARD, board, sizeof(board)) != 0 ||
        remu_poke(m, PP, &guestPointer, sizeof(guestPointer)) != 0 ||
        remu_call(m, entry, PP, (u8)seed, 0, 0) != 0 ||
        remu_stub_calls(m) != 2 ||
        remu_peek(m, TAB_BASE, D_800D3420, TAB_LEN) != 0 ||
        memcmp(D_800D3420, expected, TAB_LEN) != 0) {
        fprintf(stderr, "B578 FAIL seed=%08x oracle/table\n", seed);
        return 0;
    }
    return 1;
}

int main(void)
{
    remu_t* m = remu_create();
    u32 entry;
    u32 state = 0xB578C0DEu;
    int i;

    if (m == NULL) return 1;
    entry = remu_load_s(m, "asm/battle/matchings/main21/func_8007B578.s");
    if (entry != 0x8007B578u) {
        fprintf(stderr, "B578 FAIL load entry=%08x\n", entry);
        return 1;
    }
    for (i = 0; i < 256; ++i) {
        u32 seed = (i < 8) ? (u32)(i * 0x01010101u) :
                   (state = state * 1103515245u + 12345u);
        if (!check_one(m, entry, seed)) return 1;
    }
    printf("DIFF 8007B578 OK (256 seeds, retail-exact)\n");
    remu_destroy(m);
    return 0;
}

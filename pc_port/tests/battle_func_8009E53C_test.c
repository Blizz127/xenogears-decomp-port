#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
uint8_t g_GameState[0x4600];

s32 func_8009E53C(u8 idx);

int main(void) {
    u8* entries;
    unsigned record = 2 * 0xA4;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_GameState, 0, sizeof(g_GameState));
    *(u32*)PSX_ADDR(0x800C34B0) = 0x80012000;
    entries = PSX_ADDR(0x80012000);
    g_GameState[0x59C] = 2;

    entries[3 * 20 + 0x5B47] = 0x11;
    g_GameState[0x98D + record] = 0x11;
    assert(func_8009E53C(3) == 1);

    g_GameState[0x98D + record] = 0;
    g_GameState[0x995 + record] = 0x11;
    assert(func_8009E53C(3) == 1);

    g_GameState[0x995 + record] = 0;
    g_GameState[0x99D + record] = 0x11;
    assert(func_8009E53C(3) == 1);

    g_GameState[0x99D + record] = 0;
    assert(func_8009E53C(3) == 0);

    entries[7 * 20 + 0x5B47] = 0x29;
    g_GameState[0x98D + record] = 0x29;
    assert(func_8009E53C(7) == 1);
    assert(func_8009E53C(3) == 0);

    puts("func_8009E53C guest-RAM slot checks PASS (first/second/third/no match, indexed entry)");
    return 0;
}

/* Pins func_801CB184's name-entry decode: retail copies each 0x14-byte
 * g_GameState entry's byte pairs into ONE contiguous buffer (through the
 * 0,0 terminator pair), calls func_80033B34(buf, out, pairs) and writes the
 * 0x14 decoded bytes back.  The port once split even/odd bytes into two
 * buffers (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "main/game.h"
#include "system/menu.h"
GameState g_GameState;
static unsigned calls;
void func_80033B34(u16 *src, u8 *dst, s32 count) {
    const u8 *s = (const u8 *)src;
    unsigned entry = calls++;
    const u8 *e_before = NULL; (void)e_before;
    if (entry == 0) {
        /* entry 0 = "AB" "CD" then 0,0 -> 2 pairs, buffer holds all 6 bytes */
        assert(count == 2);
        assert(s[0] == 'A' && s[1] == 'B' && s[2] == 'C' && s[3] == 'D' && s[4] == 0 && s[5] == 0);
    }
    memset(dst, 0x40 + entry, 0x14);
}
#include "decode.inc"
int main(void) {
    u8 *gs = (u8 *)&g_GameState;
    memset(gs, 0, 0x26C);
    memcpy(gs, "ABCD\0\0", 6);
    func_801CB184();
    assert(calls == 0x26C / 0x14 + (0x26C % 0x14 != 0));
    for (int i = 0; i < 0x14; i++) assert(gs[i] == 0x40);
    for (int i = 0; i < 0x14; i++) assert(gs[0x14 + i] == 0x41);
    printf("PASS menu name decode pins (CB184 single buffer, %u entries)\n", calls);
    return 0;
}

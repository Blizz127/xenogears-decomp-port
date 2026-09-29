/* Pins func_801CB8AC's wait (retail 0x801CB8AC): pump frames while input
 * stays 8 and the card bytes +0x4FE4/5 are unchanged.  A card change returns
 * 0, and leaving through input returns func_801CACF8's answer.  The port
 * once returned 1 on a card change and capped the wait at 0x80 frames
 * (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static u8 card[sizeof(MenuUnk2)];
static int frames, change_at, input_at, prompt, acf8_calls;
void func_801D2F4C(u8 id) { prompt = id; }
s32 func_801D32B4(s32 a) { (void)a; return 0; }
u8 func_801CACF8(u8 a, u8 b, u8 c) { assert(a == 0x2F && b == 0xFF && c == 1); acf8_calls++; return 0x5A; }
void func_801C7BF4(void) {
    frames++;
    if (frames == change_at) card[MENU_CARD_OFF(0x4FE5)] ^= 1;
    if (frames == input_at) menu.input = 0;
}
#include "wait.inc"
static u8 run(int chg, int inp) {
    frames = 0; change_at = chg; input_at = inp; acf8_calls = 0;
    card[MENU_CARD_OFF(0x4FE4)] = 3; card[MENU_CARD_OFF(0x4FE5)] = 4; card[MENU_CARD_OFF(0x4FE6)] = 0;
    return func_801CB8AC(2);
}
int main(void) {
    g_Menu = &menu;
    menu.unk32C = (MenuUnk2 *)card;
    assert(run(5, -1) == 0 && frames == 5 && acf8_calls == 0);
    assert(prompt == 2 * 3 + 0x29 && card[MENU_CARD_OFF(0x4FE6)] == 2);
    assert(run(-1, 3) == 0x5A && frames == 3 && acf8_calls == 1);
    assert(run(-1, 0x200) == 0x5A && frames == 0x200); /* no frame cap */
    printf("PASS menu card wait pins (CB8AC change -> 0, input -> CACF8)\n");
    return 0;
}

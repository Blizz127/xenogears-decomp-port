/* Pins func_801C9D34 (retail 0x801C9D34): the first actionable slot is found
 * through the typed card buffer and g_Menu+0x4D8 (unk4CC[0xC]) becomes 2, not
 * the manager byte.  The port once wrote pManager[0x4D8] and read a raw host
 * offset (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static u8 manager[0x600];
static u8 card[sizeof(MenuUnk2)];
u32 D_801E981C[32];
#include "first.inc"
int main(void) {
    s32 k;
    g_Menu = &menu;
    menu.unk32C = (MenuUnk2*)card;
    menu.pManager = (MenuManager*)manager;
    for (k = 0; k < 32; k++) D_801E981C[k] = k;
    card[MENU_CARD_OFF(0x4FE4)] = 1; card[MENU_CARD_OFF(0x4FE5)] = 1;          /* both cards present */
    *(u32*)(card + MENU_CARD_OFF(0x4F88)) = 0x10000;
    memset(card + MENU_CARD_OFF(0x4FAE), 0xFF, 0x20);
    card[MENU_CARD_OFF(0x4FAE) + 7] = 3;                        /* slot 7 used */
    assert(func_801C9D34(0) == 7 && menu.unk4CC[0xC] == 2);
    assert(manager[0x4D8] == 0);
    card[MENU_CARD_OFF(0x4F8E) + 7] = 0; menu.unk4CC[0xC] = 0;
    assert(func_801C9D34(1) == 0xFF && menu.unk4CC[0xC] == 2);
    card[MENU_CARD_OFF(0x4F8E) + 7] = 1;
    assert(func_801C9D34(1) == 7);
    card[MENU_CARD_OFF(0x4FE4)] = 0;                            /* no card 0: start at 15 */
    card[MENU_CARD_OFF(0x4FE4) + 1] = 1;
    assert(func_801C9D34(2) == 16);  /* 15 is in group 0 (absent) */
    printf("PASS menu card first pins (C9D34 search, g_Menu+0x4D8)\n");
    return 0;
}

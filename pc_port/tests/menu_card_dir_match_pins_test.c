/* Pins func_801C9270's card-directory match (retail 0x801C9270): entry k of
 * card `port` names a 0x5C-byte record (index at +0x4FAE + port*16 + k)
 * whose 12 bytes at +0x18 must equal the 12-byte prefix at +0x4FCE.  The
 * port once used a 0x28 stride and compared against one fixed byte
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
u8 D_801EA6D0[0x20];
s32 D_801EA6F4;
#include "match.inc"
static const char prefix[12] = "BASLUS-00664";
int main(void) {
    s32 k;
    g_Menu = &menu;
    menu.unk32C = (MenuUnk2 *)card;
    memcpy(card + MENU_CARD_OFF(0x4FCE), prefix, 12);
    for (k = 0; k < 15; k++) {
        card[MENU_CARD_OFF(0x4FAE) + 0x10 + k] = (u8)k;          /* port 1, entry k -> record k */
        card[MENU_CARD_OFF(0xB94) + k * 0x200 + 0x123] = (u8)k;  /* table slot k */
    }
    /* Records 2 and 5 carry the prefix at the retail 0x5C stride. */
    memcpy(card + 2 * 0x5C + 0x18, prefix, 12);
    memcpy(card + 5 * 0x5C + 0x18, prefix, 12);
    /* Record 3 matches only in its first byte. */
    card[3 * 0x5C + 0x18] = 'B';
    memset(card + MENU_CARD_OFF(0x4F8E) + 0x10, 0xEE, 0x10);
    func_801C9270(1);
    for (k = 0; k < 15; k++) {
        u8 want = (k == 2 || k == 5);
        assert(card[MENU_CARD_OFF(0x4F8E) + 0x10 + k] == want);
        assert(D_801EA6D0[0x10 + k] == want);
    }
    assert(card[MENU_CARD_OFF(0x4F8E) + 0x10 + 15] == 0);
    for (k = 0; k < 0x10; k++) assert(D_801EA6D0[k] == 0);
    printf("PASS menu card dir match pins (C9270 0x5C stride, 12-byte prefix)\n");
    return 0;
}

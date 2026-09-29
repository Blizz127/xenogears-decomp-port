/* Pins func_801C9BCC (retail 0x801C9BCC): mode 1 needs a used entry
 * (+0x4FAE != 0xFF) whose +0x4F8E flag is set, and success writes 2 to
 * g_Menu+0x4D8 (unk4CC[0xC]), not to the manager.  The port once nested
 * mode 1 the wrong way round and wrote pManager[0x4D8]
 * (docs/port/RETAIL_DIVERGENCES.md). */
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
u32 D_801E981C[4] = {0, 0x12, 0, 0};
#include "act.inc"
static s32 run(s32 mode, u8 used, u8 flag) {
    memset(card, 0, sizeof(card));
    *(s32 *)(card + MENU_CARD_OFF(0x4F7C)) = 1;               /* selector -> offset 0x12 */
    *(u32 *)(card + MENU_CARD_OFF(0x4F88)) = 0x10000;
    card[MENU_CARD_OFF(0x4FE4) + 1] = 1;                      /* group 0x12/16 = 1 */
    card[MENU_CARD_OFF(0x4FAE) + 0x12] = used ? 3 : 0xFF;
    card[MENU_CARD_OFF(0x4F8E) + 0x12] = flag;
    menu.unk4CC[0xC] = 0;
    memset(manager, 0, sizeof(manager));
    return func_801C9BCC(mode);
}
int main(void) {
    g_Menu = &menu;
    menu.unk32C = (MenuUnk2 *)card;
    menu.pManager = (MenuManager *)manager;
    assert(run(1, 1, 1) == 1 && menu.unk4CC[0xC] == 2);
    assert(manager[0x4D8 % sizeof(manager)] == 0);
    assert(run(1, 1, 0) == 0 && menu.unk4CC[0xC] == 0);
    assert(run(1, 0, 1) == 0);
    assert(run(1, 0, 0) == 0);
    assert(run(0, 1, 0) == 1);
    assert(run(0, 0, 1) == 0);
    assert(run(2, 0, 0) == 1);
    printf("PASS menu card can-act pins (C9BCC mode 1, g_Menu+0x4D8)\n");
    return 0;
}

/* Emits menu_retail_layout.h: host struct offsets of the production menu
 * headers, so the header-free differential test never hard-codes them. */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include <stdio.h>
#define P(T, m) printf("#define HO_%s_%s 0x%zxu\n", #T, #m, (size_t)offsetof(T, m))
int main(void) {
    printf("/* generated */\n");
    printf("#define HO_SM_SIZE 0x%zxu\n#define HO_CARD_SIZE 0x%zxu\n", sizeof(SystemMenu), sizeof(MenuUnk2));
    printf("#define HO_WIN_SIZE 0x%zxu\n#define HO_MGR_SIZE 0x%zxu\n#define HO_CURS_SIZE 0x%zxu\n",
           sizeof(MenuWindow), sizeof(MenuManager), sizeof(MenuPointerCursors));
    printf("#define HO_POLY_SIZE 0x%zxu\n#define HO_ULONG_SIZE %zu\n#define HO_GS_SIZE 0x%zxu\n",
           sizeof(POLY_FT4), sizeof(unsigned long), sizeof(GameState));
    P(SystemMenu, unk0); P(SystemMenu, pGfxEnv); P(SystemMenu, unk2DC); P(SystemMenu, renderContext);
    P(SystemMenu, input); P(SystemMenu, unk326); P(SystemMenu, unk32A); P(SystemMenu, unk32B);
    P(SystemMenu, unk32C); P(SystemMenu, unk334); P(SystemMenu, menu2Choice); P(SystemMenu, unk33B);
    P(SystemMenu, pManager); P(SystemMenu, windows); P(SystemMenu, unk39C); P(SystemMenu, pCursors);
    P(SystemMenu, unk42C); P(SystemMenu, unk44C); P(SystemMenu, unk4CC); P(SystemMenu, selectedPartySlot);
    P(MenuUnk2, tim); P(MenuUnk2, unkB94); P(MenuUnk2, unk4F7C); P(MenuUnk2, unk4F80);
    P(MenuWindow, polysWindowBorderTop); P(MenuWindow, renderContext);
    P(MenuPointerCursors, shouldRender); P(MenuPointerCursors, unk144); P(MenuPointerCursors, renderContexts);
    P(GfxEnvironment, ot);
    return 0;
}

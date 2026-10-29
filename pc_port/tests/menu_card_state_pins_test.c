/* Pins the card-menu state values func_801CD710 stores at g_Menu+0x334 for
 * each g_Menu+0x338 mode (retail: 0 -> 7, 1 -> 6, 2 -> 2 or 3 by
 * D_80059460).  The port once stored 1 for mode 0, and read/wrote those
 * two bytes through raw host offsets of the pointer-inflated SystemMenu
 * (docs/port/RETAIL_DIVERGENCES.md); the fixture uses the typed members. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static MenuManager manager;
u8 D_80059460;
static unsigned last;
void func_801D22F4(s32 a) { (void)a; }
void func_801D2484(void) {}
static int handlerResult;
s32 func_801CD2AC(void) { last = 0x2AC; return handlerResult; }
u8 func_801CC6D8(void) { last = 0x6D8; return handlerResult; }
u8 func_801CB304(void) { last = 0x304; return handlerResult; }
s32 func_801CBD90(u8 a) { (void)a; last = 0xD90; return handlerResult; }
#include "state.inc"
static u8 run(u8 mode) {
    menu.menu2Choice = mode;
    menu.unk334 = 0xEE;
    last = 0;
    assert(func_801CD710(0) == 1);
    return menu.unk334;
}
int main(void) {
    g_Menu = &menu;
    menu.pManager = &manager;
    assert(run(0) == 7 && last == 0x2AC);
    assert(run(1) == 6 && last == 0x6D8);
    D_80059460 = 2; assert(run(2) == 2 && last == 0x304);
    D_80059460 = 1; assert(run(2) == 3 && last == 0xD90);
    /* Retail returns 0 when the action handler returned nonzero (the screen
     * then leaves), else 1. */
    handlerResult = 1; D_80059460 = 2;
    menu.menu2Choice = 2; last = 0;
    assert(func_801CD710(0) == 0 && last == 0x304);
    menu.menu2Choice = 0;
    assert(func_801CD710(0) == 0 && last == 0x2AC);
    printf("PASS menu card state pins (CD710 0->7, 1->6, 2->2/3)\n");
    return 0;
}

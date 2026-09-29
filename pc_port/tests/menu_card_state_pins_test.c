/* Pins the card-menu state values func_801CD710 stores at g_Menu+0x334 for
 * each g_Menu+0x338 mode (retail: 0 -> 7, 1 -> 6, 2 -> 2 or 3 by
 * D_80059460).  The port once stored 1 for mode 0
 * (docs/port/RETAIL_DIVERGENCES.md). */
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
void func_801CD2AC(void) { last = 0x2AC; }
void func_801CC6D8(void) { last = 0x6D8; }
void func_801CB304(void) { last = 0x304; }
void func_801CBD90(u8 a) { (void)a; last = 0xD90; }
#include "state.inc"
static u8 run(u8 mode) {
    *((u8 *)&menu + 0x338) = mode;
    *((u8 *)&menu + 0x334) = 0xEE;
    last = 0;
    assert(func_801CD710(0) == 1);
    return *((u8 *)&menu + 0x334);
}
int main(void) {
    g_Menu = &menu;
    menu.pManager = &manager;
    assert(run(0) == 7 && last == 0x2AC);
    assert(run(1) == 6 && last == 0x6D8);
    D_80059460 = 2; assert(run(2) == 2 && last == 0x304);
    D_80059460 = 1; assert(run(2) == 3 && last == 0xD90);
    printf("PASS menu card state pins (CD710 0->7, 1->6, 2->2/3)\n");
    return 0;
}

/* Pins func_801E20C8 (retail 0x801E20C8): only slot types 7..8 bounce
 * (unsigned type-7 < 2), and the member step calls func_801D9704(slotIdx,
 * dir, 0).  The port once tested the type signed (every type below 9
 * bounced) and dropped the current slot (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static MenuManager manager;
static int bounced, built, frames, last_cur = -1, last_dir = -1;
static u8 inputs[8]; static int ninputs;
void func_801C8574(s32 a) { assert(a == 4); bounced++; }
void func_801E1014(void) {}
void func_801E1398(void) {}
void func_801E1AC8(u8 slot) { (void)slot; built++; }
void func_801C7BF4(void) { menu.input = frames < ninputs ? inputs[frames] : 5; frames++; }
u8 func_801D9704(s32 cur, s32 dir, s32 gear) { assert(gear == 0); last_cur = cur; last_dir = dir; return (u8)((cur + 1) % 3); }
#include "status.inc"
int main(void) {
    g_Menu = &menu;
    menu.pManager = &manager;
    manager.currentCharacterIDs[0] = 3;
    manager.currentCharacterIDs[1] = 7;   /* skipped */
    manager.currentCharacterIDs[2] = 1;
    ninputs = 0;
    assert(func_801E20C8(0) == 1 && bounced == 0 && built == 1);
    manager.currentCharacterIDs[0] = 8;
    assert(func_801E20C8(0) == 1 && bounced == 1);
    manager.currentCharacterIDs[0] = 3;
    frames = 0; built = 0; inputs[0] = 9; ninputs = 1;
    assert(func_801E20C8(0) == 1);
    assert(last_cur == 1 && last_dir == 0 && built == 2); /* 0 -> 1 (type 7) -> 2 */
    printf("PASS menu status loop pins (E20C8 unsigned type test, D9704 current)\n");
    return 0;
}

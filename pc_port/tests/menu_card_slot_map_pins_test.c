/* Pins the host mapping of the card-screen PSX-width pointer slots
 * (src/menu/main/misc.c MenuPsxPointerSlot) and the unk4CC animation-word
 * mapping (MENU_4CC, used by the shared body func_801D01D0).
 *
 * Retail g_Menu+0x3A8..0x424 hold 32 slot-buffer pointers (func_801E5ACC
 * allocates them, func_801E5B3C frees them).  The host SystemMenu is
 * pointer-inflated, so every slot must land in unk39C[0xC + i*4]; the port
 * once mapped only the first 8 and let slots 8..31 hit raw host offsets.
 * g_Menu+0x4CC..0x4D9 must land in unk4CC: raw host g_Menu+0x4CC is inside
 * unk39C, on slot pointers 10..13. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "common.h"
#include "system/menu.h"

SystemMenu* g_Menu;

#include "slot_map.inc"

#define MENU_4CC(p, off) ((u8*)((SystemMenu*)(p))->unk4CC + ((off) - 0x4CC))

static int failures;
static void check(int ok, const char* what)
{
    if (!ok) {
        fprintf(stderr, "ASSERTION %s\n", what);
        failures++;
    }
}

int main(void)
{
    int i;
    u8* lo;
    u8* hi;

    g_Menu = calloc(1, sizeof(SystemMenu));
    lo = (u8*)g_Menu->unk39C;
    hi = lo + sizeof(g_Menu->unk39C);
    for (i = 0; i < 32; i++) {
        u32* slot = MenuPsxPointerSlot(0x3A8 + i * 4);
        check(slot == (u32*)&g_Menu->unk39C[0xC + i * 4], "slot.maps.to.unk39C");
        check((u8*)slot >= lo && (u8*)(slot + 1) <= hi, "slot.inside.unk39C");
    }
    check(MENU_4CC(g_Menu, 0x4CC) == &g_Menu->unk4CC[0], "4cc.frame.word");
    check(MENU_4CC(g_Menu, 0x4D0) == &g_Menu->unk4CC[4], "4cc.tick.word");
    check(MENU_4CC(g_Menu, 0x4D4) == &g_Menu->unk4CC[8], "4cc.pulse.word");
    check(MENU_4CC(g_Menu, 0x4D9) == &g_Menu->unk4CC[0xD], "4cc.direction.byte");
    /* The animation words must not overlap any slot pointer. */
    for (i = 0; i < 32; i++) {
        u8* s = &g_Menu->unk39C[0xC + i * 4];
        check(!(s < &g_Menu->unk4CC[0x10] && s + 4 > &g_Menu->unk4CC[0]),
              "4cc.disjoint.from.slots");
    }
    if (failures) {
        fprintf(stderr, "menu_card_slot_map_pins_test: FAIL (%d)\n", failures);
        return 1;
    }
    printf("menu_card_slot_map_pins_test: PASS\n");
    return 0;
}

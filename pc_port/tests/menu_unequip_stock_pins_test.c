/* Pins func_801E0434 (retail 0x801E0434): an item already stocked gains one
 * (cap 99) and is NOT inserted again; an unstocked item takes the first empty
 * entry with quantity 1.  The port once inserted a duplicate whenever the
 * count stayed below the cap (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
GameState g_GameState;
static SystemMenu menu;
static MenuManager manager;
#include "stock.inc"
int main(void) {
    GameCharacter *c;
    g_Menu = &menu;
    menu.pManager = &manager;
    manager.currentCharacterIDs[0] = 2;
    c = &g_GameState.characters[2];
    /* held weapon 7 at entry 3 with 5 */
    g_GameState.weaponIDs[3] = 7; g_GameState.weaponQuantities[3] = 5;
    c->unk6F[0] = 7;
    func_801E0434(0, 0);
    assert(c->unk6F[0] == 0);
    assert(g_GameState.weaponQuantities[3] == 6);
    assert(g_GameState.weaponIDs[0] == 0 && g_GameState.weaponQuantities[0] == 0);
    /* capped */
    g_GameState.weaponQuantities[3] = 99; c->unk6F[0] = 7;
    func_801E0434(0, 0);
    assert(g_GameState.weaponQuantities[3] == 99 && g_GameState.weaponIDs[0] == 0);
    /* new weapon 9 -> first empty entry (0) */
    c->unk6F[0] = 9;
    func_801E0434(0, 0);
    assert(g_GameState.weaponIDs[0] == 9 && g_GameState.weaponQuantities[0] == 1);
    /* gear part (mode 1) */
    c->gearId = 4;
    g_GameState.gears[4].field_0x0[4] = 0x21;
    func_801E0434(0, 1);
    assert(g_GameState.gears[4].field_0x0[4] == 0);
    assert(g_GameState.unk2120IDs[0] == 0x21 && g_GameState.unk20BCQuantities[0] == 1);
    printf("PASS menu unequip stock pins (E0434 no duplicate insert)\n");
    return 0;
}

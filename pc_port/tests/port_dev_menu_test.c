#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "main/game.h"
#include "port_dev_menu.h"
GameState g_GameState;
GameState *g_pGameState = &g_GameState;
static int optin, safe, queued, warp_map = -1;
static char command[80];
int xg_plat_config_int(const char *key, int fallback) { assert(!strcmp(key, "cheats.dev_menu")); return optin ? 1 : fallback; }
int PcPort_QuickCheckpointFieldIsSafe(void) { return safe; }
int PcPort_QuickCheckpointRequestWarp(int map, int entrance) { assert(entrance == 0); warp_map = map; return safe; }
int PcPort_GodModeEnabled(void) { return 0; }
int PcPort_RandomBattlesEnabled(void) { return 1; }
void PcPort_CheatQueue(const char *s) { queued++; snprintf(command, sizeof command, "%s", s); }
int main(void) {
    GameState before;
    memset(&g_GameState, 0xA5, sizeof g_GameState);
    before = g_GameState;
    PcPort_DevMenuInit();
    assert(!PcPort_DevMenuEnabled());
    assert(!PcPort_DevMenuKey(PC_DEV_OPEN, 1));
    assert(!PcPort_DevMenuFilterPad(0xFFFF));
    assert(PcPort_DevMenuAction("heal") == -1);
    assert(!memcmp(&before, &g_GameState, sizeof before) && !queued);
    optin = 1; PcPort_DevMenuInit();
    assert(PcPort_DevMenuKey(PC_DEV_OPEN, 1));
    assert(PcPort_DevMenuOpen());
    assert(PcPort_DevMenuFilterPad(0));
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1); /* cheats */
    PcPort_DevMenuKey(PC_DEV_ACCEPT, 1); /* queue refill, no writes here */
    assert(queued == 1 && !strcmp(command, "dev heal"));
    assert(!PcPort_DevMenuOpen());
    assert(PcPort_DevMenuFilterPad(0x4000));
    assert(PcPort_DevMenuFilterPad(0));
    assert(!PcPort_DevMenuFilterPad(0));
    assert(!memcmp(&before, &g_GameState, sizeof before));
    assert(PcPort_DevMenuAction("heal") == -1); /* scene lock */
    assert(!memcmp(&before, &g_GameState, sizeof before));
    memset(&g_GameState, 0, sizeof g_GameState);
    safe = 1;
    g_GameState.partyMembers[0] = 0; g_GameState.partyMembers[1] = 10; g_GameState.partyMembers[2] = CHARACTER_ID_NONE;
    g_GameState.characters[0].maxHp = 123; g_GameState.characters[0].maxMp = 45;
    g_GameState.characters[10].maxHp = 600; g_GameState.characters[10].maxMp = 300;
    g_GameState.characters[1].hp = 9;
    assert(PcPort_DevMenuAction("heal") == 1);
    assert(g_GameState.characters[0].hp == 123 && g_GameState.characters[0].mp == 45);
    assert(g_GameState.characters[10].hp == 600 && g_GameState.characters[10].mp == 300);
    assert(g_GameState.characters[1].hp == 9);
    g_GameState.gold = MAX_GOLD_AMOUNT - 2;
    assert(PcPort_DevMenuAction("gold") == 1 && g_GameState.gold == MAX_GOLD_AMOUNT);
    g_GameState.itemIDs[0] = 1; g_GameState.itemQuantities[0] = 95;
    g_GameState.itemIDs[1] = 2; g_GameState.itemQuantities[1] = 4;
    assert(PcPort_DevMenuAction("items") == 1);
    assert(g_GameState.itemQuantities[0] == 99 && g_GameState.itemQuantities[1] == 14 && !g_GameState.itemQuantities[2]);
    assert(PcPort_DevMenuAction("warp 16") == 1 && warp_map == 16);
    assert(PcPort_DevMenuAction("warp 1023") == -1 && warp_map == 16);
    puts("port_dev_menu: disabled/inert, navigation, input drain, safe-state mutations and caps PASS");
    return 0;
}

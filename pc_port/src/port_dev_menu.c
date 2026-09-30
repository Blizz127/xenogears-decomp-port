/* Opt-in host menu. Input changes menu state only; all game mutations run
 * through the existing cheat queue on the game thread, at a safe field frame. */
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "common.h"
#include "main/game.h"
#include "port_dev_menu.h"
#include "cheat_console.h"
#include "quick_checkpoint.h"
#include "god_mode.h"
#include "../include/xg_plat/config.h"

static atomic_int enabled, opened, drain, page, cursor;
static char status[96] = "CHEATS ARE OFF UNTIL SELECTED";
static const char *const home[] = { "CHEATS", "WARP BY AREA", "QUICK SAVE", "QUICK LOAD", "CLOSE" };
static const char *const cheats[] = { "REFILL PARTY HP AND EP", "ADD 10000 GOLD", "TOP UP OWNED ITEMS", "GOD MODE", "RANDOM ENCOUNTERS", "BACK" };
static const char *const areas[] = { "DISC 1 - LAHAN", "DISC 1 - BLACK MOON", "BACK" };
/* Candidate destinations are kept small. Release inclusion requires a real-
 * disc load, controllable player and subsequent exit in the warp audit. */
static const char *const lahan[] = { "LAHAN - ENTRANCE", "LAHAN - VILLAGE", "BACK" };
static const char *const forest[] = { "MOUNTAIN PATH", "BLACK MOON FOREST", "BACK" };

void PcPort_DevMenuInit(void) { atomic_store(&enabled, xg_plat_config_int("cheats.dev_menu", 0) != 0); }
int PcPort_DevMenuEnabled(void) { return atomic_load(&enabled); }
int PcPort_DevMenuOpen(void) { return PcPort_DevMenuEnabled() && atomic_load(&opened); }
int PcPort_DevMenuCursor(void) { return atomic_load(&cursor); }
const char *PcPort_DevMenuStatus(void) { return status; }
const char *PcPort_DevMenuTitle(void) {
    switch (atomic_load(&page)) {
    case 1: return "CHEATS";
    case 2: return "WARP BY AREA";
    case 3: return "DISC 1 - LAHAN";
    case 4: return "DISC 1 - BLACK MOON";
    default: return "PORT DEV MENU";
    }
}
int PcPort_DevMenuRows(void) {
    switch (atomic_load(&page)) { case 0: return 5; case 1: return 6; default: return 3; }
}
const char *PcPort_DevMenuRow(int row) {
    int p = atomic_load(&page);
    if (row < 0 || row >= PcPort_DevMenuRows()) return "";
    if (p == 1 && row == 3) return PcPort_GodModeEnabled() ? "GOD MODE - ON" : "GOD MODE - OFF";
    if (p == 1 && row == 4) return PcPort_RandomBattlesEnabled() ? "RANDOM ENCOUNTERS - ON" : "RANDOM ENCOUNTERS - OFF";
    return (p == 0 ? home : p == 1 ? cheats : p == 2 ? areas : p == 3 ? lahan : forest)[row];
}
static void enter(int p) { atomic_store(&page, p); atomic_store(&cursor, 0); }
static void close_menu(void) { atomic_store(&opened, 0); atomic_store(&drain, 1); }
int PcPort_DevMenuFilterPad(unsigned held) {
    if (!PcPort_DevMenuEnabled()) return 0;
    if (PcPort_DevMenuOpen()) return 1;
    if (!atomic_load(&drain)) return 0;
    if (!held) atomic_store(&drain, 0);
    return 1; /* consume the final release as well as held closing inputs */
}
int PcPort_DevMenuKey(int key, int pressed) {
    int p, i, n;
    if (!PcPort_DevMenuEnabled()) return 0;
    if (key == PC_DEV_OPEN) {
        if (pressed) { if (PcPort_DevMenuOpen()) close_menu(); else { enter(0); atomic_store(&opened, 1); } }
        return 1;
    }
    if (!PcPort_DevMenuOpen()) return 0;
    if (!pressed) return 1;
    p = atomic_load(&page); i = atomic_load(&cursor); n = PcPort_DevMenuRows();
    if (key == PC_DEV_UP || key == PC_DEV_DOWN) {
        atomic_store(&cursor, (i + (key == PC_DEV_UP ? n - 1 : 1)) % n);
    } else if (key == PC_DEV_BACK || (key == PC_DEV_ACCEPT && i == n - 1)) {
        if (!p) close_menu(); else enter(p >= 3 ? 2 : 0);
    } else if (key == PC_DEV_ACCEPT) {
        const char *command = NULL;
        if (p == 0) { if (i < 2) enter(i + 1); else command = i == 2 ? "save" : "load"; }
        else if (p == 1) {
            static const char *const commands[] = { "dev heal", "dev gold", "dev items", "god", "encounters" };
            command = commands[i];
        } else if (p == 2) enter(i + 3);
        else if (p == 3) command = i == 0 ? "dev warp 1" : "dev warp 2";
        else if (p == 4) command = i == 0 ? "dev warp 15" : "dev warp 16";
        if (command) { PcPort_CheatQueue(command); snprintf(status, sizeof status, "ACTION QUEUED"); close_menu(); }
    }
    return 1;
}
int PcPort_DevMenuAction(const char *command) {
    int i, map;
    if (!PcPort_DevMenuEnabled()) return -1;
    if (!PcPort_QuickCheckpointFieldIsSafe()) {
        snprintf(status, sizeof status, "WAIT FOR FREE FIELD CONTROL");
        char why[160];
        PcPort_QuickCheckpointSafetyDescribe(why, sizeof why);
        fprintf(stderr, "[dev-menu] rejected %s: field is not safe (%s)\n", command, why);
        return -1;
    }
    if (!strcmp(command, "heal")) {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            unsigned id = g_pGameState->partyMembers[i];
            if (id < MAX_GAME_CHARACTERS) {
                GameCharacter *c = &g_pGameState->characters[id];
                c->hp = c->maxHp; c->mp = c->maxMp;
            }
        }
        snprintf(status, sizeof status, "PARTY HP AND EP REFILLED");
    } else if (!strcmp(command, "gold")) {
        unsigned gold = g_pGameState->gold;
        g_pGameState->gold = gold >= MAX_GOLD_AMOUNT - 10000u ? MAX_GOLD_AMOUNT : gold + 10000u;
        snprintf(status, sizeof status, "ADDED GOLD - CAPPED AT MAXIMUM");
    } else if (!strcmp(command, "items")) {
        for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
            unsigned q = g_pGameState->itemQuantities[i];
            if (q && g_pGameState->itemIDs[i]) g_pGameState->itemQuantities[i] = q >= MAX_ITEM_QUANTITY - 10 ? MAX_ITEM_QUANTITY : q + 10;
        }
        snprintf(status, sizeof status, "OWNED ITEMS INCREASED BY 10");
    } else if (sscanf(command, "warp %d", &map) == 1 && (map == 1 || map == 2 || map == 15 || map == 16)) {
        if (!PcPort_QuickCheckpointRequestWarp(map, 0)) return -1;
        snprintf(status, sizeof status, "FIELD WARP REQUESTED");
    } else return -1;
    fprintf(stderr, "[dev-menu] %s: %s\n", command, status);
    return 1;
}

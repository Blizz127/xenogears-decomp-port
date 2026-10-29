/* Dev-menu game-state actions, run from the cheat queue ("dev <command>") on
 * the game thread.  Each writes only the game's own state, bounded by the
 * game's own limits, and only at a safe field frame (DEV_MENU_SPEC.md §8). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "main/game.h"
#include "port_dev_menu.h"
#include "quick_checkpoint.h"

/* The game's own party-member recovery routines (src/field/party/stats.c):
 * clamp-to-max HP / EP increases, indexed by party slot, as the inn and the
 * script opcodes use them.  Plain C, no script VM state involved. */
extern void FieldPartyMemberIncreaseHp(int partyIndex, int amount);
extern void FieldPartyMemberIncreaseMp(int partyIndex, int amount);
extern void PcPort_PadTestInputHold(unsigned held);
extern unsigned PcPort_FieldLoopCount(void);
extern int g_GameSceneMapNum;

static int refuse(const char *command, const char *why)
{
    fprintf(stderr, "[CHEATS] refused '%s': %s\n", command, why);
    return -1;
}

static void status(const char *text) { PcPort_DevMenuSetStatus(text); }

/* A warp the game cannot take right now (the menu has already closed with
 * "WARP REQUESTED"): say why on screen, with the quick save/load notice,
 * instead of only in the log (owner report 2026-10-05: "the warps don't
 * work").  Warps need free field control, like F11. */
static int warp_refused(const char *command)
{
    const char *why = PcPort_QuickCheckpointFieldIsSafe()
        ? "a room change or checkpoint is already pending"
        : "only in free field control (not during a scene, menu or the title)";
    PcPort_QuickCheckpointNoticeSet(PcPort_QuickCheckpointFieldIsSafe()
        ? "WARP UNAVAILABLE: ANOTHER ROOM CHANGE IS PENDING"
        : "WARP UNAVAILABLE: ONLY WHEN YOU CAN WALK FREELY");
    status("WARP UNAVAILABLE");
    return refuse(command, why);
}

/* Harness/diagnostic read-out: one line, always available while the dev menu
 * is enabled (no field-safety gate; it only reads). */
static void print_state(void)
{
    int i, pos[3] = {0, 0, 0}, have = PcPort_QuickCheckpointPlayerPosition(pos);
    unsigned ms = 0, slots = 0, total = 0;
    int input = PcPort_DevMenuInputState(&ms);
    fprintf(stderr, "[CHEATS] state: loop=%u map=%d safe=%d input=%s pos=", PcPort_FieldLoopCount(),
            (int)(g_GameSceneMapNum & 0x3FFF), PcPort_QuickCheckpointFieldIsSafe(),
            input == 0 ? "game" : input == 1 ? "menu" : "drain");
    if (have) fprintf(stderr, "(%d,%d,%d)", pos[0], pos[1], pos[2]); else fprintf(stderr, "none");
    fprintf(stderr, " party=");
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        int id = g_GamePartyMembers[i];
        if (id >= 0 && id < MAX_GAME_CHARACTERS) {
            const GameCharacter *c = &g_pGameState->characters[id];
            fprintf(stderr, "%s%d:%u/%u,%u/%u", i ? ";" : "", id, (unsigned)c->hp, (unsigned)c->maxHp,
                    (unsigned)c->mp, (unsigned)c->maxMp);
        }
    }
    for (i = 0; i < MAX_INVENTORY_ITEMS; i++)
        if (g_pGameState->itemIDs[i] && g_pGameState->itemQuantities[i]) {
            slots++; total += g_pGameState->itemQuantities[i];
        }
    fprintf(stderr, " gold=%u itemSlots=%u itemTotal=%u\n", (unsigned)g_pGameState->gold, slots, total);
}

extern void* g_FieldScriptMemory;
extern int g_GameSceneMapNum;
extern void FieldScriptMemoryWriteU16(int index, int value);
extern void func_800A30FC(void); /* the game's own script-memory -> g_GameState flush */

static unsigned scenario_now(void) { return ((unsigned short*)&g_FieldScriptMemory)[0]; }
static unsigned var_now(unsigned addr) { return ((unsigned short*)&g_FieldScriptMemory)[(addr >> 1) & 0x3FF]; }

/* Field-change request through the quick-checkpoint teardown path, then the
 * recorded script-variable values through the game's own writer and its own
 * flush (func_800A30FC) so the checkpoint copy of g_GameState carries them. */
static int warp_with(const char *command, int map, int entrance, const PcPortDevStepWrite *w, int nw, int scen)
{
    int i;
    if (!PcPort_QuickCheckpointRequestWarp(map, entrance))
        return -1;
    if (scen >= 0) FieldScriptMemoryWriteU16(0, scen);
    for (i = 0; i < nw; i++) FieldScriptMemoryWriteU16(w[i].addr, w[i].value);
    if (scen >= 0 || nw > 0) func_800A30FC();
    (void)command;
    return 0;
}

static int step_matches(const PcPortDevStep *st)
{
    if (!st->evidence) return 0;
    if (scenario_now() != st->detect_scen) return 0;
    if (st->detect_map >= 0 && (g_GameSceneMapNum & 0x3FFF) != st->detect_map) return 0;
    if (st->detect_var_addr != PC_DEV_NO_VALUE && var_now(st->detect_var_addr) != st->detect_var_value) return 0;
    return 1;
}

static int find_step(void)
{
    int i;
    for (i = 0; i < g_PcPortDevStepCount; i++)
        if (step_matches(&g_PcPortDevSteps[i])) return i;
    return -1;
}

/* Apply step `index` (detect already checked by the caller when needed). */
static int finish_step(const char *command, int index)
{
    const PcPortDevStep *st = &g_PcPortDevSteps[index];
    char toast[96];
    int i;
    if (warp_with(command, st->next_map, st->next_entrance, st->writes, st->nwrites, -1) != 0)
        return warp_refused(command);
    fprintf(stderr, "[DEV_MENU] step finished: %s '%s' (area %s), %d writes:", st->id, st->name, st->area, st->nwrites);
    for (i = 0; i < st->nwrites; i++) fprintf(stderr, " var[0x%x]=%u", st->writes[i].addr, st->writes[i].value);
    fprintf(stderr, " -> map %d entrance %d\n", st->next_map, st->next_entrance);
    snprintf(toast, sizeof toast, "DONE %.40s", st->name);
    PcPort_DevMenuSetStatus(toast);
    return 1;
}

int PcPort_DevMenuAction(const char *command)
{
    int i, index, map, entrance, scen;
    unsigned hold;
    if (!PcPort_DevMenuEnabled()) return refuse(command, "dev menu is off");
    /* Read-only / harness commands: no field gate. */
    if (!strcmp(command, "state")) { print_state(); return 1; }
    if (sscanf(command, "pad %x", &hold) == 1) {
        PcPort_PadTestInputHold(hold);
        fprintf(stderr, "[CHEATS] pad: harness hold 0x%04x\n", hold);
        return 1;
    }
    if (!strcmp(command, "hurt")) {   /* harness: put the party at 1 HP / 0 EP, like a fight would */
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            int id = g_GamePartyMembers[i];
            if (id >= 0 && id < MAX_GAME_CHARACTERS) {
                g_pGameState->characters[id].hp = 1;
                g_pGameState->characters[id].mp = 0;
            }
        }
        fprintf(stderr, "[CHEATS] hurt: party set to 1 HP / 0 EP (test harness)\n");
        return 1;
    }
    if (sscanf(command, "setgold %u", &hold) == 1) {   /* harness: set gold (capped) */
        g_pGameState->gold = hold > MAX_GOLD_AMOUNT ? MAX_GOLD_AMOUNT : hold;
        fprintf(stderr, "[CHEATS] setgold: %u (test harness)\n", (unsigned)g_pGameState->gold);
        return 1;
    }
    if (sscanf(command, "seed %d:%u", &index, &hold) == 2 && index > 0 && index < 256) {
        /* harness: give an item the way the shop does (ShopMenuHandleBoughtItems,
         * src/shop_menu/main/misc8.c:358-380): add to an owned stack or take the first empty slot. */
        for (i = 0; i < MAX_INVENTORY_ITEMS; i++)
            if (g_pGameState->itemIDs[i] == index) { hold += g_pGameState->itemQuantities[i]; break; }
        if (i == MAX_INVENTORY_ITEMS)
            for (i = 0; i < MAX_INVENTORY_ITEMS && g_pGameState->itemIDs[i]; i++) ;
        if (i == MAX_INVENTORY_ITEMS) return refuse(command, "inventory full");
        g_pGameState->itemIDs[i] = (u_char)index;
        g_pGameState->itemQuantities[i] = (u_char)(hold > MAX_ITEM_QUANTITY ? MAX_ITEM_QUANTITY : hold);
        fprintf(stderr, "[CHEATS] seed: item %d x%u in slot %d (test harness)\n", index,
                (unsigned)g_pGameState->itemQuantities[i], i);
        return 1;
    }
    if (!strncmp(command, "key ", 4)) {
        static const struct { const char *n; int k; } keys[] = {
            {"f8", PC_DEV_OPEN}, {"up", PC_DEV_UP}, {"down", PC_DEV_DOWN}, {"enter", PC_DEV_ACCEPT},
            {"esc", PC_DEV_BACK}};
        for (i = 0; i < 5; i++)
            if (!strcmp(command + 4, keys[i].n)) {
                PcPort_DevMenuKey(keys[i].k, 1);
                fprintf(stderr, "[CHEATS] key: %s\n", keys[i].n);
                return 1;
            }
        return refuse(command, "unknown key");
    }
    if (!PcPort_QuickCheckpointFieldIsSafe()) {
        char why[160];
        PcPort_QuickCheckpointSafetyDescribe(why, sizeof why);
        status("NOT ON A FREE FIELD - NO CHANGE");
        /* Warps and steps close the menu first, so the status line above is
         * never seen: show the refusal on screen as well. */
        PcPort_QuickCheckpointNoticeSet(!strncmp(command, "warp", 4) || !strncmp(command, "step", 4)
            ? "WARP UNAVAILABLE: ONLY WHEN YOU CAN WALK FREELY"
            : "DEV MENU: ONLY WHEN YOU CAN WALK FREELY");
        return refuse(command, why);
    }
    if (!strcmp(command, "heal")) {
        /* Full recovery through the game's own party routines.  Both clamp
         * to max (stats.c:7-12 / :26-31), so passing the missing amount lands
         * exactly on max and nothing else is written.  Party slots only
         * (g_GamePartyMembers, 0xFF = empty), never the benched roster. */
        int n = 0;
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            int id = g_GamePartyMembers[i];
            if (id >= 0 && id < MAX_GAME_CHARACTERS) {
                GameCharacter *c = &g_pGameState->characters[id];
                FieldPartyMemberIncreaseHp(i, c->maxHp > c->hp ? c->maxHp - c->hp : 0);
                FieldPartyMemberIncreaseMp(i, c->maxMp > c->mp ? c->maxMp - c->mp : 0);
                n++;
            }
        }
        fprintf(stderr, "[CHEATS] heal: %d party members refilled to their max HP/EP\n", n);
        status("PARTY REFILLED");
    } else if (!strcmp(command, "gold")) {
        /* Same write as the field script opcode FieldScriptIncreaseGold
         * (src/field/game_logic/gold.c:26-30) and the shop
         * (src/shop_menu/main/misc8.c:299-302): add, clamp to MAX_GOLD_AMOUNT. */
        unsigned gold = g_pGameState->gold;
        unsigned next = gold + 10000u;
        if (next > MAX_GOLD_AMOUNT) next = MAX_GOLD_AMOUNT;
        g_pGameState->gold = next;
        fprintf(stderr, "[CHEATS] gold: %u -> %u (cap %u)\n", gold, (unsigned)g_pGameState->gold,
                (unsigned)MAX_GOLD_AMOUNT);
        status("GOLD ADDED");
    } else if (!strcmp(command, "items")) {
        /* Same write as the shop's buy (ShopMenuHandleBoughtItems,
         * src/shop_menu/main/misc8.c:361-368): an owned stack gains quantity,
         * clamped to MAX_ITEM_QUANTITY.  No new slots are created. */
        int n = 0;
        for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
            unsigned q = g_pGameState->itemQuantities[i];
            if (q && g_pGameState->itemIDs[i]) {
                q += 10;
                if (q > MAX_ITEM_QUANTITY) q = MAX_ITEM_QUANTITY;
                g_pGameState->itemQuantities[i] = (u_char)q;
                n++;
            }
        }
        fprintf(stderr, "[CHEATS] items: %d owned stacks +10 (cap %u)\n", n, (unsigned)MAX_ITEM_QUANTITY);
        status("ITEMS TOPPED UP");
    } else if (sscanf(command, "warp %d", &index) == 1) {
        const PcPortDevWarp *w;
        if (index < 0 || index >= g_PcPortDevWarpCount || !g_PcPortDevWarps[index].evidence)
            return refuse(command, "not a verified warp");
        w = &g_PcPortDevWarps[index];
        if (warp_with(command, w->map, w->entrance, NULL, 0,
                      w->progression == PC_DEV_NO_SCENARIO ? -1 : (int)w->progression) != 0)
            return warp_refused(command);
        fprintf(stderr, "[CHEATS] warp requested: %s / %s (map %d entrance %d scenario %d)\n",
                w->area, w->spot, w->map, w->entrance,
                w->progression == PC_DEV_NO_SCENARIO ? -1 : (int)w->progression);
    } else if (sscanf(command, "warpx %d %d %d", &map, &entrance, &scen) == 3) {
        /* Sweep harness only: an arbitrary candidate, never in the menu. */
        const char *u = getenv("XENO_DEV_MENU_UNVERIFIED");
        if (!u || u[0] != '1') return refuse(command, "warpx needs XENO_DEV_MENU_UNVERIFIED=1");
        if (warp_with(command, map, entrance, NULL, 0, scen) != 0)
            return refuse(command, "a room change or checkpoint is already pending");
        fprintf(stderr, "[CHEATS] warpx requested: map %d entrance %d scenario %d\n", map, entrance, scen);
    } else if (!strncmp(command, "stepraw ", 8)) {
        /* Sweep harness only: one step delta given inline as
         * `stepraw <next_map> <next_entrance> <addr:value,addr:value,...>`;
         * the same apply path as a table row (finish_step). */
        const char *u = getenv("XENO_DEV_MENU_UNVERIFIED");
        PcPortDevStepWrite w[16];
        int nw = 0;
        char list[96];
        char *tok, *save = NULL;
        if (!u || u[0] != '1') return refuse(command, "stepraw needs XENO_DEV_MENU_UNVERIFIED=1");
        if (sscanf(command, "stepraw %d %d %95s", &map, &entrance, list) != 3)
            return refuse(command, "usage: stepraw <map> <entrance> <addr:value,...>");
        for (tok = strtok_r(list, ",", &save); tok && nw < 16; tok = strtok_r(NULL, ",", &save)) {
            unsigned a, v;
            if (sscanf(tok, "%u:%u", &a, &v) == 2) { w[nw].addr = (unsigned short)a; w[nw].value = (unsigned short)v; nw++; }
        }
        if (warp_with(command, map, entrance, w, nw, -1) != 0)
            return refuse(command, "a room change or checkpoint is already pending");
        fprintf(stderr, "[DEV_MENU] step finished: stepraw (%d writes) -> map %d entrance %d\n", nw, map, entrance);
        return 1;
    } else if (!strcmp(command, "step")) {
        index = find_step();
        if (index < 0) return refuse(command, "no verified step matches the current story state");
        return finish_step(command, index);
    } else if (sscanf(command, "stepx %d", &index) == 1) {
        const char *u = getenv("XENO_DEV_MENU_UNVERIFIED");
        if (!u || u[0] != '1') return refuse(command, "stepx needs XENO_DEV_MENU_UNVERIFIED=1");
        if (index < 0 || index >= g_PcPortDevStepCount) return refuse(command, "no such step");
        return finish_step(command, index);
    } else if (sscanf(command, "finish %d", &index) == 1) {
        return refuse(command, "no verified area finish");
    } else {
        return refuse(command, "unknown dev command");
    }
    return 1;
}

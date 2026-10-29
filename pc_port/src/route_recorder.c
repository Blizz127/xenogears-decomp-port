/* route_recorder.c -- TEST TOOLING: progression-state recorder for the dev-menu
 * warp / finish-step tables (DEV_MENU_SPEC.md sec. 6, 7, 7a).
 *
 * Inert unless XENO_ROUTE_LOG=<file> is set: every hook starts with a single
 * integer test, writes no guest byte and changes no game behaviour.  With the
 * variable set, each of these events appends one JSON line to the file:
 *
 *   fieldload  FieldLoad (src/field/main/misc3.c) has begun for a map
 *   change     CHANGE_FIELD (func_800932D0 / func_800933F8, src/field/main/misc11.c)
 *   var        a field-script variable write that CHANGED a value
 *              (FieldScriptMemoryWriteU16), with the writing actor and its
 *              instruction pointer -- SCRIPT_VAR_SCENARIO_FLAG is address 0
 *   trigger    an NPC talk (scriptId 2) or touch/event (scriptId 3) script is
 *              started against an actor (src/field/main/misc8.c)
 *   state      the per-frame poll saw party / gold / inventory change
 *
 * Every line carries the game's own progression state: map, entrance, the
 * scenario word, the party, gold, the inventory stacks, and (on fieldload /
 * change / trigger) the set of script variables that differ from the previous
 * line.  The analyzer (tools/analysis/route_steps.py) turns the lines into
 * steps.  REMOVAL: delete this file, its PORT_SOURCES row and the four
 * `#ifdef XENO_PC_PORT` hook calls. */
#include <execinfo.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "main/game.h"
#include "route_recorder.h"
#include "../include/xg_plat/mods.h"

extern void* g_FieldScriptMemory;
extern int g_GameSceneMapNum;
extern s32 D_800AFD1C;
extern ActorData* g_FieldScriptVMCurActor;
extern FieldActor* volatile g_FieldActors;
extern s32 D_800ADBFC;

#define VAR_WORDS 0x400   /* the field script memory is 0x800 bytes */
#define MAX_VAR_LINES_PER_TICK 96

static FILE* s_log;
static int s_init, s_on;
static unsigned s_tick, s_seq, s_writesThisTick, s_dropped;
static u16 s_prev[VAR_WORDS];
static int s_havePrev;
/* last polled inventory / party / gold */
static u8 s_prevItemQ[MAX_INVENTORY_ITEMS], s_prevItemId[MAX_INVENTORY_ITEMS];
static u8 s_prevParty[MAX_PARTY_MEMBERS];
static u32 s_prevGold;
static int s_prevMap = -2;

static u16* vars(void) { return (u16*)&g_FieldScriptMemory; }

int PcPort_RouteLogOn(void)
{
    if (!s_init) {
        const char* path = getenv("XENO_ROUTE_LOG");
        s_init = 1;
        if (path && path[0]) {
            s_log = fopen(path, "a");
            s_on = s_log != NULL;
            if (s_on) setvbuf(s_log, NULL, _IOLBF, 0);
        }
    }
    return s_on;
}

static int map_now(void) { return g_GameSceneMapNum & 0x3FFF; }

static void head(const char* ev)
{
    const u16* v = vars();
    fprintf(s_log, "{\"n\":%u,\"tick\":%u,\"ev\":\"%s\",\"map\":%d,\"ent\":%u,\"scen\":%u",
            ++s_seq, s_tick, ev, map_now(), (unsigned)v[1], (unsigned)v[0]);
}

static void party_gold_items(void)
{
    int i, first = 1;
    fprintf(s_log, ",\"party\":[");
    for (i = 0; i < MAX_PARTY_MEMBERS; i++)
        fprintf(s_log, "%s%d", i ? "," : "", (int)g_GamePartyMembers[i]);
    fprintf(s_log, "],\"gold\":%u,\"items\":[", (unsigned)g_GameState.gold);
    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (g_GameState.itemQuantities[i] && g_GameState.itemIDs[i]) {
            fprintf(s_log, "%s[%u,%u]", first ? "" : ",", (unsigned)g_GameState.itemIDs[i],
                    (unsigned)g_GameState.itemQuantities[i]);
            first = 0;
        }
    }
    fprintf(s_log, "]");
}

/* Script variables that differ from the previous snapshot, as
 * [byte address, old, new]; the snapshot is then advanced. */
static void var_diff(void)
{
    const u16* v = vars();
    int i, first = 1;
    fprintf(s_log, ",\"vdiff\":[");
    for (i = 0; i < VAR_WORDS; i++) {
        if (s_havePrev ? v[i] != s_prev[i] : v[i] != 0) {
            fprintf(s_log, "%s[%d,%u,%u]", first ? "" : ",", i * 2, s_havePrev ? (unsigned)s_prev[i] : 0u,
                    (unsigned)v[i]);
            first = 0;
        }
        s_prev[i] = v[i];
    }
    s_havePrev = 1;
    fprintf(s_log, "]");
}

static void actor_pos(int i, int* x, int* z)
{
    *x = *z = 0;
    if (i >= 0 && i < D_800ADBFC && g_FieldActors && g_FieldActors[i].pActorData) {
        ActorData* a = (ActorData*)(uintptr_t)g_FieldActors[i].pActorData;
        *x = (int)(a->position.vx >> 16);
        *z = (int)(a->position.vz >> 16);
    }
}

static void tail(void) { fprintf(s_log, "}\n"); }

void PcPort_RouteLogFieldLoad(int map)
{
    if (!PcPort_RouteLogOn()) return;
    head("fieldload");
    fprintf(s_log, ",\"to\":%d", map & 0x3FFF);
    party_gold_items();
    var_diff();
    tail();
}

void PcPort_RouteLogChangeField(int map, int entrance)
{
    if (!PcPort_RouteLogOn()) return;
    head("change");
    fprintf(s_log, ",\"from\":%d,\"to\":%d,\"to_ent\":%d,\"actor\":%d", map_now(), map & 0x3FFF, entrance,
            (int)D_800AFD1C);
    party_gold_items();
    var_diff();
    tail();
}

void PcPort_RouteLogScriptWrite(int address, int value)
{
    u16 old;
    if (!PcPort_RouteLogOn()) return;
    old = vars()[(address >> 1) & (VAR_WORDS - 1)];
    if (old == (u16)value) return;
    if (s_writesThisTick >= MAX_VAR_LINES_PER_TICK) { s_dropped++; return; }
    s_writesThisTick++;
    head("var");
    fprintf(s_log, ",\"addr\":%d,\"old\":%u,\"new\":%u,\"actor\":%d,\"ip\":%d", address, (unsigned)old,
            (unsigned)(u16)value, (int)D_800AFD1C,
            g_FieldScriptVMCurActor ? (int)g_FieldScriptVMCurActor->scriptInstructionPointer : -1);
    tail();
}

void PcPort_RouteLogTrigger(int actor, int scriptId, int routine)
{
    int x, z, px, pz;
    static int lastActor = -1, lastId = -1, lastMap = -1;
    static unsigned lastTick;
    extern s32 g_PlayerActorIndex;
    if (!PcPort_RouteLogOn()) return;
    /* The touch check re-fires every frame while the player stands in range:
     * log the first one and then only a change of actor/kind, or a re-entry
     * after 120 quiet ticks. */
    if (actor == lastActor && scriptId == lastId && map_now() == lastMap && s_tick - lastTick < 120u) {
        lastTick = s_tick;
        return;
    }
    lastActor = actor; lastId = scriptId; lastMap = map_now(); lastTick = s_tick;
    actor_pos(actor, &x, &z);
    actor_pos(g_PlayerActorIndex, &px, &pz);
    head("trigger");
    fprintf(s_log, ",\"kind\":\"%s\",\"actor\":%d,\"scriptId\":%d,\"routine\":%d,\"actor_xz\":[%d,%d],\"player_xz\":[%d,%d]",
            scriptId == 2 ? "talk" : scriptId == 3 ? "touch" : "other", actor, scriptId, routine, x, z, px, pz);
    party_gold_items();
    var_diff();
    tail();
}

/* Per-frame poll: party, gold and inventory changes (the game writes these
 * from many places; the poll keeps the recorder out of all of them). */
static void tick(const XgPlatModEventData* e, void* u)
{
    int i, changed = 0;
    (void)e; (void)u;
    if (!s_on) return;
    s_tick++;
    if (s_dropped) {
        head("var_dropped");
        fprintf(s_log, ",\"dropped\":%u", s_dropped);
        tail();
        s_dropped = 0;
    }
    s_writesThisTick = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++)
        if (s_prevParty[i] != (u8)g_GamePartyMembers[i]) changed = 1;
    if (s_prevGold != (u32)g_GameState.gold) changed = 1;
    if (memcmp(s_prevItemQ, g_GameState.itemQuantities, sizeof s_prevItemQ) ||
        memcmp(s_prevItemId, g_GameState.itemIDs, sizeof s_prevItemId)) changed = 1;
    if (map_now() != s_prevMap) changed = 1;
    if (!changed) return;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) s_prevParty[i] = (u8)g_GamePartyMembers[i];
    s_prevGold = g_GameState.gold;
    memcpy(s_prevItemQ, g_GameState.itemQuantities, sizeof s_prevItemQ);
    memcpy(s_prevItemId, g_GameState.itemIDs, sizeof s_prevItemId);
    s_prevMap = map_now();
    head("state");
    party_gold_items();
    tail();
}

/* Harness only: a crash under XENO_ROUTE_LOG leaves a backtrace in the log. */
static void crash_handler(int sig)
{
    void* frames[48];
    int n = backtrace(frames, 48);
    static const char msg[] = "\n[ROUTE] CRASH: fatal signal, backtrace follows\n";
    (void)!write(2, msg, sizeof msg - 1);
    backtrace_symbols_fd(frames, n, 2);
    signal(sig, SIG_DFL);
    raise(sig);
}

void PcPort_RouteLogInit(void)
{
    if (!PcPort_RouteLogOn()) return;
    signal(SIGSEGV, crash_handler);
    signal(SIGBUS, crash_handler);
    signal(SIGABRT, crash_handler);
    xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, tick, NULL);
    fprintf(stderr, "[ROUTE] recording progression state to XENO_ROUTE_LOG\n");
}

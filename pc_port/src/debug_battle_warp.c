/* debug_battle_warp.c -- TEST TOOLING: launch an arbitrary battle from the field.
 *
 * Enabled only by XENO_BATTLE_WARP_FILE=<path>.  Every poll the file is read;
 * if it holds a decimal battle id the file is truncated (so one write fires
 * exactly one battle) and the field is handed to that battle.
 *
 * The handoff is a transcription of the field script VM's own "start battle"
 * opcode, src/field/main/misc11.c func_80093568: the same guard set, the same
 * five stores, in the same order.  Nothing else is written, so a warped battle
 * reaches battle.bin through the retail path rather than a synthetic one --
 * which is the point: it has to be the real fight to be worth looking at.
 *
 * Without this the only way to see a specific encounter is to play the story
 * to it, and scripted boss battles (Id, for one) sit many hours past anything
 * the headless harness can currently drive.
 *
 * Also driven by the cheat console (`battle <id>`, PcPort_DebugBattleWarpQueue);
 * PcPort_DebugBattleWarp runs as a frame_tick subscriber (cheat_console.c).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

/* Field state, exactly the symbols func_80093568 touches. */
extern s32 D_800ADBDC, D_800ADBE4, D_800ADBEC, D_800ADB2C, D_800ADB90;
extern s32 D_800ADBE0, D_800ADB88;
extern s32 D_8004F308;
extern u8 D_80059508, D_800594F8, D_8005954C, D_800B2356;

/* Poll at 30 frames: often enough to feel immediate from a driver script,
 * rare enough that the stat() traffic is invisible next to a frame. */
#define WARP_POLL_FRAMES 30

/* Set by warp_read_pending when the request carried an explicit BGM id. */
static int s_pending_bgm = -1;
/* Battle id queued by the cheat console's `battle` command. */
static int s_queued_id = -1;

void PcPort_DebugBattleWarpQueue(int id)
{
    if (id < 0 || id > 255) {
        printf("[xeno-port][cheat] battle id must be 0..255\n");
        return;
    }
    s_queued_id = id;
    s_pending_bgm = -1;
    printf("[xeno-port][cheat] battle %d queued (fires when the field is idle)\n", id);
}

static int warp_read_pending(const char* path)
{
    FILE* f = fopen(path, "r+");
    long value = -1;
    long bgm = -1;
    int ok;

    if (f == NULL) return -1;
    s_pending_bgm = -1;
    ok = (fscanf(f, "%ld", &value) == 1);
    /* "<id>:<bgm>" forces the battle music track.  Retail takes it from
     * D_800B2356, which the FIELD SCRIPT sets per scene (func_80087DE0; the
     * loader defaults it to 5).  A warp jumps straight into the fight without
     * that scene ever running, so the battle inherits a stale default and the
     * wrong track -- or none.  This makes the track selectable the same way
     * the id is. */
    if (ok && fscanf(f, ":%ld", &bgm) == 1 && bgm >= 0 && bgm <= 255) {
        s_pending_bgm = (int)bgm;
    }
    if (ok) {
        /* Consume the request before firing, so a battle that takes several
         * seconds to load cannot be requeued by the next poll. */
        if (ftruncate(fileno(f), 0) != 0) ok = 0;
    }
    fclose(f);
    if (!ok || value < 0 || value > 255) return -1;
    return (int)value;
}

void PcPort_DebugBattleWarp(void)
{
    static const char* path;
    static int inited;
    static int frame;
    int id;

    if (!inited) {
        inited = 1;
        path = getenv("XENO_BATTLE_WARP_FILE");
        if (path != NULL && path[0] == '\0') path = NULL;
    }
    if (path == NULL && s_queued_id < 0) return;
    if (++frame < WARP_POLL_FRAMES) return;
    frame = 0;

    /* func_80093568's guards: the field must be idle and battle-ready.  Firing
     * through them would drop the party into a battle the field cannot be
     * returned from. */
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0 ||
        D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        return;
    }

    if (s_queued_id >= 0) {
        id = s_queued_id;
        s_queued_id = -1;
    } else {
        id = path ? warp_read_pending(path) : -1;
    }
    if (id < 0) return;

    D_8005954C = (s_pending_bgm >= 0) ? (u8)s_pending_bgm : D_800B2356;
    fprintf(stderr,
            "[xeno-port][battle-warp] launching battle id=%d bgm=%u%s "
            "(field D_800B2356=%u)\n",
            id, (unsigned)D_8005954C,
            (s_pending_bgm >= 0) ? " forced" : "", (unsigned)D_800B2356);
    fflush(stderr);

    D_80059508 = (u8)id;
    D_800594F8 = 0;
    D_800ADBDC = 0;
    D_800ADBE0 = 0;
    D_800ADB88 = 1;
}

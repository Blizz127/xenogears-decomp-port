/* field_warp_diag.c -- TEST TOOLING: one-shot debug teleport.
 *
 * Enabled only by XENO_FIELD_WARP=<map>:<x>:<z>; disabled and inert otherwise.
 * When that map is loaded and the player actor exists, the player's X/Z are
 * set once and the warp disarms.
 *
 * Why this exists: field 14's only trigger zone (the map 14 -> 13 door at
 * roughly (335,-26)) has never been reached by a walk from the post-battle
 * spawn at (115,-455), and two different search strategies stalled well short
 * of it.  That leaves two very different explanations -- the player cannot
 * PATH there, or the zone/transition itself is broken -- and no amount of
 * further walking distinguishes them.  Placing the player in the zone does:
 * if the field changes, the exit machinery is sound and the problem is
 * navigation; if it does not, the problem is the zone or its script.
 *
 * This DOES write game state, unlike PcPort_FieldPosDiag in the neighbouring
 * file, which is why it is a separate translation unit with its own switch:
 * any run that sets XENO_FIELD_WARP is a diagnostic run and its output must
 * not be quoted as evidence of ordinary play.
 *
 * REMOVAL: delete this file, its build_port.sh entry, and the
 * PcPort_FieldWarpDiag() call in pc_port/src/psyq_compat.c's Vsync shim.
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "system/math.h"

#include "quick_checkpoint.h"

extern s32 g_PlayerActorIndex;
extern int g_GameSceneMapNum;
extern s32 D_800ADBFC;   /* live field actor count */

/* XENO_FIELD_WARP_PROBE="<map>:<dwell>:x1,z1;x2,z2;..." -- walk-test several
 * places in ONE run.
 *
 * The open question for field 14 is whether the post-battle SPAWN is wrong or
 * the room's walkable GEOMETRY is short. Those are distinguished by asking, at
 * a point outside the reachable strip, "can the player move here at all?": if
 * yes the surface exists and the strip is merely disconnected from it, if no
 * there is no surface there. One warp per run makes that cost ~6 minutes per
 * point, so this teleports to each listed point in turn, holding each for
 * `dwell` field-active frames while the driver keeps pressing directions.
 *
 * Writes game state, like its neighbour above; diagnostic runs only. */
#define WARP_PROBE_MAX 12

static void PcPort_FieldWarpProbe(ActorData* actor, int mapId)
{
    static int parsed = 0;
    static int armed = 0;
    static int wantMap = -1;
    static int dwell = 600;
    /* Skip the room's setup scene before the first point: a warp landing
     * during it is simply overwritten by the scene's own placement, which is
     * why probe point 1 kept reporting the spawn instead of its target. */
    static int startDelay = 900;
    static int count = 0;
    static int px[WARP_PROBE_MAX];
    static int pz[WARP_PROBE_MAX];
    static int ticks = 0;
    static int index = -1;

    if (!parsed) {
        const char* e = getenv("XENO_FIELD_WARP_PROBE");

        parsed = 1;
        if (e != NULL && *e != '\0') {
            const char* p = e;
            int m, d, n;

            if (sscanf(p, "%d:%d:%d:%n", &m, &d, &startDelay, &n) == 3 ||
                sscanf(p, "%d:%d:%n", &m, &d, &n) == 2) {
                wantMap = m;
                dwell = d > 0 ? d : 600;
                p += n;
                while (count < WARP_PROBE_MAX && *p) {
                    int x, z, used = 0;

                    if (sscanf(p, "%d,%d%n", &x, &z, &used) != 2)
                        break;
                    px[count] = x;
                    pz[count] = z;
                    count++;
                    p += used;
                    if (*p == ';')
                        p++;
                }
                armed = count > 0;
                printf("[xeno-port][test] WARPPROBE armed map=%d dwell=%d "
                       "start=%d points=%d\n", wantMap, dwell,
                       startDelay, count);
                fflush(stdout);
            }
        }
    }
    if (!armed || mapId != wantMap || actor == NULL)
        return;
    if (index >= count)
        return;
    if (index < 0 && ticks < startDelay) {
        ticks++;
        return;
    }
    if (index < 0 || ticks >= dwell) {
        ticks = 0;
        index++;
        if (index >= count) {
            printf("[xeno-port][test] WARPPROBE done\n");
            fflush(stdout);
            return;
        }
        printf("[xeno-port][test] WARPPROBE point %d/%d -> (%d,%d)\n",
               index + 1, count, px[index], pz[index]);
        fflush(stdout);
        actor->position.vx = CONV_FROM_GTE(px[index]);
        actor->position.vz = CONV_FROM_GTE(pz[index]);
    }
    ticks++;
}

/* One-shot warp state: armed by XENO_FIELD_WARP or the cheat console's
 * `warp` command (PcPort_FieldWarpArm). */
static int armed = 0;
static int wantMap = -1;
static int wantX = 0;
static int wantZ = 0;
static int s_warp_ticks = 0;
static int s_warp_fired = 0;

void PcPort_FieldWarpArm(int map, int x, int z)
{
    wantMap = map & 0xFFF;
    wantX = x;
    wantZ = z;
    s_warp_ticks = 0;
    s_warp_fired = 0;
    armed = 1;
    printf("[xeno-port][cheat] WARP armed map=%d -> (%d,%d)\n", wantMap, wantX, wantZ);
    fflush(stdout);
}

void PcPort_FieldWarpDiag(void)
{
    static int parsed = 0;
    /* A single shot at the first field-active frame lands DURING the room's
     * setup scene, which then repositions the player and erases it (measured:
     * fired at (-151,1774), player ended at (112,-458)).  So wait `delay`
     * field-active frames, then re-apply `repeats` times spaced `period`
     * apart, which outlasts the scene's own placement. */
    static int delay = 600;
    static int repeats = 5;
    static int period = 120;
    ActorData* actor;

    if (!parsed) {
        const char* e = getenv("XENO_FIELD_WARP");

        parsed = 1;
        if (e != NULL && *e != '\0' &&
            sscanf(e, "%d:%d:%d:%d:%d", &wantMap, &wantX, &wantZ, &delay,
                   &repeats) >= 3) {
            armed = 1;
            printf("[xeno-port][test] WARP armed map=%d -> (%d,%d) "
                   "delay=%d repeats=%d\n",
                   wantMap, wantX, wantZ, delay, repeats);
            fflush(stdout);
        }
    }
    /* Stay completely inert unless a diagnostic asked for something: this
     * runs on every field frame, and resolving the player actor is not free of
     * risk (see the index bound below). */
    if (!armed && getenv("XENO_FIELD_WARP_PROBE") == NULL)
        return;
    /* Resolve the player actor FIRST and share it: the probe is armed by its
     * own variable and must still run when the one-shot warp is unset. */
    if (!PcPort_QuickCheckpointFieldIsActive())
        return;
    if (g_FieldActors == NULL)
        return;
    /* Bound the index against the live actor count for the same reason
     * quick_checkpoint.c does: g_PlayerActorIndex survives field changes, so a
     * field with fewer actors would otherwise be read past the end of the
     * table and hand back a garbage pActorData. */
    if (g_PlayerActorIndex < 0 || D_800ADBFC <= 0 ||
        g_PlayerActorIndex >= D_800ADBFC)
        return;
    actor = (ActorData*)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    if (actor == NULL)
        return;

    PcPort_FieldWarpProbe(actor, g_GameSceneMapNum & 0xFFF);

    if (!armed)
        return;
    if ((g_GameSceneMapNum & 0xFFF) != wantMap)
        return;
    if (s_warp_ticks++ < delay)
        return;
    if (((s_warp_ticks - delay) % period) != 0)
        return;

    printf("[xeno-port][test] WARP firing %d/%d: (%d,%d) -> (%d,%d)\n",
           s_warp_fired + 1, repeats, (int)CONV_TO_GTE(actor->position.vx),
           (int)CONV_TO_GTE(actor->position.vz), wantX, wantZ);
    fflush(stdout);
    /* position is stored pre-CONV_TO_GTE; invert the diag's view. */
    actor->position.vx = CONV_FROM_GTE(wantX);
    actor->position.vz = CONV_FROM_GTE(wantZ);
    if (++s_warp_fired >= repeats)
        armed = 0;
}

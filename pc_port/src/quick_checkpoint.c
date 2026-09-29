#include "quick_checkpoint.h"
#include "quick_checkpoint_file.h"
#include "quick_checkpoint_request.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "field/actor.h"

extern s16 D_800AFB54;
extern s16 func_8007B1C4(s16, s16, s32, s16*, s32*);
extern s32 func_80080968(u8*);

extern unsigned char g_GameState[];
extern s32 g_GameSceneMapNum;
extern s32 g_PlayerActorIndex;
extern FieldActor* volatile g_FieldActors;
extern s32 D_800ADB64;
extern s32 D_800ADB68;
extern u8 D_800B21D0;
extern s32 D_800ADBFC;   /* live field actor count */

static int s_fieldActive;
static PcPortQuickRequestState s_request;
static int s_loadReady;
static int s_restoreReady;
static PcPortQuickCheckpoint s_loaded;

static const char* checkpoint_path(void)
{
    const char* path = getenv("XENO_QUICKSAVE_PATH");
    return (path && path[0]) ? path : "quicksaves/quick.xgqs";
}

static ActorData* player_actor(FieldActor** field_actor)
{
    FieldActor* slot;
    if (!g_FieldActors || g_PlayerActorIndex < 0)
        return NULL;
    /* Bound the index against the LIVE actor count, not just the array
     * pointer. g_PlayerActorIndex persists across field changes, so on a field
     * with fewer actors -- the title map 490 is the case that bites -- the old
     * index reads past the end of the table and hands back a garbage
     * pActorData that checkpoint_is_safe() then dereferences: F8 at the title
     * queued a load and SIGSEGV'd. A stale index means "no player actor here",
     * which is exactly what NULL says. */
    if (D_800ADBFC <= 0 || g_PlayerActorIndex >= D_800ADBFC)
        return NULL;
    slot = &g_FieldActors[g_PlayerActorIndex];
    if (!slot->pActorData)
        return NULL;
    if (field_actor)
        *field_actor = slot;
    return (ActorData*)(uintptr_t)slot->pActorData;
}

/* Match the field's own menu-open safety gate: loaded, no menu/dialog, and no
 * player script control lock. D_80059179 is absent from that retail predicate;
 * func_800798BC can set it to 1 in an ordinary field, so it must not block a
 * stable foot checkpoint. */
static int checkpoint_is_safe(void)
{
    ActorData* actor = player_actor(NULL);
    if (!s_fieldActive || !actor || D_800ADB68 != 1 ||
        D_800ADB64 != 0xFF || D_800B21D0 != 0)
        return 0;
    return ((*(u32*)actor & 0x1800u) == 0);
}

void PcPort_QuickCheckpointRequestSave(void)
{
    if (!PcPort_QuickRequestQueue(&s_request, PC_PORT_QUICK_REQUEST_SAVE,
                                  s_fieldActive &&
                                  (g_GameSceneMapNum & 0x3FFF) != 490)) {
        fprintf(stderr, "[xeno-port][quick] F7 ignored: not in a gameplay field\n");
        return;
    }
    fprintf(stderr, "[xeno-port][quick] save queued\n");
}

void PcPort_QuickCheckpointRequestLoad(void)
{
    if (!PcPort_QuickRequestQueue(&s_request, PC_PORT_QUICK_REQUEST_LOAD,
                                  s_fieldActive)) {
        fprintf(stderr, "[xeno-port][quick] F8 ignored: not in a field\n");
        return;
    }
    fprintf(stderr, "[xeno-port][quick] load queued\n");
}

int PcPort_QuickCheckpointGetUiState(void)
{
    return s_request.ui_state;
}

/* TEST TOOLING (XENO_FIELD_POS_DIAG): the field-active latch, exposed so the
 * walk telemetry in field_pos_diag.c can skip frames where the field actors
 * are torn down.  During a battle the retail MIPS adapter still drives Vsync,
 * but g_FieldActors[player].pActorData is stale -- reading it segfaults. */
int PcPort_QuickCheckpointFieldIsActive(void)
{
    return s_fieldActive;
}

void PcPort_QuickCheckpointSetFieldActive(int active)
{
    s_fieldActive = active != 0;
    if (!s_fieldActive && s_request.action != PC_PORT_QUICK_REQUEST_NONE)
        PcPort_QuickRequestCancel(&s_request);
}

static int capture_checkpoint(PcPortQuickCheckpoint* checkpoint)
{
    ActorData* actor = player_actor(NULL);
    if (!actor)
        return -1;
    memset(checkpoint, 0, sizeof(*checkpoint));
    memcpy(checkpoint->game_state, g_GameState,
           PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
    checkpoint->map = (u16)(g_GameSceneMapNum & 0x3FFF);
    checkpoint->entrance = *(u16*)(g_GameState + 0x1932);
    checkpoint->position[0] = actor->position.vx;
    checkpoint->position[1] = actor->position.vy;
    checkpoint->position[2] = actor->position.vz;
    checkpoint->rotation[0] = actor->rotation.vx;
    checkpoint->rotation[1] = actor->rotation.vy;
    checkpoint->rotation[2] = actor->rotation.vz;
    return 0;
}

int PcPort_QuickCheckpointPoll(void)
{
    PcPortQuickCheckpoint checkpoint;
    const char* path;
    PcPortQuickRequestAction request;
    /* A title-menu poll can prepare the load; keep the result available for
     * FieldMain's next poll after the menu's own cleanup has finished. */
    if (s_loadReady)
        return 1;
    if (s_request.action == PC_PORT_QUICK_REQUEST_NONE)
        return 0;
    /* Map 490 is the retail title field. It owns script control and has no
     * controllable player, but Poll runs only after field initialization and
     * between updates, where the existing teardown/load path is safe. Allow
     * only loading here: capturing title state would overwrite earned progress.
     * Short-circuit before consulting any stale player actor on the title. */
    if (!(s_fieldActive && s_request.action == PC_PORT_QUICK_REQUEST_LOAD &&
          (g_GameSceneMapNum & 0x3FFF) == 490) && !checkpoint_is_safe()) {
        if (!s_request.wait_reported) {
            fprintf(stderr,
                    "[xeno-port][quick] queued: waiting for free field control\n");
            s_request.wait_reported = 1;
        }
        return 0;
    }
    request = PcPort_QuickRequestTakeIfSafe(&s_request, 1);

    path = checkpoint_path();
    if (request == PC_PORT_QUICK_REQUEST_SAVE) {
        const char* override = getenv("XENO_QUICKSAVE_PATH");
        if ((!override || !override[0]) &&
            mkdir("quicksaves", 0755) != 0 && errno != EEXIST) {
            fprintf(stderr,
                    "[xeno-port][quick] save failed: cannot create quicksaves (%s)\n",
                    strerror(errno));
            PcPort_QuickRequestSetResult(&s_request,
                                         PC_PORT_QUICK_UI_SAVE_ERROR);
            return 0;
        }
        if (capture_checkpoint(&checkpoint) != 0 ||
            PcPort_QuickCheckpointWriteFile(path, &checkpoint) != 0) {
            fprintf(stderr, "[xeno-port][quick] save failed: %s\n", path);
            PcPort_QuickRequestSetResult(&s_request,
                                         PC_PORT_QUICK_UI_SAVE_ERROR);
            return 0;
        }
        PcPort_QuickRequestSetResult(&s_request, PC_PORT_QUICK_UI_SAVE_OK);
        fprintf(stderr,
                "[xeno-port][quick] saved %s map=%u pos=(%d,%d,%d)\n",
                path, (unsigned)checkpoint.map,
                checkpoint.position[0] >> 16,
                checkpoint.position[1] >> 16,
                checkpoint.position[2] >> 16);
        return 0;
    }

    if (PcPort_QuickCheckpointReadFile(path, &checkpoint) != 0) {
        fprintf(stderr,
                "[xeno-port][quick] load rejected: missing or invalid %s\n",
                path);
        PcPort_QuickRequestSetResult(&s_request,
                                     PC_PORT_QUICK_UI_LOAD_ERROR);
        return 0;
    }
    if ((checkpoint.map & 0x3FFFu) >= 0x400u) {
        fprintf(stderr,
                "[xeno-port][quick] load rejected: checkpoint is not a field\n");
        PcPort_QuickRequestSetResult(&s_request,
                                     PC_PORT_QUICK_UI_LOAD_ERROR);
        return 0;
    }
    s_loaded = checkpoint;
    s_loadReady = 1;
    PcPort_QuickRequestSetResult(&s_request, PC_PORT_QUICK_UI_LOAD_OK);
    fprintf(stderr,
            "[xeno-port][quick] loading %s map=%u pos=(%d,%d,%d)\n",
            path, (unsigned)checkpoint.map,
            checkpoint.position[0] >> 16,
            checkpoint.position[1] >> 16,
            checkpoint.position[2] >> 16);
    return 1;
}

void PcPort_QuickCheckpointCommitLoad(void)
{
    if (!s_loadReady)
        return;
    memcpy(g_GameState, s_loaded.game_state,
           PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
    *(u16*)(g_GameState + 0x231A) = s_loaded.map;
    *(u16*)(g_GameState + 0x2320) = s_loaded.entrance;
    s_loadReady = 0;
    s_restoreReady = 1;
}

void PcPort_QuickCheckpointRestorePlayer(void)
{
    FieldActor* field_actor = NULL;
    ActorData* actor;
    SpriteData* sprite;
    if (!s_restoreReady)
        return;
    actor = player_actor(&field_actor);
    if (!actor || !field_actor)
        return;

    actor->position.vx = s_loaded.position[0];
    actor->position.vy = s_loaded.position[1];
    actor->position.vz = s_loaded.position[2];
    actor->move.vx = actor->move.vy = actor->move.vz = 0;
    actor->moveModified.vx = actor->moveModified.vy =
        actor->moveModified.vz = 0;
    actor->rotation.vx = s_loaded.rotation[0];
    actor->rotation.vy = s_loaded.rotation[1];
    actor->rotation.vz = s_loaded.rotation[2];
    actor->curYPos = (u16)(actor->position.vy >> 16);

    field_actor->rotation.x = s_loaded.rotation[0];
    field_actor->rotation.y = s_loaded.rotation[1];
    field_actor->rotation.z = s_loaded.rotation[2];
    field_actor->transformMatrix.t[0] = actor->position.vx >> 16;
    field_actor->transformMatrix.t[1] = actor->position.vy >> 16;
    field_actor->transformMatrix.t[2] = actor->position.vz >> 16;
    sprite = (SpriteData*)(uintptr_t)field_actor->pSpriteData;
    if (sprite) {
        sprite->position.x = actor->position.vx;
        sprite->position.y = actor->position.vy;
        sprite->position.z = actor->position.vz;
    }
    /* As in retail's func_8009E574, relocating an actor must locate its
     * triangles on every loaded walkmesh and refresh the active surface.
     * Keep the checkpoint's precise position/height rather than snapping it
     * to the integer surface point returned by the locator. */
    {
        s32 state[4][4] = {{0}};
        s16 out[4][4] = {{0}};
        s32 i;
        for (i = 0; i < D_800AFB54 - 1 && i < 4; ++i)
            actor->walkmeshTriIds[i] = func_8007B1C4(
                actor->position.vx >> 16, actor->position.vz >> 16,
                i, out[i], state[i]);
        actor->curWalkmeshTriMaterial = func_80080968((u8*)actor);
        if (actor->walkmeshId >= 0 && actor->walkmeshId < i) {
            actor->curTriNormal.vx = state[actor->walkmeshId][0];
            actor->curTriNormal.vy = state[actor->walkmeshId][1];
            actor->curTriNormal.vz = state[actor->walkmeshId][2];
        }
    }
    s_restoreReady = 0;
    fprintf(stderr, "[xeno-port][quick] player position restored\n");
}

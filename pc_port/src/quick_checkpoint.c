#include "quick_checkpoint.h"
#include "quick_checkpoint_file.h"
#include "quick_checkpoint_request.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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
extern s32 D_800ADB2C;   /* field stream consumer active (func_800854D0) */
extern void* g_FieldScriptMemory;
extern void func_800A30FC(void);
/* Dev-menu status line (port_dev_menu.c); weak so standalone tests link. */
extern void PcPort_DevMenuQuickSaveResult(const char* status) __attribute__((weak));

/* func_800A30FC writes only the saved half of g_FieldScriptMemory (0x200
 * u16 words) besides g_GameState. */
#define PC_PORT_QUICK_SCRIPT_MEMORY_BYTES 0x400u
#define PC_PORT_QUICK_SCENE_NOTICE "CAN'T QUICKSAVE DURING A SCENE"

static int s_fieldActive;
/* The event lock (see scene_lock_held) as it was at the previous FieldMain
 * poll: a scene that released control during the frame that just ran is
 * still closing (the frame's start-of-frame state sync predates it). */
static int s_sceneAtPrevPoll;
static PcPortQuickRequestState s_request;
static char s_notice[64];
static struct timespec s_noticeAt;
static int s_loadReady;
static int s_restoreReady;
static PcPortQuickCheckpoint s_loaded;
static int s_warpReady;
static int s_warpMap, s_warpEntrance;
static int s_spawnNaturally;

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

/* A field script event holds control: the event lock D_800B21D0 that the
 * script lock opcode sets (func_80093B10: D_800B21D0 = 1, encounters off)
 * and the unlock opcode clears (func_80093AC8), or the player actor's own
 * script control lock (flags & 0x1800).  This is the game's state, not a
 * per-map rule. */
static int scene_lock_held(void)
{
    ActorData* actor = player_actor(NULL);
    return D_800B21D0 != 0 || (actor && (*(u32*)actor & 0x1800u) != 0);
}

void PcPort_QuickCheckpointNoticeSet(const char* text)
{
    snprintf(s_notice, sizeof s_notice, "%s", text);
    clock_gettime(CLOCK_MONOTONIC, &s_noticeAt);
    fprintf(stderr, "[xeno-port][quick] notice: %s\n", text);
}

/* The on-screen quick save/load notice (drawn by the host overlay in
 * psycross_port_dev_menu.inl): the last message, for three seconds. */
const char* PcPort_QuickCheckpointNotice(void)
{
    struct timespec now;
    long ms;
    if (!s_notice[0])
        return NULL;
    clock_gettime(CLOCK_MONOTONIC, &now);
    ms = (long)(now.tv_sec - s_noticeAt.tv_sec) * 1000 +
         (now.tv_nsec - s_noticeAt.tv_nsec) / 1000000;
    return ms < 3000 ? s_notice : NULL;
}

void PcPort_QuickCheckpointRequestSave(void)
{
    if (!PcPort_QuickRequestQueue(&s_request, PC_PORT_QUICK_REQUEST_SAVE,
                                  s_fieldActive &&
                                  (g_GameSceneMapNum & 0x3FFF) != 490)) {
        fprintf(stderr, "[xeno-port][quick] quick save (F11) ignored: not in a gameplay field\n");
        return;
    }
    fprintf(stderr, "[xeno-port][quick] save queued\n");
}

void PcPort_QuickCheckpointSafetyDescribe(char* out, size_t n)
{
    ActorData* actor = player_actor(NULL);
    snprintf(out, n, "map=%d field=%d actor=%d ADB68=%d ADB64=0x%X B21D0=%d lock=0x%X",
             (int)(g_GameSceneMapNum & 0x3FFF), s_fieldActive, actor != NULL,
             (int)D_800ADB68, (unsigned)D_800ADB64, (int)D_800B21D0,
             actor ? (unsigned)(*(u32*)actor & 0x1800u) : 0u);
}

int PcPort_QuickCheckpointFieldIsSafe(void)
{
    return (g_GameSceneMapNum & 0x3FFF) != 490 && checkpoint_is_safe();
}

int PcPort_QuickCheckpointRequestWarp(int map, int entrance)
{
    if (!PcPort_QuickCheckpointFieldIsSafe() || s_request.action != PC_PORT_QUICK_REQUEST_NONE || s_loadReady || s_warpReady || map < 0 || map >= 0x400)
        return 0;
    s_warpMap = map; s_warpEntrance = entrance; s_warpReady = 1;
    return 1;
}

void PcPort_QuickCheckpointRequestLoad(void)
{
    if (!PcPort_QuickRequestQueue(&s_request, PC_PORT_QUICK_REQUEST_LOAD,
                                  s_fieldActive)) {
        fprintf(stderr, "[xeno-port][quick] quick load (F12) ignored: not in a field\n");
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

/* TEST TOOLING (headless drivers): XENO_QUICKLOAD_ON_TITLE=1 issues the
 * ordinary quick load -- the same request F12 queues -- once, when the title
 * field (map 490) becomes active, for runs whose video driver has no keyboard
 * (SDL offscreen).  It loads XENO_QUICKSAVE_PATH exactly as F12 at the title
 * does; no warp, console or dev menu.  Unset => nothing. */
static void PcPort_QuickCheckpointHeadlessTitleLoad(void)
{
    static int done;
    static unsigned attempts;
    extern int g_GameSceneMapNum;
    const char *flag;

    if (done || !s_fieldActive || (g_GameSceneMapNum & 0xFFF) != 490)
        return;
    /* One attempt per ~second of field polls, as a driver re-pressing F12. */
    if (attempts++ % 60u != 0u)
        return;
    flag = getenv("XENO_QUICKLOAD_ON_TITLE");
    if (flag == NULL || flag[0] == '\0' || strcmp(flag, "0") == 0)
        return;
    fprintf(stderr, "[xeno-port][quick] XENO_QUICKLOAD_ON_TITLE: requesting the F12 quick load at the title\n");
    PcPort_QuickCheckpointRequestLoad();
    /* Like a held-down F12 retried until it takes: done once queued. */
    done = s_request.action == PC_PORT_QUICK_REQUEST_LOAD;
}

void PcPort_QuickCheckpointSetFieldActive(int active)
{
    s_fieldActive = active != 0;
    s_sceneAtPrevPoll = 0;
    if (!s_fieldActive && s_request.action != PC_PORT_QUICK_REQUEST_NONE)
        PcPort_QuickRequestCancel(&s_request);
    PcPort_QuickCheckpointHeadlessTitleLoad();
}

/* The game-state image a retail save would hold now.  FieldMain syncs the
 * live script variables (g_FieldScriptMemory) into g_GameState+0x1930 once
 * per frame, at the START of the frame (func_80077DAC -> func_800A31E8 ->
 * func_800A30FC), and polls the quick save before the next frame's sync.  A
 * raw copy therefore misses everything the scripts did in the frame that just
 * ran.  On field 14 that frame is the painting scene's last one: the player
 * script sets scenario 6 (opcode 0x87 at ip 401), unlocks (FE53, ip 404) and
 * hands over control (0x0C, ip 406), so the copy said scenario 3 and loading
 * it replayed "Whew! That about does it...".  Build the image with retail's
 * own sync routine, then put both buffers back so saving changes nothing. */
static void checkpoint_state_image(uint8_t* out)
{
    static uint8_t liveState[PC_PORT_QUICK_CHECKPOINT_STATE_BYTES];
    static uint8_t liveScript[PC_PORT_QUICK_SCRIPT_MEMORY_BYTES];

    /* g_FieldScriptMemory is declared `void*` but is the script buffer
     * itself; hide its declared size from _FORTIFY_SOURCE. */
    uint8_t* script = (uint8_t*)&g_FieldScriptMemory;
    __asm__("" : "+r"(script));

    memcpy(liveState, g_GameState, sizeof liveState);
    memcpy(liveScript, script, sizeof liveScript);
    func_800A30FC();
    memcpy(out, g_GameState, sizeof liveState);
    memcpy(g_GameState, liveState, sizeof liveState);
    memcpy(script, liveScript, sizeof liveScript);
}

static int capture_checkpoint(PcPortQuickCheckpoint* checkpoint)
{
    ActorData* actor = player_actor(NULL);
    if (!actor)
        return -1;
    memset(checkpoint, 0, sizeof(*checkpoint));
    checkpoint_state_image(checkpoint->game_state);
    checkpoint->map = (u16)(g_GameSceneMapNum & 0x3FFF);
    checkpoint->entrance = *(u16*)(g_GameState + 0x1932);
    checkpoint->position[0] = actor->position.vx;
    checkpoint->position[1] = actor->position.vy;
    checkpoint->position[2] = actor->position.vz;
    checkpoint->rotation[0] = actor->rotation.vx;
    checkpoint->rotation[1] = actor->rotation.vy;
    checkpoint->rotation[2] = actor->rotation.vz;
    {
        extern uint32_t g_RandomSeed;
        checkpoint->has_random_seed = 1;
        checkpoint->random_seed = g_RandomSeed;
    }
    return 0;
}

/* Field main-loop iterations (Poll runs once per FieldMain loop pass), for
 * the heartbeat and the cheat-state tests: it stops advancing exactly when
 * the field loop stalls, unlike the presented-frame count. */
static volatile unsigned s_fieldLoopCount;
unsigned PcPort_FieldLoopCount(void) { return s_fieldLoopCount; }

int PcPort_QuickCheckpointPlayerPosition(int out[3])
{
    ActorData* actor = s_fieldActive ? player_actor(NULL) : NULL;
    if (!actor) return 0;
    out[0] = actor->position.vx >> 16;
    out[1] = actor->position.vy >> 16;
    out[2] = actor->position.vz >> 16;
    return 1;
}

int PcPort_QuickCheckpointPoll(void)
{
    PcPortQuickCheckpoint checkpoint;
    const char* path;
    PcPortQuickRequestAction request;
    int sceneNow, sceneClosing;
    /* Scene state for quick saves: an event holds control now, or held it at
     * the previous poll (released during the frame that just ran). */
    sceneNow = s_fieldActive && scene_lock_held();
    sceneClosing = s_fieldActive && s_sceneAtPrevPoll;
    if (s_fieldActive)
        s_sceneAtPrevPoll = sceneNow;
    s_fieldLoopCount++;
    PcPort_QuickCheckpointHeadlessTitleLoad();
    /* Loads/warps must leave the field consumer alive until it releases the
     * active stream. Otherwise the ring fills after teardown and the next
     * overlay's ArchiveCdDataSync(0) waits forever for unread sectors. Keep
     * the request queued and run another ordinary field frame; do not discard
     * sectors or bypass the teardown. Retail CHANGE_FIELD uses this same
     * D_800ADB2C completion flag (func_800932D0). */
    if (s_fieldActive && D_800ADB2C != 0 &&
        (s_warpReady || s_loadReady || s_request.action == PC_PORT_QUICK_REQUEST_LOAD))
        return 0;
    /* A title-menu poll can prepare the load; keep the result available for
     * FieldMain's next poll after the menu's own cleanup has finished. */
    if (s_warpReady && PcPort_QuickCheckpointFieldIsSafe()) {
        memset(&s_loaded, 0, sizeof s_loaded);
        memcpy(s_loaded.game_state, g_GameState, PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
        s_loaded.map = (u16)s_warpMap;
        s_loaded.entrance = (u16)s_warpEntrance;
        s_warpReady = 0; s_spawnNaturally = 1; s_loadReady = 1;
        fprintf(stderr, "[dev-menu] loading field %d entrance %d through field teardown\n", s_warpMap, s_warpEntrance);
    }
    if (s_loadReady)
        return 1;
    if (s_request.action == PC_PORT_QUICK_REQUEST_NONE)
        return 0;
    /* Quick saves only at settled field states: a save asked for while a
     * field script event holds control, or in the frame it lets go, is
     * refused (not deferred) with an on-screen notice.  Loads, warps and
     * transient blocks (menu request, control disabled) are unchanged. */
    if (s_request.action == PC_PORT_QUICK_REQUEST_SAVE &&
        (g_GameSceneMapNum & 0x3FFF) != 490 && (sceneNow || sceneClosing)) {
        char why[160];
        PcPort_QuickCheckpointSafetyDescribe(why, sizeof why);
        fprintf(stderr, "[xeno-port][quick] save refused: %s (%s)\n",
                sceneNow ? "field script event holds control"
                         : "field script event released control this frame",
                why);
        PcPort_QuickRequestCancel(&s_request);
        PcPort_QuickCheckpointNoticeSet(PC_PORT_QUICK_SCENE_NOTICE);
        if (PcPort_DevMenuQuickSaveResult)
            PcPort_DevMenuQuickSaveResult(PC_PORT_QUICK_SCENE_NOTICE);
        return 0;
    }
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
            PcPort_QuickCheckpointNoticeSet("QUICK SAVE FAILED");
            return 0;
        }
        if (capture_checkpoint(&checkpoint) != 0 ||
            PcPort_QuickCheckpointWriteFile(path, &checkpoint) != 0) {
            fprintf(stderr, "[xeno-port][quick] save failed: %s\n", path);
            PcPort_QuickRequestSetResult(&s_request,
                                         PC_PORT_QUICK_UI_SAVE_ERROR);
            PcPort_QuickCheckpointNoticeSet("QUICK SAVE FAILED");
            return 0;
        }
        PcPort_QuickRequestSetResult(&s_request, PC_PORT_QUICK_UI_SAVE_OK);
        PcPort_QuickCheckpointNoticeSet("QUICK SAVED");
        if (PcPort_DevMenuQuickSaveResult)
            PcPort_DevMenuQuickSaveResult("QUICK SAVED");
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
    if (s_loaded.has_random_seed) {
        extern uint32_t g_RandomSeed;
        g_RandomSeed = s_loaded.random_seed;
    }
    s_loadReady = 0;
    s_restoreReady = !s_spawnNaturally;
    s_spawnNaturally = 0;
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

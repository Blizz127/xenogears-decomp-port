#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "common.h"
#include "field/actor.h"
#include "quick_checkpoint.h"
#include "quick_checkpoint_file.h"
#include "quick_checkpoint_request.h"

unsigned char g_GameState[PC_PORT_QUICK_CHECKPOINT_STATE_BYTES];
s32 g_GameSceneMapNum;
s32 g_PlayerActorIndex;
FieldActor g_TestFieldActors[1];
FieldActor* volatile g_FieldActors = g_TestFieldActors;
s32 D_800ADB64;
s32 D_800ADB68;
s32 D_800ADBFC = 1;
u8 D_800B21D0;

static ActorData* g_actor;
static void* g_actor_mapping;
static const char* g_path;

static void fail(const char* message)
{
    fprintf(stderr, "QUICK GATE FAIL: %s\n", message);
    exit(1);
}

static void require(int condition, const char* message)
{
    if (!condition)
        fail(message);
}

static void reset_case(void)
{
    PcPort_QuickCheckpointSetFieldActive(0);
    PcPort_QuickCheckpointSetFieldActive(1);
    unlink(g_path);
    memset(g_GameState, 0x5A, sizeof(g_GameState));
    memset(g_actor, 0, sizeof(*g_actor));
    g_GameSceneMapNum = 13;
    g_PlayerActorIndex = 0;
    g_FieldActors = g_TestFieldActors;
    D_800ADBFC = 1;
    D_800ADB64 = 0xFF;
    D_800ADB68 = 1;
    D_800B21D0 = 0;
    g_actor->position.vx = 181 << 16;
    g_actor->position.vy = 0;
    g_actor->position.vz = 4 << 16;
    g_actor->rotation.vy = 0x400;
    *(u16*)(g_GameState + 0x1932) = 7;
}

static void expect_pending(const char* label)
{
    char message[128];
    snprintf(message, sizeof(message), "%s poll unexpectedly completed", label);
    require(PcPort_QuickCheckpointPoll() == 0, message);
    snprintf(message, sizeof(message), "%s did not remain pending", label);
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_SAVE_PENDING,
            message);
    snprintf(message, sizeof(message), "%s wrote a blocked checkpoint", label);
    require(access(g_path, F_OK) != 0, message);
}

static void expect_saved(const char* label)
{
    PcPortQuickCheckpoint checkpoint;
    char message[128];
    snprintf(message, sizeof(message), "%s poll did not complete", label);
    require(PcPort_QuickCheckpointPoll() == 0, message);
    snprintf(message, sizeof(message), "%s did not report success", label);
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_SAVE_OK,
            message);
    snprintf(message, sizeof(message), "%s did not write checkpoint", label);
    require(access(g_path, F_OK) == 0, message);
    snprintf(message, sizeof(message), "%s checkpoint unreadable", label);
    require(PcPort_QuickCheckpointReadFile(g_path, &checkpoint) == 0, message);
    snprintf(message, sizeof(message), "%s map mismatch", label);
    require(checkpoint.map == 13 && checkpoint.entrance == 7, message);
    snprintf(message, sizeof(message), "%s position mismatch", label);
    require(checkpoint.position[0] == (181 << 16) &&
            checkpoint.position[2] == (4 << 16), message);
}

int main(int argc, char** argv)
{
    if (argc != 2)
        fail("usage: quick_checkpoint_gate_test PATH");
    g_path = argv[1];
    setenv("XENO_QUICKSAVE_PATH", g_path, 1);

    g_actor_mapping = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    require(g_actor_mapping != MAP_FAILED, "MAP_32BIT actor allocation");
    require((uintptr_t)g_actor_mapping <= UINT32_MAX,
            "actor allocation outside PSX pointer width");
    g_actor = (ActorData*)g_actor_mapping;
    memset(g_TestFieldActors, 0, sizeof(g_TestFieldActors));
    g_TestFieldActors[0].pActorData = (u32)(uintptr_t)g_actor;

    /* Retail allows a stable foot field even when D_80059179 is 1. */
    reset_case();
    PcPort_QuickCheckpointRequestSave();
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_SAVE_PENDING,
            "stable request not queued");
    expect_saved("stable foot field");
    puts("QUICK GATE stable foot field: PASS");

    reset_case();
    *(u32*)g_actor = 0x1800;
    PcPort_QuickCheckpointRequestSave();
    expect_pending("actor control lock");

    reset_case();
    D_800ADB64 = 2;
    PcPort_QuickCheckpointRequestSave();
    expect_pending("menu request");

    reset_case();
    D_800B21D0 = 1;
    PcPort_QuickCheckpointRequestSave();
    expect_pending("transition lock");

    reset_case();
    D_800ADB68 = 0;
    PcPort_QuickCheckpointRequestSave();
    expect_pending("player control disabled");

    reset_case();
    *(u32*)g_actor = 0x1800;
    PcPort_QuickCheckpointRequestSave();
    expect_pending("locked then release before clear");
    *(u32*)g_actor = 0;
    expect_saved("naturally cleared control lock");
    puts("QUICK GATE blocked controls and natural release: PASS");

    reset_case();
    PcPort_QuickCheckpointSetFieldActive(0);
    PcPort_QuickCheckpointRequestSave();
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_SAVE_ERROR,
            "inactive request did not fail");
    require(access(g_path, F_OK) != 0, "inactive request wrote checkpoint");
    puts("QUICK GATE inactive field: PASS");

    /* A real field checkpoint can be loaded from the fully initialized title
     * loop, whose script owns control and has no usable player actor. */
    reset_case();
    PcPort_QuickCheckpointRequestSave();
    expect_saved("title fixture");
    g_GameSceneMapNum = 490;
    D_800ADB68 = 0;
    D_800B21D0 = 1;
    D_800ADBFC = 0;
    g_FieldActors = NULL;
    PcPort_QuickCheckpointRequestLoad();
    require(PcPort_QuickCheckpointPoll() == 1, "title load did not complete");
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_LOAD_OK,
            "title load did not report success");
    require(PcPort_QuickCheckpointPoll() == 1, "title handoff lost pending load");
    memset(g_GameState, 0, sizeof(g_GameState));
    PcPort_QuickCheckpointCommitLoad();
    require(*(u16*)(g_GameState + 0x231A) == 13 &&
            *(u16*)(g_GameState + 0x2320) == 7 && g_GameState[0] == 0x5A,
            "title load did not restore saved state/map/entrance");
    PcPort_QuickCheckpointRequestSave();
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_SAVE_ERROR,
            "title save must be rejected");
    { PcPortQuickCheckpoint preserved;
      require(PcPort_QuickCheckpointReadFile(g_path, &preserved) == 0 &&
              preserved.map == 13 && preserved.game_state[0] == 0x5A,
              "title save changed existing checkpoint"); }

    /* The title exemption cannot bypass locks on a gameplay map. */
    g_GameSceneMapNum = 13;
    PcPort_QuickCheckpointRequestLoad();
    require(PcPort_QuickCheckpointPoll() == 0 &&
            PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_LOAD_PENDING,
            "locked gameplay accepted title exemption");
    PcPort_QuickCheckpointSetFieldActive(0);
    g_GameSceneMapNum = 490;
    PcPort_QuickCheckpointRequestLoad();
    require(PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_LOAD_ERROR,
            "inactive title accepted load");
    PcPort_QuickCheckpointSetFieldActive(1);
    unlink(g_path);
    PcPort_QuickCheckpointRequestLoad();
    require(PcPort_QuickCheckpointPoll() == 0 &&
            PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_LOAD_ERROR,
            "missing title checkpoint was not rejected");
    { FILE* f = fopen(g_path, "wb"); require(f != NULL, "corrupt fixture");
      fputs("not a checkpoint", f); fclose(f); }
    PcPort_QuickCheckpointRequestLoad();
    require(PcPort_QuickCheckpointPoll() == 0 &&
            PcPort_QuickCheckpointGetUiState() == PC_PORT_QUICK_UI_LOAD_ERROR,
            "corrupt title checkpoint was not rejected");
    unlink(g_path);
    puts("QUICK GATE title load/overwrite protection/invalid files: PASS");

    require(munmap(g_actor_mapping, 4096) == 0, "actor unmap");
    puts("QUICK CHECKPOINT GATE PASS stable-file/blocked-controls/release");
    return 0;
}

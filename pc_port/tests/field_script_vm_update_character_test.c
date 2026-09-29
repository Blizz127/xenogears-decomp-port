/*
 * Retail behavior certificate for func_8009F5F4 (opcode A7).
 *
 * In particular, this covers the interaction block at 0x8009F720-0x8009F90C.
 * That block is responsible for ordinary talk requests, the stuck/held-button
 * retry, and the alternate interaction cooldown configured by opcode B5.
 */
#include "common.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/main.h"
#include "field/text_box.h"

#include <stdio.h>
#include <string.h>

void func_8009F5F4(void);

ActorData g_TestActor;
ActorData* g_FieldScriptVMCurActor;
FieldTextBox g_FieldTextBoxes[4];
FieldControl g_FieldControl;
CameraInterpolation g_CamInterpolation;

u16 D_800AFE9C;
s32 D_800ADB68;
s16 D_800ADB02;
u8 D_800B2354;
u16 D_800ADF68[16];
u16 D_800ADF88[16];
u16 D_800C2694;
s32 D_800ADB64;
s16 D_800B2340;
s16 D_800B2342;
s16 D_800B2344;
s32 D_800ADB28;
s32 D_800B2360;
u8 D_800B21CE;

static s32 g_InteractionResult;
static int g_InteractionCalls;
static int g_EncounterCalls;
static unsigned int g_Checks;

void PcPort_TestInputInject(u16* held) {
    (void)held;
}

void func_80079288(void) {
    g_EncounterCalls++;
}

s32 func_80081F5C(u32* actor) {
    if (actor != (u32*)&g_TestActor) {
        fprintf(stderr, "FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL interaction actor\n");
        return -1;
    }
    g_InteractionCalls++;
    return g_InteractionResult;
}

static int require(int condition, const char* message) {
    g_Checks++;
    if (!condition) {
        fprintf(stderr, "FIELD_SCRIPT_VM_UPDATE_CHARACTER FAIL %s\n", message);
        return 0;
    }
    return 1;
}

static void reset_fixture(void) {
    memset(&g_TestActor, 0, sizeof(g_TestActor));
    memset(g_FieldTextBoxes, 0, sizeof(g_FieldTextBoxes));
    memset(&g_FieldControl, 0, sizeof(g_FieldControl));
    memset(&g_CamInterpolation, 0, sizeof(g_CamInterpolation));
    memset(D_800ADF68, 0, sizeof(D_800ADF68));
    memset(D_800ADF88, 0, sizeof(D_800ADF88));

    g_FieldScriptVMCurActor = &g_TestActor;
    g_TestActor.scriptFlags.flags = 0x4000;
    for (int i = 0; i < 4; i++) {
        /* Retail considers all four slots clear only when +0x37C is nonzero. */
        g_FieldTextBoxes[i].cursor.visibility = 1;
    }
    D_800AFE9C = 0;
    D_800C2694 = 0;
    D_800ADB68 = 0;
    D_800ADB02 = 0;
    D_800B2354 = 0;
    D_800ADB64 = 0xFF;
    D_800B2340 = 0;
    D_800B2342 = 0;
    D_800B2344 = 0;
    D_800ADB28 = -1;
    D_800B2360 = 19;
    D_800B21CE = 0;
    D_800ADF68[15] = 0x345;
    D_800ADF88[15] = 0x678;
    g_CamInterpolation.targetAngleY = 0x45;
    g_InteractionResult = 0;
    g_InteractionCalls = 0;
    g_EncounterCalls = 0;
}

static int expect_common_completion(u16 expected_rotation) {
    return require(g_TestActor.scriptInstructionPointer == 1,
                   "instruction pointer advances once") &&
           require((u16)g_TestActor.rotation.vx == expected_rotation,
                   "direction result") &&
           require(D_800ADB68 == 1, "player control/run flag");
}

static int test_ordinary_interaction(void) {
    reset_fixture();
    D_800C2694 = 0x80;
    func_8009F5F4();
    return require(g_InteractionCalls == 1, "ordinary press calls interaction helper") &&
           require((g_TestActor.scriptFlags.flags & 0x800) != 0,
                   "ordinary press claims script interaction") &&
           require(D_800ADB28 == D_800B2360,
                   "ordinary press publishes party trail cursor") &&
           expect_common_completion(0x300);
}

static int test_interaction_helper_rejection(void) {
    reset_fixture();
    D_800C2694 = 0x80;
    g_InteractionResult = -1;
    func_8009F5F4();
    return require(g_InteractionCalls == 1, "rejected press still calls helper") &&
           require((g_TestActor.scriptFlags.flags & 0x800) == 0,
                   "rejected press does not claim script interaction") &&
           require(D_800ADB28 == -1, "rejected press preserves cursor") &&
           expect_common_completion(0x300);
}

static int test_ordinary_interaction_gates(void) {
    reset_fixture();
    D_800C2694 = 0x80;
    g_TestActor.scriptFlags.flags |= 0x1800;
    func_8009F5F4();
    if (!require(g_InteractionCalls == 0, "script flags gate interaction")) {
        return 0;
    }

    reset_fixture();
    D_800C2694 = 0x80;
    g_TestActor.curWalkmeshTriMaterial = 0x400000;
    func_8009F5F4();
    if (!require(g_InteractionCalls == 0, "movement material gate interaction")) {
        return 0;
    }

    reset_fixture();
    D_800C2694 = 0x80;
    D_800ADB64 = 2;
    func_8009F5F4();
    return require(g_InteractionCalls == 0, "active owner gate interaction");
}

static int test_stuck_held_retry(void) {
    reset_fixture();
    D_800ADB02 = 0x20;
    D_800AFE9C = 0x80;
    g_TestActor.curWalkmeshTriMaterial = 0x400000;
    g_TestActor.prevPosition.x = (s16)(g_TestActor.position.vx >> 16);
    g_TestActor.prevPosition.y = (s16)(g_TestActor.position.vy >> 16);
    g_TestActor.prevPosition.z = (s16)(g_TestActor.position.vz >> 16);
    func_8009F5F4();
    return require(D_800ADB02 == 0x20, "stuck count clamps to retail threshold") &&
           require(g_InteractionCalls == 1, "held button retries after stuck threshold") &&
           require((g_TestActor.scriptFlags.flags & 0x800) != 0,
                   "stuck held retry claims interaction") &&
           require(D_800ADB28 == D_800B2360,
                   "stuck held retry publishes cursor");
}

static int test_alternate_interaction_cooldown(void) {
    reset_fixture();
    D_800B2344 = 1;
    D_800B2340 = 6;
    D_800C2694 = 0x80;
    g_TestActor.curAnimationId = 7;
    func_8009F5F4();
    if (!require(g_InteractionCalls == 1, "alternate press calls helper") ||
        !require((g_TestActor.scriptFlags.flags & 0x800) != 0,
                 "alternate press claims interaction") ||
        !require(g_TestActor.curAnimationId == 0xFF,
                 "alternate press resets animation") ||
        !require(D_800ADB28 == D_800B2360,
                 "alternate press publishes cursor") ||
        !require(D_800B2342 == 5,
                 "alternate cooldown loads then decrements same frame")) {
        return 0;
    }

    reset_fixture();
    D_800B2344 = 1;
    D_800B2342 = 4;
    D_800C2694 = 0x80;
    func_8009F5F4();
    if (!require(g_InteractionCalls == 0,
                 "active cooldown suppresses repeated helper call") ||
        !require(D_800B2342 == 3, "active cooldown decrements on press")) {
        return 0;
    }

    reset_fixture();
    D_800B2344 = 1;
    D_800B2342 = 2;
    func_8009F5F4();
    return require(g_InteractionCalls == 0, "cooldown tick needs no press") &&
           require(D_800B2342 == 1, "cooldown decrements without press");
}

static int test_direction_idle_and_nonplayer_paths(void) {
    reset_fixture();
    D_800B2354 = 1;
    g_CamInterpolation.targetAngleY = 0x78;
    func_8009F5F4();
    if (!require(g_InteractionCalls == 0, "no press has no interaction") ||
        !expect_common_completion(0x600)) {
        return 0;
    }

    reset_fixture();
    D_800AFE9C = 0x1000;
    D_800ADF68[14] = 0x8000;
    func_8009F5F4();
    if (!require(g_EncounterCalls == 1, "held direction checks encounter") ||
        !require((u16)g_TestActor.rotation.vx == 0x8000,
                 "idle angle sentinel bypasses camera subtraction")) {
        return 0;
    }

    reset_fixture();
    g_FieldTextBoxes[2].cursor.visibility = 0;
    func_8009F5F4();
    if (!require((u16)g_TestActor.rotation.vx == 0x8000,
                 "dialog gate forces idle angle") ||
        !require(D_800ADB68 == 0, "dialog gate skips player-control flag") ||
        !require(g_TestActor.scriptInstructionPointer == 1,
                 "dialog gate advances instruction pointer")) {
        return 0;
    }

    reset_fixture();
    g_TestActor.scriptFlags.flags = 0x20;
    func_8009F5F4();
    if (!require(g_TestActor.scriptFlags.flags == 0x01000020,
                 "nonplayer path sets retail idle bit") ||
        !require(g_TestActor.scriptInstructionPointer == 1,
                 "nonplayer path advances instruction pointer")) {
        return 0;
    }

    reset_fixture();
    g_TestActor.scriptFlags.flags = 0x20;
    D_800B21CE = 1;
    func_8009F5F4();
    return require(g_TestActor.scriptFlags.flags == 0x20,
                   "global VM ownership suppresses nonplayer idle bit");
}

int main(void) {
    if (!test_ordinary_interaction() ||
        !test_interaction_helper_rejection() ||
        !test_ordinary_interaction_gates() ||
        !test_stuck_held_retry() ||
        !test_alternate_interaction_cooldown() ||
        !test_direction_idle_and_nonplayer_paths()) {
        return 1;
    }
    printf("FIELD_SCRIPT_VM_UPDATE_CHARACTER PASS checks=%u\n", g_Checks);
    return 0;
}

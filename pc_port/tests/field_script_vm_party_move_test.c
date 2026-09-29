#include "common.h"
#include "field/actor.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

s32 func_8009AEE0(s32 partySlot, s32 targetX, s32 targetZ, s32 direction);
void func_8009B210(void);
void func_8009B398(void);

FieldActor g_TestFieldActors[4];
FieldActor* volatile g_FieldActors = g_TestFieldActors;
ActorData g_TestActorData[4];
static u8 g_TestSpriteData[4][0x40];
ActorData* g_FieldScriptVMCurActor;
void* g_FieldScriptVMCurScriptData;
s32 D_800AFD1C;
s32 g_PlayerActorIndex;
s32 D_8005A444[3];
u16 g_FieldAngleToDirectionLUT[8];
s16 D_800B2348;
u8 D_800B21CC;
u8 D_800B21CE;
s32 D_800B2360;
s32 D_800B2364;
s32 D_800B2368;
int D_800B00C0;

static s32 g_Args[6];
static s32 g_Directions[3];
static int g_ArgumentCalls[6];
static int g_ArgumentMaskFailures;
static u8 g_ExpectedArgumentMask;
static int g_FacingCalls;
static s32 g_FacingVector[3];
static int g_PlaceCalls;
static s32 g_PlaceX;
static s32 g_PlaceZ;
static ActorData* g_PlaceObservedActor;
static s32 g_PlaceObservedActorIndex;
static int g_ResetTrailCalls;

VECTOR* Square0(VECTOR* input, VECTOR* output) {
    output->vx = input->vx * input->vx;
    output->vy = input->vy * input->vy;
    output->vz = input->vz * input->vz;
    return output;
}

int SquareRoot0(int value) {
    unsigned long n = value < 0 ? 0 : (unsigned long)value;
    unsigned long result = 0;
    while ((result + 1) * (result + 1) <= n) {
        result++;
    }
    return (int)result;
}

s32 func_8007B694(s32* vector) {
    g_FacingCalls++;
    memcpy(g_FacingVector, vector, sizeof(g_FacingVector));
    return 0x234;
}

void func_8009E574(s16 x, s16 z) {
    g_PlaceCalls++;
    g_PlaceX = x;
    g_PlaceZ = z;
    g_PlaceObservedActor = g_FieldScriptVMCurActor;
    g_PlaceObservedActorIndex = D_800AFD1C;
}

void func_80081C54(s32 actorIndex) {
    if (actorIndex != 3) {
        fprintf(stderr, "FIELD_SCRIPT_VM_PARTY_MOVE FAIL reset actor=%d\n", actorIndex);
    }
    g_ResetTrailCalls++;
}

static int argument_value(int ordinal, int index, int expectedIndex, int mask) {
    g_ArgumentCalls[ordinal]++;
    if (index != expectedIndex || (u8)mask != g_ExpectedArgumentMask) {
        g_ArgumentMaskFailures++;
    }
    return g_Args[ordinal];
}

int FieldScriptArgument1(int index, int mask) {
    return argument_value(0, index, 1, mask);
}
int FieldScriptArgument2(int index, int mask) {
    return argument_value(1, index, 3, mask);
}
int FieldScriptArgument3(int index, int mask) {
    return argument_value(2, index, 5, mask);
}
int FieldScriptArgument4(int index, int mask) {
    return argument_value(3, index, 7, mask);
}
int FieldScriptArgument5(int index, int mask) {
    return argument_value(4, index, 9, mask);
}
int FieldScriptArgument6(int index, int mask) {
    return argument_value(5, index, 0xB, mask);
}

int FieldScriptVMGetArgument(int index) {
    if (index == 0xE) return g_Directions[0];
    if (index == 0x10) return g_Directions[1];
    if (index == 0x12) return g_Directions[2];
    fprintf(stderr, "FIELD_SCRIPT_VM_PARTY_MOVE FAIL direction index=%d\n", index);
    return 0;
}

static int require(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "FIELD_SCRIPT_VM_PARTY_MOVE FAIL %s\n", message);
        return 0;
    }
    return 1;
}

static void set_integer_position(ActorData* actor, s16 x, s16 y, s16 z) {
    actor->position.vx = (s32)x * 65536;
    actor->position.vy = (s32)y * 65536;
    actor->position.vz = (s32)z * 65536;
}

static void copy_position_to_previous(ActorData* actor) {
    actor->prevPosition.x = (s16)(actor->position.vx >> 16);
    actor->prevPosition.y = (s16)(actor->position.vy >> 16);
    actor->prevPosition.z = (s16)(actor->position.vz >> 16);
}

static void reset_fixture(void) {
    static u8 bytecode[0x100];
    memset(g_TestFieldActors, 0, sizeof(g_TestFieldActors));
    memset(g_TestActorData, 0, sizeof(g_TestActorData));
    memset(g_TestSpriteData, 0, sizeof(g_TestSpriteData));
    memset(bytecode, 0, sizeof(bytecode));
    memset(g_Args, 0, sizeof(g_Args));
    memset(g_Directions, 0xFF, sizeof(g_Directions));
    memset(g_ArgumentCalls, 0, sizeof(g_ArgumentCalls));
    memset(g_FieldAngleToDirectionLUT, 0, sizeof(g_FieldAngleToDirectionLUT));
    g_ArgumentMaskFailures = 0;
    g_ExpectedArgumentMask = 0xA5;
    g_FacingCalls = 0;
    memset(g_FacingVector, 0, sizeof(g_FacingVector));
    g_PlaceCalls = 0;
    g_PlaceX = g_PlaceZ = 0;
    g_PlaceObservedActor = NULL;
    g_PlaceObservedActorIndex = -1;
    g_ResetTrailCalls = 0;
    for (int i = 0; i < 4; i++) {
        g_TestFieldActors[i].pActorData = (u32)(uintptr_t)&g_TestActorData[i];
        g_TestFieldActors[i].pSpriteData = (u32)(uintptr_t)&g_TestSpriteData[i][0];
        g_TestActorData[i].moveSpeed = 0x100;
        D_8005A444[i < 3 ? i : 0] = i < 3 ? i : D_8005A444[0];
    }
    g_FieldScriptVMCurActor = &g_TestActorData[3];
    g_FieldScriptVMCurScriptData = bytecode;
    D_800AFD1C = 3;
    g_PlayerActorIndex = 3;
    D_800B2348 = 0;
    D_800B21CC = 0;
    D_800B21CE = 0;
    D_800B2360 = D_800B2364 = D_800B2368 = 0;
    D_800B00C0 = 0;
}

static int test_helper_rejects_missing_and_hidden_members(void) {
    reset_fixture();
    D_8005A444[1] = 0xFF;
    if (!require(func_8009AEE0(1, 10, 20, 0xFF) == 0,
                 "missing party member return") ||
        !require(g_FacingCalls == 0 && g_PlaceCalls == 0,
                 "missing party member side effects")) {
        return 0;
    }
    D_8005A444[1] = 1;
    g_TestFieldActors[1].status = 0x20;
    return require(func_8009AEE0(1, 10, 20, 0xFF) == 0,
                   "hidden party member return") &&
           require(g_FacingCalls == 0 && g_PlaceCalls == 0,
                   "hidden party member side effects");
}

static int test_helper_arrival_and_facing_sources(void) {
    reset_fixture();
    ActorData* actor = &g_TestActorData[0];
    set_integer_position(actor, 0, 2, 0);
    actor->scriptFlags.flags = 0x02400800;
    actor->rotation.vy = 0x155;
    actor->unk11C = 0x2AA;
    actor->unk6E = 9;
    g_FieldAngleToDirectionLUT[2] = 0x456;

    if (!require(func_8009AEE0(0, 3, 4, 2) == 0, "arrival return") ||
        !require(*(s32*)&g_TestSpriteData[0][0x18] == 0x40000,
                 "arrival initializes movement quantum") ||
        !require(actor->position.vx == 3 * 65536 && actor->position.vz == 4 * 65536,
                 "arrival snaps target position") ||
        !require((u16)actor->rotation.vx == 0x8456 &&
                 (u16)actor->rotation.vy == 0x8456,
                 "arrival direction LUT facing") ||
        !require(actor->unk6E == 0, "arrival clears stuck counter") ||
        !require(actor->scriptFlags.flags == (0x02400800u & 0xFDDFF7FFu),
                 "arrival clears retail movement flags")) {
        return 0;
    }

    reset_fixture();
    actor = &g_TestActorData[0];
    actor->rotation.vy = 0x166;
    if (!require(func_8009AEE0(0, 0, 0, 0xFF) == 0,
                 "stored rotation arrival return") ||
        !require((u16)actor->rotation.vx == 0x8166,
                 "0xFF direction uses current rotation")) {
        return 0;
    }

    reset_fixture();
    actor = &g_TestActorData[0];
    actor->scriptFlags.flags = 0x8000;
    actor->unk11C = 0x321;
    return require(func_8009AEE0(0, 0, 0, 2) == 0,
                   "alternate rotation arrival return") &&
           require((u16)actor->rotation.vx == 0x8321,
                   "0x8000 flag uses alternate rotation");
}

static int test_helper_walk_and_stuck_fallback(void) {
    reset_fixture();
    ActorData* actor = &g_TestActorData[0];
    set_integer_position(actor, 0, 0, 0);
    copy_position_to_previous(actor);
    *(s32*)&g_TestSpriteData[0][0x18] = 0x8000;
    actor->unk6E = 7;

    if (!require(func_8009AEE0(0, 10, -3, 0xFF) == -1,
                 "walk-in-progress return") ||
        !require(actor->unk6E == 8, "walk increments unchanged-position counter") ||
        !require((actor->scriptFlags.flags & 0x400000) != 0,
                 "walk arms movement flag") ||
        !require((u16)actor->rotation.vx == 0x234 &&
                 (u16)actor->rotation.vy == 0x234,
                 "walk faces target") ||
        !require(g_FacingCalls == 1 && g_FacingVector[0] == 10 &&
                 g_FacingVector[1] == 0 && g_FacingVector[2] == -3,
                 "walk facing vector") ||
        !require(g_PlaceCalls == 0, "ordinary walk does not force placement")) {
        return 0;
    }

    actor->prevPosition.x = 1;
    if (!require(func_8009AEE0(0, 10, -3, 0xFF) == -1,
                 "moving actor remains in progress") ||
        !require(actor->unk6E == 0, "position change resets stuck counter")) {
        return 0;
    }

    reset_fixture();
    actor = &g_TestActorData[1];
    set_integer_position(actor, 0, 0, 0);
    copy_position_to_previous(actor);
    *(s32*)&g_TestSpriteData[1][0x18] = 0x8000;
    actor->unk6E = 0x40;
    ActorData* savedActor = g_FieldScriptVMCurActor;
    s32 savedIndex = D_800AFD1C;
    if (!require(func_8009AEE0(1, 30, 40, 3) == 0,
                 "stuck fallback return") ||
        !require(g_PlaceCalls == 1 && g_PlaceX == 30 && g_PlaceZ == 40,
                 "stuck fallback placement") ||
        !require(g_PlaceObservedActor == actor && g_PlaceObservedActorIndex == 1,
                 "stuck fallback temporary actor context") ||
        !require(g_FieldScriptVMCurActor == savedActor && D_800AFD1C == savedIndex,
                 "stuck fallback restores VM context") ||
        !require(actor->position.vx == 30 * 65536 && actor->position.vz == 40 * 65536,
                 "stuck fallback snaps target")) {
        return 0;
    }

    reset_fixture();
    actor = &g_TestActorData[2];
    *(s32*)&g_TestSpriteData[2][0x18] = 0x8000;
    D_800B2348 = 1;
    return require(func_8009AEE0(2, 20, 0, 0xFF) == 0,
                   "forced fallback return") &&
           require(g_PlaceCalls == 1, "D_800B2348 forces placement fallback");
}

static void prepare_handler_args(void) {
    static const s32 x[3] = {10, 20, 30};
    static const s32 z[3] = {11, 21, 31};
    for (int i = 0; i < 3; i++) {
        set_integer_position(&g_TestActorData[i], (s16)x[i], 0, (s16)z[i]);
        copy_position_to_previous(&g_TestActorData[i]);
        g_Args[i * 2] = x[i];
        g_Args[i * 2 + 1] = z[i];
        g_Directions[i] = 0xFF;
    }
}

static int test_group_handler_special_stop(void) {
    reset_fixture();
    u8* bytecode = (u8*)g_FieldScriptVMCurScriptData;
    g_FieldScriptVMCurActor->scriptInstructionPointer = 0x10;
    bytecode[0x10 + 0xD] = g_ExpectedArgumentMask;
    g_Args[0] = 0x7FFF;
    D_8005A444[1] = 0xFF;
    g_TestActorData[0].rotation.vy = 0x123;
    g_TestActorData[2].rotation.vy = (s16)0x8456;

    func_8009B398();
    return require(g_FieldScriptVMCurActor->scriptInstructionPointer == 0x24,
                   "special stop instruction length") &&
           require(D_800B21CE == 1, "special stop control flag") &&
           require((u16)g_TestActorData[0].rotation.vx == 0x8123 &&
                   (u16)g_TestActorData[0].rotation.vy == 0x8123,
                   "special stop first member facing") &&
           require((u16)g_TestActorData[2].rotation.vx == 0x8456,
                   "special stop third member facing") &&
           require(g_ArgumentCalls[0] == 1 && g_ArgumentMaskFailures == 0,
                   "special stop argument decode");
}

static int test_group_handler_complete_and_retry(void) {
    reset_fixture();
    prepare_handler_args();
    u8* bytecode = (u8*)g_FieldScriptVMCurScriptData;
    g_FieldScriptVMCurActor->scriptInstructionPointer = 0x20;
    bytecode[0x20 + 0xD] = g_ExpectedArgumentMask;
    D_800B21CC = 9;
    D_800B2348 = 7;

    func_8009B398();
    if (!require(g_FieldScriptVMCurActor->scriptInstructionPointer == 0x34,
                 "group complete instruction length") ||
        !require(D_800B21CC == 0 && D_800B2348 == 0,
                 "group complete clears controls") ||
        !require(D_800B21CE == 1 && D_800B00C0 == 1,
                 "group complete yields and owns player") ||
        !require(g_ArgumentCalls[0] == 2 && g_ArgumentCalls[1] == 1 &&
                 g_ArgumentCalls[2] == 1 && g_ArgumentCalls[3] == 1 &&
                 g_ArgumentCalls[4] == 1 && g_ArgumentCalls[5] == 1 &&
                 g_ArgumentMaskFailures == 0,
                 "group complete argument offsets and mask")) {
        return 0;
    }

    reset_fixture();
    prepare_handler_args();
    bytecode = (u8*)g_FieldScriptVMCurScriptData;
    g_FieldScriptVMCurActor->scriptInstructionPointer = 0x50;
    bytecode[0x50 + 0xD] = g_ExpectedArgumentMask;
    set_integer_position(&g_TestActorData[0], 0, 0, 0);
    copy_position_to_previous(&g_TestActorData[0]);
    *(s32*)&g_TestSpriteData[0][0x18] = 0x8000;

    func_8009B398();
    return require(g_FieldScriptVMCurActor->scriptInstructionPointer == 0x4F,
                   "group retry rewinds one byte") &&
           require(D_800B21CC == 1 && D_800B21CE == 1 && D_800B00C0 == 1,
                   "group retry control flags");
}

static int test_group_follow_opcode_always_yields(void) {
    reset_fixture();
    for (int i = 0; i < 3; i++) {
        set_integer_position(&g_TestActorData[i], 4, 0, 5);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = 0x60;
    D_800B00C0 = 0;
    D_800B21CE = 9;
    D_800B2360 = D_800B2364 = D_800B2368 = 7;

    func_8009B210();
    return require(g_FieldScriptVMCurActor->scriptInstructionPointer == 0x61,
                   "follow opcode complete instruction length") &&
           require(D_800B00C0 == 1, "follow opcode yields even when all members arrive") &&
           require(D_800B21CC == 0 && D_800B2348 == 0,
                   "follow opcode clears controls") &&
           require(D_800B21CE == 0 && D_800B2360 == 0 &&
                   D_800B2364 == 0 && D_800B2368 == 0,
                   "follow opcode resets trail state") &&
           require(g_ResetTrailCalls == 0x20, "follow opcode resets 32 trail records");
}

int main(void) {
    if (!test_helper_rejects_missing_and_hidden_members() ||
        !test_helper_arrival_and_facing_sources() ||
        !test_helper_walk_and_stuck_fallback() ||
        !test_group_handler_special_stop() ||
        !test_group_handler_complete_and_retry() ||
        !test_group_follow_opcode_always_yields()) {
        return 1;
    }
    puts("FIELD_SCRIPT_VM_PARTY_MOVE PASS retail_slices=8009AEE0-8009B15C,8009B210-8009B338,8009B398-8009B664");
    return 0;
}

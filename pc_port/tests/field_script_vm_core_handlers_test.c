#include "common.h"
#include "field/actor.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void FieldScriptVMWritePartyLeaderCharacterID(void);
void FieldScriptWriteActorDistance(void);

FieldActor g_TestFieldActors[4];
FieldActor* volatile g_FieldActors = g_TestFieldActors;
ActorData g_TestActorData[4];
ActorData* g_FieldScriptVMCurActor;
s32 g_PlayerActorIndex;

static int g_ActorIndexA;
static int g_ActorIndexB;
static int g_MagnitudeCalls;
static s32 g_LastMagnitudeX;
static s32 g_LastMagnitudeZ;
static int g_WriteCalls;
static int g_WriteAddress;
static int g_WriteValue;

int FieldScriptVMGetInstructionArgument(int offset) {
    if (offset != 1) {
        fprintf(stderr, "unexpected instruction argument offset %d\n", offset);
        return 0;
    }
    return 0x8124;
}

u32 FieldScriptVMGetActorIndex(int offset) {
    if (offset == 3) {
        return (u32)g_ActorIndexA;
    }
    if (offset == 4) {
        return (u32)g_ActorIndexB;
    }
    fprintf(stderr, "unexpected actor-index offset %d\n", offset);
    return 0xFF;
}

long FieldGetVec2Magnitude(long x, long z) {
    g_MagnitudeCalls++;
    g_LastMagnitudeX = (s32)x;
    g_LastMagnitudeZ = (s32)z;
    return 0x3456;
}

void FieldScriptMemoryWriteU16(int address, int value) {
    g_WriteCalls++;
    g_WriteAddress = address;
    g_WriteValue = value;
}

static void reset_fixture(void) {
    memset(g_TestFieldActors, 0, sizeof(g_TestFieldActors));
    memset(g_TestActorData, 0, sizeof(g_TestActorData));
    g_FieldScriptVMCurActor = &g_TestActorData[3];
    for (int i = 0; i < 4; i++) {
        g_TestFieldActors[i].pActorData = (u32)(uintptr_t)&g_TestActorData[i];
    }
    g_PlayerActorIndex = 1;
    g_ActorIndexA = 0;
    g_ActorIndexB = 2;
    g_MagnitudeCalls = 0;
    g_LastMagnitudeX = 0;
    g_LastMagnitudeZ = 0;
    g_WriteCalls = 0;
    g_WriteAddress = 0;
    g_WriteValue = 0;
}

static int require(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "FIELD_SCRIPT_VM_CORE_HANDLERS FAIL %s\n", message);
        return 0;
    }
    return 1;
}

static int test_party_leader_character_id(void) {
    reset_fixture();
    g_TestActorData[1].characterId = -7;
    g_TestActorData[3].characterId = 42;
    g_TestActorData[3].scriptInstructionPointer = 0x120;

    FieldScriptVMWritePartyLeaderCharacterID();

    return require(g_WriteCalls == 1, "leader write count") &&
           require(g_WriteAddress == 0x8124, "leader destination") &&
           require(g_WriteValue == -7, "leader must use player actor character id") &&
           require(g_TestActorData[3].scriptInstructionPointer == 0x123,
                   "leader instruction length");
}

static int test_actor_distance(void) {
    reset_fixture();
    g_TestActorData[0].position.vx = 50 << 16;
    g_TestActorData[0].position.vz = -12 * 65536;
    g_TestActorData[2].position.vx = -5 * 65536;
    g_TestActorData[2].position.vz = -30 * 65536;
    g_TestActorData[3].scriptInstructionPointer = 0x200;

    FieldScriptWriteActorDistance();

    return require(g_MagnitudeCalls == 1, "distance magnitude call count") &&
           require(g_LastMagnitudeX == 55, "distance x delta") &&
           require(g_LastMagnitudeZ == 18, "distance z delta") &&
           require(g_WriteCalls == 1, "distance write count") &&
           require(g_WriteAddress == 0x8124, "distance destination") &&
           require(g_WriteValue == 0x3456, "distance result") &&
           require(g_TestActorData[3].scriptInstructionPointer == 0x205,
                   "distance instruction length");
}

static int test_actor_distance_invalid_ids(void) {
    reset_fixture();
    g_ActorIndexA = 0xFF;
    g_ActorIndexB = 2;
    g_TestActorData[3].scriptInstructionPointer = 0x300;

    FieldScriptWriteActorDistance();
    if (!require(g_MagnitudeCalls == 0, "invalid first actor must skip magnitude") ||
        !require(g_WriteCalls == 1 && g_WriteValue == 0,
                 "invalid first actor writes zero distance") ||
        !require(g_TestActorData[3].scriptInstructionPointer == 0x305,
                 "invalid first actor instruction length")) {
        return 0;
    }

    reset_fixture();
    g_ActorIndexA = 0;
    g_ActorIndexB = 0xFF;
    FieldScriptWriteActorDistance();
    return require(g_MagnitudeCalls == 0, "invalid second actor must skip magnitude") &&
           require(g_WriteCalls == 1 && g_WriteValue == 0,
                   "invalid second actor writes zero distance");
}

int main(void) {
    if (!test_party_leader_character_id() ||
        !test_actor_distance() ||
        !test_actor_distance_invalid_ids()) {
        return 1;
    }
    puts("FIELD_SCRIPT_VM_CORE_HANDLERS PASS retail_slices=80099F48-80099FC4,8008E1B4-8008E298");
    return 0;
}

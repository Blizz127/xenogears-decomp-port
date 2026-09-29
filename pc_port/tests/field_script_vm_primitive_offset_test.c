#include "common.h"
#include "field/actor.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void func_8008B5D4(void);

FieldActor g_TestFieldActors[1];
FieldActor* volatile g_FieldActors = g_TestFieldActors;
ActorData g_TestScriptActor;
ActorData* g_FieldScriptVMCurActor = &g_TestScriptActor;
s32 D_800AFD1C;
int g_FieldCurRenderContextIndex;

static u8 g_ModelData[0x20] __attribute__((aligned(8)));
static u8 g_ModelHeader[0x30] __attribute__((aligned(8)));
static u8 g_Commands[0x80] __attribute__((aligned(8)));
static u8 g_BufferA[0x100] __attribute__((aligned(8)));
static u8 g_BufferB[0x100] __attribute__((aligned(8)));
static s16 g_XOffset;
static s16 g_YOffset;
static int g_ArgumentCalls;
static int g_BadArgumentOffset;

short FieldScriptVMGetInstructionArgumentS16(int offset) {
    g_ArgumentCalls++;
    if (offset == 1) return g_XOffset;
    if (offset == 3) return g_YOffset;
    g_BadArgumentOffset = offset;
    return 0;
}

static int require(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "FIELD_SCRIPT_VM_PRIMITIVE_OFFSET FAIL %s\n", message);
        return 0;
    }
    return 1;
}

static void put_u32(u8* destination, u32 value) {
    memcpy(destination, &value, sizeof(value));
}

static void reset_fixture(void) {
    memset(g_TestFieldActors, 0, sizeof(g_TestFieldActors));
    memset(&g_TestScriptActor, 0, sizeof(g_TestScriptActor));
    memset(g_ModelData, 0, sizeof(g_ModelData));
    memset(g_ModelHeader, 0, sizeof(g_ModelHeader));
    memset(g_Commands, 0, sizeof(g_Commands));
    memset(g_BufferA, 0x41, sizeof(g_BufferA));
    memset(g_BufferB, 0xB2, sizeof(g_BufferB));
    D_800AFD1C = 0;
    g_FieldCurRenderContextIndex = 0;
    g_XOffset = -3;
    g_YOffset = 5;
    g_ArgumentCalls = 0;
    g_BadArgumentOffset = 0;
    g_TestFieldActors[0].pModelData = (u32)(uintptr_t)g_ModelData;
    put_u32(g_ModelData + 0x04, (u32)(uintptr_t)g_ModelHeader);
    put_u32(g_ModelData + 0x08, (u32)(uintptr_t)g_BufferA);
    put_u32(g_ModelData + 0x0C, (u32)(uintptr_t)g_BufferB);
    put_u32(g_ModelHeader + 0x10, (u32)(uintptr_t)g_Commands);
}

static void seed_coordinates(u8* record, int vertices, u8 base) {
    static const u8 offsets[8] = {0x0C, 0x0D, 0x14, 0x15, 0x1C, 0x1D, 0x24, 0x25};
    for (int i = 0; i < vertices * 2; i++) {
        record[offsets[i]] = (u8)(base + i * 9);
    }
}

static void apply_expected(u8* current, u8* alternate, int vertices, s16 x, s16 y) {
    static const u8 offsets[8] = {0x0C, 0x0D, 0x14, 0x15, 0x1C, 0x1D, 0x24, 0x25};
    for (int i = 0; i < vertices; i++) {
        u8 xOffset = offsets[i * 2];
        u8 yOffset = offsets[i * 2 + 1];
        current[xOffset] = (u8)(current[xOffset] + x);
        current[yOffset] = (u8)(current[yOffset] + y);
        alternate[xOffset] = current[xOffset];
        alternate[yOffset] = current[yOffset];
    }
}

static int test_skips_control_and_updates_triangles_and_quads(void) {
    reset_fixture();
    u8* command = g_Commands;
    put_u32(command, 0x000000C4u);
    command += 4;
    put_u32(command, (2u << 16) | 0x20u);
    command += 4 + 2 * 8;
    put_u32(command, (1u << 16) | 0x28u);
    *(u16*)(g_ModelHeader + 0x06) = 3;

    seed_coordinates(g_BufferA + 0x00, 3, 0x03);
    seed_coordinates(g_BufferA + 0x20, 3, 0xE8);
    seed_coordinates(g_BufferA + 0x40, 4, 0x71);
    u8 expectedA[sizeof(g_BufferA)];
    u8 expectedB[sizeof(g_BufferB)];
    memcpy(expectedA, g_BufferA, sizeof(expectedA));
    memcpy(expectedB, g_BufferB, sizeof(expectedB));
    apply_expected(expectedA + 0x00, expectedB + 0x00, 3, g_XOffset, g_YOffset);
    apply_expected(expectedA + 0x20, expectedB + 0x20, 3, g_XOffset, g_YOffset);
    apply_expected(expectedA + 0x40, expectedB + 0x40, 4, g_XOffset, g_YOffset);
    u8 commandsBefore[sizeof(g_Commands)];
    memcpy(commandsBefore, g_Commands, sizeof(commandsBefore));
    g_TestScriptActor.scriptInstructionPointer = 0x120;

    func_8008B5D4();

    return require(memcmp(g_BufferA, expectedA, sizeof(g_BufferA)) == 0,
                   "current primitive bytes") &&
           require(memcmp(g_BufferB, expectedB, sizeof(g_BufferB)) == 0,
                   "alternate primitive coordinate publication") &&
           require(memcmp(g_Commands, commandsBefore, sizeof(g_Commands)) == 0,
                   "command stream must remain read-only") &&
           require(g_ArgumentCalls == 2 && g_BadArgumentOffset == 0,
                   "signed argument offsets") &&
           require(g_TestScriptActor.scriptInstructionPointer == 0x125,
                   "instruction length");
}

static int test_render_page_one_swaps_current_and_alternate(void) {
    reset_fixture();
    put_u32(g_Commands, (1u << 16) | 0x20u);
    *(u16*)(g_ModelHeader + 0x06) = 1;
    g_FieldCurRenderContextIndex = 1;
    g_XOffset = 2;
    g_YOffset = -2;
    seed_coordinates(g_BufferB, 3, 0x31);
    u8 expectedA[sizeof(g_BufferA)];
    u8 expectedB[sizeof(g_BufferB)];
    memcpy(expectedA, g_BufferA, sizeof(expectedA));
    memcpy(expectedB, g_BufferB, sizeof(expectedB));
    apply_expected(expectedB, expectedA, 3, g_XOffset, g_YOffset);

    func_8008B5D4();
    return require(memcmp(g_BufferB, expectedB, sizeof(g_BufferB)) == 0,
                   "page-one current primitive bytes") &&
           require(memcmp(g_BufferA, expectedA, sizeof(g_BufferA)) == 0,
                   "page-one alternate primitive bytes");
}

static int test_zero_command_count_only_advances_ip(void) {
    reset_fixture();
    u8 beforeA[sizeof(g_BufferA)];
    u8 beforeB[sizeof(g_BufferB)];
    memcpy(beforeA, g_BufferA, sizeof(beforeA));
    memcpy(beforeB, g_BufferB, sizeof(beforeB));
    g_TestScriptActor.scriptInstructionPointer = 0x10;

    func_8008B5D4();
    return require(memcmp(g_BufferA, beforeA, sizeof(g_BufferA)) == 0 &&
                   memcmp(g_BufferB, beforeB, sizeof(g_BufferB)) == 0,
                   "zero-count buffers") &&
           require(g_TestScriptActor.scriptInstructionPointer == 0x15,
                   "zero-count instruction length");
}

int main(void) {
    if (!test_skips_control_and_updates_triangles_and_quads() ||
        !test_render_page_one_swaps_current_and_alternate() ||
        !test_zero_command_count_only_advances_ip()) {
        return 1;
    }
    puts("FIELD_SCRIPT_VM_PRIMITIVE_OFFSET PASS retail_slice=8008B5D4-8008B894");
    return 0;
}

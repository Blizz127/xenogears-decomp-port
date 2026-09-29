/* Production-linked CHANGE_FIELD (func_800932D0) certificate.
 *
 * Compiles the shipped src/field/main/misc11.c body and executes the
 * map-1-actor-50 leave encoding (98 0f 80 01 80). The handler must write
 * map 15 / entrance 1 and must not read a story-flag slot.
 */
#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/script_vm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void func_800932D0(void);

s32 D_800B00C0;
s32 D_800ADBDC = 1;
s32 D_800ADBE4 = 1;
s32 D_800ADB2C;
s32 D_8004F308;
s32 D_800ADB90;
s32 D_800ADB70;
s32 D_800ADBEC = 1;
s32 g_GameSceneMapNum = 1;
s32 D_800AFD1C;
s32 D_800AFFEC;
s32 D_800ADB1C;
s32 D_800ADBE0 = 1;
s32 g_FieldSystemMode;
s32 g_FieldScriptMaxInstructionCount;
s32 D_8004F350;
s8 D_80059171;
s32 D_800ADB64 = 0xFF;
u16 D_800B236C;
s32 D_800ADBD8;
s32 D_800B0064;
u8 D_800B02C8;
s32 D_800B0048;
s32 D_800AFD14;
u32 g_PcPortOpcode56Func8009FEE4ConditionCount;
char D_8006FD44;
char D_8006FD84;

FieldControl g_FieldControl;
ActorData* g_FieldScriptVMCurActor;
void* g_FieldScriptVMCurScriptData;
void* g_FieldScriptMemory;
FieldScene g_Scene;
CameraInterpolation g_CamInterpolation;
u8 D_800B21D0[4];
u8 D_800B21D1[4];

static ActorData s_actor;
static u8 s_script[16];
static u16 s_mem[0x40];
static int s_var_reads;
static int s_arg_reads[8];
static int s_write_addr[8];
static int s_write_val[8];
static int s_writes;

int FieldScriptVMGetInstructionArgument(int argumentIndex) {
    u_char* p = (u_char*)g_FieldScriptVMCurScriptData
                + g_FieldScriptVMCurActor->scriptInstructionPointer
                + argumentIndex;
    return p[0] | (p[1] << 8);
}

int FieldScriptVMGetArgument(int index) {
    int n;
    if (index >= 0 && index < 8) {
        s_arg_reads[index]++;
    }
    n = FieldScriptVMGetInstructionArgument(index);
    if (!(n & 0x8000)) {
        s_var_reads++;
        return (int)s_mem[(n & 0xFFFF) >> 1];
    }
    return n & 0x7FFF;
}

int FieldScriptVMGetVariableValue(int address) {
    s_var_reads++;
    return (int)s_mem[(address & 0xFFFF) >> 1];
}

void FieldScriptMemoryWriteU16(u16 address, u16 value) {
    if (s_writes < 8) {
        s_write_addr[s_writes] = address;
        s_write_val[s_writes] = value;
    }
    s_writes++;
    s_mem[address >> 1] = value;
}

int FieldGetPlayerActorDirection(void) { return 0x200; }
int FieldGetCameraDirection(void) { return 0x600; }

void PcPort_FieldOpcode56TransitionIntercept(s32 a, u32 b, s32 c, u16 d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
}
void PcPort_FieldOpcode56RecordControlLock(s16 a, s32 b, u16 c) {
    (void)a;
    (void)b;
    (void)c;
}

static int require(int cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "CHANGE_FIELD FAIL %s\n", msg);
        return 0;
    }
    return 1;
}

int main(void) {
    int ok = 1;
    /* Retail map-1 actor 50 IP 0x183b encoding. */
    memcpy(s_script, "\x98\x0f\x80\x01\x80", 5);
    memset(&s_actor, 0, sizeof(s_actor));
    memset(s_mem, 0, sizeof(s_mem));
    s_actor.scriptInstructionPointer = 0;
    g_FieldScriptVMCurActor = &s_actor;
    g_FieldScriptVMCurScriptData = s_script;
    g_FieldScriptMemory = s_mem;
    g_FieldControl.isRandomEncountersEnabled = 0;
    g_GameSceneMapNum = 1;

    func_800932D0();

    ok &= require(g_GameSceneMapNum == 15, "map becomes 15");
    ok &= require(s_mem[2 >> 1] == 1, "entrance var2 becomes 1");
    ok &= require(s_actor.scriptInstructionPointer == 5, "IP += 5");
    ok &= require(g_FieldControl.isRandomEncountersEnabled == (s16)-1,
                  "encounters locked");
    ok &= require(s_arg_reads[1] >= 1 && s_arg_reads[3] >= 1,
                  "reads arg1/arg3 immediates");
    ok &= require(s_var_reads == 1,
                  "only func_80092F44's var 0x12 increment, no story-flag read");
    ok &= require(D_800B00C0 == 1, "yields after arming");
    if (!ok) {
        return 1;
    }
    printf("CHANGE_FIELD PASS map=15 entrance=1 ip+=5 no-story-var-gate\n");
    return 0;
}

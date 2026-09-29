/* The transition keeps field mode active while its actor allocation is stale. */
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include "common.h"
#include "field/actor.h"
#include "field/main.h"

FieldActor* volatile g_FieldActors;
__typeof__(g_pFieldTriggerZones) g_pFieldTriggerZones;
void* g_FieldScriptMemory;
void* D_8005A4E0;
s32 g_PlayerActorIndex, g_FieldNumActors, D_800ADBFC;
s32 D_800ADB68, D_800ADB64;
int g_GameSceneMapNum;
u16 D_800AFE9C, D_800C2694;
u8 D_800B21D0, D_800ADB04;
static int active;
int PcPort_QuickCheckpointFieldIsActive(void) { return active; }
int NormalClip(int a, int b, int c) {
    (void)a; (void)b; (void)c;
    abort(); /* No geometry may be examined in either unavailable state. */
}
extern void PcPort_FieldPosDiag(void);
static void stale_pointer_read(int signal_number) {
    (void)signal_number;
    _Exit(99);
}
int main(void) {
    assert(signal(SIGSEGV, stale_pointer_read) != SIG_ERR);
    assert(setenv("XENO_FIELD_POS_DIAG", "1", 1) == 0);
    g_FieldActors = (FieldActor*)(uintptr_t)1;
    g_pFieldTriggerZones = (__typeof__(g_pFieldTriggerZones))(uintptr_t)1;
    D_8005A4E0 = (void*)(uintptr_t)1;
    g_PlayerActorIndex = 1;
    g_FieldNumActors = 38;
    active = 0;
    D_800ADB04 = 1;
    PcPort_FieldPosDiag();
    active = 1;
    D_800ADB04 = 0;
    PcPort_FieldPosDiag();
    return 0;
}

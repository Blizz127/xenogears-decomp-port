#include <stdint.h>
#include <string.h>

void func_800A22AC(int routine)
{
    _Static_assert(sizeof(ActorData) == 312, "field actor snapshot size");

    D_800B06B8 = g_FieldActors;
    g_FieldScriptVMCurActor = (ActorData *)(uintptr_t)D_800B06B8->pActorData;
    void *snapshot = HeapAlloc(sizeof(ActorData), 1);
    memcpy(snapshot, (const void *)(uintptr_t)D_800B06B8->pActorData,
           sizeof(ActorData));

    for (int i = 0; i < ACTOR_MAX_NUM_SCRIPTS; ++i) {
        ActorScriptSlot *slot = &g_FieldScriptVMCurActor->scripts[i];
        slot->currentIP = 0xffff;
        slot->waitTimer = 0;
        slot->scriptId = 0xff;
        slot->flags_0 = 0xffff;
        slot->state = 0;
        slot->flags_0x12 = 15;
        slot->isInUse = 0;
        slot->flags_0x17 = 0;
    }

    D_800AFD1C = 0;
    D_800ADB1C = 0;
    D_800AFFEC = 0;
    int offset = FieldScriptGetBytecodeOffset(0, routine);
    g_FieldScriptVMCurActor->scriptInstructionPointer = (uint16_t)offset;
    FieldScriptVMRun(65535);

    D_800ADB1C = 1;
    memcpy((void *)(uintptr_t)D_800B06B8->pActorData, snapshot,
           sizeof(ActorData));
    HeapFree(snapshot);
}

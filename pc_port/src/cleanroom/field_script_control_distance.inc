extern int FieldScriptVMGetArgument(int);
extern long FieldGetVec3Magnitude(long, long, long);
typedef s16 FieldDistanceHalf __attribute__((__may_alias__));

static s16 fieldDistanceCoordinate(const volatile int *fixed)
{
    return ((const volatile FieldDistanceHalf *)fixed)[1];
}

void func_80093B10(void)
{
    *(volatile short *)&g_FieldControl.isRandomEncountersEnabled = -1;
    *(volatile s8 *)D_800B21D0 = 1;
    *(volatile s8 *)D_800B21D1 = 1;
    volatile s32 *sceneFlags = &g_Scene.unk48;
    u32 flags = (u32)*sceneFlags;
    int ready = *(const volatile s32 *)&D_800ADBDC;
    *sceneFlags = (s32)(flags | 0xc000u);
    if (ready)
        ready = *(const volatile s32 *)&D_800ADBE4;
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    u16 cursor = actor->scriptInstructionPointer;
    if (!ready) {
        *(volatile int *)&D_800B00C0 = 1;
        actor->scriptInstructionPointer = (u16)(cursor - 1);
    } else {
        actor->scriptInstructionPointer = (u16)(cursor + 1);
    }
}

void FieldScriptCheckActorDistance(void)
{
    if (FieldScriptVMGetActorIndex(1) != 255) {
        int index = FieldScriptVMGetActorIndex(1);
        const volatile FieldActor *actors = g_FieldActors;
        const volatile FieldActor *current = *(FieldActor *volatile *)&D_800B06B8;
        const volatile ActorData *from = (const void *)(uintptr_t)current->pActorData;
        const volatile ActorData *to = (const void *)(uintptr_t)actors[index].pActorData;
        int x = fieldDistanceCoordinate(&from->position.vx);
        x -= fieldDistanceCoordinate(&to->position.vx);
        int y = fieldDistanceCoordinate(&from->position.vy);
        int z = fieldDistanceCoordinate(&from->position.vz);
        z -= fieldDistanceCoordinate(&to->position.vz);
        y -= fieldDistanceCoordinate(&to->position.vy);
        int length = (s32)(u32)FieldGetVec3Magnitude(x, y, z);
        if (length < FieldScriptVMGetArgument(2)) {
            volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
            actor->scriptInstructionPointer = (u16)(actor->scriptInstructionPointer + 6);
            return;
        }
    }
    int jump = FieldScriptVMGetInstructionArgument(4);
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer = (u16)jump;
}

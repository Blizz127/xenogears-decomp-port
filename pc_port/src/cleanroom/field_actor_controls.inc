extern int D_800AFD1C;
extern u32 FieldScriptVMGetActorIndex(int offset);
extern int FieldScriptVMGetArgument(int offset);
extern int func_8009CD18(int *index);
extern void func_8009E574(int x, int z);

static FieldActor *FieldActorControlRecord(u32 index) {
    uintptr_t base = (uintptr_t)g_FieldActors;
    u32 displacement = index * (u32)sizeof(FieldActor);
    if (displacement <= 0x7fffffffu)
        return (FieldActor *)(base + displacement);
    return (FieldActor *)(base - (0u - displacement));
}

static void FieldActorControlCloseTextBox(void) {
    int index;
    if (func_8009CD18(&index) == 0)
        g_FieldTextBoxes[index].status = 0;
}

void func_8009DAC4(void) {
    if (FieldScriptVMGetActorIndex(1) != 255u) {
        u32 index = FieldScriptVMGetActorIndex(1);
        ActorData *target = (ActorData *)(uintptr_t)FieldActorControlRecord(index)->pActorData;
        target->scriptFlags.flags |= 1u;
        target->flags |= 0x00100000u;

        index = FieldScriptVMGetActorIndex(1);
        FieldActorControlRecord(index)->status |= 0x20;
        FieldActorControlCloseTextBox();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009DC4C(void) {
    if (FieldScriptVMGetActorIndex(1) != 255u) {
        u32 index = FieldScriptVMGetActorIndex(1);
        ActorData *target = (ActorData *)(uintptr_t)FieldActorControlRecord(index)->pActorData;
        s16 rotation;
        target->moveModified.vx = 0;
        target->moveModified.vy = 0;
        target->moveModified.vz = 0;
        target->move.vx = 0;
        target->move.vy = 0;
        target->move.vz = 0;
        target->scriptFlags.flags |= 1u;
        rotation = (s16)((int)((u16)target->rotation.vx & 0x7fffu) - 32768);
        target->rotation.vy = rotation;
        target->rotation.vx = rotation;
        FieldActorControlCloseTextBox();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

static u8 *FieldActorControlWaitTimer(ActorData *actor) {
    uintptr_t first = (uintptr_t)&actor->scripts[0].waitTimer;
    return (u8 *)(first + (uintptr_t)actor->curScriptIndex * sizeof(ActorScriptSlot));
}

void FieldScriptVMHandlerSleep(void) {
    u8 *timer = FieldActorControlWaitTimer(g_FieldScriptVMCurActor);
    u8 remaining = *timer;
    if (remaining != 0) {
        *timer = (u8)(remaining - 1);
    } else {
        int duration = FieldScriptVMGetArgument(1);
        *FieldActorControlWaitTimer(g_FieldScriptVMCurActor) = (u8)duration;
    }
    if (*FieldActorControlWaitTimer(g_FieldScriptVMCurActor) == 0) {
#ifndef NPC_EVENT_MUTANT_NO_IP
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
#endif
    }
    D_800B00C0 = 1;
}

static int FieldActorControlHighHalf(s32 coordinate) {
    int high = (int)((u32)coordinate >> 16);
    return high < 32768 ? high : high - 65536;
}

void func_8009E428(void) {
    ActorData *actor = g_FieldScriptVMCurActor;
    const u8 *script = (const u8 *)g_FieldScriptVMCurScriptData;
    const ActorData *positionActor;
    actor->walkmeshId = script[(u32)actor->scriptInstructionPointer + 1u];
    positionActor = (const ActorData *)(uintptr_t)FieldActorControlRecord((u32)D_800AFD1C)->pActorData;
    func_8009E574(FieldActorControlHighHalf(positionActor->position.vx),
                  FieldActorControlHighHalf(positionActor->position.vz));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

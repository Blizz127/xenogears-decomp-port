extern int g_GameSceneMapNum;
extern int D_800B2268;
extern void func_80021BCC(void *sprite, u32 speed);
extern void *HeapAlloc(u32 size, u32 flags);
extern int FieldScriptArgument1(int offset, int packed);
extern int FieldScriptArgument2(int offset, int packed);
extern int FieldScriptArgument3(int offset, int packed);
extern int FieldScriptArgument4(int offset, int packed);
extern int FieldScriptArgument5(int offset, int packed);
extern int FieldScriptArgument6(int offset, int packed);
extern int FieldScriptArgument7(int offset, int packed);
extern int FieldScriptArgument8(int offset, int packed);
extern void GameWaitForCdData(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);

void FieldScriptVMHandlerEnableActorVM(void) {
    if (FieldScriptVMGetActorIndex(1) != 255u) {
        u32 index = FieldScriptVMGetActorIndex(1);
        const FieldActor *record = FieldActorControlRecord(index);
        ActorData *actor = (ActorData *)(uintptr_t)record->pActorData;
        actor->scriptFlags.flags &= ~1u;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerShowActorById(void) {
    if (FieldScriptVMGetActorIndex(1) != 255u) {
        u32 index = FieldScriptVMGetActorIndex(1);
        volatile FieldActor *record = FieldActorControlRecord(index);
        volatile ActorData *actor = (ActorData *)(uintptr_t)record->pActorData;
        if ((actor->flags & (1u << 20)) == 0) {
            record->status &= ~0x20;
            actor->flags &= ~(1u << 25);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009DF78(void) {
    if (FieldScriptVMGetActorIndex(1) != 255u) {
        u32 index = FieldScriptVMGetActorIndex(1);
        const volatile FieldActor *record = FieldActorControlRecord(index);
        volatile ActorData *actor = (ActorData *)(uintptr_t)record->pActorData;
        actor->flags |= 1u << 25;
        actor = (ActorData *)(uintptr_t)record->pActorData;
        actor->flags |= 1u << 11;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E094(void) {
    u16 speed = (u16)FieldScriptVMGetArgument(1);
    int index = *(const volatile int *)&D_800AFD1C;
    volatile ActorData *actor = g_FieldScriptVMCurActor;
    const FieldActor *record;
    actor->moveSpeed = speed;
    record = FieldActorControlRecord((u32)index);
    func_80021BCC((void *)(uintptr_t)record->pSpriteData, speed);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009E10C(void) {
    u32 input = (u32)FieldScriptVMGetArgument(1);
    ActorData *actor = g_FieldScriptVMCurActor;
    u32 selected = ((input & 0x01u) << 7) | ((input & 0x04u) << 3) |
                   ((input & 0x08u) << 1) | ((input & 0x10u) >> 1) |
                   ((input & 0x20u) >> 3) | ((input & 0x40u) << 21);
    actor->scriptFlags.flags = (actor->scriptFlags.flags & ~0x080000bcu) | selected;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

/* The slot at 0x12c is a word of bitfields. Access its aligned backing word
 * without assuming a struct bitfield order or dropping a same-value write. */
typedef u32 FieldActorSceneFlagWord __attribute__((__may_alias__));
static volatile FieldActorSceneFlagWord *FieldActorSceneBufferFlags(void) {
    ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    return (volatile FieldActorSceneFlagWord *)((u8 *)actor + 0x12c);
}

void func_8009E91C(void) {
    static int (*const arguments[])(int, int) = {
        FieldScriptArgument1, FieldScriptArgument2, FieldScriptArgument3, FieldScriptArgument4,
        FieldScriptArgument5, FieldScriptArgument6, FieldScriptArgument7, FieldScriptArgument8
    };
    const u32 allocated = 1u << 12;
    volatile FieldActorSceneFlagWord *flags;
    int slot;
    if ((*FieldActorSceneBufferFlags() & allocated) == 0) {
        void *buffer = HeapAlloc(16, 0);
        g_FieldScriptVMCurActor->unk114 = (u32)(uintptr_t)buffer;
    }
    flags = FieldActorSceneBufferFlags();
    *flags |= allocated;
    for (slot = 0; slot < 8; ++slot) {
        const u8 *script = (const u8 *)g_FieldScriptVMCurScriptData;
        int packed = script[(u32)g_FieldScriptVMCurActor->scriptInstructionPointer + 17u];
        u16 value = (u16)arguments[slot](2 * slot + 1, packed);
        u8 *buffer = (u8 *)(uintptr_t)g_FieldScriptVMCurActor->unk114;
        __builtin_memcpy(buffer + 2 * slot, &value, sizeof value);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 18;
}

void func_8009FB98(void) {
    ActorData *actor;
    const u8 *script;
    g_GameSceneMapNum |= 0xc000;
    GameWaitForCdData();
    GamePartySyncSkinData();
    GamePartySyncStreamedData();
    actor = g_FieldScriptVMCurActor;
    script = (const u8 *)g_FieldScriptVMCurScriptData;
    D_800B2268 = script[(u32)actor->scriptInstructionPointer + 1u];
    actor->scriptInstructionPointer += 2;
}

void func_80093E30(void)
{
    extern void FieldDoorPlaySound(int, int) __asm__("func_80085634");
    extern void FieldDoorRefreshMatrix(s32) __asm__("func_80072254");
    extern s32 D_800AFD1C;
    const u32 opened = 1u << 20;
    const u32 opening = 1u << 5;
    ActorData* volatile* current = &g_FieldScriptVMCurActor;
    volatile s32* actorIndex = &D_800AFD1C;
    volatile ActorData* actor = *current;

    if (actor->scriptFlags.flags & opened) {
        actor->scriptInstructionPointer += 2u;
    } else {
        /* The public struct exposes this word's individual bitfields. */
        volatile u32* flags = (volatile u32*)((volatile u8*)actor + 0x12c);
        u32 state = *flags;
        if (!(state & opening)) {
            *flags = state | opening;
            actor->curDoorStep = 0;
            FieldDoorPlaySound(8, 3);
        } else {
            ++actor->curDoorStep;
            actor = *current;
            if (actor->curDoorStep < 31u) {
                u16 cursor = actor->scriptInstructionPointer;
                void* volatile* currentBank = &g_FieldScriptVMCurScriptData;
                const volatile u8* bank = *currentBank;
                u8 selector = bank[cursor + 1u];
                s32 index = *actorIndex;
                volatile FieldActor* records = g_FieldActors;
                s16 angle = records[index].rotation.y;
                records[index].rotation.y = (s16)(angle + (selector ? -32 : 32));
            } else {
                u32 scriptFlags = actor->scriptFlags.flags;
                actor->curDoorStep = 0;
                actor->scriptFlags.flags = scriptFlags | opened;
                flags = (volatile u32*)((volatile u8*)actor + 0x12c);
                state = *flags;
                volatile ActorData* cursorActor = *current;
                *flags = state & ~opening;
                cursorActor->scriptInstructionPointer += 2u;
            }
        }
    }
    FieldDoorRefreshMatrix(*actorIndex);
}

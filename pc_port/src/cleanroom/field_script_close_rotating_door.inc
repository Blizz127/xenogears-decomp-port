void func_80093FC0(void)
{
    extern void FieldDoorPlaySound(int, int) __asm__("func_80085634");
    extern void FieldDoorRefreshMatrix(s32) __asm__("func_80072254");
    extern s32 D_800AFD1C;

    const u32 open = 1u << 20;
    const u32 moving = 1u << 5;
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    volatile s32 *currentIndex = &D_800AFD1C;
    volatile ActorData *actor = *current;

    if (!(actor->scriptFlags.flags & open)) {
        actor->scriptInstructionPointer += 2u;
    } else {
        volatile u32 *phase = (volatile u32 *)((volatile u8 *)actor + 0x12c);
        u32 state = *phase;
        if (!(state & moving)) {
            *phase = state | moving;
            actor->curDoorStep = 0;
            FieldDoorPlaySound(8, 3);
        } else {
            ++actor->curDoorStep;
            actor = *current;
            if (actor->curDoorStep < 31u) {
                unsigned int ip = actor->scriptInstructionPointer;
                void *volatile *currentScript = &g_FieldScriptVMCurScriptData;
                const volatile u8 *script = *currentScript;
                u8 direction = script[ip + 1u];
                s32 index = *currentIndex;
                volatile FieldActor *records = g_FieldActors;
                s16 rotation = records[index].rotation.y;
                records[index].rotation.y = (s16)(rotation + (direction ? 32 : -32));
            } else {
                u32 flags = actor->scriptFlags.flags;
                actor->curDoorStep = 0;
                volatile ActorData *cursorActor = *current;
                actor->scriptFlags.flags = flags & ~open;
                phase = (volatile u32 *)((volatile u8 *)actor + 0x12c);
                *phase &= ~moving;
                cursorActor->scriptInstructionPointer += 2u;
            }
        }
    }
    FieldDoorRefreshMatrix(*currentIndex);
}

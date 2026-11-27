void func_800947B0(void)
{
    if (FieldScriptVMGetActorIndex(2) != 255) {
        int index = FieldScriptVMGetActorIndex(2);
        volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
        unsigned int ip = actor->scriptInstructionPointer;
        const volatile u8 *bank = *(void *volatile *)&g_FieldScriptVMCurScriptData;
        FieldActor *pool = *(FieldActor *volatile *)&g_FieldActors;
        unsigned int mode = bank[ip + 1];
        if (mode < 6) {
            unsigned int amount = (unsigned int)FieldScriptVMGetArgument(3);
            volatile u16 *component =
                (volatile u16 *)((u8 *)&pool[index].rotation + 2 * (mode / 2));
            unsigned int current = *component;
            *component = (u16)((mode & 1) ? current - amount : current + amount);
        }
        func_80072254(FieldScriptVMGetActorIndex(2));
    }
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 5;
}

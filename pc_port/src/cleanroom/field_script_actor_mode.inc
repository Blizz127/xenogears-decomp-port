static void fieldScriptSetActorMode(unsigned int mode)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    /* The low two bits share their 32-bit flag word with other actor state. */
    volatile u32 *flags = (volatile u32 *)((volatile u8 *)actor + 0x12c);
    *flags = (*flags & ~3u) | mode;
    int value = FieldScriptVMGetArgument(1);
    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    unsigned int ip = actor->scriptInstructionPointer;
    actor->unk70 = (u16)value;
    actor->scriptInstructionPointer = (u16)(ip + 3);
}

void func_800946BC(void)
{
    fieldScriptSetActorMode(1);
}

void func_80094710(void)
{
    fieldScriptSetActorMode(2);
}

void func_80094764(void)
{
    fieldScriptSetActorMode(3);
}

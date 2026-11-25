void func_8009E1A0(void)
{
    ActorData* volatile* actorSlot = &g_FieldScriptVMCurActor;
    void* volatile* bankSlot = &g_FieldScriptVMCurScriptData;
    volatile ActorData* actor = *actorSlot;
    const volatile u8* bank = *bankSlot;

    u16 firstCursor = actor->scriptInstructionPointer;
    u32 flags = actor->flags;
    u16 secondCursor = actor->scriptInstructionPointer;
    u8 first = bank[firstCursor + 1];
    u16 advanceFrom = actor->scriptInstructionPointer;

    flags = (flags & ~7u) | (first & 7u);
    actor->flags = flags;
    u8 second = bank[secondCursor + 1];
    actor->scriptInstructionPointer = advanceFrom + 2;
    actor->flags = (flags & ~0x38u) | ((second >> 1) & 0x38u);
}

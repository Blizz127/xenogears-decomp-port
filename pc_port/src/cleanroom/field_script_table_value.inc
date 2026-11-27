void FieldScriptMemoryWriteU16(int address, int value);

static unsigned int fieldScriptTableOffset(void)
{
    unsigned int base = (unsigned int)FieldScriptVMGetInstructionArgument(1);
    unsigned int offset = (unsigned int)FieldScriptVMGetArgument(5);
    return (u16)(base + offset);
}

void func_80093CD0(void)
{
    unsigned int offset = fieldScriptTableOffset();
    int destination = (u16)FieldScriptVMGetInstructionArgument(3);
    const volatile u8 *bank = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    int value = bank[offset];
    FieldScriptMemoryWriteU16(destination, value);
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 7;
}

void func_80093D48(void)
{
    unsigned int offset = fieldScriptTableOffset();
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile u8 *bank = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned int ip = actor->scriptInstructionPointer;
    unsigned int signedMode = bank[ip + 7];
    int destination = (u16)FieldScriptVMGetInstructionArgument(3);
    bank = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    int high = bank[offset + 1];
    int value = (high << 8) | bank[offset];
    if (signedMode && value >= 0x8000) {
        value -= 0x10000;
    }
    FieldScriptMemoryWriteU16(destination, value);
    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 8;
}

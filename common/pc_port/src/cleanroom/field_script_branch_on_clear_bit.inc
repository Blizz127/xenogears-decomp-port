void func_8008D780(void)
{
    int variable = ((u16)FieldScriptVMGetInstructionArgument(1)) >> 4;
    int bit = FieldScriptVMGetInstructionArgument(1) & 15;
    u32 value = (u32)FieldScriptVMGetVariableValue(variable);

    ActorData* volatile* current = &g_FieldScriptVMCurActor;
    volatile ActorData* actor;
    u16 next;
    if (value & (1u << bit)) {
        actor = *current;
        next = actor->scriptInstructionPointer + 5u;
    } else {
        next = (u16)FieldScriptVMGetInstructionArgument(3);
        actor = *current;
    }
    actor->scriptInstructionPointer = next;
}

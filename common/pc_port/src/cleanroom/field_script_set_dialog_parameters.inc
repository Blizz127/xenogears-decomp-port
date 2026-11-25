extern int FieldScriptVMGetArgument(int offset);

void func_8009CEE0(void)
{
    ActorData* volatile* current = &g_FieldScriptVMCurActor;
    int value = FieldScriptVMGetArgument(1);
    volatile ActorData* actor = *current;
    actor->dialogPixelWidth = (s16)value;

    value = FieldScriptVMGetArgument(3);
    actor = *current;
    actor->dialogPixelHeight = (s16)value;

    value = FieldScriptVMGetArgument(5);
    actor = *current;
    actor->dialogWidth = (u8)((u32)value * 3u);

    value = FieldScriptVMGetArgument(7);
    actor = *current;
    actor->dialogHeight = (u8)value;

    value = FieldScriptVMGetArgument(9);
    actor = *current;
    u16 cursor = actor->scriptInstructionPointer;
    actor->dialogFlags = (u32)value;
    actor->scriptInstructionPointer = cursor + 11;
}

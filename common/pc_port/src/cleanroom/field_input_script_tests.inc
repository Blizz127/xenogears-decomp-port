static void field_script_update_input_cursor(int matched)
{
    ActorData *volatile *currentActor = &g_FieldScriptVMCurActor;
    if (matched) {
        volatile ActorData *actor = *currentActor;
        actor->scriptInstructionPointer += 5u;
    } else {
        u16 target = (u16)FieldScriptVMGetInstructionArgument(3);
        volatile ActorData *actor = *currentActor;
        actor->scriptInstructionPointer = target;
    }
}

void FieldScriptCheckControllerInputExact(u_short state)
{
    u16 required = (u16)FieldScriptVMGetInstructionArgument(1);
    field_script_update_input_cursor(state == required);
}

void FieldScriptVMCheckControllerInput(u_short state)
{
    u16 required = (u16)FieldScriptVMGetInstructionArgument(1);
    field_script_update_input_cursor((state & required) != 0);
}

void func_80096178(void)
{
    FieldScriptCheckControllerInputExact(*(const volatile u16 *)&D_800AFC6C);
}

void func_800961A0(void)
{
    FieldScriptVMCheckControllerInput(*(const volatile u16 *)&D_800AFE9C);
}

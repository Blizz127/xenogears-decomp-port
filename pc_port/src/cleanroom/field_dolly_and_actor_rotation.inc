static void field_set_dolly_byte(volatile unsigned char *setting)
{
    int value = FieldScriptVMGetArgument(1);
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    volatile ActorData *actor = *current;

    *setting = (unsigned char)value;
    actor->scriptInstructionPointer += 3;
}

void FieldScriptSetDollySet(void)
{
    field_set_dolly_byte(&g_Scene.dollySet);
}

void FieldScriptSetDollyStop(void)
{
    field_set_dolly_byte(&g_Scene.dollyStop);
}

void FieldSetCurrentActorRotation(int rotation)
{
    ActorData *volatile *current = &g_FieldScriptVMCurActor;
    volatile ActorData *actor;
    unsigned short cursor;
    short tagged = (int)((unsigned int)rotation & 0x7fffU) - 32768;

    if (D_800ADB1C == 0) {
        actor = *current;
        actor->rotation.vx = tagged;
        actor->rotation.vy = tagged;
        actor->rotation.vz = tagged;
    }

    actor = *current;
    cursor = actor->scriptInstructionPointer;
    actor->rotation.vx = tagged;
    actor->rotation.vy = tagged;
    actor->scriptInstructionPointer = (unsigned short)(cursor + 3);
}

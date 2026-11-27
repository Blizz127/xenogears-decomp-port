void func_80094918(void)
{
    const u8* script = g_FieldScriptVMCurScriptData;
    u8 component = script[g_FieldScriptVMCurActor->scriptInstructionPointer + 3];

    if (component <= 2) {
        s16 angle = (s16)FieldScriptVMGetArgument(1);
        SVEC* rotation = &g_FieldActors[D_800AFD1C].rotation;

        if (component == 0) {
            rotation->x = angle;
        } else if (component == 1) {
            rotation->y = angle;
        } else {
            rotation->z = angle;
        }
    }

    s32 actorIndex = D_800AFD1C;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    func_80072254(actorIndex);
}

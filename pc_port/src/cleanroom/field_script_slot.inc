void func_800A1A8C(void)
{
    int slotIndex;

    for (slotIndex = 0; slotIndex < ACTOR_MAX_NUM_SCRIPTS; ++slotIndex) {
        if (g_FieldScriptVMCurActor->scripts[slotIndex].flags_0x12 == 7) {
            u16 savedIP = FieldScriptGetBytecodeOffset(D_800AFD1C, 1);
            g_FieldScriptVMCurActor->scripts[slotIndex].currentIP = savedIP;
        }
    }

    slotIndex = g_FieldScriptVMCurActor->curScriptIndex;
    if (slotIndex < ACTOR_MAX_NUM_SCRIPTS) {
        g_FieldScriptVMCurActor->scripts[slotIndex].flags_0x12 = 15;
        g_FieldScriptVMCurActor->scripts[slotIndex].scriptId = 255;
    }
    D_800B00C0 = 1;
}

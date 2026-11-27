extern s32 D_800ADBDC, D_800ADBE4;

void func_80093C6C(void)
{
    if (*(volatile s32 *)&D_800ADBDC == 0 || *(volatile s32 *)&D_800ADBE4 == 0) {
        *(volatile s32 *)&D_800B00C0 = 1;
        return;
    }
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    *(volatile s16 *)&g_FieldControl.isRandomEncountersEnabled = -1;
    actor->scriptInstructionPointer += 1;
}

extern int FieldScriptVMGetArgument(int index);
extern int FieldScriptVMGetInstructionArgument(int index);

static inline volatile ActorData *fieldWorldControlActor(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

static void fieldScriptSetCompassHidden(u8 hidden)
{
    extern u8 FieldCompassHidden __asm__("D_800B21D1");
    volatile ActorData *actor = fieldWorldControlActor();
    *(volatile u8 *)&FieldCompassHidden = hidden;
    actor->scriptInstructionPointer += 1u;
}

void FieldScriptEnableCompass(void)
{
    fieldScriptSetCompassHidden(0);
}

void FieldScriptDisableCompass(void)
{
    fieldScriptSetCompassHidden(1);
}

void FieldScriptVMHandlerDisableRandomEncounters(void)
{
    volatile ActorData *actor = fieldWorldControlActor();
    *(volatile s16 *)&g_FieldControl.isRandomEncountersEnabled = 0;
    actor->scriptInstructionPointer += 1u;
}

void FieldScriptVMHandlerSetControllerBtnMask(void)
{
    int buttons = FieldScriptVMGetInstructionArgument(1);
    volatile ActorData *actor = fieldWorldControlActor();
    *(volatile s16 *)&g_FieldControl.controllerBtnMask = (s16)buttons;
    actor->scriptInstructionPointer += 3u;
}

void func_80095300(void)
{
    int angle = FieldScriptVMGetArgument(1);
    volatile ActorData *actor = fieldWorldControlActor();
    *(volatile s16 *)&g_FieldControl.unkAngle = (s16)angle;
    actor->scriptInstructionPointer += 3u;
}

void func_80093A68(void)
{
    volatile s32 *state = &g_Scene.unk48;
    u32 flags = (u32)*state;
    volatile ActorData *actor = fieldWorldControlActor();
    *state = (s32)(flags & 0x7fffu);
    actor->scriptInstructionPointer += 1u;
}

void func_80093A98(void)
{
    volatile s32 *state = &g_Scene.unk48;
    u32 flags = (u32)*state;
    volatile ActorData *actor = fieldWorldControlActor();
    *state = (s32)(flags | 0x8000u);
    actor->scriptInstructionPointer += 1u;
}

extern int FieldScriptVMGetArgument(int index);
extern void FieldScriptMemoryWriteU16(int address, int value);
extern s8 D_80059171;
extern s8 D_800B21D0[], D_800B21D1[];
extern s32 D_800ADB64, D_8004F350, D_8004F318, D_8004F328;

static volatile ActorData *fieldSceneControlActor(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

static void fieldScriptRequestTransition(int mode)
{
    int effect = FieldScriptVMGetArgument(1);
    volatile ActorData *actor = fieldSceneControlActor();
    *(volatile u8 *)&D_80059171 = (u8)effect;
    *(volatile s32 *)&D_800ADB64 = mode;
    *(volatile s32 *)&D_800B00C0 = 1;
    u32 generation = *(volatile u32 *)&D_8004F350;
    unsigned int ip = actor->scriptInstructionPointer;
    *(volatile u32 *)&D_8004F350 = generation + 1u;
    actor->scriptInstructionPointer = (u16)(ip + 3);
}

void func_80093824(void)
{
    fieldScriptRequestTransition(3);
}

void func_800939A0(void)
{
    fieldScriptRequestTransition(4);
}

void func_80093AC8(void)
{
    *(volatile s16 *)&g_FieldControl.isRandomEncountersEnabled = 0;
    *(volatile u8 *)D_800B21D0 = 0;
    *(volatile u8 *)D_800B21D1 = 0;
    u32 flags = (u32)*(volatile s32 *)&g_Scene.unk48;
    volatile ActorData *actor = fieldSceneControlActor();
    *(volatile s32 *)&g_Scene.unk48 = (s32)(flags & 0x3fffu);
    actor->scriptInstructionPointer += 1;
}

static void fieldScriptSetSceneModeByte(u8 value)
{
    volatile ActorData *actor = fieldSceneControlActor();
    *(volatile u8 *)D_800B21D0 = value;
    actor->scriptInstructionPointer += 1;
}

void func_80093BB0(void)
{
    fieldScriptSetSceneModeByte(0);
}

void func_80093BD4(void)
{
    fieldScriptSetSceneModeByte(1);
}

void func_800945D4(void)
{
    *(volatile s32 *)&D_8004F318 = 0;
    *(volatile s32 *)&D_8004F328 = 255;
    u32 high = (u8)FieldScriptVMGetArgument(1);
    u32 low = (u8)FieldScriptVMGetArgument(3);
    FieldScriptMemoryWriteU16(10, (int)((high << 8) | low));
    volatile ActorData *actor = fieldSceneControlActor();
    actor->scriptInstructionPointer += 5;
}

void func_80094650(void)
{
    volatile ActorData *actor = fieldSceneControlActor();
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned int source = actor->scriptInstructionPointer;
    unsigned int ip = actor->scriptInstructionPointer;
    int value = script[source + 1];
    actor->scriptInstructionPointer = (u16)(ip + 2);
    *(volatile s32 *)&D_8004F328 = value;
}

void func_8009468C(void)
{
    volatile ActorData *actor = fieldSceneControlActor();
    unsigned int ip = actor->scriptInstructionPointer;
    *(volatile s32 *)&D_8004F318 = 0;
    *(volatile s32 *)&D_8004F328 = 255;
    actor->scriptInstructionPointer = (u16)(ip + 1);
}

void func_80093A04(void)
{
    extern int FieldScriptVMGetArgument(int);
    int argument;
    ActorData *actor;
    u32 counter;
    u16 cursor;

    argument = FieldScriptVMGetArgument(1);
    actor = *(ActorData * volatile *)&g_FieldScriptVMCurActor;
    *(volatile unsigned char *)&D_80059171 = (unsigned char)argument;
    *(volatile s32 *)&D_800ADB64 = 5;
    *(volatile int *)&D_800B00C0 = 1;
    counter = *(volatile u32 *)&D_8004F350;
    cursor = *(volatile u16 *)&actor->scriptInstructionPointer;
    *(volatile u32 *)&D_8004F350 = counter + 1u;
    *(volatile u16 *)&actor->scriptInstructionPointer = (u16)(cursor + 3u);
}

extern int FieldGetCameraDirection(void);

s32 func_8009BE58(void)
{
    unsigned int camera = (u16)FieldGetCameraDirection();
    const volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile u32 *flags = (const volatile u32 *)((const volatile u8 *)actor + 0x12c);
    unsigned int direction = (*flags >> 9) & 7u;
    return ((direction - camera) & 7u) < 5u;
}

static void fieldDialogueForCurrentActor(int mode)
{
    func_8009C5A8(*(volatile s32 *)&D_800AFD1C, mode);
}

void func_8009C0B4(void)
{
    fieldDialogueForCurrentActor(0);
}

void func_8009C0DC(void)
{
    fieldDialogueForCurrentActor(1);
}

void func_8009C104(void)
{
    fieldDialogueForCurrentActor(2);
}

void func_8009C12C(void)
{
    fieldDialogueForCurrentActor(3);
}

void func_8009CCF8(int bit)
{
    volatile u16 *flags = D_800B2174;
    unsigned int mask = 1u << ((u32)bit & 31u);
    *flags = (u16)(*flags | mask);
}

void func_8009CE48(void)
{
    volatile ActorData *actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned int widthIp = actor->scriptInstructionPointer;
    unsigned int heightIp = actor->scriptInstructionPointer;
    actor->dialogPixelWidth = (short)(2u * script[widthIp + 1]);
    unsigned int columnsIp = actor->scriptInstructionPointer;
    actor->dialogPixelHeight = (short)script[heightIp + 2];
    actor->dialogWidth = (u8)(3u * script[columnsIp + 3]);
    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned int rowsIp = actor->scriptInstructionPointer;
    actor->dialogHeight = script[rowsIp + 4];
    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 5;
}

extern VECTOR g_CamAtMovementFrom, g_CamAtMovementTo, g_CamEyeMovementFrom, g_CamEyeMovementTo;
extern s16 D_800AF93A[];
extern int FieldScriptArgument1(int index, int mask);
extern int FieldScriptArgument2(int index, int mask);
extern int FieldScriptArgument3(int index, int mask);
/* Bind the mathematical SDK entries independently of the game's trig macros. */
extern int CameraPivotCosine(int angle) __asm__("rcos");
extern int CameraPivotSine(int angle) __asm__("rsin");
extern void CameraPivotWrite(int destination, int value) __asm__("FieldScriptMemoryWriteU16");

void func_80091318(void)
{
    const volatile VECTOR* const pivots[] = {
        &g_CamAtMovementFrom, &g_CamAtMovementTo, &g_CamEyeMovementFrom, &g_CamEyeMovementTo
    };
    unsigned int mode = SCRIPT_READ_U8_REL(1);
    /* Unsupported selectors have no defined portable pivot. */
    VECTOR pivot = {0};
    if (mode < 4) {
        const volatile VECTOR* source = pivots[mode];
        pivot.vx = source->vx;
        pivot.vy = source->vy;
        pivot.vz = source->vz;
    }

    int horizontal = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(8));
    int pitch = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(8));
    u32 distance = (u32)FieldScriptArgument3(6, SCRIPT_READ_U8_REL(8));
    s32 phase = 3072 + ((s32)(2912u * (u32)pitch) >> 8);
    volatile s16* scale = D_800AF93A;

    VECTOR work = {0};
    work.vx = pivot.vx;
    u32 vertical = ((u32)CameraPivotCosine(phase) * distance) << 5;
    s32 y = (s32)(0u - vertical) >> 16;
    work.vy = (s32)((u32)pivot.vy + (((u32)y * (u32)*scale) << 4));

    u32 forward = ((u32)CameraPivotSine(phase) * distance) << 5;
    s32 z = (s32)forward >> 16;
    work.vz = (s32)((u32)pivot.vz + (((u32)z * (u32)*scale) << 4));
    func_80091008(&work, &pivot, horizontal);

    int destination = (u16)FieldScriptVMGetInstructionArgument(9);
    CameraPivotWrite(destination, (s32)work.vx >> 16);
    destination = (u16)FieldScriptVMGetInstructionArgument(11);
    CameraPivotWrite(destination, (s32)work.vz >> 16);
    destination = (u16)FieldScriptVMGetInstructionArgument(13);
    CameraPivotWrite(destination, (s32)work.vy >> 16);

    volatile u16* cursor = &g_FieldScriptVMCurActor->scriptInstructionPointer;
    volatile int* budget = &g_FieldScriptMaxInstructionCount;
    u32 nextBudget = (u32)*budget + 1u;
    u16 nextCursor = *cursor + 15u;
    *budget = (s32)nextBudget;
    *cursor = nextCursor;
}

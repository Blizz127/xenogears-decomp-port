extern VECTOR g_CamAtMovementTo, g_CamEyeMovementTo;
extern s16 D_800AF93A[];
extern long CameraMetricsMagnitude3(long x, long y, long z) __asm__("FieldGetVec3Magnitude");
extern long CameraMetricsMagnitude2(long x, long z) __asm__("FieldGetVec2Magnitude");
extern void CameraMetricsWrite(int destination, int value) __asm__("FieldScriptMemoryWriteU16");

void func_80090E70(void)
{
    /* Snapshot the Q16 differences before any of the camera services run. */
    u32 dx = (u32)g_CamEyeMovementTo.vx - (u32)g_CamAtMovementTo.vx;
    u32 dy = (u32)g_CamEyeMovementTo.vy - (u32)g_CamAtMovementTo.vy;
    u32 dz = (u32)g_CamEyeMovementTo.vz - (u32)g_CamAtMovementTo.vz;
    s32 x = (s32)dx >> 16;
    s32 y = (s32)dy >> 16;
    s32 z = (s32)dz >> 16;
    s32 distance = (s32)CameraMetricsMagnitude3(x, y, z) / 2;

    D_800AF93A[0] = 4096;
    int yawAngle = ratan2((s32)(0u - dz), (s32)(0u - dx));
    int yaw = (0u - (u32)yawAngle - 1024u) & 0xfff;
    int horizontal = (s32)CameraMetricsMagnitude2(x, z);
    int pitchAngle = ratan2(horizontal, (s32)(0u - dy) >> 16);
    int pitch = ((s32)(0u - (u32)pitchAngle * 360u) >> 12) + 91;

    int destination = (u16)FieldScriptVMGetInstructionArgument(1);
    CameraMetricsWrite(destination, yaw);
    destination = (u16)FieldScriptVMGetInstructionArgument(3);
    CameraMetricsWrite(destination, pitch);
    destination = (u16)FieldScriptVMGetInstructionArgument(5);
    CameraMetricsWrite(destination, distance);

    /* Sample both before either store: the cursor can overlap the budget. */
    volatile u16* cursor = &g_FieldScriptVMCurActor->scriptInstructionPointer;
    volatile int* budget = &g_FieldScriptMaxInstructionCount;
    u32 nextBudget = (u32)*budget + 1u;
    u16 nextCursor = *cursor + 7u;
    *budget = (s32)nextBudget;
    *cursor = nextCursor;
}

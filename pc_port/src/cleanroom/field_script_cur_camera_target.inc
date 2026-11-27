#include <stdint.h>

static int32_t FieldScriptCameraTargetIntegerPart(int32_t coordinate)
{
    /* The signed high half rounds a negative fraction toward minus infinity. */
    return coordinate / 65536 - (coordinate % 65536 < 0);
}

void FieldScriptWriteCurCameraTarget(void)
{
    uint16_t destination;
    ActorData *actor;
    volatile uint16_t *cursor_address;
    volatile int32_t *budget_address;
    uint16_t cursor;
    int32_t budget;

    destination = (uint16_t)FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(destination, FieldScriptCameraTargetIntegerPart(g_CameraAt.vx));

    destination = (uint16_t)FieldScriptVMGetInstructionArgument(3);
    FieldScriptMemoryWriteU16(destination, FieldScriptCameraTargetIntegerPart(g_CameraAt.vz));

    destination = (uint16_t)FieldScriptVMGetInstructionArgument(5);
    FieldScriptMemoryWriteU16(destination, FieldScriptCameraTargetIntegerPart(g_CameraAt.vy));

    actor = g_FieldScriptVMCurActor;
    /* Preserve the VM's read-before-write order when the fields overlap. */
    cursor_address = &actor->scriptInstructionPointer;
    budget_address = &g_FieldScriptMaxInstructionCount;
    budget = *budget_address;
    cursor = *cursor_address;
    *budget_address = budget == INT32_MAX ? INT32_MIN : budget + 1;
    *cursor_address = (uint16_t)(cursor + 7u);
}

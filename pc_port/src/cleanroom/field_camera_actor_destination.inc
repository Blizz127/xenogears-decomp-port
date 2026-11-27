#include <stdint.h>

extern uint32_t fieldCameraResolveActor(int) __asm__("func_8009CD7C");

static void fieldCameraDestinationForActor(volatile VECTOR *destination)
{
    uint32_t index = fieldCameraResolveActor(1);
    FieldActor *base = g_FieldActors;
    uint32_t displacement = index * (uint32_t)sizeof(FieldActor);
    intptr_t signedDistance = (intptr_t)(displacement & INT32_MAX);
    if (displacement > INT32_MAX)
        signedDistance -= (intptr_t)INT32_MAX + 1;
    const volatile FieldActor *record =
        (const volatile FieldActor *)((uintptr_t)base + (uintptr_t)signedDistance);
    uintptr_t address = record->pActorData;
    const volatile ActorData *source = (const volatile ActorData *)address;
    volatile ActorData *scriptActor = fieldCameraActor();
    destination->vx = source->position.vx;
    destination->vy = source->position.vy;
    destination->vz = source->position.vz;
    int budget = *(volatile int *)&g_FieldScriptMaxInstructionCount;
    fieldCameraFinish(scriptActor, budget, 2);
}

void FieldScriptSetCameraTargetMovementDestToActor(void)
{
    fieldCameraDestinationForActor(&g_CamAtMovementTo);
}

void FieldScriptSetCameraPosMovementDestToActor(void)
{
    fieldCameraDestinationForActor(&g_CamEyeMovementTo);
}

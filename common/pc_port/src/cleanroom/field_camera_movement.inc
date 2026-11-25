#include "common.h"
#include "field/actor.h"
#include "field/camera.h"

extern VECTOR g_CamAtMovementTo;
extern VECTOR g_CamEyeMovementTo;
extern s16 g_CamAtMovementDuration;
extern s16 g_CamEyeMovementDuration;

extern int FieldScriptArgument1(int index, int mask);
extern int FieldScriptArgument2(int index, int mask);
extern int FieldScriptArgument3(int index, int mask);
extern int FieldScriptVMGetArgument(int index);

/* Interpret the low halfword as signed before scaling. Both endpoints and
 * the product fit in a 32-bit int, including -32768 * 65536. */
static int camera_fixed_coordinate(int argument)
{
    unsigned int bits = (unsigned int)argument;
    int coordinate = (int)(bits & 0x7fffu) - (int)(bits & 0x8000u);
    return coordinate * 65536;
}

static void camera_finish_instruction(unsigned int length)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    unsigned int *count = (unsigned int *)&g_FieldScriptMaxInstructionCount;

    *count += 1u;
    actor->scriptInstructionPointer = (u16)(actor->scriptInstructionPointer + length);
}

#define CAMERA_DESTINATION_TAG() \
    (((const u8 *)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 7])

static void camera_set_destination(VECTOR *destination)
{
    destination->vx = camera_fixed_coordinate(FieldScriptArgument1(1, CAMERA_DESTINATION_TAG()));
    destination->vz = camera_fixed_coordinate(FieldScriptArgument2(3, CAMERA_DESTINATION_TAG()));
    destination->vy = camera_fixed_coordinate(FieldScriptArgument3(5, CAMERA_DESTINATION_TAG()));
    camera_finish_instruction(8);
}
#undef CAMERA_DESTINATION_TAG

void FieldScriptSetCameraTargetMovementFrom(void)
{
    camera_set_destination(&g_CamAtMovementFrom);
}

void FieldScriptSetCameraPosMovementFrom(void)
{
    camera_set_destination(&g_CamEyeMovementFrom);
}

void FieldScriptSetCameraTargetMovementDest(void)
{
    camera_set_destination(&g_CamAtMovementTo);
}

void FieldScriptSetCameraPosMovementDest(void)
{
    camera_set_destination(&g_CamEyeMovementTo);
}

void FieldScriptSetCameraInterpolationStep(void)
{
    int target;
    int eye;

    g_CamInterpolation.atStepDistance = FieldScriptVMGetArgument(1);
    eye = FieldScriptVMGetArgument(3);
    target = g_CamInterpolation.atStepDistance;
    g_CamInterpolation.eyeStepDistance = eye;
    if (target == 0)
        g_CamInterpolation.atStepDistance = 1;
    if (eye == 0)
        g_CamInterpolation.eyeStepDistance = 1;
    camera_finish_instruction(5);
}

void FieldScriptWaitForCameraMovement(void)
{
    int mask = FieldScriptVMGetArgument(1);
    int active = ((mask & FIELD_CAMERA_AT_MOVEMENT_ACTIVE) && g_CamAtMovementDuration) ||
                 ((mask & FIELD_CAMERA_EYE_MOVEMENT_ACTIVE) && g_CamEyeMovementDuration);

    D_800B00C0 = 1;
    if (!active)
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

#include <stdint.h>

extern VECTOR g_CameraEye, g_CameraEye2, g_CameraAt2;
extern VECTOR g_CamEyeMovementFrom, g_CamEyeMovementTo;
extern VECTOR g_CamAtMovementFrom, g_CamAtMovementTo;
extern int FieldScriptVMGetInstructionArgument(int);
extern int FieldScriptArgument1(int, int);
extern void fieldCameraWriteVariable(int, int) __asm__("FieldScriptMemoryWriteU16");

/* Each script read observes the signed upper half of a live Q16 coordinate. */
typedef int16_t FieldCameraHalf __attribute__((__may_alias__));

static volatile ActorData *fieldCameraActor(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

typedef uint16_t FieldCameraCursor __attribute__((__may_alias__));

static volatile FieldCameraCursor *fieldCameraCursor(volatile ActorData *actor)
{
    return (volatile FieldCameraCursor *)((volatile unsigned char *)actor +
        __builtin_offsetof(ActorData, scriptInstructionPointer));
}

static void fieldCameraWriteComponent(int offset, const volatile void *component)
{
    int destination = (uint16_t)FieldScriptVMGetInstructionArgument(offset);
    const volatile uint8_t *bytes = component;
    int value = *(const volatile FieldCameraHalf *)(bytes + 2);
    fieldCameraWriteVariable(destination, value);
}

static void fieldCameraFinish(volatile ActorData *actor, int budget, unsigned length)
{
    uint16_t cursor = *fieldCameraCursor(actor);
    *(volatile int *)&g_FieldScriptMaxInstructionCount =
        budget == INT32_MAX ? INT32_MIN : budget + 1;
    *fieldCameraCursor(actor) = (uint16_t)(cursor + length);
}

static void fieldCameraWriteVector(volatile VECTOR *vector)
{
    fieldCameraWriteComponent(1, &vector->vx);
    fieldCameraWriteComponent(3, &vector->vz);
    fieldCameraWriteComponent(5, &vector->vy);
    volatile ActorData *actor = fieldCameraActor();
    int budget = *(volatile int *)&g_FieldScriptMaxInstructionCount;
    fieldCameraFinish(actor, budget, 7);
}

void FieldScriptWriteCurCameraPosition(void)
{
    fieldCameraWriteVector(&g_CameraEye);
}

void FieldScriptWriteCameraTweenTarget(void)
{
    fieldCameraWriteVector(&g_CameraAt2);
}

void FieldScriptWriteCameraTweenPosition(void)
{
    fieldCameraWriteVector(&g_CameraEye2);
}

static void fieldCameraResetFrom(const volatile VECTOR *source, volatile VECTOR *from)
{
    int z = source->vz;
    int x = source->vx;
    int y = source->vy;
    from->vz = z;
    volatile ActorData *actor = fieldCameraActor();
    from->vx = x;
    int budget = *(volatile int *)&g_FieldScriptMaxInstructionCount;
    from->vy = y;
    fieldCameraFinish(actor, budget, 1);
}

void FieldScriptResetCameraTargetMovement(void)
{
    fieldCameraResetFrom(&g_CameraAt2, &g_CamAtMovementFrom);
}

void FieldScriptResetCameraPosMovement(void)
{
    fieldCameraResetFrom(&g_CameraEye2, &g_CamEyeMovementFrom);
}

void FieldScriptResetCameraMovements(void)
{
    const volatile VECTOR *at = &g_CameraAt2;
    const volatile VECTOR *eye = &g_CameraEye2;
    volatile VECTOR *atFrom = &g_CamAtMovementFrom;
    volatile VECTOR *atTo = &g_CamAtMovementTo;
    volatile VECTOR *eyeFrom = &g_CamEyeMovementFrom;
    volatile VECTOR *eyeTo = &g_CamEyeMovementTo;

    int atZ = at->vz;
    int atX = at->vx;
    int atY = at->vy;
    int eyeX = eye->vx;
    int eyeY = eye->vy;
    int eyeZ = eye->vz;
    atFrom->vz = atZ;
    atTo->vz = atZ;
    volatile ActorData *actor = fieldCameraActor();
    atFrom->vx = atX;
    atTo->vx = atX;
    int budget = *(volatile int *)&g_FieldScriptMaxInstructionCount;
    atFrom->vy = atY;
    atTo->vy = atY;
    eyeFrom->vx = eyeX;
    eyeFrom->vy = eyeY;
    eyeFrom->vz = eyeZ;
    eyeTo->vx = eyeX;
    eyeTo->vy = eyeY;
    eyeTo->vz = eyeZ;
    fieldCameraFinish(actor, budget, 1);
}

void func_8008FABC(void)
{
    volatile ActorData *actor = fieldCameraActor();
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned cursor = *fieldCameraCursor(actor);
    int value = FieldScriptArgument1(1, script[cursor + 3u]);
    int16_t yaw = (int16_t)(uint16_t)value;
    volatile FieldScene *scene = &g_Scene;
    scene->sceneAngle.vy = yaw;
    actor = fieldCameraActor();
    scene->unk7C = (uint32_t)(int32_t)yaw;
    *fieldCameraCursor(actor) += 4;
}

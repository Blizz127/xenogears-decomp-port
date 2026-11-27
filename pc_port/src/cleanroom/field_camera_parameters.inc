/* Script selector order is shared by both camera parameter operations. */
static volatile VECTOR *const fieldCameraParameters[] = {
    &g_CamAtMovementFrom, &g_CamAtMovementTo,
    &g_CamEyeMovementFrom, &g_CamEyeMovementTo
};

static unsigned fieldCameraSelector(unsigned offset)
{
    volatile ActorData *actor = fieldCameraActor();
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned cursor = *fieldCameraCursor(actor);
    return script[cursor + offset];
}

typedef struct {
    int x, y, z;
} FieldCameraPoint;

static FieldCameraPoint fieldCameraReadParameter(void)
{
    unsigned source = fieldCameraSelector(1);
    FieldCameraPoint point = {0, 0, 0};
    /* Invalid script sources expose stale stack words on Disc 1. Supply zero
       coordinates on the native host and complete the command normally. */
    if (source < 4) {
        const volatile VECTOR *v = fieldCameraParameters[source];
        point.x = v->vx;
        point.y = v->vy;
        point.z = v->vz;
    }
    return point;
}

static void fieldCameraAdvanceParameter(unsigned length)
{
    volatile ActorData *actor = fieldCameraActor();
    int budget = *(volatile int *)&g_FieldScriptMaxInstructionCount;
    fieldCameraFinish(actor, budget, length);
}

void FieldScriptSetCameraMovementParameter(void)
{
    FieldCameraPoint point = fieldCameraReadParameter();
    unsigned target = fieldCameraSelector(2);
    if (target < 4) {
        volatile VECTOR *v = fieldCameraParameters[target];
        v->vx = point.x;
        v->vy = point.y;
        v->vz = point.z;
    }
    fieldCameraAdvanceParameter(3);
}

static void fieldCameraWriteParameterWord(int offset, int coordinate)
{
    int destination = (uint16_t)FieldScriptVMGetInstructionArgument(offset);
    int upper = (int)((uint32_t)coordinate >> 16);
    if (upper >= 0x8000)
        upper -= 0x10000;
    fieldCameraWriteVariable(destination, upper);
}

void FieldScriptWriteCameraMovementParameter(void)
{
    FieldCameraPoint point = fieldCameraReadParameter();
    fieldCameraWriteParameterWord(2, point.x);
    fieldCameraWriteParameterWord(4, point.z);
    fieldCameraWriteParameterWord(6, point.y);
    fieldCameraAdvanceParameter(8);
}

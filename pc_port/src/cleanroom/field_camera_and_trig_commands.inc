int FieldGetCameraDirection(void) {
    u32 sector = ((u32)(s32)g_CamInterpolation.targetAngleY - 256u) >> 9;
    return (int)((7u - sector) & 7u);
}

void FieldScriptWriteCameraDirection(void) {
    u16 destination = (u16)FieldScriptVMGetInstructionArgument(1);
    u16 direction = (u16)FieldGetCameraDirection();
    FieldScriptMemoryWriteU16(destination, direction);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

static unsigned field_trig_argument_mask(void) {
    const u8 *script = g_FieldScriptVMCurScriptData;
    unsigned cursor = g_FieldScriptVMCurActor->scriptInstructionPointer;
    return script[cursor + 7u];
}

enum FieldTrigOperation { FIELD_TRIG_COSINE, FIELD_TRIG_SINE, FIELD_TRIG_ATAN2 };

static void field_script_trig(enum FieldTrigOperation operation) {
    u16 destination = (u16)FieldScriptVMGetInstructionArgument(1);
    int first = FieldScriptArgument2(3, field_trig_argument_mask());
    int second = FieldScriptArgument3(5, field_trig_argument_mask());
    s32 value;
    if (operation == FIELD_TRIG_ATAN2) {
        value = (s16)(u16)ratan2(first, second);
    } else {
        u32 factor = (u32)(operation == FIELD_TRIG_COSINE ? rcos(first) : rsin(first));
        u32 product = factor * (u32)second;
        value = (s32)(product >> 12);
        if (product & 0x80000000u)
            value -= 0x100000;
    }
    FieldScriptMemoryWriteU16(destination, value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptCos(void) { field_script_trig(FIELD_TRIG_COSINE); }
void FieldScriptSin(void) { field_script_trig(FIELD_TRIG_SINE); }
void FieldScriptAtan2(void) { field_script_trig(FIELD_TRIG_ATAN2); }

extern u16 g_FieldAngleToDirectionLUT[];
extern int FieldScriptGetCurActorDirection(void);
extern void FieldSetCurrentActorRotation(int);

static u16 field_script_direction_entry(u32 index) {
    intptr_t offset = (s32)(index * 2u);
    return *(const u16 *)((uintptr_t)g_FieldAngleToDirectionLUT + offset);
}

void FieldScriptSetActorRotation(int angle) {
    if (FieldScriptVMGetActorIndex(1) != 255u) {
        u32 id = FieldScriptVMGetActorIndex(1);
        intptr_t offset = (s32)(id * (u32)sizeof(FieldActor));
        FieldActor *record = (FieldActor *)((uintptr_t)g_FieldActors + offset);
        volatile ActorData *target = (ActorData *)(uintptr_t)record->pActorData;
        s16 value = (s16)((u16)angle | 0x8000u);
        if (D_800ADB1C == 0) {
            target->rotation.vx = value;
            target->rotation.vy = value;
            target->rotation.vz = value;
        }
        target->rotation.vx = value;
        target->rotation.vy = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_8009AB08(int angle) {
    u32 relative = (u32)angle - (u16)g_CamInterpolation.targetAngleY;
    s16 value = (s16)((relative & 0xfffu) | 0x8000u);
    ActorData *actor = g_FieldScriptVMCurActor;
    int flat = D_800ADB1C == 0;
    actor->rotation.vx = value;
    actor->rotation.vy = value;
    if (flat)
        actor->rotation.vz = value;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptRotateActorClockwise(void) {
    u32 step = (u32)FieldScriptVMGetArgument(1);
    u32 current = (u32)FieldScriptGetCurActorDirection();
    FieldSetCurrentActorRotation(field_script_direction_entry((current + step) & 7u));
}

void FieldScriptRotateActorCounterClockwise(void) {
    u32 step = (u32)FieldScriptVMGetArgument(1);
    u32 current = (u32)FieldScriptGetCurActorDirection();
    FieldSetCurrentActorRotation(field_script_direction_entry((current - step) & 7u));
}

void FieldScriptSetActorDirection(void) {
    u32 index = (u32)FieldScriptVMGetArgument(2);
    FieldScriptSetActorRotation(field_script_direction_entry(index));
}

void FieldScriptVMHandlerSetCurActorRotation(void) {
    u32 index = (u32)FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(field_script_direction_entry(index));
}

void func_8009ACB4(void) {
    u32 index = (u32)FieldScriptVMGetArgument(1);
    func_8009AB08(field_script_direction_entry(index));
}

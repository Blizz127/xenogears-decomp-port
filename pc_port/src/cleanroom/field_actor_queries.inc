static int field_actor_octant(s16 yaw) {
    u32 rounded = (u32)((s32)yaw + 256);
    return ((rounded >> 9) + 2u) & 7u;
}

int FieldScriptGetCurActorDirection(void) {
    return field_actor_octant(g_FieldScriptVMCurActor->rotation.vy);
}

void FieldScriptVMWriteCurCharacterID(void) {
    u16 address = (u16)FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(address, g_FieldScriptVMCurActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerGetActorDirection(void) {
    u16 address = (u16)FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(address, field_actor_octant(g_FieldScriptVMCurActor->rotation.vy));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerGetActorPosition(void) {
    u32 id = FieldScriptVMGetActorIndex(1);
    if (id != 255u) {
        static const unsigned char axes[] = {0, 2, 1};
        intptr_t offset = (s32)(id * (u32)sizeof(FieldActor));
        for (int i = 0; i < 3; ++i) {
            u16 address = (u16)FieldScriptVMGetInstructionArgument(2 + i * 2);
            FieldActor *record = (FieldActor *)((uintptr_t)g_FieldActors + offset);
            FieldScriptMemoryWriteU16(address, record->childMatrix.t[axes[i]]);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptVMHandlerPlayAnimation(void) {
    volatile ActorData *actor = g_FieldScriptVMCurActor;
    const volatile u8 *instruction = g_FieldScriptVMCurScriptData + actor->scriptInstructionPointer;
    actor->flags &= ~(1u << 24);
    u16 cursor = actor->scriptInstructionPointer;
    u8 animation = instruction[1];
    actor->scriptInstructionPointer = (u16)(cursor + 2u);
    actor->unkAnimationId = animation;
}

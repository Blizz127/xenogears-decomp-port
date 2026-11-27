static FieldActor *FieldLinearActor(void) {
    uintptr_t base = (uintptr_t)g_FieldActors;
    u32 offset = (u32)D_800AFD1C * (u32)sizeof(FieldActor);
    if (offset <= 0x7fffffffu)
        return (FieldActor *)(base + offset);
    return (FieldActor *)(base - (0u - offset));
}

static u8 FieldLinearScriptByte(unsigned offset) {
    const u8 *script = g_FieldScriptVMCurScriptData;
    return script[(unsigned)g_FieldScriptVMCurActor->scriptInstructionPointer + offset];
}

static s32 FieldLinearFixed(int argument) {
    return (s32)((u32)argument << 16);
}

static s32 FieldLinearDifference(s32 target, s32 position) {
    return (s32)((u32)target - (u32)position);
}

static u8 *FieldLinearSlot(ActorData *actor) {
    return (u8 *)actor + __builtin_offsetof(ActorData, scripts)
         + (unsigned)actor->curScriptIndex * sizeof(ActorScriptSlot) + 4;
}

static u16 FieldLinearReadSlot(ActorData *actor) {
    u16 value;
    __builtin_memcpy(&value, FieldLinearSlot(actor), sizeof(value));
    return value;
}

static void FieldLinearSetSlot(ActorData *actor, u16 value) {
    __builtin_memcpy(FieldLinearSlot(actor), &value, sizeof(value));
}

void func_80098CAC(s32 distanceMode) {
    FieldActor *record = FieldLinearActor();
    const ActorData *fieldData = (const ActorData *)(uintptr_t)record->pActorData;
    SpriteData *sprite = (SpriteData *)(uintptr_t)record->pSpriteData;
    ActorData *actor = g_FieldScriptVMCurActor;
    s32 scale = (fieldData->flags & 0x2000u) ? 0x08000000 : 0x04000000;
    s32 speed = MIPS_DIV(scale, actor->moveSpeed) >> 16;
    s16 animation = 1;
    if (speed == 0)
        speed = 1;

    actor->scriptFlags.flags |= 0x10000u;
    if (FieldLinearScriptByte(1) == 0) {
        s32 x = FieldLinearFixed(FieldScriptArgument1(2, FieldLinearScriptByte(8)));
        s32 z = FieldLinearFixed(FieldScriptArgument2(4, FieldLinearScriptByte(8)));
        s32 y = FieldLinearFixed(FieldScriptArgument3(6, FieldLinearScriptByte(8)));
        actor = g_FieldScriptVMCurActor;
        s32 distance = (s32)FieldGetVec3Magnitude(
            FieldLinearDifference(x, actor->position.vx) >> 16,
            FieldLinearDifference(y, actor->position.vy) >> 16,
            FieldLinearDifference(z, actor->position.vz) >> 16);
        actor = g_FieldScriptVMCurActor;
        s32 frames = MIPS_DIV(distance, speed);
        actor->unk102 = (s16)frames;
        if (actor->unk102 == 0)
            actor->unk102 = (s16)((u32)frames + 1u);
        actor->unkD0.vy = MIPS_DIV(FieldLinearDifference(y, actor->position.vy), actor->unk102);
        actor->unkD0.vx = MIPS_DIV(FieldLinearDifference(x, actor->position.vx), actor->unk102);
        actor->unkD0.vz = MIPS_DIV(FieldLinearDifference(z, actor->position.vz), actor->unk102);
        if ((x >> 16) != (actor->position.vx >> 16) ||
            (z >> 16) != (actor->position.vz >> 16)) {
            s32 angle = (s32)ratan2(actor->unkD0.vz >> 16, actor->unkD0.vx >> 16);
            actor = g_FieldScriptVMCurActor;
            actor->rotation.vx = (s16)(0u - (u32)angle);
            actor->rotation.vy = actor->rotation.vx;
        }
        actor->scriptInstructionPointer += 9;
    } else {
        if (actor->unk102 <= 0 || FieldLinearReadSlot(actor) == 0) {
            actor->scriptInstructionPointer -= 9;
            if (FieldLinearReadSlot(actor) != 0) {
                s32 oldX = actor->position.vx;
                s32 oldY = actor->position.vy;
                s32 oldZ = actor->position.vz;
                s32 x = FieldLinearFixed(FieldScriptArgument1(2, FieldLinearScriptByte(8)));
                g_FieldScriptVMCurActor->position.vx = x;
                s32 z = FieldLinearFixed(FieldScriptArgument2(4, FieldLinearScriptByte(8)));
                g_FieldScriptVMCurActor->position.vz = z;
                s32 y = FieldLinearFixed(FieldScriptArgument3(6, FieldLinearScriptByte(8)));
                actor = g_FieldScriptVMCurActor;
                actor->position.vy = y;
                actor->moveModified.vx = FieldLinearDifference(actor->position.vx, oldX);
                actor->moveModified.vy = FieldLinearDifference(actor->position.vy, oldY);
                actor->moveModified.vz = FieldLinearDifference(actor->position.vz, oldZ);
            }
            animation = actor->defaultAnimationId;
            actor->scriptInstructionPointer += distanceMode == 0 ? 11 : 13;
            FieldLinearSetSlot(actor, 0xffffu);
        } else {
            actor->position.vx = (s32)((u32)actor->position.vx + (u32)actor->unkD0.vx);
            actor->position.vz = (s32)((u32)actor->position.vz + (u32)actor->unkD0.vz);
            actor->moveModified.vy = actor->unkD0.vy;
            actor->moveModified.vz = actor->unkD0.vz;
            actor->position.vy = (s32)((u32)actor->position.vy + (u32)actor->unkD0.vy);
            actor->moveModified.vx = actor->unkD0.vx;
            D_800B00C0 = 1;
            FieldLinearSetSlot(actor, (u16)(FieldLinearReadSlot(actor) - 1u));
        }
        actor->unk102 = (s16)((u16)actor->unk102 - 1u);
        FieldLinearActor()->transformMatrix.t[0] = actor->position.vx >> 16;
        FieldLinearActor()->transformMatrix.t[1] = actor->position.vy >> 16;
        FieldLinearActor()->transformMatrix.t[2] = actor->position.vz >> 16;
        sprite->position.x = actor->position.vx;
        sprite->position.y = actor->position.vy;
        sprite->position.z = actor->position.vz;
    }

    actor = g_FieldScriptVMCurActor;
    if (actor->unkAnimationId != 255)
        animation = actor->unkAnimationId;
    if (actor->curAnimationId != animation && (actor->scriptFlags.flags & 0x02000000u) == 0) {
        actor->curAnimationId = animation;
        func_800821F4(sprite, animation, D_800B06B8);
    }
    func_80081F80(sprite, g_FieldScriptVMCurActor->rotation.vx, D_800B06B8);
}

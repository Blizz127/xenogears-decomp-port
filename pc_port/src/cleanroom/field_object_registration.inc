void func_800A1364(void) {
    volatile FieldActor *record = FieldActorControlRecord((u32)D_800AFD1C);
    record->status = (s16)(((u16)record->status & 0xf07fu) | 0x0200u);

    int sprite = FieldScriptVMGetArgument(1);
    const u8 *resource = g_FieldSpriteData;
    s32 index = D_800AFD1C;
    s32 offset;
    __builtin_memcpy(&offset, resource + 4, sizeof(offset));
    void *image = (void *)((uintptr_t)resource + (intptr_t)offset);
    func_80076AC0(index, 0, image, 0, 0, 128, 1);
    func_800A0C94();

    volatile ActorData *actor = g_FieldScriptVMCurActor;
    actor->scriptInstructionPointer += 3;
    u32 script_flags = actor->scriptFlags.flags;
    index = D_800AFD1C;
    actor->scriptFlags.flags = script_flags | 0x100u;
    record = FieldActorControlRecord((u32)index);
    record->status = (s16)((u16)record->status & ~0x20u);
    actor->flags = (actor->flags | 0x2000u) & ~0x800u;

    volatile s32 *count = &D_800B2264;
    intptr_t slot = (s32)((u32)*count * 2u);
    volatile u16 *sprite_slot = (volatile u16 *)((uintptr_t)D_800B21DC + slot);
    *sprite_slot = (u16)((u32)sprite * 2u);
    volatile u8 *flag_slot = (volatile u8 *)((uintptr_t)D_800B225F + (intptr_t)*count);
    *flag_slot = 0;
    u32 active = (u32)*count & 7u;
    actor = *(ActorData *volatile *)&g_FieldScriptVMCurActor;
    u32 packed;
    void *binding = (u8 *)(uintptr_t)actor + 0x12c;
    __builtin_memcpy(&packed, binding, sizeof(packed));
    packed = (packed & ~(7u << 13)) | (active << 13);
    __builtin_memcpy(binding, &packed, sizeof(packed));
    *count = (s32)((u32)*count + 1u);
}

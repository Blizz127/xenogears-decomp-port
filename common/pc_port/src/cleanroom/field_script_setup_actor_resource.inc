void func_800A0D3C(void)
{
    typedef s32 ResourceOffset __attribute__((__may_alias__, __aligned__(1)));
    u8* resource = *(void* volatile*)&g_FieldSpriteData;
    s32 index = *(volatile s32*)&D_800AFD1C;
    s32 offset = *(const volatile ResourceOffset*)(resource + 4);
    uintptr_t address = (uintptr_t)resource + (uintptr_t)(intptr_t)offset;

    func_80076AC0(index, 0, (void*)address, 0, 0, 0x80, 1);
    func_800A0C94();

    volatile ActorData* actor = *(ActorData* volatile*)&g_FieldScriptVMCurActor;
    u16 cursor = actor->scriptInstructionPointer;
    u32 scriptFlags = actor->scriptFlags.flags;
    u32 flags = actor->flags;
    actor->scriptFlags.flags = scriptFlags | 0x100u;
    actor->flags = flags | 0x800u;
    actor->scriptInstructionPointer = (u16)(cursor + 1);
}

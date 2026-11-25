void func_80039748(int address, u_char value)
{
    SoundSpuMemoryBlock *const block = SoundSpuMemoryFindBlock(address);

    if (block) {
        ((volatile SoundSpuMemoryBlock *)block)->unk1 = value;
    }
}

void func_80039D2C(s32 enabled)
{
    volatile u16 *const flags = (volatile u16 *)&g_SoundControlFlags;

    if (enabled) {
        *flags = (u16)(*flags | 0x0800u);
    } else {
        func_80039FF8();
        *flags = (u16)(*flags & 0xf7ffu);
    }
}

void func_80039E18(s32 event)
{
    if (*(const volatile u16 *)&g_SoundControlFlags & 0x0800u) {
        *(volatile int *)&D_80059404 = 2;
        func_8003B644(0x600c, event, 0x6000, 0x4000);
    }
}

void func_80039EC4(s32 event, s32 channel)
{
    if (*(const volatile u16 *)&g_SoundControlFlags & 0x0800u) {
        const u32 command = 0x2000u | (((u32)channel & 0xfeu) ^ 8u);

        *(volatile int *)&D_80059404 = 2;
        func_8003B644(command, event, 0x6000, 0x4000);
    }
}

void func_8003AFA0(AudioManager *manager)
{
    typedef u32 AliasWord __attribute__((__may_alias__));
    u32 remaining = ((const volatile AudioManager *)manager)->elementCount;
    uintptr_t address = (uintptr_t)manager + __builtin_offsetof(AudioManager, elements);

    while (remaining) {
        volatile AudioElement *const element = (volatile AudioElement *)address;
        const u32 firstWord = *(const volatile AliasWord *)address;

        if ((firstWord & 0x101u) == 0x101u && !(element->active_flag & 0x0030u)) {
            const u16 status = element->status_flags;

            element->status_flags = (u16)(status | 1u);
        }
        address += sizeof(AudioElement);
        --remaining;
    }
}

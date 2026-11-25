void func_80037F44(void)
{
    volatile u16 *const flags = (volatile u16 *)&g_SoundControlFlags;
    const u16 previous = *flags;

    if (!(previous & 1u)) {
        const u32 event = *(const volatile u32 *)&g_unk_SoundEvent;

        *flags = (u16)(previous | 1u);
        EnableEvent((long)event);
    }
}

void func_80037F88(void)
{
    volatile u16 *const flags = (volatile u16 *)&g_SoundControlFlags;

    if (*flags & 1u) {
        const u32 event = *(const volatile u32 *)&g_unk_SoundEvent;

        DisableEvent((long)event);
        *flags = (u16)(*flags & 0xfffeu);
    }
}

void func_80038624(void)
{
    SoundFile *volatile *const head = &g_SoundSedsLinkedList;

    func_80039FF8();
    *head = NULL;
}

void func_8003869C(void)
{
    func_80039CC4();
    func_80039FF8();
}

int func_80038824(void)
{
    const u16 flags = *(const volatile u16 *)&g_SoundControlFlags;

    if (flags & 0x0600u) {
        return 2;
    }
    return (flags & 0x0100u) != 0;
}

void func_800388D4(s32 enabled)
{
    volatile u16 *const flags = (volatile u16 *)&g_SoundControlFlags;
    const u16 previous = *flags;

    *flags = enabled ? (u16)(previous | 0x1000u) : (u16)(previous & 0xefffu);
}

void func_8003890C(AudioManager *manager, s32 enabled)
{
    volatile u16 *const flags = (volatile u16 *)&manager->unk_Flags;
    const u16 previous = *flags;

    *flags = enabled ? (u16)(previous & 0xfffeu) : (u16)(previous | 1u);
}

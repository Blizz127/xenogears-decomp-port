void func_80039C4C(AudioManager *manager)
{
    if (!manager || (uintptr_t)manager >= 0x0000800000000000ull) {
        return;
    }

    volatile u16 *const flags = (volatile u16 *)&manager->unk_Flags;
    *flags = (u16)(*flags & 0x7fffu);
    SoundReleaseAllVoices(manager);
}

void func_80039C8C(AudioManager *manager, s32 steps)
{
    if (!manager || (uintptr_t)manager >= 0x0000800000000000ull) {
        return;
    }
    func_8003A89C(manager, 0, steps);
}

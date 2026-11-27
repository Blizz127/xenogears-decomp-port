extern void SoundInitializeAudioManager(AudioManager *manager);

void func_8003B32C(AudioManager *manager)
{
    manager->unk_Flags = 2;
    manager->unk2[0] = -1;
    manager->unk2[1] = 127;
    manager->elementCount = (u8)D_80059478;
    manager->unk_0x15[1] = 0;
    manager->unk_0x15[2] = 0;
    manager->unk_0x18 = 127;
    manager->unk_0x19 = 0;

    SoundInitializeAudioManager(manager);
}

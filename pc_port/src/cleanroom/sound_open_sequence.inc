/* SPDX-License-Identifier: MIT */
void *func_800397FC(s32 fileAddress, s32 level, s32 steps)
{
    if (!PcPort_AdmitSoundSequence())
        return NULL;

    SoundFile *file = (SoundFile *)(intptr_t)fileAddress;
    AudioManager *manager = func_80039850(file);
    func_80039A80(manager, level, steps);
    return manager;
}

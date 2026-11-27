/* SPDX-License-Identifier: MIT */
void func_80039F9C(s32 soundId, s32 slot, s32 volume, s32 pan)
{
    if (!(g_SoundControlFlags & 0x0800u) || !PcPort_AdmitSoundSequence())
        return;

    const s32 group = 0x2000 | (((u32)slot ^ 8u) & 0xfeu);
    D_80059404 = 2;
    func_8003B644(group, soundId, (s32)(s8)volume * 256, (s32)(s8)pan * 256);
}

void func_80039F18(s32 soundId, s32 volume, s32 pan)
{
    if (!(g_SoundControlFlags & 0x0800u) || !PcPort_AdmitSoundSequence())
        return;

    const u16 selection = (u16)func_8003A65C(soundId, 2);
    D_80059404 = 2;
    func_8003B644((s16)(selection | 0x2000u), soundId,
                  (s32)(s8)volume * 256, (s32)(s8)pan * 256);
}

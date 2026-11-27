/* SPDX-License-Identifier: MIT */
void func_80039E60(s32 sound_id)
{
    if (!(g_SoundControlFlags & 0x0800u))
        return;

    if (!PcPort_AdmitSoundSequence())
        return;

    const uint16_t voice_group = (uint16_t)func_8003A65C(sound_id, 2);
    D_80059404 = 2;
    const s32 command = (int16_t)(voice_group | 0x2000u);
    func_8003B644(command, sound_id, 0x6000, 0x4000);
}

#include <string.h>

static void soundArmEffectElement(AudioElement *element, SoundFile *file,
                                  u16 scriptOffset, SoundWDSEntry *bank,
                                  u16 gain, u16 pan)
{
    u8 *record = (u8 *)element;
    u32 script = SOUND_PTR_TO_PSX((u8 *)file + scriptOffset);
    element->active_flag = (u16)(0x409u | ((file->unk10 & 1u) << 1));
    element->status_flags = 0x170;
    element->unk_0x04 = 0;
    memcpy(record + 0x10, &script, sizeof(script));
    element->unk14 = script;
    memset(record + 0x18, 0, 11);
    record[0x25] = (u8)file->sndId;
    element->unk2C = SOUND_PTR_TO_PSX(bank);
    memset(element->voice_data.unkC, 0, 4);
    element->fermataDuration = 0;
    record[0x60] = 0;
    element->unk_0x62 = 15;
    record[0x64] = 0;
    element->octave = 60;
    element->unk_0x6E = 0;
    memset(element->unk_0x70, 0, 2);
    memset(element->unk_0x70 + 2, 0xff, 2);
    element->unk_0x74 = pan;
    memcpy(element->unk_0x76, &gain, sizeof(gain));
    memset(record + 0x78, 0, 3);
    record[0x7b] = 127;
    memset(record + 0xce, 0, 8);
    element->unk_0xF6 = 0;
    element->unk_0x116 = 0;
    element->unk_0x136 = 0;
    memset(record + 0x156, 0, 2);

    if (bank)
        func_8003E5BC(0, element);
    element->voice_data.modeFlags = 0;
    element->voice_data.priority = 0x200;
    SoundAssignVoiceToChannelAndStop(&element->voice_data, element->voice_number);
}

void func_8003B644(s32 selection, s32 packedId, s32 volume, s32 pan)
{
    if (!PcPort_AdmitSoundSequence())
        return;
    s32 bankId = (s16)((u32)packedId >> 16);
    SoundFile *file = g_SoundSedsLinkedList;
    while (file && file->sedId != bankId)
        file = SOUND_PSX_TO_PTR(SoundFile, file->pNext);
    if (!file)
        return;

    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    SoundWDSEntry *bank = SoundFindWdsEntry(file->sndId);
    if (!bank)
        bank = g_SoundWdsLinkedList;

    u16 id = (u16)packedId;
    u8 weight = ((u8 *)file)[(u16)file->unk18 + id];
    u16 gain = (u16)(((u32)(s32)(s16)volume * weight) >> 7);
    if (gain > 0x7fffu)
        gain = 0x7fffu;
    const u16 *scripts = SOUND_FILE_SCRIPT_OFFSETS(file) + 2u * id;

    s32 count = D_80059404;
    DisableEvent(g_unk_SoundEvent);
    for (s32 i = 0; i < count; ++i) {
        AudioElement *element = &manager->elements[(u8)selection + i];
        u8 *record = (u8 *)element;
        record[7] = (u8)((u32)selection >> 8);
        memcpy(record + 8, &packedId, sizeof(packedId));
        memcpy(record + 12, &D_80059504, sizeof(D_80059504));
        u32 channelBit = 1u << ((u8)element->unk_0x06[0] & 31u);
        if (!scripts[i]) {
            manager->unk_0x48 &= (s32)~channelBit;
            element->active_flag = 0;
            SoundReleaseVoiceFromChannel(&element->voice_data, element->voice_number);
        } else {
            manager->unk_0x48 |= (s32)channelBit;
            soundArmEffectElement(element, file, scripts[i], bank, gain, (u16)pan);
        }
    }
    manager->unk_Flags = (s16)((u16)manager->unk_Flags | 0x8000u);
    EnableEvent(g_unk_SoundEvent);
}

#include <string.h>

static s16 soundPostpassHalf(const AudioElement *element, unsigned offset)
{
    s16 value;
    memcpy(&value, (const u8 *)element + offset, sizeof(value));
    return value;
}

static void soundUpdateVoiceGain(const AudioManager *manager, AudioElement *element)
{
    s32 envelope = soundPostpassHalf(element, 0x7a);
    s32 depth = soundPostpassHalf(element, 0xd2);
    envelope -= (envelope * depth) >> 15;
    if (envelope < 0)
        envelope = 0;
    if (envelope > 32767)
        envelope = 32767;
    const s32 elementGain = (envelope * soundPostpassHalf(element, 0x76)) >> 15;
    s32 master = (s16)((u32)manager->unk_Interpolator_0x70.currentValue >> 16);
    s32 gain = (elementGain * master) >> 16;

    s32 pan = 0x4000;
    if ((u16)g_SoundControlFlags & 0x100u) {
        pan = (s16)((u32)manager->unk_Interpolator_0x88.currentValue >> 16);
        pan += (s16)element->unk_0x74 + soundPostpassHalf(element, 0xd4);
        if (pan < 0)
            pan = 0;
        if (pan > 0x7f00)
            pan = 0x7f00;
    }
    const s32 distance = pan <= 0x4000 ? pan : 0x8000 - pan;
    const s32 nearGain = 0x7f00 - ((37 * distance) >> 6);
    const s32 farGain = (90 * distance) >> 6;
    s16 nearVolume = (s16)((gain * nearGain) >> 15);
    s16 farVolume = (s16)((gain * farGain) >> 15);
    element->voice_data.volume.left = pan <= 0x4000 ? nearVolume : farVolume;
    element->voice_data.volume.right = pan <= 0x4000 ? farVolume : nearVolume;
}
void func_8003EBF0(AudioManager *manager, AudioElement *elements, s32 count)
{
    if ((u16)manager->unk_Flags & 0x20u)
        return;
    for (s32 i = 0; i < count; ++i) {
        AudioElement *element = &elements[i];
        if (!element->active_flag)
            continue;
        SoundVoiceData *voice = &element->voice_data;
        u16 pending = element->status_flags;

        if (pending & 0x100u) {
            soundUpdateVoiceGain(manager, element);
            voice->flags |= 1u;
        }
        if (pending & 0x200u) {
            u32 sum = (u32)manager->unk_Interpolator_0x7c.currentValue >> 16;
            sum += element->unk68 >> 16;
            sum += (u16)soundPostpassHalf(element, 0xd0);
            voice->pitch = (u16)func_8003EEA0((u32)(s32)(s16)sum) & 0x3fffu;
            voice->flags |= 4u;
        }
        if ((pending & 1u) && !(element->active_flag & 0x20u))
            SoundAssignVoiceToChannelAndPlay(voice, element->voice_number);
        if (pending & 2u)
            SoundStopVoiceOnChannel(voice, element->voice_number);
        element->status_flags = 0;
    }
}

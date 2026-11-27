/* SPDX-License-Identifier: MIT */
#include <string.h>

void func_8003A838(AudioManager *manager, s32 value, s32 frames)
{
    const s32 target = value == 0 ? 256 : value;
    const u32 fixed = (u32)target << 16;
    AudioInterpolator *ramp = &manager->unk_Interpolator_0x64;
    ramp->targetValue = (s16)target;
    ramp->counter = (s16)frames;
    if (frames != 0) {
        const s32 distance = (s32)(fixed - (u32)ramp->currentValue);
        ramp->stepIncrement = (s32)((s64)distance / frames);
    } else {
        s16 scale;
        memcpy(&scale, (const u8 *)&manager->unk_0x58 + sizeof scale, sizeof scale);
        ramp->currentValue = (s32)fixed;
        manager->unk_0x54 = (s32)((u32)target * (u32)(s32)scale);
    }
}

static void SoundSetManagerVoiceParameter(AudioManager *manager, AudioInterpolator *ramp,
                                          s32 value, s32 frames, u16 flags)
{
    const u32 target = (u32)value << 8;
    ramp->targetValue = (s16)target;
    if (frames == 0) {
        ramp->currentValue = (s32)(target << 16);
        ramp->counter = 0;
        unk_SoundSetFlagsOnActiveVoices(flags, manager);
        return;
    }

    const s32 current = ramp->currentValue >> 8;
    const s32 delta = (s32)((target << 8) - (u32)current);
    if (delta != 0) {
        const s64 increment = (s64)delta / frames;
        ramp->counter = (s16)frames;
        ramp->stepIncrement = (s32)((u32)increment << 8);
    }
}

void func_8003A948(void *state, s32 value, s32 frames)
{
    AudioManager *manager = state;
    SoundSetManagerVoiceParameter(manager, &manager->unk_Interpolator_0x7c,
                                 value, frames, 0x200);
}

void func_8003A9BC(void *state, s32 value, s32 frames)
{
    AudioManager *manager = state;
    SoundSetManagerVoiceParameter(manager, &manager->unk_Interpolator_0x88,
                                 value, frames, 0x100);
}

void func_8003AAC4(void *state, u32 mask)
{
    AudioManager *manager = state;
    if (!manager)
        return;

    memcpy(manager->unk_0x4c, &mask, sizeof mask);
    const unsigned count = manager->elementCount;
    u8 *record = (u8 *)manager + __builtin_offsetof(AudioManager, elements);
    for (unsigned i = 0; i < count; ++i, record += sizeof(AudioElement), mask >>= 1) {
        AudioElement *element = (AudioElement *)record;
        const u16 flags = element->active_flag;
        const u16 requested = (mask & 1u) ? 0x20 : 0;
        if (flags == 0 || (flags & 0x20) == requested)
            continue;

        element->active_flag = flags ^ 0x20;
        if (manager->unk_Flags >= 0)
            continue;

        if (requested) {
            SoundStopVoiceOnChannel(&element->voice_data, element->voice_number);
        } else if ((flags & (SOUND_CHANNEL_PERCUSSION_ACTIVE | SOUND_CHANNEL_HOLD_NOTE)) ==
                   SOUND_CHANNEL_HOLD_NOTE) {
            SoundAssignVoiceToChannelAndPlay(&element->voice_data, element->voice_number);
        }
    }
}

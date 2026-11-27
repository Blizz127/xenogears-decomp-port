/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>

static uint16_t SoundStepRead16(const void *object, size_t offset)
{
    uint16_t value;
    memcpy(&value, (const uint8_t *)object + offset, sizeof value);
    return value;
}

static void SoundStepWrite16(void *object, size_t offset, uint16_t value)
{
    memcpy((uint8_t *)object + offset, &value, sizeof value);
}

static uint32_t SoundStepRead32(const void *object, size_t offset)
{
    uint32_t value;
    memcpy(&value, (const uint8_t *)object + offset, sizeof value);
    return value;
}

static void SoundStepWrite32(void *object, size_t offset, uint32_t value)
{
    memcpy((uint8_t *)object + offset, &value, sizeof value);
}

static uint16_t SoundStepDecrement(void *record, size_t offset)
{
    uint16_t left = (uint16_t)(SoundStepRead16(record, offset) - 1u);
    SoundStepWrite16(record, offset, left);
    return left;
}

static void SoundStepManagerLevel(AudioManager *manager)
{
    if (manager->unk_0x60 == 0)
        return;
    --manager->unk_0x60;
    uint32_t level = manager->unk_0x60
        ? (uint32_t)manager->unk_0x58 + (uint32_t)manager->unk_0x5c
        : (uint32_t)SoundStepRead16(manager, 0x62) << 16;
    SoundStepWrite32(manager, 0x58, level);
    manager->unk_0x54 = (int16_t)(level >> 16) * (int16_t)SoundStepRead16(manager, 0x66);
}

static void SoundStepShortRamp(AudioElement *element, unsigned control_bit,
                               size_t destination, size_t step, size_t remaining)
{
    if (!(element->unk_0x04 & control_bit))
        return;
    uint16_t value;
    if (SoundStepDecrement(element, remaining) == 0) {
        value = SoundStepRead16(element, step + 2);
        element->unk_0x04 &= (uint16_t)~control_bit;
    } else {
        value = (uint16_t)(SoundStepRead16(element, destination) + SoundStepRead16(element, step));
    }
    SoundStepWrite16(element, destination, value);
    element->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
}

void func_8003C4C4(AudioManager *manager, AudioElement *elements, s32 count)
{
    SoundStepManagerLevel(manager);
    for (s32 i = 0; i < count; ++i) {
        AudioElement *element = &elements[i];
        if (!element->active_flag || !element->fermataDuration)
            continue;
        if (SoundStepDecrement(element, 0x5c) == 1 && (element->active_flag & 0x1000u)) {
            element->voice_data.flags |= 0x80u;
            element->voice_data.unkAdsr6 = 6;
        }
        if (!SoundStepDecrement(element, 0x5e)) {
            element->active_flag |= SOUND_CHANNEL_REST_NOTE;
            element->status_flags |= SOUND_STATUS_REST_NOTE;
        }
        if ((element->unk_0x04 & 1u) != 0) {
            element->unk68 += SoundStepRead32(element, 0x84);
            element->status_flags |= 0x200;
            if (!(element->unk_0x04 & 2u) && !SoundStepDecrement(element, 0x94))
                element->unk_0x04 &= (uint16_t)~1u;
        }
        if ((element->unk_0x04 & 8u) != 0) {
            SoundStepWrite32(element, 0x78, SoundStepRead32(element, 0x78) + SoundStepRead32(element, 0x88));
            element->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
            if (!SoundStepDecrement(element, 0x96))
                element->unk_0x04 &= (uint16_t)~8u;
        }
        SoundStepShortRamp(element, 0x10, 0x74, 0x90, 0x98);
        SoundStepShortRamp(element, 0x20, 0x76, 0x8c, 0x9a);
    }
}

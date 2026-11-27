/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>

u8 *func_8003D0E8(u8 *cursor, AudioManager *manager, AudioElement *element)
{
    (void)element;
    int16_t scale;
    memcpy(&scale, (const uint8_t *)manager + 0x66, sizeof scale);
    manager->unk_0x54 = cursor[0] * scale;
    manager->unk_0x58 = (int32_t)cursor[0] * 65536;
    return cursor + 1;
}

u8 *func_8003D7C8(u8 *cursor, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    uint16_t increment = (uint16_t)((cursor[0] << 8) | cursor[1]);
    uint16_t pitch = (uint16_t)((uint16_t)element->unk_0x6E + increment);
    memcpy(&element->unk_0x6E, &pitch, sizeof pitch);
    element->status_flags |= 0x0200;
    return cursor + 2;
}

u8 *func_8003DBE4(u8 *cursor, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    uint8_t *voice = (uint8_t *)element;
    const uint16_t ticks = cursor[1];
    const uint8_t difference = (uint8_t)(cursor[2] - cursor[0]);
    if (ticks == 0 || difference == 0) {
        element->unk_0x04 &= (uint16_t)~0x0100u;
    } else {
        /* The sweep uses a signed eight-bit difference in the top byte. */
        const int32_t signed_difference = difference < 128 ? difference : (int)difference - 256;
        const int32_t step = (signed_difference * 16777216) / (int32_t)ticks;
        const uint16_t start = (uint16_t)cursor[0] << 8;
        memcpy(voice + 0x7c, &step, sizeof step);
        memcpy(voice + 0x80, &ticks, sizeof ticks);
        memcpy(voice + 0x82, &start, sizeof start);
        element->unk_0x04 = (uint16_t)((element->unk_0x04 | 0x0100u) & ~0x0008u);
    }
    return cursor + 3;
}

u8 *func_8003CEF0(u8 *cursor, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    uint8_t *voice = (uint8_t *)element;
    uint16_t entry;
    memcpy(&entry, voice + 0x72, sizeof entry);
    ++entry;
    memcpy(voice + 0x72, &entry, sizeof entry);

    uint8_t *record = voice + 0x9c + (size_t)entry * 12;
    record[0] = (uint8_t)(cursor[0] - 1);
    record[2] = (uint8_t)element->octave;
    const uint32_t guest_cursor = (uint32_t)(uintptr_t)(cursor + 1);
    memcpy(record + 4, &guest_cursor, sizeof guest_cursor);
    return cursor + 1;
}

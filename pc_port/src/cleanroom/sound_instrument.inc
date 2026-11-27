/* SPDX-License-Identifier: MIT
 * Independently written from the public sound ABI and synthetic instrument
 * input/output measurements. The caller supplies the bank and its records.
 */
#include <stddef.h>
#include <string.h>

void func_8003E5BC(s32 instrument, AudioElement *element)
{
    const u16 index = (u16)instrument;
    element->unk_0x26 = (u8)index;
    if (index >= 128u || element->unk2C == 0u)
        return;

    const uintptr_t bank = element->unk2C;
    const uintptr_t address = bank + sizeof(SoundWDSEntry) + 16u * index;
    if (address < 0x10000u)
        return;

    /* The supported hosts use the same little-endian words as the bank.
     * Copy the packed fields without requiring host word alignment. */
    const u8 *record = (const u8 *)address;
    u32 spu_base, start_units;
    u16 loop_units;
    memcpy(&spu_base, (const void *)(bank + offsetof(SoundWDSEntry, spuMemoryAddress)),
           sizeof(spu_base));
    memcpy(&start_units, record, sizeof(start_units));
    memcpy(&loop_units, record + 4, sizeof(loop_units));
    memcpy(&element->unk_0x6C, record + 6, sizeof(element->unk_0x6C));

    SoundVoiceData *voice = &element->voice_data;
    voice->startAddress = spu_base + 8u * start_units;
    voice->loopAddress = 8u * (start_units + loop_units);
    voice->unkAdsr1 = record[12] & 7u;
    voice->unkAdsr2 = (record[12] >> 4) & 7u;
    voice->unkAdsr3 = record[13] & 7u;
    voice->unkAdsr4 = record[8] & 0x7fu;
    voice->adsrDR = record[9] & 0x0fu;
    voice->unkAdsr5 = record[10] & 0x7fu;
    voice->unkAdsr6 = record[11] & 0x1fu;
    voice->adsrSR = record[9] >> 4;
    element->unk_0x28 = voice->unkAdsr6;
    element->active_flag |= SOUND_CHANNEL_CHANGE_INSTRUMENT;
}

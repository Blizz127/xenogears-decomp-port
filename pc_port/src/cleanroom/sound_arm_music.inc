/* SPDX-License-Identifier: MIT
 * Native music-manager initialization, independently authored from observed
 * service calls and byte ranges changed by synthetic song input. */
#include <stddef.h>
#include <string.h>

static u16 MusicReadHalfword(const void *address)
{
    u16 value;
    memcpy(&value, address, sizeof(value));
    return value;
}

static void MusicWriteHalfword(void *address, u16 value)
{
    memcpy(address, &value, sizeof(value));
}

void func_8003B424(AudioManager *manager)
{
    const unsigned count = manager->elementCount;
    if (count == 0)
        return;

    const u32 song_address = (u32)manager->unk_0x8;
    const u8 *song = SOUND_PSX_TO_PTR(const u8, song_address);
    const int bank_id = (s16)MusicReadHalfword(&manager->unk_0x15[1]);
    SoundWDSEntry *bank = SoundFindWdsEntry(bank_id);
    if (bank == NULL)
        bank = g_SoundWdsLinkedList;

    u32 active = 0;
    for (unsigned i = 0; i < count; ++i) {
        /* Managers are sized for the element count supplied by the song. */
        u8 *records = (u8 *)manager + offsetof(AudioManager, elements);
        AudioElement *element = (AudioElement *)(records + i * sizeof(AudioElement));
        const u16 track_offset = MusicReadHalfword(song + 0x22 + i * 2);
        if (track_offset == 0) {
            element->active_flag = 0;
            continue;
        }

        const u32 bit = 1u << (i & 31);
        u32 channel_mask;
        memcpy(&channel_mask, manager->unk_0x4c, sizeof(channel_mask));
        active |= bit;
        element->active_flag = SOUND_CHANNEL_REST_NOTE | 1;
        if (channel_mask & bit)
            element->active_flag |= 0x20;
        element->active_flag |= (u16)manager->unk_Flags & 4;
        element->status_flags = 0x170;
        element->unk_0x04 = 0;
        element->unk_0x06[0] = (s8)i;
        element->unk_0x06[1] = 16;
        const u32 song_id = MusicReadHalfword(song + 0x10);
        memcpy(&element->unk_0x06[2], &song_id, sizeof(song_id));

        const u32 script = song_address + track_offset;
        memcpy(&element->unk_0x06[10], &script, sizeof(script));
        element->unk14 = script;
        element->savedScriptIP = 0;
        memset(element->unk_0x1C, 0, sizeof(element->unk_0x1C));
        element->fermataDuration = 0;
        element->unk_0x5E[2] = 0;
        element->unk_0x62 = 15;
        element->unk_0x64[0] = 0;
        element->octave = 60;
        element->unk_0x6E = 0;
        MusicWriteHalfword(element->unk_0x70, 0);
        MusicWriteHalfword(element->unk_0x70 + 2, 0xffff);
        element->unk_0x74 = 0x4000;
        MusicWriteHalfword(element->unk_0x76, 0x6000);
        const u32 initial_volume = 0x7f000000;
        memcpy(element->unk_0x76 + 2, &initial_volume, sizeof(initial_volume));
        memset(element->voice_data.unkC, 0, 4);
        element->unk_0xCE = 0;
        memset(element->unk_0xD0, 0, 6);
        element->unk_0xF6 = 0;
        element->unk_0x116 = 0;
        element->unk_0x136 = 0;
        MusicWriteHalfword(element->unk_0x138 + 30, 0);

        element->unk2C = SOUND_PTR_TO_PSX(bank);
        ((u8 *)element)[0x25] = manager->unk_0x15[1];
        if (bank != NULL)
            func_8003E5BC(0, element);
        const u32 channel = i - 1;
        element->voice_number = (u8)channel;
        element->voice_data.modeFlags = 0;
        element->voice_data.priority = 256;
        SoundAssignVoiceToChannelAndStop(&element->voice_data, channel);
    }
    manager->unk_0x48 = (s32)active;
}

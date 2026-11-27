/* SPDX-License-Identifier: MIT */

void func_8003B0AC(AudioManager *manager, SoundFile *file)
{
    const u8 *header = (const u8 *)file;
    s32 manager_bytes = SoundCalculateAudioManagerSize(header[0x14]);
    u8 *percussion = (u8 *)manager + manager_bytes;
    manager->unk_0xc = (s32)SOUND_PTR_TO_PSX(percussion);

    u32 remaining = header[0x15];
    if (remaining == 0)
        return;

    u16 records_offset;
    __builtin_memcpy(&records_offset, header + SOUND_FILE_SCRIPT_OFFSETS_OFFSET, sizeof(records_offset));
    const u8 *record = header + records_offset;
    do {
        u32 parameters;
        __builtin_memcpy(&parameters, record + 1, sizeof(parameters));
        __builtin_memcpy(percussion + record[0] * sizeof(parameters), &parameters, sizeof(parameters));
        record += 1 + sizeof(parameters);
    } while (--remaining);
}

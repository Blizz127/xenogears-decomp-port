/* SPDX-License-Identifier: MIT */
void func_8003CC84(AudioManager *manager, AudioElement *element, s32 note)
{
    const uint8_t *table = SOUND_PSX_TO_PTR(const uint8_t, (uint32_t)manager->unk_0xc);
    if (!table)
        return;
    const uint8_t *record = table + (size_t)(uint8_t)note * 4u;

    func_8003E5BC(record[0], element);
    const uint32_t pitch = ((uint32_t)record[1] << 8)
                         + (uint16_t)element->unk_0x6C
                         + (uint16_t)element->unk_0x6E;
    element->unk68 = pitch << 16;
    const uint16_t pan = (uint16_t)((uint16_t)record[3] << 8);
    element->status_flags |= 0x100;
    element->unk_0x74 = pan;
}

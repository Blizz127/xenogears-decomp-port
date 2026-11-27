/* SPDX-License-Identifier: MIT
 * Independently implemented from observed voice-register transactions. */
void func_8003EB5C(void)
{
    const u32 pending = (u32)g_unk_VoicesNeedingProcessing;
    const u32 key_off = pending | g_SoundKeyOffFlags;
    if (key_off == 0)
        return;

    /* ADSR2 is the sixth halfword in each eight-halfword voice record. */
    for (unsigned voice = 0; voice < 24; ++voice) {
        if ((pending & (1u << voice)) != 0) {
            volatile u16 *adsr2 = &g_pSoundSpuRegisters->raw[8 * voice + 5];
            *adsr2 = (u16)((*adsr2 & 0xffc0u) | 6u);
        }
    }

    /* The key-off command spans two independently written halfwords. */
    g_pSoundSpuRegisters->raw[0x18c / 2] = (u16)key_off;
    g_pSoundSpuRegisters->raw[0x18e / 2] = (u16)(key_off >> 16);
    g_unk_VoicesNeedingProcessing = 0;
    g_SoundKeyOffFlags = 0;
}

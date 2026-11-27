/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>

void func_8003AA30(AudioManager *manager)
{
    DisableEvent(g_unk_SoundEvent);
    if ((u16)g_SoundControlFlags & 0x1000u) {
        const u8 *settings = manager->unk_0x40;
        int16_t depth;
        memcpy(&depth, settings + 4, sizeof depth);
        SoundSetReverbModeWithAllocation(settings[1], depth, settings[2], settings[3]);
    }
    SoundSetFlagsOnActiveVoices(manager, 0xffff);
    func_8003AFA0(manager);
    manager->unk_Flags = (s16)(((u16)manager->unk_Flags | 0x8000u) & ~0x0100u);
    EnableEvent(g_unk_SoundEvent);
}

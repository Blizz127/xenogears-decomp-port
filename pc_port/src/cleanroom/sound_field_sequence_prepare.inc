/* SPDX-License-Identifier: MIT */
#include <string.h>

void func_8003B22C(AudioManager *manager)
{
    /* The host manager stores the sequence address in a signed 32-bit slot. */
    const u8 *sequence = (const u8 *)(intptr_t)manager->unk_0x8;
    if ((s16)SoundUnkDebug0((void *)sequence) != 0) {
        SoundHandleError(10);
        return;
    }

    manager->unk_Flags |= 1;
    memmove(manager->unk2, sequence + 0x10, 2);
    manager->elementCount = sequence[0x14];
    memmove(manager->unk_0x15 + 1, sequence + 0x16, 2);
    u8 mix[2];
    memcpy(mix, sequence + 0x18, sizeof(mix));
    manager->unk_0x18 = mix[0];
    manager->unk_0x19 = mix[1];

    u8 *reverb = manager->unk_0x40;
    reverb[1] = sequence[0x1a];
    s16 depth = (s16)((u16)sequence[0x1b] << 8);
    memcpy(reverb + 4, &depth, sizeof(depth));
    reverb[2] = sequence[0x1c];
    reverb[3] = sequence[0x1d];
    if (g_SoundControlFlags & 0x1000)
        SoundSetReverbModeWithAllocation((s8)reverb[1], depth, reverb[2], reverb[3]);
    SoundInitializeAudioManager(manager);
}

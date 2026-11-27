/* SPDX-License-Identifier: MIT */
#include <stdint.h>

void func_8003A89C(AudioManager *manager, s32 level, s32 steps)
{
    if (!manager)
        return;
    AudioInterpolator *gain = &manager->unk_Interpolator_0x70;
    gain->targetValue = (s16)((u32)level << 8);

    if (steps == 0) {
        gain->currentValue = (s32)((u32)level << 24);
        gain->counter = 0;
        unk_SoundSetFlagsOnActiveVoices(0x100, manager);
    } else {
        s32 previous = gain->currentValue >> 8;
        s32 difference = (s32)(((u32)level << 16) - (u32)previous);
        if (!difference)
            return;

        int64_t increment = (int64_t)difference / steps;
        gain->stepIncrement = (s32)((u32)increment << 8);
        gain->counter = (s16)steps;
    }
    if (level && ((u16)manager->unk_Flags & 0x100u))
        func_8003AA30(manager);
}

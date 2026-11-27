/* Per-element pitch controls for the active sequence manager. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    SOUND_PITCH_CURRENT = 0,
    SOUND_PITCH_DELTA = 0x16,
    SOUND_PITCH_TARGET = 0x18,
    SOUND_PITCH_TICKS = 0x24
};

static void sound_pitch_store(AudioElement *element, unsigned field, u16 value)
{
    memcpy(element->unk_0x76 + field, &value, sizeof value);
}

static void sound_pitch_update(AudioElement *element, s32 level, s32 ticks, int ramp)
{
    u32 scaled = (u32)level << 8;
    if (!ramp) {
        sound_pitch_store(element, SOUND_PITCH_CURRENT, (u16)scaled);
        element->status_flags = SOUND_STATUS_VOLUME_CHANGE;
        return;
    }

    s16 current;
    memcpy(&current, element->unk_0x76 + SOUND_PITCH_CURRENT, sizeof current);
    if (scaled == (u32)(s32)current)
        return;
    if (!ticks)
        ticks = 1;
    s32 difference = (s32)(scaled - (u32)(s32)current);
    sound_pitch_store(element, SOUND_PITCH_TARGET, (u16)scaled);
    sound_pitch_store(element, SOUND_PITCH_TICKS, (u16)ticks);
    sound_pitch_store(element, SOUND_PITCH_DELTA, (u16)((int64_t)difference / ticks));
    element->unk_0x04 |= 0x20u;
}

static void sound_pitch_apply(s32 selector, s32 level, s32 ticks, int pair, int ramp)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    s32 allocated = D_80059478;
    if (!pair && allocated == 0)
        return;
    unsigned first = pair ? (((u8)selector ^ 8u) & 0xfeu) : 0;
    unsigned count = pair ? 2u : (unsigned)allocated;
    if (!manager || allocated < 0 || first > (unsigned)allocated ||
        count > (unsigned)allocated - first) {
        fprintf(stderr, "[xeno-port] sequence pitch request has no valid channel range\n");
        return;
    }
    /* The allocating service sizes the variable tail from the channel count. */
    unsigned char *channels = (unsigned char *)manager + __builtin_offsetof(AudioManager, elements);
    for (unsigned i = first; i < first + count; ++i) {
        AudioElement *element = (AudioElement *)(channels + (size_t)i * sizeof(AudioElement));
        if (!(element->active_flag & 1u))
            continue;
        if (!pair) {
            u32 key;
            memcpy(&key, &element->unk_0x06[2], sizeof key);
            if (key != (u32)selector)
                continue;
        }
        sound_pitch_update(element, level, ticks, ramp);
    }
}

void func_8003A2E4(s32 key, s32 level)
{
    sound_pitch_apply(key, level, 0, 0, 0);
}

void func_8003A344(s32 channel, s32 level)
{
    sound_pitch_apply(channel, level, 0, 1, 0);
}

void func_8003A3B8(s32 key, s32 level, s32 ticks)
{
    sound_pitch_apply(key, level, ticks, 0, 1);
}

void func_8003A450(s32 channel, s32 level, s32 ticks)
{
    sound_pitch_apply(channel, level, ticks, 1, 1);
}

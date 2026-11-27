#include <stddef.h>
#include <string.h>

static AudioElement *sound_outer_voice(AudioManager *manager, unsigned slot)
{
    unsigned char *first = (unsigned char *)manager + offsetof(AudioManager, elements);
    return (AudioElement *)(first + slot * sizeof(AudioElement));
}

void func_8003A55C(s32 voice, s32 pan)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    unsigned pair = ((u8)voice ^ 8u) & ~1u;
    if (!manager || D_80059478 <= (s32)(pair + 1))
        return;

    for (unsigned i = pair; i < pair + 2; ++i) {
        AudioElement *element = sound_outer_voice(manager, i);
        if (element->active_flag & 1u) {
            element->unk_0x74 = (u16)((u8)pan * 256u);
            element->status_flags = SOUND_STATUS_VOLUME_CHANGE;
        }
    }
}

u32 func_8003A5D0(s32 sound_id)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    u32 active = 0;
    if (!manager)
        return active;
    for (s32 i = 0; i < D_80059478 && i < 32; ++i) {
        AudioElement *element = sound_outer_voice(manager, (unsigned)i);
        if (element->active_flag & 1u) {
            u32 id;
            memcpy(&id, &element->unk_0x06[2], sizeof id);
            if (sound_id == -1 || id == (u32)sound_id)
                active |= 1u << (unsigned)i;
        }
    }
    return active;
}

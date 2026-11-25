#include <stdint.h>

void func_8003E5BC(s32 instrument, AudioElement *element);

u8 *SoundScriptSetManagerUnk1a(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioManager *state = manager;
    const volatile u8 *operand = script;

    (void)element;
    state->unk_0x1a = *operand;
    return script + 1;
}

u8 *SoundScriptAddManagerUnk1a(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioManager *state = manager;
    const volatile u8 *operand = script;
    u8 previous = state->unk_0x1a;
    u8 increment = *operand;

    (void)element;
    state->unk_0x1a = (u8)(previous + increment);
    return script + 1;
}

static void sound_script_update_flag_pair(volatile uint16_t *primary,
                                          volatile uint16_t *companion,
                                          uint16_t primary_mask, int set)
{
    uint16_t primary_value = *primary;
    uint16_t companion_value = *companion;

    if (set) {
        primary_value |= primary_mask;
        companion_value |= 1u;
    } else {
        primary_value &= (uint16_t)~primary_mask;
        companion_value &= (uint16_t)~1u;
    }
    *primary = primary_value;
    *companion = companion_value;
}

u8 *SoundScriptSetUnkCEAndF6(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_update_flag_pair(&element->unk_0xCE, &element->unk_0xF6, 1, 1);
    return script;
}

u8 *SoundScriptClearUnkCEAndF6(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_update_flag_pair(&element->unk_0xCE, &element->unk_0xF6, 1, 0);
    return script;
}

u8 *SoundScriptSetUnkCEAndUnk116(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_update_flag_pair(&element->unk_0xCE, &element->unk_0x116, 2, 1);
    return script;
}

u8 *SoundScriptClearUnkCEAndUnk116(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_update_flag_pair(&element->unk_0xCE, &element->unk_0x116, 2, 0);
    return script;
}

u8 *SoundScriptSetUnkCEAndUnk136(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_update_flag_pair(&element->unk_0xCE, &element->unk_0x136, 4, 1);
    return script;
}

u8 *SoundScriptCallE5BC(u8 *script, AudioManager *manager, AudioElement *element)
{
    const volatile AudioElement *state = element;
    s32 instrument = (u8)state->unk_0x26;

    (void)manager;
    func_8003E5BC(instrument, element);
    return script;
}

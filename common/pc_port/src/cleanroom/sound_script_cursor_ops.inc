#include <stdint.h>

u8 *SoundScriptNop(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return script;
}

u8 *SoundScriptNop2(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return script;
}

u8 *SoundScriptNop3(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return script;
}

u8 *SoundScriptNop4(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return script;
}

u8 *SoundScriptNop5(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return script;
}

u8 *SoundScriptDefaultHandler(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return script;
}

u8 *SoundScriptDefaultSkip3(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return (u8 *)((uintptr_t)script + (uintptr_t)3);
}

u8 *SoundScriptSkip3(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    (void)element;
    return (u8 *)((uintptr_t)script + (uintptr_t)3);
}

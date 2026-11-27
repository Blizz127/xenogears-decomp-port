/* Native call signatures and virtual memory bounds for the accepted resident entries. */

s32 func_8003A82C(u16 *arg0)
{
    static const SoundResidentEntry entry = { 0x8003a82cu, 12, { 18, 0, 0 }, 1, 0, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

void func_8003ABE8(u8 *arg0, u8 arg1)
{
    static const SoundResidentEntry entry = { 0x8003abe8u, 8, { 28, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, (uintptr_t)arg1, 0 };
    sound_resident_run(&entry, args);
}

u8 *func_8003CD08(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003cd08u, 40, { 1, 0, 94 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003CE68(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003ce68u, 52, { 2, 64, 0 }, 7, 2, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003CE9C(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003ce9cu, 36, { 2, 60, 0 }, 7, 2, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D110(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d110u, 44, { 1, 92, 0 }, 7, 2, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D13C(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d13cu, 64, { 2, 100, 0 }, 7, 2, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D1BC(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d1bcu, 76, { 2, 124, 0 }, 7, 2, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D2D0(u8 *arg0, s32 arg1, u8 *arg2)
{
    static const SoundResidentEntry entry = { 0x8003d2d0u, 48, { 1, 0, 97 }, 5, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, (uintptr_t)arg1, (uintptr_t)arg2 };
    return (u8 *)sound_resident_run(&entry, args);
}

s32 func_8003D300(s32 arg0, s32 *arg1, u16 *arg2)
{
    static const SoundResidentEntry entry = { 0x8003d300u, 40, { 0, 16, 2 }, 6, 4, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, (uintptr_t)arg1, (uintptr_t)arg2 };
    return (s32)sound_resident_run(&entry, args);
}

u8 *func_8003D370(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d370u, 52, { 0, 0, 56 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D3A4(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d3a4u, 52, { 0, 0, 56 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D60C(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d60cu, 52, { 3, 0, 87 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D79C(u8 *arg0, s32 arg1, void *arg2)
{
    static const SoundResidentEntry entry = { 0x8003d79cu, 44, { 1, 0, 112 }, 5, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, (uintptr_t)arg1, (uintptr_t)arg2 };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D7FC(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d7fcu, 88, { 2, 0, 150 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003D884(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d884u, 52, { 1, 0, 114 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DAB0(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003dab0u, 60, { 1, 0, 244 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DB2C(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003db2cu, 44, { 1, 0, 124 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DB58(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003db58u, 64, { 1, 0, 124 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DB98(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003db98u, 76, { 2, 0, 152 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DE18(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003de18u, 60, { 1, 0, 276 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DEB4(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003deb4u, 48, { 1, 0, 118 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DEE4(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003dee4u, 88, { 2, 0, 154 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

u8 *func_8003DF3C(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003df3cu, 60, { 1, 0, 308 }, 7, 4, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

s32 func_8003E160(s32 arg0, s32 arg1, u16 *arg2)
{
    static const SoundResidentEntry entry = { 0x8003e160u, 32, { 0, 0, 312 }, 4, 4, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, (uintptr_t)arg1, (uintptr_t)arg2 };
    return (s32)sound_resident_run(&entry, args);
}

void func_8003E3E0(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003e3e0u, 44, { 32, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    sound_resident_run(&entry, args);
}

u8 *func_8003E4BC(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003e4bcu, 52, { 1, 104, 0 }, 7, 2, 4, 0 };
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(&entry, args);
}

void func_8003F190(u16 *arg0)
{
    static const SoundResidentEntry entry = { 0x8003f190u, 20, { 32, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)arg0, 0, 0 };
    sound_resident_run(&entry, args);
}

s32 func_8003F1A4(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f1a4u, 72, { 20, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

s32 func_8003F1EC(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f1ecu, 84, { 32, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

s32 func_8003F240(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f240u, 96, { 32, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

s32 func_8003F2A0(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f2a0u, 104, { 32, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

s32 func_8003F308(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f308u, 76, { 20, 0, 0 }, 1, 1, 0, SOUND_RESIDENT_SEED | SOUND_RESIDENT_RANDOM_CALL };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

s32 func_8003F354(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f354u, 108, { 20, 0, 0 }, 1, 1, 0, SOUND_RESIDENT_SEED | SOUND_RESIDENT_RANDOM_CALL };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

s32 func_8003F3C0(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f3c0u, 108, { 20, 0, 0 }, 1, 1, 0, SOUND_RESIDENT_SEED | SOUND_RESIDENT_RANDOM_CALL };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

void func_8003F42C(s32 arg0)
{
    static const SoundResidentEntry entry = { 0x8003f42cu, 16, { 0, 0, 0 }, 0, 0, 0, SOUND_RESIDENT_SEED };
    const uintptr_t args[3] = { (uintptr_t)arg0, 0, 0 };
    sound_resident_run(&entry, args);
}

s32 func_8003F43C(void)
{
    static const SoundResidentEntry entry = { 0x8003f43cu, 44, { 0, 0, 0 }, 0, 0, 0, SOUND_RESIDENT_SEED };
    const uintptr_t args[3] = { 0, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}

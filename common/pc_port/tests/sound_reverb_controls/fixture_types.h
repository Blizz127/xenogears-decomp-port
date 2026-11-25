#ifndef REVERB_FIXTURE_TYPES_H
#define REVERB_FIXTURE_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef u32 SoundPsxAddress;
#define SOUND_PTR_TO_PSX(p) ((SoundPsxAddress)(uintptr_t)(p))

/* The manager fixture models only the scalar locations in the write contract. */
typedef struct {
    s32 currentValue;
    u32 untouched;
    u16 counter;
    u16 tail;
} FixtureInterpolator;

typedef struct {
    u8 prefix[0x1a];
    u8 unk_0x1a, unk_0x1b;
    u32 gap_1c;
    u32 unk_0x20, unk_0x24, unk_0x28;
    u32 gap_2c;
    u16 unk_0x30, unk_0x32, unk_0x34, unk_0x36;
    u16 unk_0x38, unk_0x3a, unk_0x3c, unk_0x3e;
    u32 gap_40[2];
    u32 unk_0x48, gap_4c, unk_0x50, unk_0x54, unk_0x58, unk_0x5c;
    u16 unk_0x60, gap_62;
    FixtureInterpolator unk_Interpolator_0x64, unk_Interpolator_0x70;
    FixtureInterpolator unk_Interpolator_0x7c, unk_Interpolator_0x88;
    u8 trailing[24];
} AudioManager;
_Static_assert(offsetof(AudioManager, unk_0x1a) == 0x1a, "manager prefix");
_Static_assert(offsetof(AudioManager, unk_Interpolator_0x88.counter) == 0x90,
               "manager final counter");

/* Stand-ins for the mock sound services. Surrounding words act as sentinels. */
typedef struct { s16 left, right; } SpuVolume;
typedef struct {
    u32 guard, mask;
    struct { s32 reverb, mix; } cd;
    u32 unused[2];
} SpuCommonAttr;
typedef struct {
    SpuCommonAttr commonAttr;
    s16 currentReverbDepth;
    u16 guard;
} SoundVolumeController;
enum { SPU_REV_MODE_MAX = 10, SPU_COMMON_CDREV = 0x100, SPU_COMMON_CDMIX = 0x200 };

extern SoundVolumeController g_SoundVolumeController;
extern SpuVolume g_SoundReverbDepth;
extern s32 g_ReverbWorkAreaSizes[SPU_REV_MODE_MAX], g_SoundReverbMemoryHandle;
extern u8 g_SoundReverbType, g_SoundReverbDelay, g_SoundReverbFeedback;
extern s32 g_SoundUploadDestBuffer, g_SoundUploadSourceAddr, g_SoundUploadBytesRemaining;
extern short g_SoundControlFlags;

void SpuSetCommonAttr(SpuCommonAttr *attr);
void *SoundHeapAllocate(u32 size);
void SpuGetReverbModeType(long *out);
u32 SoundSpuMemoryAllocateBlockAtAddress(s32 size, s32 address, s32 type);
void SpuSetReverbModeDepth(short left, short right);
long SpuSetReverbModeType(long value);
void SpuSetReverbModeDelayTime(long value);
void SpuSetReverbModeFeedback(long value);
#endif

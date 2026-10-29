#ifndef FIELD_SOUND_EVENTS_RETAIL_TEST_H
#define FIELD_SOUND_EVENTS_RETAIL_TEST_H
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
extern s16 D_800C3A38;
extern u16 D_800AE060[];
extern s32 D_800C3A64;
extern u16 D_800C3A2C;
extern s32 D_800B06A0;
extern void *D_800B235C;
/* Canonical ABI: src/slus_006.64/system/sound.c, not the old field prototype. */
void func_80039EC4(s32 event, s32 channel);
void func_80085678(void);
#endif

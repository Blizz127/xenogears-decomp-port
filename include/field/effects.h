#ifndef _XENO_FIELD_EFFECTS_H
#define _XENO_FIELD_EFFECTS_H

#include "psyq/libgpu.h"

typedef struct {
    short isActive;
    short isFinished;
    short duration;
    char _pad[2];
    int v1;
    int v2;
    int v3;
    int v4;
    int v5;
    int v6;
    int delta1;
    int delta2;
    int delta3;
    int delta4;
    int delta5;
    int delta6;
    short unk38;
    short unk3C;
#ifdef XENO_PC_PORT
    /* Field state and overlay code address this as a packed PSX record. Keep
     * the four pointer slots 32-bit so fades still begin at retail offset
     * 0x4C; the native executable and its emulated heap live below 4 GiB. */
    u32 buffer0;
    u32 buffer1;
    u32 buffer2;
    u32 buffer3;
#else
    DR_MOVE* buffer0;
    DR_MOVE* buffer1;
    POLY_FT4* buffer2;
    POLY_FT4* buffer3;
#endif
} FieldDistortion;

typedef struct {
    DR_MODE drawModes[2];
    TILE tiles[2];
    int r0;
    int g0;
    int b0;
    int redDelta;
    int greenDelta;
    int blueDelta;
    short semitransparency;
    short isVisible;
    short duration;
    short _pad;
} FieldFade;

typedef struct {
    FieldDistortion distortion;
    FieldFade fades[2];
} FieldEffects;

#ifdef XENO_PC_PORT
_Static_assert(offsetof(FieldDistortion, buffer0) == 0x3C,
               "FieldDistortion buffer0 must retain its PSX offset");
_Static_assert(offsetof(FieldDistortion, buffer1) == 0x40,
               "FieldDistortion buffer1 must retain its PSX offset");
_Static_assert(offsetof(FieldDistortion, buffer2) == 0x44,
               "FieldDistortion buffer2 must retain its PSX offset");
_Static_assert(offsetof(FieldDistortion, buffer3) == 0x48,
               "FieldDistortion buffer3 must retain its PSX offset");
_Static_assert(sizeof(FieldDistortion) == 0x4C,
               "FieldDistortion must retain its PSX size");
_Static_assert(offsetof(FieldEffects, fades) == 0x4C,
               "FieldEffects fades must retain their PSX offset");
_Static_assert(sizeof(FieldEffects) == 0xFC,
               "FieldEffects must retain its PSX size");
#endif

extern FieldEffects g_FieldEffects;

#endif

#ifndef _XENO_FIELD_GRAPHICS_H
#define _XENO_FIELD_GRAPHICS_H

#include "psyq/libgpu.h"

void FieldClampPolyFT4UVs(POLY_FT4* poly, short u0, short v0, short u1, short v1,
                         short u2, short v2, short u3, short v3);

typedef struct {
    DR_MODE drModes[0x21][2];
    SPRT sprites[0x21][2];
} SpriteList;

typedef struct {
    DR_MODE drModes[0x10][2];
    SPRT sprites[0x10][2];
} SpriteList2;

typedef struct {
    DR_MODE drModes[5][2];
    SPRT sprites[5][2];
} SpriteList3;


typedef struct {
    DR_MODE drModes[5][2];
    RECT rects[5][2];
    POLY_FT4 polys[5][2];
} PolyList2;

#endif

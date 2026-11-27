/* SPDX-License-Identifier: MIT
 * Both host objects describe these sprite records with native pointers.
 * Member names, types, extents and bit widths follow layout/verified.json.
 * Linux object SHA-256   71df56750a2a2f5eb483ab6c571331996ba189f636000117f445ea511988713c
 * Windows object SHA-256 da5191e34a01a44722d5a25f17cb6a14a8caec9daa666e9a0adc1e799c63b58d
 */
#ifndef ANIMATION_SPRITE_RECORDS_H
#define ANIMATION_SPRITE_RECORDS_H

#include <psx/libgte.h>

typedef struct IVEC3 {
    int x;
    int y;
    int z;
} IVEC3;

typedef struct SVEC {
    short x;
    short y;
    short z;
} SVEC;

typedef struct SVEC2 {
    short x;
    short y;
} SVEC2;

typedef struct SpriteAnimPackage {
    void *pFrames;
    SVEC2 tex;
    SVEC2 clut;
    void *pPalette;
    void *pAnimations;
} SpriteAnimPackage;

typedef struct SpriteAnimPackageFileHeader {
    u32 field_0x0;
    u32 animationsOffset;
    u32 framesOffset;
    u32 paletteOffset;
} SpriteAnimPackageFileHeader;

typedef struct SpriteDataI3 {
    s32 field_0x0;
    s32 field_0x4;
    s32 field_0x8;
    s16 field_0xC;
    s16 texX;
    s16 texY;
    s16 field_0x12;
    s16 entityId;
    s16 field_0x16;
    void *field_0x18;
} SpriteDataI3;

typedef struct SpriteDirectionTransform {
    u8 translationX;
    u8 translationY;
    s16 rotationX;
    s16 rotationY;
    s16 rotationZ;
} SpriteDirectionTransform;

typedef struct SpriteDirectionTransforms {
    SpriteDirectionTransform dirTransforms[8];
} SpriteDirectionTransforms;

typedef struct SpriteDataI1 {
    SVEC rotation;
    SVEC scale;
    MATRIX transformMatrix;
    void *pFramesData;
    void *pCurRenderFramesData;
    SpriteDirectionTransforms *pDirTransforms;
    void *pNextSpriteData;
    u8 offsetX;
    u8 offsetY;
    s16 field_0x3E;
} SpriteDataI1;

typedef struct SpriteData {
    IVEC3 position;
    IVEC3 step;
    int moveSpeed;
    u32 gravity;
    SpriteDataI1 *pBase;
    void *pVramData;
    u8 red;
    u8 green;
    u8 blue;
    u8 prim;
    s16 scale;
    s16 field_0x2e;
    s16 field_0x30;
    s16 direction;
    s16 curSpriteFrame;
    s16 field_0x36;
    s16 field_0x38;
    u16 field_0x3A;
    u32 field_0x3C_0 : 20;
    u32 field_0x3C_1 : 4;
    u32 field_0x3C_2 : 4;
    u32 field_0x3C_6 : 1;
    u32 field_0x3C_5 : 1;
    u32 field_0x3C_3 : 1;
    u32 field_0x3C_4 : 1;
    u32 field_0x40;
    void *pCurAnimFile;
    void *pDefaultAnimFile;
    void *pSpecialAnimFile;
    u32 field_0x50;
    u32 field_0x54;
    void *pCurAnimation;
    u32 field_0x5C;
    u32 field_0x60;
    void *pSpriteBytecode;
    void *field_0x68;
    void *field_0x6C;
    void *field_0x70;
    void *pTargetEntitySprite;
    u32 field_0x78;
    void *field_0x7C;
    s16 field_0x80;
    s16 field_0x82;
    s16 field_0x84;
    s16 allocatedDataSize;
    void *field_0x88;
    u8 stackIndex;
    u8 field_0x8D;
    u8 stack[16];
    s16 animScriptWaitTimer;
    s16 field_0xA0;
    s16 field_0xA2;
    s16 field_0xA4;
    s16 field_0xA6;
    s32 flags_0xA8;
    s32 flags_0xAC;
    s32 flags_0xB0;
    SpriteDataI1 inner1;
    SpriteDataI3 inner3;
    SpriteAnimPackage i2CurPackage;
    SpriteDirectionTransforms directionTransforms;
} SpriteData;

#endif /* ANIMATION_SPRITE_RECORDS_H */

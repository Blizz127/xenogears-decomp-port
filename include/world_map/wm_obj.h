#ifndef WORLD_MAP_WM_OBJ_H
#define WORLD_MAP_WM_OBJ_H

#include "common.h"

/* One slot of the world-map object pool (D_8009BE24, 0x40 slots of 0x80
 * bytes, allocated with HeapAlloc(0x2000)). Field names are placeholders
 * for the offsets the matched code touches; the rest is padding.
 *
 * No pointer-typed members: guest pointers inside a slot must stay
 * u32/s32 so the slot is 0x80 bytes on the 64-bit port too, and
 * &D_8009BE24[i] strides exactly as on the PSX. */
typedef struct WmObj {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ u8 unk8[0x10];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[4];
    /* 0x28 */ s32 pos[3];
    /* 0x34 */ u8 unk34[4];
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 unk44[8];
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ u8 unk5C[0x24];
} WmObj; /* size 0x80 */

/* Compile-time size check both the PSX compiler and the port accept. */
typedef char WmObjSizeIs0x80[(sizeof(WmObj) == 0x80) ? 1 : -1];

/* libgte-shaped helpers the world-map code uses on raw memory (the
 * scratchpad, the 0x54-byte records): VECTOR, SVECTOR and a 32-byte MATRIX. */
typedef struct { s32 vx, vy, vz, pad; } WmVec;
typedef struct { s16 vx, vy, vz, pad; } WmSVec;
typedef struct { s32 m[8]; } WmMat;

#endif /* WORLD_MAP_WM_OBJ_H */

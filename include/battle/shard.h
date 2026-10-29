/* The battle shard burst: a [2][14][20] grid of 0x7C-byte shards at +0x3C of
 * a timer task's +4 block, updated by func_800B6F0C (mainc95.c) and drawn by
 * func_800B7160 (main96.c).  Each shard has a rotation (+0) and its per-tick
 * delta (+8), a position (+0x10), a double-buffered flat-textured triangle
 * (+0x24; the buffer index is D_800C3EB0 + 0x8C84), a start delay (+0x64), a
 * 16.16 velocity (+0x68) and a gravity term (+0x78). */
#ifndef BATTLE_SHARD_H
#define BATTLE_SHARD_H

#include "psyq/libgte.h"

/* POLY_FT3's layout (psyq/libgpu.h is not included by these TUs). */
typedef struct {
    u32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
} BattleShardPoly;

typedef struct {
    SVECTOR rot;
    SVECTOR drot;
    VECTOR pos;
    s32 unk20;
    BattleShardPoly prims[2];
    s32 delay;
    VECTOR vel;
    s32 gravity;
} BattleShard;

typedef struct {
    u8 pad[0x38];
    s32 frame;
    BattleShard grid[2][14][20];
} BattleShardSet;

#endif

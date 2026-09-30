/* Retail TU part 1 of main73.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-1 C bodies. */
#define BATTLE_TU_PART 1
#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgte.h"

/* A 0x7C-byte node of the list func_800AF180 / func_800AF270 walk. */
typedef struct BattleNode7C {
    struct BattleNode7C* parent;
    u8 unk4[3];
    u8 pending;
    u8 unk8[2];
    u16 index;
    u8 unkC[0x64];
    s32 handle70;
    s32 handle74;
    s32 handle78;
} BattleNode7C;

void func_800A23E8(s32, s32);

/* Distance from the halfword point at +0x88/+0x8A/+0x8C to the word position
 * at (+0x4)->+0x5C/+0x60/+0x64. */
s32 func_800AEEF8(u8* obj) {
    s32* pos = *(s32**)(obj + 4);
    s32 dx = *(s16*)(obj + 0x88) - pos[0x5C / 4];
    s32 dy = *(s16*)(obj + 0x8A) - pos[0x60 / 4];
    s32 dz = *(s16*)(obj + 0x8C) - pos[0x64 / 4];

    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}

INCLUDE_ASM("asm/battle/nonmatchings/main73_p1", func_800AEF68);

/* Move node `index`'s pending flag from `src` to `dst`, release its three
 * handles, then recurse into every later node whose parent it is. */
void func_800AF180(s32 ctx, s32 index, BattleNode7C* src, BattleNode7C* dst) {
    s32 i;
    s32 count;
    BattleNode7C* p;

    p = src;
    count = src->index;
    src[index].pending = 0;
    dst[index].pending = 1;
    func_800A23E8(ctx, src[index].handle70);
    src[index].handle70 = 0;
    func_800A23E8(ctx, src[index].handle74);
    src[index].handle74 = 0;
    func_800A23E8(ctx, src[index].handle78);
    src[index].handle78 = 0;
    for (i = 1; i < count; i++) {
        p++;
        if (p->parent == &src[index]) {
            func_800AF180(ctx, p->index, src, dst);
        }
    }
}

/* A 0x7C-byte node of the list func_800AF180 / func_800AF270 walk. */

/* Move every later node's pending flag from src to dst. */
void func_800AF270(BattleNode7C* src, BattleNode7C* dst) {
    BattleNode7C* p = src;
    s32 i;
    s32 count = src->index;

    for (i = 1; i < count; i++) {
        p++;
        dst++;
        if (p->pending) {
            p->pending = 0;
            dst->pending = 1;
        }
    }
}


/* n = OuterProduct12(a, b); returns ((dot(vec, n) * 16) / (|n| + 1) << 8)
 * / divisor as an s16. */
s16 func_800AF2C4(VECTOR* vec, VECTOR* a, VECTOR* b, s32 divisor) {
    VECTOR n;
    s32 dot;
    s32 len;

    OuterProduct12(a, b, &n);
    dot = n.vx * vec->vx + n.vy * vec->vy + n.vz * vec->vz;
    len = SquareRoot0(n.vx * n.vx + n.vy * n.vy + n.vz * n.vz) + 1;
    return ((dot * 16) / len << 8) / divisor;
}
#endif
#include "main73.c"

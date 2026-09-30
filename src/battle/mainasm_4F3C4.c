/* battle mainasm_4F3C4: the retail-asm run at file offset 0x4F3C4, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];

/* scan 11 mask bits; for each set bit whose slot record pointer
 * (D_800C3EB0 + 0x8C8C)[i] is non-null, store value at record +0x74 and
 * append the record to out; out[count] = NULL; return count. */
s32 func_800BEEB4(u32 mask, u8** out, s32 value) {
    s32 i;
    s32 count;
    s32 off;
    s32 n;
    u8* p;
    u8** dst;
    u8* rec;

    count = i = 0;
    off = 0x8C8C;
    n = 11;
    p = D_800C3EB0;
    dst = out;
    while (i != n) {
        if (mask & 1) {
            rec = *(u8**)(p + off);
            if (rec != NULL) {
                *(s32*)(rec + 0x74) = value;
                *dst++ = rec;
                count++;
            }
        }
        p += 4;
        i++;
        mask = (u16)mask >> 1;
    }
    out[count] = NULL;
    return count;
}

typedef struct {
    s16 x;
    s16 z;
} PairXZ;

extern s16 func_80023124(PairXZ a, PairXZ b);

/* func_80023124(b's (x, z), a's (x, z)) with x at +2 and z at +0xA. */
s16 func_800BEF24(s16* a, s16* b) {
    PairXZ pa;
    PairXZ pb;
    s32 ax, az, bx, bz;

    ax = a[1];
    pa.x = ax;
    az = a[5];
    pa.z = az;
    bx = b[1];
    pb.x = bx;
    bz = b[5];
    pb.z = bz;
    return func_80023124(pb, pa);
}

INCLUDE_ASM("asm/battle/nonmatchings/mainasm_4F3C4", func_800BEF8C);
extern u8* D_800C3610[];
extern u8 D_800C3EB0[];
extern void func_800245D8(void* pSpriteData, s16 animIndex);

/* switch the D_800C3610 view to slot `slot`: if the previous record is
 * shown and its side entry (D_800C3EB0, 0x1C stride, selected by the
 * +0xAC/+0xA8 bits) is not locked (+7), replay its animation (+0xB0);
 * then record the slot (+0x24) and its record pointer (+4). */
void func_800BEFF4(s32 slot) {
    u8* rec = *(u8**)(D_800C3610[0] + 4);

    if (rec != NULL && *(s32*)(D_800C3610[0] + 0x24) != slot) {
        u8* base = D_800C3EB0;
        u32 top = *(u32*)(rec + 0xA8) >> 30;
        u32 idx = ((*(u32*)(rec + 0xAC) & 3) << 2) | top;

        if (base[idx * 0x1C + 7] == 0) {
            func_800245D8(rec, *(s8*)(rec + 0xB0));
        }
    }
    {
        u8* cur = D_800C3610[0];
        u8* b = D_800C3EB0;

        *(s32*)(cur + 0x24) = slot;
        *(u8**)(cur + 4) = *(u8**)(slot * 4 + b + 0x8C8C);
    }
}

#endif

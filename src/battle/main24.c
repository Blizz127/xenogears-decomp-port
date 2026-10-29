#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
/* Declared u32 as in main26/main28/mainasm_D854.  func_8007BD5C keeps the
 * result unmasked and masks it at each use; it matches with this
 * declaration and a u32 `r`, and also with a u8 declaration and a u8 `r`
 * (a u8 declaration with a u32 `r` masks it once at the call). */
extern u32 func_8001BD40(u32 lo, u8 hi);

void func_8007BC84(u8 **pp, s32 idx) {
    u8 v = func_8001BD40(0, (*pp)[2]);
    u32 row = (u32)D_800D3430 + ((idx & 0xFF) << 6);

    *(u8 *)(row + (*pp)[1]) = v;
}

extern u8 D_800D3420[];
extern u16 func_80089B50(u16 lo, u16 hi);

/* func_8007BCE8.s: store func_80089B50(0, p[2] | p[3] << 8), a random value
 * in [0, that bound], in halfword slot p[1] of row idx of D_800D3420 (64-byte
 * rows).  The bound is built in place (hi = p[2], then hi |= p[3] << 8):
 * retail ors into p[2]'s argument register.  The one-expression form (as
 * in func_8007B3E4) puts p[3] << 8 there instead. */
void func_8007BCE8(u8** pp, s32 idx) {
    u16* row;
    u16 hi = (*pp)[2];
    u16 v;

    hi |= (*pp)[3] << 8;
    v = func_80089B50(0, hi);
    row = (u16*)(D_800D3420 + ((idx & 0xFF) << 6));
    row[(*pp)[1]] = v;
}

extern u8 D_800D32A1[][8];
extern u8 D_800C3EB4[][28];
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);

/* func_8007BD5C.s, byte-exact: battle-script op on p = *ppBoard.  Clear
 * halfword p[1] of row `index` of D_800D3420, then pick slots
 * r = func_8001BD40(0, 2) not tried yet: when func_8007A628(r, p[2])
 * accepts r and its D_800D32A1 byte is clear, store func_80089C08(r) in
 * that halfword and return; otherwise mark r tried, until all three are.
 * The func_8007D478 form (mainasm_D854). */
void func_8007BD5C(u8** ppBoard, u8 index) {
    u8 tried[3];
    u16* row;
    u16* dst;
    u16 v;
    u32 r;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    tried[0] = tried[1] = tried[2] = 0;
    row[(*ppBoard)[1]] = 0;
    while (!(tried[0] & tried[1] & tried[2])) {
        r = func_8001BD40(0, 2);
        if (tried[(u8)r] == 0) {
            if ((u8)func_8007A628((u8)r, (*ppBoard)[2]) != 0 && D_800D32A1[(u8)r][0] == 0) {
                v = func_80089C08(r);
                dst = (u16*)(D_800D3420 + ((u32)index << 6));
                dst[(*ppBoard)[1]] = v;
                return;
            }
            tried[(u8)r] = 1;
        }
    }
}
/* The 64-byte enemy AI records at D_800D3400 (index = actor - 3): +0x00
 * the script func_800799C8 runs, +0x08 / +0x0C the ones func_80079AB0 /
 * func_80079C24 run, then the battle-script ops' slots: four words at
 * +0x10, eight halfwords at +0x20 and sixteen bytes at +0x30. */
typedef struct {
    u8* script;
    u8 pad4[4];
    u8* script8;
    u8* scriptC;
    u32 w[4];
    u16 h[8];
    u8 b[16];
} EnemyAiRec;
extern EnemyAiRec D_800D3400__rec[] asm("D_800D3400");

/* func_8007BEA8.s, byte-exact: battle-script op on p = *ppBoard.  Clear
 * halfword slot p[1] of enemy `index`'s D_800D3400 record, then draw
 * r = func_8001BD40(0, 2) not tried yet: when func_8007A628(r, p[2])
 * accepts r, r's group (byte 0 of its 28-byte D_800C3EB4 row) is the same as
 * that of slot index + 3 and r's D_800D32A1 byte is clear, store
 * func_80089C08(r) in the slot and return; otherwise mark r tried, until
 * all three are.  func_8007BD5C plus the group test.
 * Codegen: the slot goes through the D_800D3400 record view (field h at
 * +0x20).  A `D_800D3420 + index * 64` row pointer instead gets its `la`
 * scheduled ahead of the p[1] load into another register; through the
 * record, retail's `la` lands after that load, in p's register.  Matches
 * under gcc 2.6.x (main24's preset) and 2.7.2. */
void func_8007BEA8(u8** ppBoard, u8 index) {
    u8 tried[3];
    u16 v;
    u32 r;

    tried[0] = tried[1] = tried[2] = 0;
    D_800D3400__rec[index].h[(*ppBoard)[1]] = 0;
    while (!(tried[0] & tried[1] & tried[2])) {
        r = func_8001BD40(0, 2);
        if (tried[(u8)r] == 0) {
            if ((u8)func_8007A628((u8)r, (*ppBoard)[2]) != 0
                && D_800C3EB4[index + 3][0] == D_800C3EB4[(u8)r][0]
                && D_800D32A1[(u8)r][0] == 0) {
                v = func_80089C08(r);
                D_800D3400__rec[index].h[(*ppBoard)[1]] = v;
                return;
            }
            tried[(u8)r] = 1;
        }
    }
}
/* func_8007C040.s / func_8007C33C.s, byte-exact: battle-script ops on
 * p = *ppBoard.  Clear halfword p[1] of row `index` of D_800D3420, collect
 * the slots i in 3..10 that func_8007A628(i, p[2]) accepts, whose
 * D_800D32A1 byte is clear and whose group (byte 0 of the 28-byte
 * D_800C3EB4 row) is the same as (func_8007C040) or differs from
 * (func_8007C33C) the group of slot index + 3, and store func_80089C08 of
 * a random one (func_8001BD40(0, count - 1)) in that halfword.  gcc 2.6.x
 * code (main24 is BattleGcc263); the func_8007D344 form. */
void func_8007C040(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 3;
    s32 n = 0;
    u16* row;
    u16* dst;
    u16 v;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[1]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A628((u8)i, (*ppBoard)[2]) != 0
            && D_800C3EB4[index + 3][0] == D_800C3EB4[i][0]
            && D_800D32A1[i][0] == 0) {
            list[n] = i;
            n++;
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[1]] = v;
    }
}
/* func_8007C1A4.s, byte-exact: battle-script op on p = *ppBoard.  Clear
 * halfword slot p[1] of enemy `index`'s D_800D3400 record, then draw
 * r = func_8001BD40(0, 2) not tried yet: when func_8007A628(r, p[2])
 * accepts r, r's group (byte 0 of its 28-byte D_800C3EB4 row) differs from
 * that of slot index + 3 and r's D_800D32A1 byte is clear, store
 * func_80089C08(r) in the slot and return; otherwise mark r tried, until
 * all three are.  func_8007BD5C plus the group test.
 * Codegen: the slot goes through the D_800D3400 record view (field h at
 * +0x20).  A `D_800D3420 + index * 64` row pointer instead gets its `la`
 * scheduled ahead of the p[1] load into another register; through the
 * record, retail's `la` lands after that load, in p's register.  Matches
 * under gcc 2.6.x (main24's preset) and 2.7.2. */
void func_8007C1A4(u8** ppBoard, u8 index) {
    u8 tried[3];
    u16 v;
    u32 r;

    tried[0] = tried[1] = tried[2] = 0;
    D_800D3400__rec[index].h[(*ppBoard)[1]] = 0;
    while (!(tried[0] & tried[1] & tried[2])) {
        r = func_8001BD40(0, 2);
        if (tried[(u8)r] == 0) {
            if ((u8)func_8007A628((u8)r, (*ppBoard)[2]) != 0
                && D_800C3EB4[index + 3][0] != D_800C3EB4[(u8)r][0]
                && D_800D32A1[(u8)r][0] == 0) {
                v = func_80089C08(r);
                D_800D3400__rec[index].h[(*ppBoard)[1]] = v;
                return;
            }
            tried[(u8)r] = 1;
        }
    }
}
void func_8007C33C(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 3;
    s32 n = 0;
    u16* row;
    u16* dst;
    u16 v;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[1]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A628((u8)i, (*ppBoard)[2]) != 0
            && D_800C3EB4[index + 3][0] != D_800C3EB4[i][0]
            && D_800D32A1[i][0] == 0) {
            list[n] = i;
            n++;
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[1]] = v;
    }
}
#endif


#ifndef XENO_PC_PORT
extern u8 D_800D2E06[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E0C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);


/* func_8007C4A0.s */
void func_8007C4A0(u8** ppBoard, u8 index) {
    u32 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 0;
    u8* s1 = D_800D2E06;

    for (; i < 3; i++, s1 += 2) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            s16 v = *(s16*)s1;

            if ((s32)best >= (s32)v) {
                best = *s1;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007C580.s */
void func_8007C580(u8** ppBoard, u8 index) {
    u32 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 3;
    s32 skip = (index & 0xFF) + 3;
    u8* s1 = D_800D2E0C;

    for (; i < 0xB; i++, s1 += 2) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            s16 v = *(s16*)s1;

            if ((s32)best >= (s32)v && i != skip) {
                best = *s1;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007C678.s */
void func_8007C678(u8** ppBoard, u8 index) {
    u32 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    u32 off;

    for (i = 0, off = 0; i < 3; i++, off += 0x170) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            u32 v = *(u16*)(D_800CCD34 + off);

            if (best >= v) {
                best = v;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007C75C.s */
void func_8007C75C(u8** ppBoard, u8 index) {
    u32 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    u32 off;

    for (i = 3, off = 0x450; i < 0xB; i++, off += 0x170) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            u32 v = *(u16*)(D_800CCD34 + off);

            if (best >= v) {
                best = v;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}

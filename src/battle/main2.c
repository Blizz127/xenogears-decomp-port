#include "common.h"
#ifndef XENO_PC_PORT
void AddPrim(s32, void *);
extern void *D_800C3E24;
extern s32 D_800CCB04;
extern void * D_800D2D28;
void func_80034888(s32, s32, s32);
extern s32 D_800CCB34;
extern s32 D_800D2DAC;
extern void *D_800D3278;
void func_80074D4C(void);
void func_80074F70(void);
#endif

#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif


#ifndef XENO_PC_PORT
void func_800728B8(void *prims, s32 count, s32 start);
extern void *D_800D2D28;
extern void *D_800D2DB4;
void func_80074AB8(void);
void func_80074EEC(void);
#endif

#ifndef XENO_PC_PORT
extern u8 D_800D2CAF;
extern u16 D_800D2C94;
extern u16 D_800C48E8;
extern u8* D_800C3EAC;
extern u8 D_800D3014;
extern void func_80079E18(u8 index);
extern void func_80079E4C(u8 index);
extern void func_800716D8(void);

extern u8 D_800C3E8C;
extern u8 D_800D3298;
extern u8 D_800C4922;
extern u8 D_800D2CA4[];
extern u16 D_800CCD64__rows[][184] asm("D_800CCD64");
extern u16 D_800CCD68__rows[][184] asm("D_800CCD68");
extern u16 D_800CCD1C__rows[][184] asm("D_800CCD1C");
extern u8 D_800C3EB4__r[][28] asm("D_800C3EB4");
extern u8 D_800C3E3D[];
extern s32 D_800C3DDC;
extern u8 D_800D3718[];
extern u8 D_800C3D1A[];
extern u16 D_800C3254[];
extern u8 D_800D3280;
extern s32 D_800CCB04__a_s32[] asm("D_800CCB04");
extern u8* D_800C3EA4__b asm("D_800C3EA4");
extern u16 D_8005941C;
extern u8 D_800D36C0;
extern u8 D_800C48EA[];
extern void func_80085350(void);
extern void func_80071A08(void);
extern u16 func_80099890(u8 actor);
extern void func_800799C8(u8 actor, s32 flag);
extern u8* GetStringEntry(s32 table, s32 id);
extern s32 SystemRenderStringEntry(u8* str, u8* buf, s32 w, s32 flags);
extern void func_800769E8(u8* rect, u8* buf);
extern void func_80079778(u8 actor);
extern void func_80071A8C(void);
extern void func_80071964(void);
extern void func_80070EB0(s32 arg);
extern void func_80080160(u8 actor);
extern void func_80080C94(u8 actor);
extern void func_800BCD98(s32 arg);
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80079C24(void);
extern void func_80071A38(void);
void func_80072270(void);
extern u8 func_800841E0(u8 idx);
extern u8 func_80085310(u8 idx, u8 v);
extern s32 func_80080AE4(u8 actor);
extern void func_800BA4E0(s32 arg);
void func_8007252C(void);
extern void func_80085B58(u8 actor);
extern void func_800BFE48(void);
extern void func_800718BC(void);

extern u16 D_800D2C9E;
extern u16 D_800D2E06[];
extern u8 D_800D2DE4[];
extern u16 func_80089C9C(s32 mask, u8 idx);

extern u16 D_800D39E0;
extern u8 D_800C402F[];
extern u8 D_800D2E5C;
extern u8 D_800D2E60;
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80085AC4(u8 a0);
extern void func_80071B94(s32 a0);

typedef struct {
    u16 unk0;
    u8 unk2[0x2E];
    u16 unk30;
    u16 unk32;
    u16 unk34;
    u8 unk36[0x82];
    u32 unkB8;
    u8 unkBC[0x18];
    u16 unkD4;
    u8 unkD6[0x9A];
} M2Actor; /* 0x170 */
typedef struct {
    u8 unk0[3];
    u8 unk3;
    u8 unk4;
    u8 unk5[0x17];
} M2Group; /* 0x1C */
extern M2Actor D_800CCD34[];
extern M2Group D_800C3EB4[];
extern u8 D_800D2DCC[];
extern u8 D_800D2DE4[];
extern u16 D_800D39DC;
extern u16 D_800C3608;
extern u8 D_800C48EA[];
extern u8 D_800C3D0C__b[] asm("D_800C3D0C");
extern u16 func_80089C08(u8 idx);
extern u16 func_80089C9C(s32 mask, u8 idx);
extern u16 func_80089C48(u8 idx);

/* Party (0..2) and enemy (3..10) status scan: sets D_800D39DC bits and the
 * battle-end flag D_800C48EA.  The enemy loop walks explicit byte offsets:
 * `ao` = i * sizeof(M2Actor), `go` = i * sizeof(M2Group), and the mask loop
 * reuses `go` as its actor offset.  Retail's registers need that reuse
 * (the shared offset outranks `k` in global alloc) and the for-init order;
 * the volatile D_800D2DE4 stores keep loop.c from turning the two stores
 * into a pointer giv. */
void func_8007252C(void) {
    s32 i;
    s32 k;
    s32 mask;
    u8* q;
    s32 ff;
    s32 go;
    s32 ao;

    D_800D39DC = 0;
    for (i = 0; i < 3; i++) {
        if (D_800D2DCC[i] != 0) {
            if (D_800CCD34[i].unk30 & 0xC000) {
                if (D_800CCD34[i].unk30 & 0x8000) {
                    D_800CCD34[i].unk0 = 0;
                }
                D_800D2DE4[i] = 0xFF;
            } else if (D_800C3EB4[i].unk3 == 0) {
                D_800D39DC |= func_80089C08(i);
            }
        }
    }
    for (i = 3, ff = 0xFF, q = D_800C3D0C__b + 15, k = 0, ao = 0x450, go = 0x54; i < 11; q += 4, k++, ao += 0x170, i++, go += 0x1C) {
        if (D_800D2DCC[k + 3] != 0) {
            if (((u8*)D_800C3EB4 + 4)[go] == 0) {
                if (*(u16*)((u8*)D_800CCD34 + 0x30 + ao) & 0xC000) {
                    if (func_80089C9C(D_800C3608, i) == 0) {
                        *(u16*)((u8*)D_800CCD34 + ao) = 0;
                        *(volatile u8*)&D_800D2DE4[k + 3] = ff;
                        continue;
                    }
                    D_800D39DC |= func_80089C08(i);
                    continue;
                }
            } else {
                if (*(u16*)((u8*)D_800CCD34 + 0xD4 + ao) & 0xC000) {
                    if (func_80089C9C(D_800C3608, i) == 0) {
                        *(u32*)((u8*)D_800CCD34 + 0xB8 + ao) = 0;
                        *(volatile u8*)&D_800D2DE4[k + 3] = ff;
                        continue;
                    }
                    D_800D39DC |= func_80089C08(i);
                    continue;
                }
            }
            if (((u8*)D_800C3EB4 + 3)[go] == 0 && *q == 0) {
                D_800D39DC |= func_80089C08(i);
            }
        }
    }
    if (!(D_800D39DC & 0x7F8)) {
        D_800C48EA[0] = 1;
    }
    if (!(D_800D39DC & 0x7)) {
        D_800C48EA[0] = 0x81;
    }
    if (D_800C48EA[0] == 0) {
        mask = D_800D39DC;
        for (i = 0, go = 0; i < 11; i++, go += 0x170) {
            if (func_80089C9C(mask, i) && (*(u16*)((u8*)D_800CCD34 + 0x34 + go) & 0x1000)) {
                mask &= func_80089C48(i);
            }
        }
        if (mask == 0) {
            for (i = 0; i < 11; i++) {
                if (func_80089C9C(D_800D39DC, i) && (D_800CCD34[i].unk34 & 0x1000)) {
                    D_800CCD34[i].unk34 &= ~0x1000;
                    return;
                }
            }
        }
    }
}

#ifndef XENO_PC_PORT
/* Link `count` primitives of 0x28 bytes, every other one from `start`. */
void func_800728B8(void* prims, s32 count, s32 start) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(D_800CCB04 + 4, (u8*)prims + (start + i * 2) * 0x28);
    }
}
#endif

#ifndef XENO_PC_PORT
extern void SetShadeTex(void* p, s32 tge);


/* func_80072938.s, byte-exact WITH A CONSTRUCT: for each primitive i from
 * start to end - 1, SetShadeTex(prims + (i * 2 + D_800CCB34) * 0x28, 0)
 * and set its colour bytes +4/+5/+6 to (0x80, 0, 0), or (0x80, 0x80, 0) for
 * mode 1.  Construct (decomp-permuter result, not the original code): the
 * last store sits in `if (end) ... else ...` with identical arms; the
 * branch only adds the block boundary that gives retail's register
 * allocation (without it the last block's registers differ: 348 vs 356).
 * Counted apart from natural matches. */
void func_80072938(void* prims, s32 start, s32 end, u8 mode) {
    s32 i;
    s32 k;

    for (i = start; i < end; i++) {
        k = i * 2;
        SetShadeTex((u8*)prims + (k + D_800CCB34) * 0x28, 0);
        if (mode != 1) {
            ((u8*)prims + (k + D_800CCB34) * 0x28)[4] = 0x80;
            ((u8*)prims + (k + D_800CCB34) * 0x28)[5] = 0;
        } else {
            ((u8*)prims + (k + D_800CCB34) * 0x28)[4] = 0x80;
            ((u8*)prims + (k + D_800CCB34) * 0x28)[5] = 0x80;
        }
        /* CONSTRUCT (not the original code): the two identical arms only add
         * the block boundary retail's register allocation implies. */
        if (end) {
            ((u8*)prims + (k + D_800CCB34) * 0x28)[6] = 0;
        } else {
            ((u8*)prims + (k + D_800CCB34) * 0x28)[6] = 0;
        }
    }
}
#endif
extern void* D_800C3EA4;
extern u16 D_800C3254[];
extern u8 D_800D3280;
extern u8 D_800D2D88[];
extern u16 D_800C3076[];
extern s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y);
extern u8 D_800C3E08[];
extern u8 D_800D2D54[];
extern u16 D_800C3068[];

/* func_80072A9C.s, byte-exact: draw slot `slot`'s three-part label, the
 * sibling of func_80072DA8 (same prims, count and x formula).  Glyphs
 * D_800C3E08[0..2] that are not 0xFF go at x index i, glyph 0x71 at x
 * index 3, then the D_800D2D54[4..6] glyphs that are not 0xFF at x
 * indexes 4 upward (one per drawn glyph).  Each func_80076A10 call adds
 * its sprite count to the slot's D_800D2D28 +0xE0 word; with `flag`,
 * func_80072938 then colours the prims from the old count to the new
 * one.  x = D_800C3068[slot * 24 + index] + D_800C3254[D_800D3280 * 3 +
 * slot], y = 0x10.  Matches under gcc 2.6.x. */
void func_80072A9C(s32 slot, u8 flag) {
    s32 start;
    s32 i;
    s32 k;

    start = *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0);
    for (i = 0; i < 3; i++) {
        if (D_800C3E08[i] != 0xFF) {
            *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) += func_80076A10(D_800C3E08[i] + 0x67,
                (u8*)D_800C3EA4 + (slot * 0xC80 + 0x3A88) + *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) * 80,
                D_800C3068[slot * 24 + i] + D_800C3254[D_800D3280 * 3 + slot], 0x10);
        }
    }
    *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) += func_80076A10(0x71,
        (u8*)D_800C3EA4 + (slot * 0xC80 + 0x3A88) + *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) * 80,
        D_800C3068[slot * 24 + 3] + D_800C3254[D_800D3280 * 3 + slot], 0x10);
    for (i = 4, k = 4; i < 7; i++) {
        if (D_800D2D54[i] != 0xFF) {
            *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) += func_80076A10(D_800D2D54[i] + 0x67,
                (u8*)D_800C3EA4 + (slot * 0xC80 + 0x3A88) + *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) * 80,
                D_800C3068[slot * 24 + k] + D_800C3254[D_800D3280 * 3 + slot], 0x10);
            k++;
        }
    }
    if (flag) {
        func_80072938((u8*)D_800C3EA4 + (slot * 0xC80 + 0x3A88), start, *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0), flag);
    }
}

/* func_80072DA8.s, byte-exact: draw the five-glyph label of slot `slot`.
 * For each D_800D2D88 glyph that is not 0xFF, func_80076A10(glyph + 0x67,
 * the slot's prims at D_800C3EA4 + 0x3A88 + slot * 0xC80 + count * 80,
 * x = D_800C3076[slot * 24 + i] + D_800C3254[D_800D3280 * 3 + slot], y =
 * 0x10) adds its sprite count to the slot's D_800D2D28 +0xE0 word.  With
 * `flag`, func_80072938 then colours the prims from the old count to the
 * new one.  Codegen: the record offset is parenthesised (retail hoists
 * slot * 0xC80 + 0x3A88 as one value); func_80076A10 takes an s32 glyph
 * as its main4 definition does (main2 declared u8, which masks the
 * argument; retail passes glyph + 0x67 unmasked). */
void func_80072DA8(s32 slot, u8 flag) {
    s32 start;
    s32 i;

    start = *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0);
    for (i = 0; i < 5; i++) {
        if (D_800D2D88[i] != 0xFF) {
            *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) += func_80076A10(D_800D2D88[i] + 0x67,
                (u8*)D_800C3EA4 + (slot * 0xC80 + 0x3A88) + *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0) * 80,
                D_800C3076[slot * 24 + i] + D_800C3254[D_800D3280 * 3 + slot], 0x10);
        }
    }
    if (flag) {
        func_80072938((u8*)D_800C3EA4 + (slot * 0xC80 + 0x3A88), start, *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xE0), flag);
    }
}
extern s16 D_800CCD34__r[][184] asm("D_800CCD34");
extern s16 D_800CCD36__r[][184] asm("D_800CCD36");
extern s32 D_800CCDEC__r[][92] asm("D_800CCDEC");
extern s32 D_800CCDF0__r[][92] asm("D_800CCDF0");
extern u16 D_800D3330;
extern u16 D_800D2FE0;
extern u16 D_800D2E58;
extern u16 D_800D2D38;
extern s32 D_800D333C;
extern s32 D_800D3018;
extern s32 D_800D3668;
extern u8 D_800C3E08[];
extern u8 D_800D2D58[];
extern u8 D_800D2D88[];


/* func_80072F38.s, byte-exact under gcc 2.6.3 (BattleGcc263).  Cache actor
 * `actor`'s +0x4C/+0x4E halfwords and +0x104/+0x108 words in the HUD
 * globals; grade the +0x104/+0x108 pair (mp != 0) or the +0x4C/+0x4E pair as
 * 2 (current at most 1/8 of the maximum), 1 (at most 1/4) or 0; split the
 * +0x4C and +0x4E copies into three decimal digits (D_800C3E08, D_800D2D58)
 * and the +0x104 copy into five (D_800D2D88), in place; blank leading zeros
 * with 0xFF (the last digit stays) and return the grade.  Codegen: one s16 /
 * s32 local per digit, each byte store right after its digit (storing the
 * digits at the end loses the in-place global updates, 1048 vs 1096). */
s32 func_80072F38(s32 actor, u8 mp) {
    s16 max = D_800CCD36__r[actor][0];
    s16 cur = D_800CCD34__r[actor][0];
    s32 val = D_800CCDEC__r[actor][0];
    s32 cap = D_800CCDF0__r[actor][0];
    s32 level;
    s16 h0;
    s16 h1;
    s16 m0;
    s16 m1;
    s32 p0;
    s32 p1;
    s32 p2;
    s32 p3;
    s32 i;

    D_800D3330 = max;
    D_800D2FE0 = max;
    D_800D2E58 = cur;
    D_800D2D38 = cur;
    D_800D333C = val;
    D_800D3018 = val;
    D_800D3668 = cap;
    level = 0;
    if (mp) {
        if (cap / 8 >= val) {
            level = 2;
        } else if (cap / 4 >= val) {
            level = 1;
        }
    } else {
        if (max / 8 >= cur) {
            level = 2;
        } else if (max / 4 >= cur) {
            level = 1;
        }
    }
    h0 = (s16)D_800D2D38 / 100;
    D_800D2D38 -= h0 * 100;
    D_800C3E08[0] = h0;
    h1 = (s16)D_800D2D38 / 10;
    D_800D2D38 -= h1 * 10;
    D_800C3E08[1] = h1;
    D_800C3E08[2] = D_800D2D38;
    m0 = (s16)D_800D2FE0 / 100;
    D_800D2FE0 -= m0 * 100;
    D_800D2D58[0] = m0;
    m1 = (s16)D_800D2FE0 / 10;
    D_800D2FE0 -= m1 * 10;
    D_800D2D58[1] = m1;
    D_800D2D58[2] = D_800D2FE0;
    p0 = D_800D3018 / 10000;
    D_800D3018 -= p0 * 10000;
    D_800D2D88[0] = p0;
    p1 = D_800D3018 / 1000;
    D_800D3018 -= p1 * 1000;
    D_800D2D88[1] = p1;
    p2 = D_800D3018 / 100;
    D_800D3018 -= p2 * 100;
    D_800D2D88[2] = p2;
    p3 = D_800D3018 / 10;
    D_800D3018 -= p3 * 10;
    D_800D2D88[3] = p3;
    D_800D2D88[4] = D_800D3018;
    for (i = 0; i < 2; i++) {
        if (D_800C3E08[i] != 0) {
            break;
        }
        D_800C3E08[i] = 0xFF;
    }
    for (i = 0; i < 2; i++) {
        if (D_800D2D58[i] != 0) {
            break;
        }
        D_800D2D58[i] = 0xFF;
    }
    for (i = 0; i < 4; i++) {
        if (D_800D2D88[i] != 0) {
            break;
        }
        D_800D2D88[i] = 0xFF;
    }
    return level;
}
extern u8 D_800C3CF9[];
extern u8 D_800D32A0[];
extern u16 D_800CCDC4__rows[][184] asm("D_800CCDC4");
extern void func_8008AAA0(s16 value);

/* func_80073380.s, byte-exact: draw slot `slot`'s four-digit value.
 * func_8008AAA0 splits the value into the D_800C3CF4 digit bytes: the
 * D_800D32A0 byte (8-byte rows) when the slot's D_800C3EA4 byte at
 * +0x853D + slot * 0x1E4 is 1, else its D_800CCDC4 halfword.  Each digit
 * of D_800C3CF9[0..3] that is not 0xFF is drawn with
 * func_80076A10(digit + 0x83, the slot's prims at D_800C3EA4 + 0x6008 +
 * slot * 0x140 + count * 80, x = slot * 96 + D_800C3254[D_800D3280 * 3 +
 * slot] + 0x4A + i * 6, y = 0x25), adding to the slot's D_800D2D28 +0xEC
 * count; finally D_800D2D28 +0x99 + slot = the low byte of D_800CCB34.
 * Codegen: the +0x853D test goes through a row pointer from a
 * block-local base (func_8009DA04), the value is converted once at the
 * shared func_8008AAA0 call, and the prims offset is parenthesised, as in
 * func_80072DA8. */
void func_80073380(s32 slot) {
    s32 i;
    u16 v;
    u8* base;
    u8* p;

    base = D_800C3EA4;
    p = base + slot * 0x1E4;
    if (p[0x853D] == 1) {
        v = D_800D32A0[slot * 8];
    } else {
        v = D_800CCDC4__rows[slot][0];
    }
    func_8008AAA0(v);
    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xEC) += func_80076A10(D_800C3CF9[i] + 0x83,
                (u8*)D_800C3EA4 + (slot * 0x140 + 0x6008) + *(s32*)((u8*)D_800D2D28 + slot * 4 + 0xEC) * 80,
                slot * 96 + (D_800C3254[D_800D3280 * 3 + slot] + 0x4A) + i * 6, 0x25);
        }
    }
    ((u8*)D_800D2D28 + slot)[0x99] = *(u8*)&D_800CCB34;
}
extern void* D_800C3EA4;
typedef struct {
    u8 state;
    u8 unk1[0x1B];
} BattleSlotState;
extern BattleSlotState D_800C3EB6__s[] asm("D_800C3EB6");
extern u8 D_800D32A1[];
extern u16 D_800C3254[];
extern u8 D_800D3280;
extern void* D_800D2F5C;
extern s32 D_800CCB34__a[] asm("D_800CCB34");
extern u8 D_800D2D24[];
extern s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y);
extern s32 func_80076A6C(u8 glyph, u8* prims, s16 x, s16 y);
extern void func_80076C34(u8* prim);
extern s32 func_80025FA8(void* font, s32 glyph, u8* prims, s32 buf, s32 x, s32 y, s32 sx, s32 sy, s32 a8);
extern void func_800765C4(s32 slot);
extern void func_80076710(s32 slot);

/* Party HUD per-slot state switch.  Shapes retail's codegen depends on:
 * `base` holds D_800D2D28 before `+ i` (retail adds base first);
 * D_800D32A1[i << 3] becomes its own loop.c pointer giv (la init, += 8);
 * the D_800CCB34 call arguments are read through the split-array alias,
 * which gives retail's a3-first argument loads. */
void func_80073538(void) {
    s32 i;
    s32 k;
    s32 scale = 0;
    u8* p;
    u8* base;

    *(u8*)(D_800D2D28 + 0x77) = 0;
    for (i = 0; i < 3; i++) {
        if (D_800C3EB6__s[i].state == 0x7F) {
            continue;
        }
        switch ((p = (base = (u8*)D_800D2D28) + i)[0x90]) {
        case 0:
            p[0x74] = 0;
            *(u8*)(D_800D2D28 + i + 0x74) +=
                func_80076A10(0x8F, D_800C3EA4 + 0x2E08 + i * 0x320 + *(u8*)(D_800D2D28 + i + 0x74) * 0x50,
                              i * 0x60 + (D_800C3254[D_800D3280 * 3 + i] + 0x44), 0x1F);
            *(u8*)(D_800D2D28 + i + 0x74) +=
                func_80076A6C(0x52, D_800C3EA4 + 0x2E08 + i * 0x320 + *(u8*)(D_800D2D28 + i + 0x74) * 0x50,
                              i * 0x60 + (D_800C3254[D_800D3280 * 3 + i] + 0x48), 0x1C);
            k = *(u8*)(D_800D2D28 + i + 0x74) * 2;
            *(u8*)(D_800D2D28 + i + 0x74) +=
                func_80076A6C(0x53, D_800C3EA4 + 0x2E08 + i * 0x320 + *(u8*)(D_800D2D28 + i + 0x74) * 0x50,
                              i * 0x60 + (D_800C3254[D_800D3280 * 3 + i] + 0x48), 0x1C);
            for (; k < *(u8*)(D_800D2D28 + i + 0x74) * 2; k += 2) {
                func_80076C34(D_800C3EA4 + 0x2E08 + i * 0x320 + (k + D_800CCB34) * 0x28);
            }
            *(u8*)(D_800D2D28 + i + 0x84) = D_800CCB34;
            *(u8*)(D_800D2D28 + i + 0xCC) = 1;
            break;
        case 1:
            scale = 0xC00;
            *(u8*)(D_800D2D28 + 0x77) =
                func_80025FA8(D_800D2F5C, 0x52, D_800C3EA4 + 0x3768, D_800CCB34__a[0], 0x10, 0x98, 0x1000, 0x1000, scale);
            k = *(u8*)(D_800D2D28 + 0x77) * 2;
            *(u8*)(D_800D2D28 + 0x77) += func_80025FA8(D_800D2F5C, 0x53,
                                                      D_800C3EA4 + 0x3768 + *(u8*)(D_800D2D28 + 0x77) * 0x50,
                                                      D_800CCB34__a[0], 0x10, 0x98, 0x1000, 0x1000, scale);
            for (; k < *(u8*)(D_800D2D28 + 0x77) * 2; k += 2) {
                func_80076C34(D_800C3EA4 + 0x3768 + (k + D_800CCB34) * 0x28);
            }
            *(u8*)(D_800D2D28 + 0x87) = D_800CCB34;
            break;
        case 3:
            if (D_800D32A1[i << 3] != 0 && D_800D2D24[i] != 7) {
                p[0xCC] = 0;
            } else {
                p[0x74] = 0;
            }
            func_800765C4(i);
        case 2:
            func_80076710(i);
            if (D_800D32A1[i << 3] == 0 || D_800D2D24[i] == 7) {
                *(u8*)(D_800D2D28 + i + 0x7F) = 0;
            }
            break;
        }
    }
}
extern void *D_800C3EA4;
extern s32 D_800CCB04__a_s32[] asm("D_800CCB04");

void func_80073A58(void) {
    if (*(u8 *)((s8*)(D_800C3EA4) + 0x6415) != 0) {
        *(u8 *)((s8*)(((*(u8 *)((s8*)(D_800C3EA4) + 0x6414) * 0x18) + D_800C3EA4)) + 0x63CC) = (u8) *(u8 *)((s8*)(D_800C3EA4) + 0x6410);
        *(u8 *)((s8*)(((*(u8 *)((s8*)(D_800C3EA4) + 0x6414) * 0x18) + D_800C3EA4)) + 0x63CD) = (u8) *(u8 *)((s8*)(D_800C3EA4) + 0x6410);
        *(u8 *)((s8*)(((*(u8 *)((s8*)(D_800C3EA4) + 0x6414) * 0x18) + D_800C3EA4)) + 0x63CE) = (u8) *(u8 *)((s8*)(D_800C3EA4) + 0x6410);
        AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8 *)((s8*)(D_800C3EA4) + 0x6414) * 0x18) + 0x63C8 + D_800C3EA4);
        AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8 *)((s8*)(D_800C3EA4) + 0x6414) * 0xC) + 0x63F8 + D_800C3EA4);
    }
}

extern u8 D_800C3EB6__rows[][28] asm("D_800C3EB6");

/* func_80073B64.s, byte-exact: add the battle HUD prims to the order
 * table.
 * - The D_800D2D28 +0x97 sprites from +0x98 (AddPrim, 16-byte stride at
 *   D_800C3EA4 + 0x908).
 * - Four func_800728B8(prims, count, start) strips at +0x2E08 + i * 0x320
 *   (counts +0x74, starts +0x84).
 * - Per active slot (+0xCC): the +0x853D-selected strip (kind 1 at
 *   +0x835C, kind 2 at +0x83FC) and the +0x6008 strip (+0xEC count, +0x99
 *   start).
 * - The +0x9C8 strip when +0xAF is set, and the three +0x3A88 label
 *   strips (+0xE0 counts, +0x93 starts).
 * - Per slot whose D_800C3EB6 row is not 0x7F: the +0x818 strip (one
 *   prim from +0x83), and for active slots the i * 0x1E0 strip and one
 *   D_800CCB34-buffer prim at +0x740.
 * Codegen: every offset is parenthesised and added to D_800C3EA4 last (as
 * in func_80072DA8); the D_800D2D28 bytes are read as (base + i)[off];
 * D_800C3EB6 goes through a row view; the +0x853D row pointer comes from
 * a block-local base (as in func_80073380).  Matches under gcc 2.6.x. */
void func_80073B64(void) {
    s32 i;
    u8* p;
    u8* base;

    for (i = 0; i < ((u8*)D_800D2D28)[0x97]; i++) {
        AddPrim(D_800CCB04 + 4, (u8*)D_800C3EA4 + ((((u8*)D_800D2D28)[0x98] + i * 2) * 16 + 0x908));
    }
    for (i = 0; i < 4; i++) {
        func_800728B8((u8*)D_800C3EA4 + (i * 0x320 + 0x2E08), ((u8*)D_800D2D28 + i)[0x74], ((u8*)D_800D2D28 + i)[0x84]);
    }
    for (i = 0; i < 3; i++) {
        if (((u8*)D_800D2D28 + i)[0xCC] != 0) {
            base = D_800C3EA4;
            p = base + i * 0x1E4;
            switch (p[0x853D]) {
            case 1:
                func_800728B8((u8*)D_800C3EA4 + (i * 0x1E4 + 0x835C), p[0x853E], p[0x853C]);
                break;
            case 2:
                func_800728B8((u8*)D_800C3EA4 + (i * 0x1E4 + 0x835C) + 0xA0, p[0x853F], p[0x853C]);
                break;
            }
            func_800728B8((u8*)D_800C3EA4 + (i * 0x140 + 0x6008), *(s32*)((u8*)D_800D2D28 + i * 4 + 0xEC),
                          ((u8*)D_800D2D28 + i)[0x99]);
        }
    }
    if (((u8*)D_800D2D28)[0xAF] != 0) {
        func_800728B8((u8*)D_800C3EA4 + 0x9C8, ((u8*)D_800D2D28)[0x7B], ((u8*)D_800D2D28)[0xA4]);
    }
    for (i = 0; i < 3; i++) {
        func_800728B8((u8*)D_800C3EA4 + (i * 0xC80 + 0x3A88), *(s32*)((u8*)D_800D2D28 + i * 4 + 0xE0),
                      ((u8*)D_800D2D28 + i)[0x93]);
    }
    for (i = 0; i < 3; i++) {
        if (D_800C3EB6__rows[i][0] != 0x7F) {
            func_800728B8((u8*)D_800C3EA4 + (i * 0x50 + 0x818), 1, ((u8*)D_800D2D28)[0x83]);
            if (((u8*)D_800D2D28 + i)[0xCC] != 0) {
                func_800728B8((u8*)D_800C3EA4 + i * 0x1E0, ((u8*)D_800D2D28 + i)[0x78], ((u8*)D_800D2D28)[0xA2]);
                AddPrim(D_800CCB04__a_s32[0] + 4, (u8*)D_800C3EA4 + ((i * 2 + D_800CCB04__a_s32[12]) * 36 + 0x740));
            }
        }
    }
}
extern void *D_800C3EA4;

void func_80073E88(void) {
    s32 i;

    if (*(u8 *)((s8*)(D_800D2D28) + 0xCB) != 0) {
        for (i = 0; i < 2; i++) {
            func_800728B8(D_800C3EA4 + 0x641C + i * 0xFA0, *(s32 *)((s8*)(D_800D2D28) + 0xD0 + i * 4), *(u8 *)((s8*)(D_800D2D28) + 0xA3));
        }
    }
}

extern void *D_800C3EA4;

void func_80073F08(void) {
    if (*(u8 *)((s8*)(D_800D2D28) + 0x9C) != 0) {
        func_800728B8(D_800C3EA4 + 0xBA8, *(s32 *)((s8*)(D_800D2D28) + 0xF8), *(u8 *)((s8*)(D_800D2D28) + 0xA5));
    }
    if (*(u8 *)((s8*)(D_800D2D28) + 0x9E) != 0) {
        func_800728B8(D_800C3EA4 + 0x27C8, *(s32 *)((s8*)(D_800D2D28) + 0x100), *(u8 *)((s8*)(D_800D2D28) + 0xA7));
    }
    if (*(u8 *)((s8*)(D_800D2D28) + 0x9D) != 0) {
        func_800728B8(D_800C3EA4 + 0x1E68, *(s32 *)((s8*)(D_800D2D28) + 0xFC), *(u8 *)((s8*)(D_800D2D28) + 0xA6));
    }
}

extern u8 D_800C3E4C;
extern u8* D_800D2E38[];

/* Link prims into the OT at D_800CCB04 + 4: the D_800C3EA4 +0x8920 prim
 * for buffer D_800CCB34 (0xC stride); while D_800C3E4C is 1, a 0x34-byte
 * prim for each of the four slots whose D_800D2D28 +0x7F flag is set; the
 * +0x8908 prim; then for each of the seven D_800D2E38 records whose
 * D_800D2D28 +0xB0 flag is set, func_800728B8 on the record and ten of its
 * prims picked by the record's +0x5A4 index. */
void func_80073FB8(void) {
    s32 i;
    u8* e;

    AddPrim(D_800CCB04__a_s32[0] + 4, (D_800CCB34 * 0xC) + 0x8920 + D_800C3EA4);
    for (i = 0; i < 4; i++) {
        if (((u8*)D_800D2D28 + i)[0x7F] != 0 && D_800C3E4C == 1) {
            AddPrim(D_800CCB04__a_s32[0] + 4, ((i * 2 + ((u8*)D_800D2D28 + i)[0x84]) * 0x34) + 0x5A0 + D_800C3EA4);
        }
    }
    AddPrim(D_800CCB04__a_s32[0] + 4, (D_800CCB34 * 0xC) + 0x8908 + D_800C3EA4);
    for (i = 0; i < 7; i++) {
        if (*((u8*)D_800D2D28 + i + 0xB0) != 0) {
            e = D_800D2E38[i];
            func_800728B8(e, *(s32*)(e + 0x5A0), e[0x5A4]);
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x280));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x2D0));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x320));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x370));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x140));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x190));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x1E0));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x28 + 0x230));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0x24 + 0x3C0));
            AddPrim(D_800CCB04__a_s32[0] + 4, e + (e[0x5A4] * 0xC + 0x408));
        }
    }
}
extern u8 D_800C3EB6[];
extern u8 D_800D32A1[];

#ifndef XENO_PC_PORT
void func_800742A0(void) {
    s32 temp_s1;
    s32 temp_v0_2;
    s32 var_s0;
    s32 var_s2;
    void *temp_v0;

    var_s0 = 0;
    var_s2 = 0;
    do {
        if (*(u8 *)((s8*)((D_800D2D28 + var_s0)) + 0x7C) != 0) {
            temp_v0 = (var_s0 * 4) + D_800D2D28;
            *(s32 *)((s8*)(temp_v0) + 0xE0) = 0;
            *(s32 *)((s8*)(temp_v0) + 0xEC) = 0;
            temp_s1 = var_s0 * 8;
            if (D_800C3EB6[var_s2] != 0x7F) {
                func_80073380(var_s0);
                temp_v0_2 = func_80072F38(var_s0, D_800D32A1[temp_s1]);
                if (D_800D32A1[temp_s1] != 0) {
                    func_80072DA8(var_s0, temp_v0_2 & 0xFF);
                } else {
                    func_80072A9C(var_s0, temp_v0_2 & 0xFF);
                }
                *(u8 *)((s8*)((D_800D2D28 + var_s0)) + 0x93) = (u8) D_800CCB34;
                *(s8 *)((s8*)((D_800D2D28 + var_s0)) + 0x7C) = 0;
            }
        }
        var_s0 += 1;
        var_s2 += 0x1C;
    } while (var_s0 < 3);
}
#endif /* !XENO_PC_PORT: host uses instruction lift */

/* func_800743A4.s: when the prim set at *(D_800C3EA4 + 0xA230) is enabled
 * (+0x669), add its four double-buffered 0x28-byte prims (0x50 apart; the
 * buffer index is +0x668) to OT entry 1 (D_800CCB04 + 4).  The set pointer
 * is re-read after every AddPrim, as in retail. */
void func_800743A4(void) {
    if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x669) != 0) {
        AddPrim(D_800CCB04__a_s32[0] + 4,
                (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x668) * 0x28) + *(u8**)(D_800C3EA4 + 0xA230));
        AddPrim(D_800CCB04__a_s32[0] + 4,
                (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x668) * 0x28) + 0x50 + *(u8**)(D_800C3EA4 + 0xA230));
        AddPrim(D_800CCB04__a_s32[0] + 4,
                (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x668) * 0x28) + 0xA0 + *(u8**)(D_800C3EA4 + 0xA230));
        AddPrim(D_800CCB04__a_s32[0] + 4,
                (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x668) * 0x28) + 0xF0 + *(u8**)(D_800C3EA4 + 0xA230));
    }
}
extern void *D_800C3EA4;
extern s32 D_800CCB04__a_s32[] asm("D_800CCB04");

void func_800744BC(void) {
    AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8 *)((s8*)(*(void **)((s8*)(D_800C3EA4) + 0xA230)) + 0x66A) * 0x28) + 0x320 + *(void **)((s8*)(D_800C3EA4) + 0xA230));
    AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8 *)((s8*)(*(void **)((s8*)(D_800C3EA4) + 0xA230)) + 0x66A) * 0x28) + 0x370 + *(void **)((s8*)(D_800C3EA4) + 0xA230));
}

extern void *D_800C3EA4;
extern s32 D_800CCB04__a_s32[] asm("D_800CCB04");

void func_80074554(void) {
    AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8 *)((s8*)(*(void **)((s8*)(D_800C3EA4) + 0xA230)) + 0x66A) * 0x28) + 0x280 + *(void **)((s8*)(D_800C3EA4) + 0xA230));
    AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8 *)((s8*)(*(void **)((s8*)(D_800C3EA4) + 0xA230)) + 0x66A) * 0x28) + 0x2D0 + *(void **)((s8*)(D_800C3EA4) + 0xA230));
}

/* func_800745EC.s, byte-exact: draw the current menu panel's extra prims
 * (state D_800D2D28 +0xB7 = 1..5).  Each state runs func_800743A4, then
 * adds its double-buffered prims of the panel set (re-read from
 * D_800C3EA4 + 0xA230 after every AddPrim, as in retail) to OT entry 1:
 * 1: six 0x28-byte prims from +0x410 when +0x669 is set; with +0x66B the
 * cursor prims (+0x3C0 if +0x66E, +0x140), the +0x190 strip
 * (func_800728B8) and func_80074554 / func_800744BC; 2: +0x140 / +0x190
 * with +0x66B, func_800744BC / func_80074554 with +0x66D; 3: +0x5F0
 * (0x24) and +0x638 (0x18) with +0x66F; 4: state 1's +0x66B part without
 * the strip; 5: nothing more. */
void func_800745EC(void) {
    switch (((u8*)D_800D2D28)[0xB7]) {
    case 1:
        func_800743A4();
        if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x669) != 0) {
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x410 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x460 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x4B0 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x500 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x550 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x5A0 + *(u8**)(D_800C3EA4 + 0xA230));
        }
        if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66B) != 0) {
            if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66E) != 0) {
                AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x3C0 + *(u8**)(D_800C3EA4 + 0xA230));
            }
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66A) * 0x28) + 0x140 + *(u8**)(D_800C3EA4 + 0xA230));
            func_800728B8(*(u8**)(D_800C3EA4 + 0xA230) + 0x190, *(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66E), *(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66A));
            func_80074554();
            func_800744BC();
        }
        break;
    case 2:
        func_800743A4();
        if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66B) != 0) {
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66A) * 0x28) + 0x140 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66A) * 0x28) + 0x190 + *(u8**)(D_800C3EA4 + 0xA230));
        }
        if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66D) != 0) {
            func_800744BC();
            func_80074554();
        }
        break;
    case 3:
        func_800743A4();
        if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66F) != 0) {
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x24) + 0x5F0 + *(u8**)(D_800C3EA4 + 0xA230));
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x18) + 0x638 + *(u8**)(D_800C3EA4 + 0xA230));
        }
        break;
    case 4:
        func_800743A4();
        if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66B) != 0) {
            if (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66E) != 0) {
                AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66C) * 0x28) + 0x3C0 + *(u8**)(D_800C3EA4 + 0xA230));
            }
            AddPrim(D_800CCB04__a_s32[0] + 4, (*(u8*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x66A) * 0x28) + 0x140 + *(u8**)(D_800C3EA4 + 0xA230));
            func_80074554();
            func_800744BC();
        }
        break;
    case 5:
        func_800743A4();
        break;
    }
}

#ifndef XENO_PC_PORT
void func_80074AB8(void) {
    u16 temp_v0;

    if (*(u8 *)((s8*)(D_800D2D28) + 0xC7) != 0) {
        func_800728B8(D_800D2DB4 + 0x3E80, *(u8 *)((s8*)(D_800D2DB4) + 0x5D77), *(u8 *)((s8*)(D_800D2DB4) + 0x5D86));
        func_800728B8(D_800D2DB4 + 0x5280, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7C), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8B));
        func_800728B8(D_800D2DB4 + 0x53C0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7D), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8C));
        func_800728B8(D_800D2DB4 + 0x43D0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7F), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8E));
    }
    if (*(u8 *)((s8*)(D_800D2D28) + 0xAD) != 0) {
        func_800728B8(D_800D2DB4 + 0x46A0, 0x14U, *(u8 *)((s8*)(D_800D2DB4) + 0x5D98));
        func_800728B8(D_800D2DB4, *(u8 *)((s8*)(D_800D2DB4) + 0x5D70), *(u8 *)((s8*)(D_800D2DB4) + 0x5D92));
        func_800728B8(D_800D2DB4 + 0x1720, *(u8 *)((s8*)(D_800D2DB4) + 0x5D74), *(u8 *)((s8*)(D_800D2DB4) + 0x5D83));
        temp_v0 = *(u16 *)((s8*)(D_800D2DB4) + 0x5DA2) + 1;
        *(u16 *)((s8*)(D_800D2DB4) + 0x5DA2) = temp_v0;
        if ((s16) temp_v0 < 0xF) {
            func_800728B8(D_800D2DB4 + 0x4CE0, *(u8 *)((s8*)(D_800D2DB4) + 0x5DA1), *(u8 *)((s8*)(D_800D2DB4) + 0x5DA0));
        } else if ((s16) temp_v0 >= 0x15) {
            *(u16 *)((s8*)(D_800D2DB4) + 0x5DA2) = 0U;
        }
        func_800728B8(D_800D2DB4 + 0x3AC0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D75), *(u8 *)((s8*)(D_800D2DB4) + 0x5D84));
        func_800728B8(D_800D2DB4 + 0x3E30, *(u8 *)((s8*)(D_800D2DB4) + 0x5D76), *(u8 *)((s8*)(D_800D2DB4) + 0x5D85));
        func_800728B8(D_800D2DB4 + 0x4E70, *(u8 *)((s8*)(D_800D2DB4) + 0x5D78), *(u8 *)((s8*)(D_800D2DB4) + 0x5D87));
        func_800728B8(D_800D2DB4 + 0x4FB0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D79), *(u8 *)((s8*)(D_800D2DB4) + 0x5D88));
        func_800728B8(D_800D2DB4 + 0x50A0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7A), *(u8 *)((s8*)(D_800D2DB4) + 0x5D89));
        func_800728B8(D_800D2DB4 + 0x51E0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7B), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8A));
        func_800728B8(D_800D2DB4 + 0x2530, *(u8 *)((s8*)(D_800D2DB4) + 0x5D7E), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8D));
        func_800728B8(D_800D2DB4 + 0x32A0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D96), *(u8 *)((s8*)(D_800D2DB4) + 0x5D97));
        func_800728B8(D_800D2DB4 + 0x3C0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D71), *(u8 *)((s8*)(D_800D2DB4) + 0x5D93));
        func_800728B8(D_800D2DB4 + 0xD20, *(u8 *)((s8*)(D_800D2DB4) + 0x5D72), *(u8 *)((s8*)(D_800D2DB4) + 0x5D94));
        func_800728B8(D_800D2DB4 + 0xFA0, *(u8 *)((s8*)(D_800D2DB4) + 0x5D73), *(u8 *)((s8*)(D_800D2DB4) + 0x5D95));
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80074D4C(void) {
    s32 temp_a0;
    s32 temp_v0;
    s32 temp_v0_2;
    s32 var_s0;

    if (*(u8 *)((s8*)(D_800D2D28) + 0xC6) != 0) {
        if (*(u8 *)((s8*)(D_800C3E24) + 0xE5) != 0) {
            temp_v0 = *(s32 *)((s8*)(D_800C3E24) + 0xE0) - 4;
            *(s32 *)((s8*)(D_800C3E24) + 0xE0) = temp_v0;
            if (temp_v0 < 0x40) {
                *(u8 *)((s8*)(D_800C3E24) + 0xE5) = 0U;
                *(s32 *)((s8*)(D_800C3E24) + 0xE0) = 0x40;
            }
            goto block_6;
        }
        temp_v0_2 = *(s32 *)((s8*)(D_800C3E24) + 0xE0) + 4;
        *(s32 *)((s8*)(D_800C3E24) + 0xE0) = temp_v0_2;
        var_s0 = 0;
        if (temp_v0_2 >= 0x100) {
            *(u8 *)((s8*)(D_800C3E24) + 0xE5) = 1U;
            *(s32 *)((s8*)(D_800C3E24) + 0xE0) = 0xFC;
block_6:
            var_s0 = 0;
        }
        do {
            if (*(u8 *)((s8*)((D_800C3E24 + var_s0)) + 0xE6) != 0) {
                temp_a0 = var_s0 * 2;
                *(u8 *)((s8*)((((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24)) + 4) = (u8) *(s32 *)((s8*)(D_800C3E24) + 0xE0);
                *(s8 *)((s8*)((((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24)) + 5) = 0;
                *(s8 *)((s8*)((((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24)) + 6) = 0;
                AddPrim(D_800CCB04 + 4, ((temp_a0 + *(u8 *)((s8*)(D_800C3E24) + 0xE4)) * 0x1C) + D_800C3E24);
            }
            var_s0 += 1;
        } while (var_s0 < 4);
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80074EEC(void) {
    if (*(u8 *)((s8*)(D_800D2D28) + 0xA8) != 0) {
        func_800728B8(D_800D2DB4 + 0x5550, *(u8 *)((s8*)(D_800D2DB4) + 0x5D80), *(u8 *)((s8*)(D_800D2DB4) + 0x5D8F));
        func_800728B8(D_800D2DB4 + 0x5640, *(u8 *)((s8*)(D_800D2DB4) + 0x5D81), *(u8 *)((s8*)(D_800D2DB4) + 0x5D90));
        func_800728B8(D_800D2DB4 + 0x5C80, *(u8 *)((s8*)(D_800D2DB4) + 0x5D82), *(u8 *)((s8*)(D_800D2DB4) + 0x5D91));
    }
}
#endif


#ifndef XENO_PC_PORT
void func_80074F70(void) {
    if (*(u8 *)((s8*)(D_800D2D28) + 0xC8) != 0) {
        AddPrim(D_800CCB04 + 4, (*(u8 *)((s8*)(D_800D3278) + 0x7F4) * 0x28) + 0x7A4 + D_800D3278);
    }
    if (*(u8 *)((s8*)(D_800D2D28) + 0xC9) != 0) {
        func_80034888(D_800D2DAC, D_800CCB04 + 4, D_800CCB34);
    }
}
#endif

extern u8 D_800D3725[][0x60];
extern u8 D_800D3726[][0x60];
extern u8 D_800D36C8[][0x60];
/* Retail passes x zero-extended to 16 bits and the rest unconverted. */
extern void func_80076C78(u8* p, u16 x, s32 y, s32 u, s32 v, s32 w);

/* Draw the eight 0x60-byte gauge records at D_800D36C8: each active one
 * (+0x5D set) gets its +0x5E width centred on x 0x40 (record 0, y 0x2C) or
 * 0x9A (the rest, y 0xCA, v = i / 2 * 13), then the record's current-buffer
 * prim is linked into the OT. */
void func_8007500C(void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (D_800D3725[i][0] != 0) {
            if (i == 0) {
                func_80076C78(D_800D36C8[i] + D_800CCB34 * 0x28, 0x40 - D_800D3726[i][0] / 2, 0x2C, 0, 0,
                              D_800D3726[i][0]);
            } else {
                func_80076C78(D_800D36C8[i] + D_800CCB34 * 0x28, 0x9A - D_800D3726[i][0] / 2, 0xCA, 0,
                              (u8)(i / 2 * 13), D_800D3726[i][0]);
            }
            AddPrim(D_800CCB04__a_s32[0] + 4, D_800D36C8[i] + D_800CCB04__a_s32[12] * 0x28);
        }
    }
}
/* func_80075168.s, byte-exact: refresh the three party gauges.  For slot
 * i (state byte D_800C3EA4 + i * 0x1E4 + 0x853D) the gauge is the POLY_G4
 * at D_800C3EA4 + 0x740 + (i * 2 + the D_800CCB34 buffer) * 0x24; its
 * left x is i * 0x60 + 0x28 + D_800C3254[D_800D3280 * 3 + i], y 0x22 (top)
 * / 0x26 (bottom).
 * - State 1: width D_800D32A0[i * 8] * 2, vertex 0/1 colours green (0, 0xFF, 0).
 * - State 2: width = hp * 100 / max * 0x15E0 / 10000 (D_800CCDC4 /
 *   D_800CCDC6 of the 0x170-byte record, unsigned); vertex 0/1 colours blue at
 *   hp >= max / 4, yellow at hp >= max / 8, else (0xFF, 0x7F, 0x7F).
 * Other states leave the gauge alone.
 * Codegen: the state-2 width is computed once into a u32 local before the
 * stores (inline twice it is one word longer); every store goes through
 * the same base + (i * 2 + buffer) * 0x24 + offset expression (local
 * macros).  Matches under 2.6.3 (preset) and 2.6.0. */
extern u16 D_800CCDC6__rows[][184] asm("D_800CCDC6");

#define H75_P(off) ((u8*)D_800C3EA4 + (i * 2 + D_800CCB34__a[0]) * 0x24 + (off))
#define H75_H(off, v) (*(s16*)H75_P(off) = (v))
#define H75_B(off, v) (*(u8*)H75_P(off) = (v))
#define H75_X() (i * 0x60 + (D_800C3254[D_800D3280 * 3 + i] + 0x28))

void func_80075168(void) {
    s32 i;
    u8* base;
    u8* p;
    u32 cur;
    u32 max;
    u32 w;

    for (i = 0; i < 3; i++) {
        base = D_800C3EA4;
        p = base + i * 0x1E4;
        switch (p[0x853D]) {
        case 1:
            H75_H(0x748, H75_X());
            H75_H(0x74A, 0x22);
            H75_H(0x750, H75_X() + D_800D32A0[i << 3] * 2);
            H75_H(0x752, 0x22);
            H75_H(0x758, H75_X());
            H75_H(0x75A, 0x26);
            H75_H(0x760, H75_X() + D_800D32A0[i << 3] * 2);
            H75_H(0x762, 0x26);
            H75_B(0x744, 0);
            H75_B(0x745, 0xFF);
            H75_B(0x746, 0);
            H75_B(0x74C, 0);
            H75_B(0x74D, 0xFF);
            H75_B(0x74E, 0);
            break;
        case 2:
            cur = D_800CCDC4__rows[i][0];
            max = D_800CCDC6__rows[i][0];
            w = cur * 100 / max * 0x15E0 / 10000;
            H75_H(0x748, H75_X());
            H75_H(0x74A, 0x22);
            H75_H(0x750, i * 0x60 + 0x28 + D_800C3254[D_800D3280 * 3 + i] + w);
            H75_H(0x752, 0x22);
            H75_H(0x758, H75_X());
            H75_H(0x75A, 0x26);
            H75_H(0x760, i * 0x60 + 0x28 + D_800C3254[D_800D3280 * 3 + i] + w);
            H75_H(0x762, 0x26);
            if (cur >= max / 4) {
                H75_B(0x744, 0);
                H75_B(0x745, 0);
                H75_B(0x746, 0xFF);
                H75_B(0x74C, 0);
                H75_B(0x74D, 0);
                H75_B(0x74E, 0xFF);
            } else if (cur >= max / 8) {
                H75_B(0x744, 0xFF);
                H75_B(0x745, 0xFF);
                H75_B(0x746, 0);
                H75_B(0x74C, 0xFF);
                H75_B(0x74D, 0xFF);
                H75_B(0x74E, 0);
            } else {
                H75_B(0x744, 0xFF);
                H75_B(0x745, 0x7F);
                H75_B(0x746, 0x7F);
                H75_B(0x74C, 0xFF);
                H75_B(0x74D, 0x7F);
                H75_B(0x74E, 0x7F);
            }
            break;
        }
    }
}
#undef H75_P
#undef H75_H
#undef H75_B
#undef H75_X
/* func_80075938.s, byte-exact: per-frame party gauge pass.  Runs
 * func_80075168, clears D_800D2D28[0x82], then for slots 0-2 whose state
 * byte (D_800C3EB6 rows) is not 0x7F:
 * - outside the command menu (D_800D2D28[0x90 + i] == 0): the time gauge
 *   POLY_GT4 at D_800C3EA4 + 0x5A0 + (i * 2 + the D_800CCB34 buffer) *
 *   0x34, width w = (100 - D_800D2E06[i] * 100 / D_800D2E06[i - 11]) *
 *   0x15E0 / 10000, x from D_800C3254 as func_80075168, y 0x1A / 0x1E, uvs
 *   from the work-area offsets +0xA244 / +0xA248, CLUT +0xA2B2 / +0xA2B0 /
 *   +0xA2AC by the D_800CCD6C|D_800CCD6E bit 15 / D_800CCD64 bit 12 status;
 *   D_800D2D28[0x7F + i] = 1;
 * - in the menu: the vertical gauge at +0x6D8 + buffer * 0x34 (x 0xC/0x14,
 *   y 0xCE - w * 2 .. 0xCE), w = (100 - D_800D2E06[i]) * 0x15E0 / 10000 or
 *   the HP share (D_800CCDC4 / D_800CCDC6) when D_800D32A1[i * 8] is set
 *   and D_800D2D24[i] != 7, CLUT +0xA2AE, and D_800D2D28[0x82] = 1 while
 *   D_800D2D28[0xCB] is set.
 * State-0x7F slots get D_800D2D28[0x7F + i] = 0.
 * CAVEAT (retail behaviour kept): each pass first clamps w[i] to >= 0
 * before either branch assigns it, so it reads the stack array's previous
 * contents (never used afterwards).
 * Codegen: w is a stack array (retail keeps it at sp+0x10); the u-coordinate
 * sums go through block-local s32 copies of w[i]; work-area stores through
 * local base + offset macros (#undef'd after) and an H59Big record view
 * for the CLUT fields.  Matches under 2.6.3 (preset) and 2.6.0. */
extern s16 D_800D2E06__s[] asm("D_800D2E06");
extern u16 D_800CCD64__rows[][184] asm("D_800CCD64");
extern u16 D_800CCD6C__rows[][184] asm("D_800CCD6C");
extern u16 D_800CCD6E__rows[][184] asm("D_800CCD6E");
typedef struct {
    u8 v[8];
} H59Row8;
extern H59Row8 D_800D32A1__r[] asm("D_800D32A1");
typedef struct {
    u8 v;
} H59Byte;
typedef struct {
    s16 x0;
    s16 y0;
    u8 r0;
    u8 g0;
    u16 f;
    u8 pad[0x2C];
} H59R;
typedef struct {
    u8 pad[0x5A8];
    H59R rec[1];
} H59Big;
extern H59Byte D_800D2D24__b[] asm("D_800D2D24");

#define H59_P(off) ((u8*)D_800C3EA4 + (i * 2 + D_800CCB34__a[0]) * 0x34 + (off))
#define H59_H(off, v) (*(s16*)H59_P(off) = (v))
#define H59_B(off, v) (*(u8*)H59_P(off) = (v))
#define H59_E(off) ((u8*)D_800C3EA4 + D_800CCB34__a[0] * 0x34 + (off))
#define H59_EH(off, v) (*(s16*)H59_E(off) = (v))
#define H59_EB(off, v) (*(u8*)H59_E(off) = (v))
#define H59_G(off) (((u8*)D_800C3EA4)[off])

void func_80075938(void) {
    s32 i;
    s32 w[3];
    u16 v;

    func_80075168();
    *(u8*)(D_800D2D28 + 0x82) = 0;
    for (i = 0; i < 3; i++) {
        if (D_800C3EB6__s[i].state != 0x7F) {
            if (w[i] < 0) {
                w[i] = 0;
            }
            if (((u8*)D_800D2D28 + i)[0x90] == 0) {
                w[i] = (100 - D_800D2E06__s[i] * 100 / D_800D2E06__s[i - 11]) * 0x15E0 / 10000;
                H59_H(0x5A8, D_800C3254[D_800D3280 * 3 + i] + i * 0x60 + 0x2D);
                H59_H(0x5AA, 0x1A);
                H59_H(0x5B4, i * 0x60 + (D_800C3254[D_800D3280 * 3 + i] + 0x2C) + ((u16)w[i] + 1));
                H59_H(0x5B6, 0x1A);
                H59_H(0x5C0, D_800C3254[D_800D3280 * 3 + i] + i * 0x60 + 0x2D);
                H59_H(0x5C2, 0x1E);
                H59_H(0x5CC, i * 0x60 + (D_800C3254[D_800D3280 * 3 + i] + 0x2C) + ((u16)w[i] + 1));
                H59_H(0x5CE, 0x1E);
                H59_B(0x5AC, (H59_G(0xA244) & 0x3F) * 2);
                H59_B(0x5AD, H59_G(0xA248));
                { s32 t = w[i]; H59_B(0x5B8, (t + (H59_G(0xA244) & 0x3F)) * 2); }
                H59_B(0x5B9, H59_G(0xA248));
                H59_B(0x5C4, (H59_G(0xA244) & 0x3F) * 2);
                H59_B(0x5C5, H59_G(0xA248) + 4);
                { s32 t = w[i]; H59_B(0x5D0, (t + (H59_G(0xA244) & 0x3F)) * 2); }
                H59_B(0x5D1, H59_G(0xA248) + 4);
                if ((D_800CCD6C__rows[i][0] | D_800CCD6E__rows[i][0]) & 0x8000) {
                    v = *(u16*)((u8*)D_800C3EA4 + 0xA2B2);
                } else if (D_800CCD64__rows[i][0] & 0x1000) {
                    v = *(u16*)((u8*)D_800C3EA4 + 0xA2B0);
                } else {
                    v = *(u16*)((u8*)D_800C3EA4 + 0xA2AC);
                }
                ((H59Big*)D_800C3EA4)->rec[i * 2 + D_800CCB34__a[0]].f = v;
                ((u8*)D_800D2D28 + i)[0x7F] = 1;
            } else {
                w[i] = (100 - D_800D2E06__s[i]) * 0x15E0 / 10000;
                if (D_800D32A1__r[i].v[0] != 0 && D_800D2D24__b[i].v != 7) {
                    w[i] = D_800CCDC4__rows[i][0] * 100 / D_800CCDC6__rows[i][0] * 0x15E0 / 10000;
                }
                H59_EH(0x6E0, 0xC);
                H59_EH(0x6E2, 0xCE);
                H59_EH(0x6EC, 0xC);
                H59_EH(0x6EE, 0xCE - w[i] * 2);
                H59_EH(0x6F8, 0x14);
                H59_EH(0x6FA, 0xCE);
                H59_EH(0x704, 0x14);
                H59_EH(0x706, 0xCE - w[i] * 2);
                H59_EB(0x6E4, (H59_G(0xA244) & 0x3F) * 2);
                H59_EB(0x6E5, H59_G(0xA248));
                { s32 t = w[i]; H59_EB(0x6F0, (t + (H59_G(0xA244) & 0x3F)) * 2); }
                H59_EB(0x6F1, H59_G(0xA248));
                H59_EB(0x6FC, (H59_G(0xA244) & 0x3F) * 2);
                H59_EB(0x6FD, H59_G(0xA248) + 4);
                { s32 t = w[i]; H59_EB(0x708, (t + (H59_G(0xA244) & 0x3F)) * 2); }
                H59_EB(0x709, H59_G(0xA248) + 4);
                ((H59Big*)D_800C3EA4)->rec[D_800CCB34__a[0] + 6].f = *(u16*)((u8*)D_800C3EA4 + 0xA2AE);
                if (((u8*)D_800D2D28)[0xCB] != 0) {
                    ((u8*)D_800D2D28)[0x82] = 1;
                }
            }
        } else {
            ((u8*)D_800D2D28 + i)[0x7F] = 0;
        }
    }
}
#undef H59_P
#undef H59_H
#undef H59_B
#undef H59_E
#undef H59_EH
#undef H59_EB
#undef H59_G
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C492A;
#endif
extern void func_8008FAD8(void);
extern void func_800742A0(void);
extern void func_80075938(void);
extern void func_80073538(void);
extern void func_80073A58(void);
extern void func_80073B64(void);
extern void func_80073E88(void);
extern void func_80073F08(void);
extern void func_8007500C(void);
extern void func_80074EEC(void);
extern void func_800745EC(void);
extern void func_80074D4C(void);
extern void func_80073FB8(void);
extern void func_80088B80(void);
extern void func_80074AB8(void);
/* func_80076418.s: the per-frame battle render chain, skipped entirely once
 * D_800C492A is set. */
void func_80076418(void) {
    if (D_800C492A == 0) {
        func_8008FAD8();
        func_800742A0();
        func_80075938();
        func_80073538();
        func_80073A58();
        func_80073B64();
        func_80073E88();
        func_80073F08();
        func_8007500C();
        func_80074EEC();
        func_800745EC();
        func_80074D4C();
        func_80073FB8();
        func_80088B80();
        func_80074AB8();
    }
}


extern u8 D_8005959C;
extern void func_8008FAD8(void);
extern void func_801DE594(void);
extern void func_80073FB8(void);


/* func_800764B4.s */
void func_800764B4(void) {
    D_8005959C = 0;
    func_8008FAD8();
    func_801DE594();
    func_80073FB8();
}
/* func_800764EC.s */
void func_800764EC(void) {
    func_8008FAD8();
    func_80073538();
    func_80073F08();
    func_8007500C();
    func_80074F70();
    func_80073FB8();
    func_80088B80();
    func_80074AB8();
}

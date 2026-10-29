#include "common.h"


#ifndef XENO_PC_PORT
void func_80099CF0(void *arg0, void *arg1, void *arg2);
#endif

#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
typedef struct {
    u8 unk0[0x38];
    s16 unk38;
    s16 unk3A;
    u8 unk3C;
    u8 unk3D[0x1];
    u8 unk3E;
    u8 unk3F;
    u16 unk40;
    u16 unk42;
    u16 unk44;
    u16 unk46;
    u8 unk48[0x2];
    u8 unk4A;
    u8 unk4B[0x1];
    u8 unk4C;
    u8 unk4D;
    u8 unk4E;
    u8 unk4F;
    u8 unk50[0x4];
    u8 unk54;
    u8 unk55[0x1];
    u8 unk56;
    u8 unk57[0x9];
    u32 unk60;
    u32 unk64;
    u16 unk68;
    u8 unk6A[0x6];
    u16 unk70;
    u16 unk72;
    u8 unk74;
    u8 unk75[0x9];
    u16 unk7E;
    u8 unk80[0x18];
    u8 unk98;
    u8 unk99[0x3];
    u8 unk9C;
    u8 unk9D[0x1];
    u8 unk9E;
    u8 unk9F;
    u8 unkA0[0x2C];
} M53Stats;
typedef struct {
    u8 unk0[0x4];
    u8 unk4;
    u8 unk5[0x17];
    u8 unk1C;
    u8 unk1D[0xB];
    u8 unk28;
    u8 unk29;
    u8 unk2A;
    u8 unk2B;
    u8 unk2C;
    u8 unk2D;
    u8 unk2E;
    u8 unk2F;
    u8 unk30;
    u8 unk31;
    u16 unk32;
    s16 unk34;
    u8 unk36[0x6];
    s32 unk3C;
    s32 unk40;
    u8 unk44[0x8];
    u16 unk4C;
    u16 unk4E;
    u16 unk50;
    u16 unk52;
    u8 unk54[0x2];
    u8 unk56;
    u8 unk57[0x1];
    u8 unk58;
    u8 unk59;
    u8 unk5A;
    u8 unk5B;
    u8 unk5C;
    u8 unk5D[0x1];
    u8 unk5E;
    u8 unk5F;
    u8 unk60[0x2];
    u8 unk62;
    u8 unk63[0x17];
    s16 unk7A;
    u8 unk7C[0x6];
    u16 unk82;
    u8 unk84[0x6];
    u16 unk8A;
    u8 unk8C[0x14];
    u8 unkA0;
    u8 unkA1[0xCF];
} M53Rec;
extern M53Stats* D_800D2D6C__s asm("D_800D2D6C");
extern M53Rec* D_800C3E00__s asm("D_800C3E00");
extern u8 D_800D2D24[];
extern u8 D_800C34AD;
typedef struct {
    u8 unk0[0x5F54];
    s32 unk5F54[3];
    s32 unk5F60[3];
} M53Tail;
typedef struct {
    u8 unk0[0x1028];
    struct {
        u8 b0;
        u8 unk1[7];
    } r8[3];
} M53Row8;
typedef struct {
    u16 v;
    u8 unk2[0x68E];
} M53Row690;
typedef struct {
    u8 unk0[0x2814];
    M53Row690 rows[3];
} M53Big;
extern u8 g_GameState__b[] asm("g_GameState");
typedef struct {
    u8 unk0[4];
    u16 unk4;
    u8 unk6[0x14];
    u16 unk1A;
    u8 unk1C[4];
} M53Char; /* 0x20 */
typedef struct {
    u8 unk0[0x16C0];
    M53Char chars[0x20]; /* indexed by the record +0x56 character id */
} M53State;
extern M53State g_GameState__s asm("g_GameState");

/* func_80097D5C.s: for each of the three D_800D2D24 slots that is not
 * 0x7F, point D_800C3E00 at its 0x170-byte record in the D_800C34B0 block
 * (D_800D2D6C at +0xA4) and set it up for battle: slot-id overrides (9,
 * 0xA), the stat row at +0x1028 and the +0xFD0/+0x1000 copies, the
 * +0x28..+0x2F bonuses added into +0x58..+0x5F, the +0x4C..+0x52 pairs
 * raised by +0x30/+0x31 twentieths and clamped to 999/99, the D_800D2D6C
 * sums, the 0x690-row value at +0x2814, the g_GameState character flags
 * and the per-slot special cases.  Then count the empty slots out of
 * D_800C34AD and clear the +0x5F54/+0x5F60 words.  The records are structs
 * on purpose: retail schedules and caches as for struct-field accesses. */
void func_80097D5C(void) {
    u8 i;
    u8 j;
    u8* sub;
    u8 t;
    u16 bits;
    s32 off;
    s32 off4;

    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] == 0x7F) {
            continue;
        }
        D_800C3E00__s = (M53Rec*)(D_800C34B0 + i * 0x170);
        D_800D2D6C__s = (M53Stats*)((u8*)D_800C3E00__s + 0xA4);
        sub = (u8*)D_800C3E00__s + 0x148;
        if (D_800D2D24[i] == 9) {
            g_GameState__b[0x886] = 9;
            D_800C3E00__s->unk56 = 9;
        }
        if (D_800D2D24[i] == 0xA) {
            g_GameState__b[0x92A] = 0xA;
            D_800C3E00__s->unk56 = 0xA;
        }
        if (D_800C3E00__s->unk56 == 4) {
            ((M53Row8*)D_800C34B0)->r8[i].b0 = D_800C3E00__s->unk4 + D_800C3E00__s->unk1C;
        } else {
            ((M53Row8*)D_800C34B0)->r8[i].b0 = D_800C3E00__s->unk58 + D_800C3E00__s->unk4;
        }
        off = i * 8;
        (D_800C34B0 + off)[0x1029] = D_800C3E00__s->unk5E;
        (D_800C34B0 + off)[0x102A] = D_800C3E00__s->unk59 + D_800C3E00__s->unk2D;
        (D_800C34B0 + off)[0x102B] = D_800C3E00__s->unk5F;
        (D_800C34B0 + off)[0x102C] = D_800C3E00__s->unk5B;
        (D_800C34B0 + off)[0x102D] = D_800C3E00__s->unk5C;
        (D_800C34B0 + off)[0x102E] = D_800C3E00__s->unk5A;
        *(s32*)(D_800C34B0 + off + 0xFD0) = D_800C3E00__s->unk3C;
        *(s32*)(D_800C34B0 + off + 0xFD4) = D_800C3E00__s->unk40;
        off4 = i * 4;
        *(u16*)(D_800C34B0 + off4 + 0x1000) = D_800C3E00__s->unk4E;
        *(u16*)(D_800C34B0 + off4 + 0x1002) = D_800C3E00__s->unk52;
        D_800C3E00__s->unk34 = 0;
        D_800C3E00__s->unk58 += D_800C3E00__s->unk28;
        D_800C3E00__s->unk59 += D_800C3E00__s->unk29;
        D_800C3E00__s->unk5A += D_800C3E00__s->unk2A;
        D_800C3E00__s->unk5B += D_800C3E00__s->unk2B;
        D_800C3E00__s->unk5C += D_800C3E00__s->unk2C;
        D_800C3E00__s->unk5E += D_800C3E00__s->unk2E;
        D_800C3E00__s->unk5F += D_800C3E00__s->unk2F;
        if (D_800C3E00__s->unk5A > 0x10) {
            D_800C3E00__s->unk5A = 0x10;
        }
        if (D_800C3E00__s->unk32 & 0x100) {
            D_800C3E00__s->unk5E += D_800C3E00__s->unk5E >> 2;
            D_800C3E00__s->unk5F += D_800C3E00__s->unk5F >> 2;
        }
        D_800C3E00__s->unk4C += D_800C3E00__s->unk4E * D_800C3E00__s->unk30 / 20;
        D_800C3E00__s->unk50 += D_800C3E00__s->unk52 * D_800C3E00__s->unk31 / 20;
        D_800C3E00__s->unk4E += D_800C3E00__s->unk4E * D_800C3E00__s->unk30 / 20;
        D_800C3E00__s->unk52 += D_800C3E00__s->unk52 * D_800C3E00__s->unk31 / 20;
        if (D_800C3E00__s->unk4C > 999) {
            D_800C3E00__s->unk4C = 999;
        }
        if (D_800C3E00__s->unk50 > 99) {
            D_800C3E00__s->unk50 = 99;
        }
        if (D_800C3E00__s->unk4E > 999) {
            D_800C3E00__s->unk4E = 999;
        }
        if (D_800C3E00__s->unk52 > 99) {
            D_800C3E00__s->unk52 = 99;
        }
        D_800C34B0[i * 0x170 + 0x159] = D_800C34B0[i * 0x170 + 0x158] = 5;
        sub[0] = 0;
        D_800D2D6C__s->unk70 += D_800D2D6C__s->unk40;
        D_800D2D6C__s->unk72 += D_800D2D6C__s->unk42;
        D_800D2D6C__s->unk68 += D_800D2D6C__s->unk44 + D_800D2D6C__s->unk46;
        D_800D2D6C__s->unk9C += D_800D2D6C__s->unk4C;
        D_800D2D6C__s->unk9F += D_800D2D6C__s->unk4D;
        D_800D2D6C__s->unk98 += D_800D2D6C__s->unk4E - D_800D2D6C__s->unk4A;
        D_800D2D6C__s->unk9E += D_800D2D6C__s->unk54;
        t = D_800D2D6C__s->unk4F;
        if (t != 0 && D_800C3E00__s->unkA0 != 0x12) {
            ((M53Big*)D_800C34B0)->rows[i].v = D_800D2D6C__s->unk64 / 10 * t * 2 / 9;
            ((M53Big*)D_800C34B0)->rows[i].v = ((M53Big*)D_800C34B0)->rows[i].v / 10U;
            ((M53Big*)D_800C34B0)->rows[i].v = ((M53Big*)D_800C34B0)->rows[i].v * 10;
        }
        sub[1] = 0;
        if (g_GameState__s.chars[D_800C3E00__s->unk56].unk4 & 0x1C00) {
            sub[1] = 1;
        }
        if (g_GameState__s.chars[D_800C3E00__s->unk56].unk4 & 0x380) {
            sub[1] = 2;
        }
        if (g_GameState__s.chars[D_800C3E00__s->unk56].unk4 & 0x70) {
            sub[1] = 3;
        }
        D_800D2D6C__s->unk3F = D_800D2D6C__s->unk74;
        D_800D2D6C__s->unk3E = D_800D2D6C__s->unk74;
        D_800D2D6C__s->unk3F += D_800D2D6C__s->unk56;
        D_800D2D6C__s->unk3E += D_800D2D6C__s->unk56;
        if (D_800C3E00__s->unk8A & 0x2000) {
            for (j = 0; j < 0x26; j++) {
                (D_800C34B0 + (i * 0x5F0 + j * 0x28))[0x106B] *= 2;
            }
            for (j = 0; j < 0x2A; j++) {
                (D_800C34B0 + (i * 0x690 + j * 0x28))[0x223B] *= 2;
            }
        }
        if (D_800C3E00__s->unk32 & 0x4000) {
            for (j = 0; j < 0x26; j++) {
                (D_800C34B0 + (i * 0x5F0 + j * 0x28))[0x106B] =
                    ((D_800C34B0 + (i * 0x5F0 + j * 0x28))[0x106B] + 1) >> 1;
            }
            for (j = 0; j < 0x2A; j++) {
                (D_800C34B0 + (i * 0x690 + j * 0x28))[0x223B] =
                    ((D_800C34B0 + (i * 0x690 + j * 0x28))[0x223B] + 1) >> 1;
            }
        }
        if (D_800C3E00__s->unk62 >= 0x32) {
            g_GameState__s.chars[D_800C3E00__s->unk56].unk4 |= 8;
        }
        if (D_800C3E00__s->unk56 == 7) {
            D_800D2D6C__s->unk60 = D_800C3E00__s->unk4C * 50;
            D_800D2D6C__s->unk64 = D_800C3E00__s->unk4E * 50;
            D_800D2D6C__s->unk3C = D_800C3E00__s->unk58;
            D_800D2D6C__s->unk70 = D_800C3E00__s->unk59 * 12;
            D_800D2D6C__s->unk72 = D_800C3E00__s->unk5C * 6;
            D_800D2D6C__s->unk98 = D_800C3E00__s->unk5A;
            bits = D_800D2D6C__s->unk7E;
            D_800D2D6C__s->unk7E = bits | 0x3C4;
            if (D_800C3E00__s->unk82 & 0x2000) {
                D_800D2D6C__s->unk7E = bits | 0x13C4;
            }
        }
        if (D_800C3E00__s->unkA0 == 0xF) {
            g_GameState__s.chars[D_800C3E00__s->unk56].unk1A &= 0x8FFF;
        }
        if (D_800C3E00__s->unkA0 == 0x12) {
            D_800C3E00__s->unk7A = 0x238;
            D_800D2D6C__s->unk38 = 0x26AC;
            D_800D2D6C__s->unk3A = 0x26AC;
            sub[1] = 0;
            *(u16*)(g_GameState__b + 0x173A) = 0x8000;
        }
        if (*(u16*)(g_GameState__b + 0x1930) >= 0xE7) {
            t = g_GameState__b[0x2355];
            if (!(t & 0x80)) {
                g_GameState__b[0x89A] = 0x27;
                g_GameState__b[0x834] = 0x1E;
                g_GameState__b[0x2355] = t | 0x80;
            }
        }
    }
    D_800C34AD = 3;
    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] == 0x7F) {
            D_800C34AD--;
        }
    }
    for (i = 0; i < 3; i++) {
        ((M53Tail*)D_800C34B0)->unk5F54[i] = 0;
        ((M53Tail*)D_800C34B0)->unk5F60[i] = 0;
    }
}
extern u8* D_800C34B0;
extern u8* D_800D2D6C;
extern u8* D_800C3E00;
extern u8 D_800D2D24[];
extern u8 D_800D2D10[];
extern u16 D_800C3AA4[];
extern u8 g_GameState__b[] asm("g_GameState");

/* For each of the first three D_800D2D24 slots not set to 0x7F: select its
 * 0x170-byte record (D_800C3E00, D_800D2D6C = record + 0xA4), keep its +0x7A
 * halfword in D_800C3AA4, and for every nonzero D_800D2D10[j] subtract it
 * from the battle part's +0x98 byte and add the part's +0x50 + j byte
 * instead, capping +0x98 at 0x10.  Then raw g_GameState patches: 7 at
 * +0x7E6, bit 0x800 of +0x17C2 when +0x17DA has bit 0x2000, and a fixed set
 * of byte values while the +0x1930 halfword is below 0xBB. */
void func_8009892C(void) {
    u8 i;
    u8 j;
    u8* rec;

    for (i = 0; i < 3; i++) {
        if (D_800D2D24[i] != 0x7F) {
            rec = D_800C34B0 + i * 0x170;
            D_800C3E00 = rec;
            D_800D2D6C = rec + 0xA4;
            D_800C3AA4[i] = ((u16*)rec)[0x7A / 2];
            for (j = 0; j < 4; j++) {
                if (D_800D2D10[j] != 0) {
                    D_800D2D6C[0x98] -= D_800D2D10[j];
                    D_800D2D6C[0x98] += (D_800D2D6C + j)[0x50];
                }
            }
            if (D_800D2D6C[0x98] > 0x10) {
                D_800D2D6C[0x98] = 0x10;
            }
        }
    }
    g_GameState__b[0x7E6] = 7;
    if (*(u16*)(g_GameState__b + 0x17DA) & 0x2000) {
        *(u16*)(g_GameState__b + 0x17C2) |= 0x800;
    }
    if (*(u16*)(g_GameState__b + 0x1930) < 0xBB) {
        g_GameState__b[0x9EC] = 0xA;
        g_GameState__b[0xA90] = 0xA;
        g_GameState__b[0x10F8] = 9;
        g_GameState__b[0x119C] = 9;
        g_GameState__b[0x1240] = 8;
        g_GameState__b[0x12E4] = 0xC;
        g_GameState__b[0x1388] = 0xC;
        g_GameState__b[0x11CE] = 0x58;
        g_GameState__b[0xDF7] = 0;
        g_GameState__b[0x131C] = 0x28;
    }
}
extern u8* D_800C34B0;
extern u8* D_800D2D6C;
extern u8* D_800C3E00;
extern u8* D_800C3DFC;
extern s32 rand(void);
extern void func_80094D24(void);

/* Base value for actor `idx` (0x170-byte records at D_800C34B0: a
 * GameCharacter copy, then the battle part at +0xA4).  Selects the actor
 * (D_800C3E00, D_800D2D6C).  For idx < 3, the 0x28-byte entry picked by the
 * D_800C34B0 +0x5FC2 byte in the idx block at +0x1058 (0x5F0 apart), or at
 * +0x2228 (0x690 apart) while record +0x15A bit 7 is set, goes to D_800C3DFC
 * and the value is 9 * (stat - entry +0x27), at least 9, where stat is
 * agility (+0x5A) or the battle part's +0x98 byte respectively; for idx >= 3
 * it is 9 * agility.  The value, capped at 0xA0 above 0xA5, becomes
 * 0xA5 - value with a random -3..+4 spread; func_80094D24 runs before the
 * return. */
u8 func_80098AF8(u8 idx) {
    u8* rec;
    u16 v;

    rec = D_800C34B0 + idx * 0x170;
    D_800D2D6C = rec + 0xA4;
    D_800C3E00 = rec;
    if (idx < 3) {
        if (!(rec[0x15A] & 0x80)) {
            D_800C3DFC = (idx * 0x5F0 + 0x1058) + D_800C34B0 + D_800C34B0[0x5FC2] * 0x28;
            if (rec[0x5A] > (s8)D_800C3DFC[0x27]) {
                v = (rec[0x5A] - (s8)D_800C3DFC[0x27]) * 9;
            } else {
                v = 9;
            }
        } else {
            D_800C3DFC = (idx * 0x690 + 0x2228) + D_800C34B0 + D_800C34B0[0x5FC2] * 0x28;
            if (rec[0x13C] > (s8)D_800C3DFC[0x27]) {
                v = (rec[0x13C] - (s8)D_800C3DFC[0x27]) * 9;
            } else {
                v = 9;
            }
        }
    } else {
        v = rec[0x5A] * 9;
    }
    if (v > 0xA5) {
        v = 0xA0;
    }
    v = 0xA5 - v;
    v -= rand() % 8 - 4;
    func_80094D24();
    return v;
}
void func_80097D08();                            /* extern */
extern u8* D_800C34B0;
extern u16 D_800D220E[];
extern s16 D_800D2C98;
extern u16 D_800D2C9E;

void func_80098C6C(s32 arg0) {
    s32 var_s0;
    s32 var_s1;

    func_80097D08();
    var_s1 = 1;
    var_s0 = 0;
    do {
        if (var_s1 & *(u16 *)(D_800C34B0 + 0x5FAC)) {
            func_80098D2C(var_s0 & 0xFF, arg0 & 0xFF);
        }
        var_s1 <<= 1;
        var_s0 += 1;
    } while ((u32) (var_s0 & 0xFF) < 0xBU);
    *(u16 *)(D_800C34B0 + 0x5FB0) = D_800D220E[(arg0 & 0xFF) * 8];
    if (D_800D2C9E & 0x8000) {
        D_800D2C98 = 0xC2;
    }
}

typedef struct {
    s32 unk0[0xD];
    u16 unk34;
    u16 unk36;
    u16 unk38;
    u8 unk3A[0x12];
    s16 unk4C;
    u16 unk4E;
    u8 unk50[0x6];
    u8 unk56;
    u8 unk57[0x21];
    u16 unk78;
    u16 unk7A;
    u16 unk7C;
    u16 unk7E;
    u16 unk80;
    u16 unk82;
    u16 unk84;
    u16 unk86;
    u16 unk88;
    u16 unk8A;
    u16 unk8C;
    u16 unk8E;
    u16 unk90[7];
    u8 unk9E[0x6];
    u8 unkA4[0xB6];
    u8 unk15A;
    u8 unk15B;
    u8 unk15C[0x14];
} M53Actor;

extern M53Actor D_800CCCE8__a[] asm("D_800CCCE8");

typedef struct {
    u8 unk0[0x8];
    u8 unk8;
    u8 unk9;
    s16 unkA;
    s16 unkC;
    u16 unkE;
} M53Entry;

/* 0xA4-byte g_GameState character records viewed from their +0x2B0 word. */
typedef struct {
    s32 unk0;
    s32 unk4;
    u8 unk8[0x9C];
} M53GsRow;

extern M53GsRow g_GameState__r2B0[] asm("g_GameState+0x2B0");
extern u8 D_800D2C88[];
extern s32 D_800D2C54[];
extern u8 D_800D2CAF;
extern void func_8009AC48(u8 idx, u8 check);
extern void func_8009B684(u8 menu, u16 mask);
void func_800995A0(u8 actor, u8 menu, u16 button, u8 value);
s32 func_80099498(void);

/* func_80098D2C.s, byte-exact: apply item/effect entry `target` (16-byte
 * records at D_800CCCE8 + 0x5518) to actor `actor`.  Damage (0x8000: 50 per
 * unit into the +0x5F6C word, doubled by +0x86 bit 7) and healing (0x4000:
 * 10 per unit into D_800D2C54) set the D_800D2C88 result and stop; else an
 * actor with +0x15A bit 7 refuses everything but kind 1.  The other flag
 * bits clear or set the +0x7A..+0x8E status words (0x100 also revives:
 * clears all of them and sets +0x4C from +0x4E), 0x80 toggles +0x7C via
 * func_80099498, and kind 1 runs the sub-switch on unk8 (10..15).
 * Codegen: the s32 padding keeps the record 4-byte aligned for
 * func_80099890's word compares; heal (10) and dmg (50) are variables, so
 * retail multiplies with mult; the g_GameState row view keeps the frame. */
void func_80098D2C(u8 actor, u8 target) {
    u8 hits = 0;
    M53Actor* rec = &D_800CCCE8__a[actor];
    M53Entry* entry = (M53Entry*)((u8*)D_800CCCE8__a + 0x5518) + target;
    u8* stats = D_800CCCE8__a[actor].unkA4;
    s32 heal = 10;
    u8 i;

    if (entry->unkA & 0x8000) {
        u8 n = entry->unk8;
        s32 dmg = 50;

        D_800D2C88[actor] = 2;
        *(s32*)((u8*)D_800CCCE8__a + 0x5F6C + actor * 4) = n * dmg;
        if (rec->unk86 & 0x80) {
            *(s32*)(D_800C34B0 + 0x5F6C + actor * 4) *= 2;
        }
        if (rec->unk36 & 0x8000) {
            D_800D2C88[actor] = 0;
        }
        hits = 1;
    }
    if (entry->unkA & 0x4000) {
        hits++;
        D_800D2C54[actor] = heal * entry->unk8;
        D_800D2C88[actor] = 3;
    }
    if (hits != 0) {
        return;
    }
    if ((D_800CCCE8__a[actor].unk15A & 0x80) && entry->unkA != 1) {
        D_800D2CAF = 0x30;
        D_800D2C9E |= 0x8000;
        return;
    }
    if (entry->unkA & 0x2000) {
        rec->unk7C &= 0x8000;
    }
    if (entry->unkA & 0x1000) {
        rec->unk80 = 0;
        rec->unk7A &= 0xFFDF;
    }
    if (entry->unkA & 0x100) {
        if (D_800CCCE8__a[actor].unk56 == 2) {
            func_8009AC48(actor, 0);
        }
        rec->unk7C = 0;
        rec->unk80 = 0;
        rec->unk84 = 0;
        rec->unk88 = 0;
        rec->unk8C = 0;
        rec->unk4C = rec->unk4E * entry->unk8 / 10;
        *(u16*)(D_800C34B0 + 0x5FB6) |= 1 << actor;
    }
    if (entry->unkA & 0x800) {
        rec->unk84 |= entry->unkC;
        func_800995A0(actor, 5, entry->unkC, 5);
        func_8009B684(5, entry->unkC);
    }
    if (entry->unkA & 0x400) {
        rec->unk88 |= entry->unkC;
        func_800995A0(actor, 7, entry->unkC, 5);
        func_8009B684(7, entry->unkC);
    }
    if (entry->unkA & 0x200) {
        rec->unk8C |= entry->unkC;
        if ((entry->unkC & 0xF000) && !(rec->unk8E & 0xF000)) {
            rec->unk8C &= 0xFFF;
            rec->unk8C |= entry->unkC;
            func_800995A0(actor, 9, entry->unkC, 5);
            func_8009B684(9, entry->unkC);
        }
        if ((entry->unkC & 0xF00) && !(rec->unk8E & 0xF00)) {
            rec->unk8C &= 0xF0FF;
            rec->unk8C |= entry->unkC;
            func_800995A0(actor, 9, entry->unkC, 5);
            func_8009B684(9, entry->unkC);
        }
    }
    if (entry->unkA & 0x20) {
        rec->unk7E |= 0x3F7C;
    }
    if (entry->unkA & 0x10) {
        rec->unk7E |= 0xFFFE;
    }
    if (entry->unkA & 8) {
        (&rec->unk84)[entry->unk8] = 0;
        switch (entry->unk8) {
        case 0:
            D_800C34B0[0x5FC7] = 0x35;
            break;
        case 2:
            D_800C34B0[0x5FC7] = 0x36;
            break;
        case 4:
            D_800C34B0[0x5FC7] = 0x37;
            break;
        }
    }
    if ((entry->unkA & 0x80) && (entry->unkC & 2)) {
        if (actor >= 3) {
            D_800C34B0[0x5FC7] = 0x33;
            return;
        }
        if (entry->unk9 == 0) {
            if (func_80099498() & 0xFF) {
                rec->unk7C |= entry->unkC;
                D_800C34B0[0x5FC7] = 0x31;
                rec->unk7A = 0xFFEF;
            } else {
                D_800C34B0[0x5FC7] = 0x32;
            }
        } else {
            rec->unk7C &= ~entry->unkC;
            rec->unk7A = D_800C3AA4[actor];
        }
    }
    if (entry->unkA == 1) {
        switch (entry->unk8) {
        case 10:
            rec->unk38 = 0;
            rec->unk38 |= entry->unkC;
            break;
        case 11:
            if (!(rec->unk7E & entry->unkC)) {
                rec->unk7C |= entry->unkC;
                func_800995A0(actor, 0, entry->unkC, entry->unk9);
                func_8009B684(0, entry->unkC);
            }
            break;
        case 12:
            if (!(rec->unk82 & entry->unkC)) {
                rec->unk80 |= entry->unkC;
                func_800995A0(actor, 2, entry->unkC, entry->unk9);
                func_8009B684(2, entry->unkC);
            }
            break;
        case 13:
            stats[0x99] += entry->unkC;
            D_800C34B0[0x5FC7] = 0x28;
            break;
        case 14:
            g_GameState__r2B0[rec->unk56].unk0 = 1;
            g_GameState__r2B0[rec->unk56].unk4 = 1;
            break;
        case 15:
            for (i = 0; i < 7; i++) {
                rec->unk90[i] += 10;
            }
            break;
        }
    }
}
extern u8 D_800C34AD;
extern u8* D_800C34B0;

s32 func_80099498(void) {
    u8 i;
    u8 count;
    s32 result;

    result = 0;
    switch (D_800C34AD) {
    case 2:
        count = 0;
        for (i = 0; i < 3; i++) {
            if (*(u16 *)((u8 *)((s32)D_800C34B0 - (-(i * 0x170))) + 0x7C) & 0xC002) {
                count++;
            }
        }
        if (count == 0) {
            result = 1;
        }
        break;
    case 3:
        count = 0;
        for (i = 0; i < 3; i++) {
            if (*(u16 *)((u8 *)((s32)D_800C34B0 - (-(i * 0x170))) + 0x7C) & 0xC002) {
                count++;
            }
        }
        if (count != 2) {
            result = 1;
        }
        break;
    case 1:
        break;
    }
    return result;
}

extern u8 D_800C3E04;
extern u8* D_800C3E34;
extern u8 D_800CCCE8[];
extern u8 D_800CCD72[];
extern u8 D_800CCE44[];

/* func_800995A0.s, byte-exact under gcc 2.6.3 (BattleGcc263).  Store an
 * actor's value for a (menu, button) pair into byte +0x15C + slot of its
 * 0x170-byte D_800CCCE8 record (D_800CCE44 row).  The value doubles when the
 * current actor's (D_800C3E04) +0x8A word has 0x2000.  Slots: menu 0
 * 0x2000/0x1000 -> 0/1; menu 2 0x1000/0x800 -> 2/3; menu 5 0x8000/0x4000 ->
 * 4/5; menu 7 0x8000/0x4000/0x1000 -> 9/0xA/0xB; menu 9 0x1000..0x8000 ->
 * 0xC, 0x100..0x800 -> 0xD; any other pair (slot 0xF) stores nothing.  For
 * menus 5/7/9 the D_800C3E34 +0x88 | +0x8A word doubles the value (0x5) and
 * halves it (0xA), and the actor record's +0x32 bit 0x40 doubles it again.
 * Codegen: each switch lists its cases in slot order (retail's inline and
 * out-of-line bodies; menu 0's 0x2000 case becomes a store-flag); slot = 0xF
 * comes after the first test; the row pointer is a local (la + add, then
 * + slot). */
void func_800995A0(u8 actor, u8 menu, u16 button, u8 value) {
    u8 slot;
    u8* rec;
    u16 dir;
    u8* row;

    if (*(u16*)(D_800CCD72 + D_800C3E04 * 368) & 0x2000) {
        value *= 2;
    }
    slot = 0xF;
    rec = D_800CCCE8 + actor * 368;
    if (menu == 0) {
        switch (button) {
        case 0x2000:
            slot = 0;
            break;
        case 0x1000:
            slot = 1;
            break;
        }
    }
    if (menu == 2) {
        switch (button) {
        case 0x1000:
            slot = 2;
            break;
        case 0x800:
            slot = 3;
            break;
        }
    }
    if (menu == 5) {
        switch (button) {
        case 0x8000:
            slot = 4;
            break;
        case 0x4000:
            slot = 5;
            break;
        }
    }
    if (menu == 7) {
        switch (button) {
        case 0x8000:
            slot = 9;
            break;
        case 0x4000:
            slot = 0xA;
            break;
        case 0x1000:
            slot = 0xB;
            break;
        }
    }
    if (menu == 9) {
        switch (button) {
        case 0x8000:
        case 0x4000:
        case 0x2000:
        case 0x1000:
            slot = 0xC;
            break;
        case 0x800:
        case 0x400:
        case 0x200:
        case 0x100:
            slot = 0xD;
            break;
        }
    }
    if (slot != 0xF) {
        switch (menu) {
        case 5:
        case 7:
        case 9:
            dir = *(u16*)(D_800C3E34 + 0x88) | *(u16*)(D_800C3E34 + 0x8A);
            if (dir & 5) {
                value *= 2;
            }
            if (dir & 0xA) {
                value /= 2;
            }
            break;
        }
        if (*(u16*)(rec + 0x32) & 0x40) {
            switch (menu) {
            case 5:
            case 7:
            case 9:
                value *= 2;
                break;
            }
        }
        row = D_800CCE44 + actor * 368;
        row[slot] = value;
    }
}
/* func_80099890.s, byte-exact under gcc 2.6.3 (BattleGcc263).  Count down an
 * actor's status timers (bytes +0x15C + k of its 0x170-byte D_800CCCE8
 * record): when the record's +0x15A has 0x80, hand the +0xA4 block, the
 * record and the timers to func_80099CF0 and return 0.  Otherwise each
 * active status (+0x7C 0x1000; +0x80 0x1000 / 0x800; +0x84 bits
 * 0x8000..0x800 and +0x88 bits 0x8000 / 0x4000 / 0x1000 unless the same bit is
 * set in +0x86 / +0x8A; +0x8C 0xF000 / 0xF00 unless set in +0x8E) decrements
 * its timer and, when it reaches 0, clears the status bit (+0x80 0x800 also
 * clears +0x7A 0x20) and ORs its result bit (0x4000 down to 4) into the
 * returned mask.  Codegen: with the record view 4-byte aligned (the s32
 * padding) each "bit set and not blocked" pair of u16 tests compiles to one
 * word compare, as in retail (u8 padding keeps the tests separate); timers
 * count down by + 0xFF as in func_80099CF0. */
u16 func_80099890(u8 actor) {
    M53Actor* rec = &D_800CCCE8__a[actor];
    u8* stats = D_800CCCE8__a[actor].unkA4;
    u8* timers = D_800CCCE8__a[actor].unk15C;
    u16 result;

    if (D_800CCCE8__a[actor].unk15A & 0x80) {
        func_80099CF0(stats, rec, timers);
        return 0;
    }
    result = 0;
    if (rec->unk7C & 0x1000) {
        timers[1] += 0xFF;
        if (timers[1] == 0) {
            result |= 0x4000;
            rec->unk7C &= 0xEFFF;
        }
    }
    if (rec->unk80 & 0x1000) {
        timers[2] += 0xFF;
        if (timers[2] == 0) {
            result |= 0x2000;
            rec->unk80 &= 0xEFFF;
        }
    }
    if (rec->unk80 & 0x800) {
        timers[3] += 0xFF;
        if (timers[3] == 0) {
            result |= 0x1000;
            rec->unk80 &= 0xF7FF;
            rec->unk7A &= 0xFFDF;
        }
    }
    if ((rec->unk84 & 0x8000) && !(rec->unk86 & 0x8000)) {
        timers[4] += 0xFF;
        if (timers[4] == 0) {
            result |= 0x800;
            rec->unk84 &= 0x7FFF;
        }
    }
    if ((rec->unk84 & 0x4000) && !(rec->unk86 & 0x4000)) {
        timers[5] += 0xFF;
        if (timers[5] == 0) {
            result |= 0x400;
            rec->unk84 &= 0xBFFF;
        }
    }
    if ((rec->unk84 & 0x2000) && !(rec->unk86 & 0x2000)) {
        timers[6] += 0xFF;
        if (timers[6] == 0) {
            result |= 0x200;
            rec->unk84 &= 0xDFFF;
        }
    }
    if ((rec->unk84 & 0x1000) && !(rec->unk86 & 0x1000)) {
        timers[7] += 0xFF;
        if (timers[7] == 0) {
            result |= 0x100;
            rec->unk84 &= 0xEFFF;
        }
    }
    if ((rec->unk84 & 0x800) && !(rec->unk86 & 0x800)) {
        timers[8] += 0xFF;
        if (timers[8] == 0) {
            result |= 0x80;
            rec->unk84 &= 0xF7FF;
        }
    }
    if ((rec->unk88 & 0x8000) && !(rec->unk8A & 0x8000)) {
        timers[9] += 0xFF;
        if (timers[9] == 0) {
            result |= 0x40;
            rec->unk88 &= 0x7FFF;
        }
    }
    if ((rec->unk88 & 0x4000) && !(rec->unk8A & 0x4000)) {
        timers[0xA] += 0xFF;
        if (timers[0xA] == 0) {
            result |= 0x20;
            rec->unk88 &= 0xBFFF;
        }
    }
    if ((rec->unk88 & 0x1000) && !(rec->unk8A & 0x1000)) {
        timers[0xB] += 0xFF;
        if (timers[0xB] == 0) {
            result |= 0x10;
            rec->unk88 &= 0xEFFF;
        }
    }
    if ((rec->unk8C & 0xF000) && !(rec->unk8E & 0xF000)) {
        timers[0xC] += 0xFF;
        if (timers[0xC] == 0) {
            result |= 8;
            rec->unk8C &= 0x0FFF;
        }
    }
    if ((rec->unk8C & 0xF00) && !(rec->unk8E & 0xF00)) {
        timers[0xD] += 0xFF;
        if (timers[0xD] == 0) {
            result |= 4;
            rec->unk8C &= 0xF0FF;
        }
    }
    return result;
}

#ifndef XENO_PC_PORT
void func_80099CF0(void *arg0, void *arg1, void *arg2) {
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0x200) {
        *(u8 *)((s8*)(arg2) + 1) = (u8) (*(u8 *)((s8*)(arg2) + 1) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 1) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xFDFF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0x100) {
        *(u8 *)((s8*)(arg2) + 2) = (u8) (*(u8 *)((s8*)(arg2) + 2) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 2) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xFEFF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0x80) {
        *(u8 *)((s8*)(arg2) + 3) = (u8) (*(u8 *)((s8*)(arg2) + 3) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 3) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xFF7F);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0x20) {
        *(u8 *)((s8*)(arg2) + 4) = (u8) (*(u8 *)((s8*)(arg2) + 4) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 4) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xFFDF);
            *(u16 *)((s8*)(arg1) + 0x7C) = (u16) (*(u16 *)((s8*)(arg1) + 0x7C) & 0xEFFF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0x10) {
        *(u8 *)((s8*)(arg2) + 5) = (u8) (*(u8 *)((s8*)(arg2) + 5) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 5) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xFFEF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0xF000) {
        *(u8 *)((s8*)(arg2) + 7) = (u8) (*(u8 *)((s8*)(arg2) + 7) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 7) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xFFF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x7C) & 0xF00) {
        *(u8 *)((s8*)(arg2) + 8) = (u8) (*(u8 *)((s8*)(arg2) + 8) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 8) == 0) {
            *(u16 *)((s8*)(arg0) + 0x7C) = (u16) (*(u16 *)((s8*)(arg0) + 0x7C) & 0xF0FF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x80) & 0x1000) {
        *(u8 *)((s8*)(arg2) + 0xA) = (u8) (*(u8 *)((s8*)(arg2) + 0xA) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 0xA) == 0) {
            *(u16 *)((s8*)(arg0) + 0x80) = (u16) (*(u16 *)((s8*)(arg0) + 0x80) & 0xEFFF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x80) & 0x40) {
        *(u8 *)((s8*)(arg2) + 0xB) = (u8) (*(u8 *)((s8*)(arg2) + 0xB) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 0xB) == 0) {
            *(u16 *)((s8*)(arg0) + 0x80) = (u16) (*(u16 *)((s8*)(arg0) + 0x80) & 0xFFBF);
        }
    }
    if (*(u16 *)((s8*)(arg0) + 0x80) & 0x20) {
        *(u8 *)((s8*)(arg2) + 0xC) = (u8) (*(u8 *)((s8*)(arg2) + 0xC) + 0xFF);
        if (*(u8 *)((s8*)(arg2) + 0xC) == 0) {
            *(u16 *)((s8*)(arg0) + 0x80) = (u16) (*(u16 *)((s8*)(arg0) + 0x80) & 0xFFDF);
        }
    }
}
#endif

#endif
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifdef XENO_PC_PORT
/* Port body; the retail build compiles the byte-exact C in the #else branch.
 */
/* func_80099FB0.s: copy the four D_800C3DFC status bytes into the
 * D_800C34B0 +0x5FBC block, mirror +0x5FC2 into +0x5FC0, and when the
 * copied +0x5FBE byte has no low-6 bits, fold in the top nibble of the
 * combined D_800C3E00 +0x8C/+0x8E halfwords. */
void func_80099FB0(void) {
    u32 m = *(u16*)(D_800C3E00 + 0x8C) | *(u16*)(D_800C3E00 + 0x8E);
    u8* p;
    u8 v;

    D_800C34B0[0x5FBC] = D_800C3DFC[0x20];
    D_800C34B0[0x5FBD] = D_800C3DFC[0x21];
    D_800C34B0[0x5FBE] = D_800C3DFC[0x22];
    D_800C34B0[0x5FBF] = D_800C3DFC[0x23];
    D_800C34B0[0x5FC0] = D_800C34B0[0x5FC2];
    p = D_800C34B0;
    v = p[0x5FBE];
    if ((v & 0x3F) == 0) {
        p[0x5FBE] = (m >> 12) | v;
    }
}
#else
/* func_80099FB0.s, byte-exact: as func_8009C9C4 (main59) does with other
 * halfwords, the flag bits are computed before the +0x5FBE test. */
extern u8* D_800C3DFC;

void func_80099FB0(void) {
    u32 a0 = *(u16*)(D_800C3E00 + 0x8C);
    u32 a1 = *(u16*)(D_800C3E00 + 0x8E);
    u32 v;
    u32 bits;

    D_800C34B0[0x5FBC] = D_800C3DFC[0x20];
    D_800C34B0[0x5FBD] = D_800C3DFC[0x21];
    D_800C34B0[0x5FBE] = D_800C3DFC[0x22];
    D_800C34B0[0x5FBF] = D_800C3DFC[0x23];
    D_800C34B0[0x5FC0] = D_800C34B0[0x5FC2];
    bits = (a0 | a1) >> 12;
    v = D_800C34B0[0x5FBE];
    if ((v & 0x3F) == 0) {
        D_800C34B0[0x5FBE] = bits | v;
    }
}
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif


/* func_8009A074.s */
void func_8009A074(void) {
    if ((*(u16*)(D_800C34B0 + 0x5FAC) & 7) != 0) {
        *(u16*)(D_800C34B0 + 0x5FAC) = *(u16*)(D_800C34B0 + 0x5FAE) & 7;
    }
    if ((*(u16*)(D_800C34B0 + 0x5FAC) & 0x7F8) != 0) {
        *(u16*)(D_800C34B0 + 0x5FAC) = *(u16*)(D_800C34B0 + 0x5FAE) & 0x7F8;
    }
}

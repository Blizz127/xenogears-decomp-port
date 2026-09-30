#include "common.h"
#ifndef XENO_PC_PORT
s32 rand();
extern void *D_800C34B0;
extern void * D_800C3DFC;
extern void * D_800C3E00;
extern s8 D_800D2DC4;
void func_80096824(void);
#endif

#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif


#ifndef XENO_PC_PORT
s8 func_80097964(u8, u8, u16);
void func_800995A0(u8, u8, u16, s32);
extern void *D_800C3DFC;
extern void *D_800C3E00;
extern u8 D_800C3E50;
void func_80095B44(void);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_8008FE18);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_8009023C);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80090310);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800904A0);
extern u8 D_800D2CE0[];
extern u8* func_8008AC00(s32 size);
extern void bzero(void* p, s32 n);
extern s32 D_800D329C;
extern u8* GetStringEntry(s32 table, u8 id);
extern s32 SystemRenderStringEntry(u8* str, u8* buf, s32 w, s32 flags);
extern void func_800769E8(s16* rect, u8* buf);
extern s32 D_800C3EA4;
extern u8 D_800CCB34;
extern void func_80076C78(u8* p, s32 x, s32 y, s32 a3, s32 a4, s32 n);
extern u_int HeapFree(void* pMem);

/* Render string D_800D2CE0[b * 2 + a] into a 0x39-wide buffer, upload it at
 * (0x3C0, 0) and set up the text sprite. */
void func_8009070C(u8 a, u8 b) {
    s16 r[4];
    u8 id = D_800D2CE0[b * 2 + a];
    u8* buf = func_8008AC00(0x39);
    s32 n;

    bzero(buf, 0x618);
    n = SystemRenderStringEntry(GetStringEntry(D_800D329C, id), buf, 0x39, 0);
    r[0] = 0x3C0;
    r[2] = 0x3C;
    r[1] = 0;
    r[3] = 0xD;
    func_800769E8(r, buf);
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x280 + *(s32*)&D_800CCB34 * 40, 0x30, 0x42, 0, 0, n & 0xFF);
    HeapFree(buf);
}

extern u8 D_800D2CE0[];
extern u8 D_800C3D70[];
extern s32 D_800C3EA4;
extern u8 D_800CCB34;
extern void func_80090310(u8 a, u8 b);
extern void func_8009070C(u8 a, u8 b);
extern void func_800904A0(u8 a, u8 b);

/* Show text slot (a, b): its string id comes from D_800D2CE0 (mode 0) or
 * D_800C3D70; mode 0 with a non-zero id also renders it and flags +0x66D. */
void func_8009080C(u8 a, u8 b, u8 mode) {
    u8 id;
    u8* w;

    if (mode == 0) {
        id = D_800D2CE0[b * 2 + a];
    } else {
        id = D_800C3D70[b * 2 + a];
    }
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66D] = 0;
    func_80090310(a, b);
    if (id != 0 && mode == 0) {
        func_8009070C(a, b);
        func_800904A0(a, b);
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66D] = 1;
    }
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66A] = D_800CCB34;
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66B] = 1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main51", func_8009093C);
extern s32 func_80076A10(s32, s32, s16, s16);
extern s32 D_800C3EA4;
extern u8 D_800CCB34;
extern u8* D_800D2D28;

void func_80090B90(s32 x, s32 y, s32* frame, u8* timer) {
    if (++*timer >= 3) {
        if (--*frame < 0) {
            *frame = 4;
        }
        *timer = 0;
    }
    *(s32*)(D_800D2D28 + 0x100) = func_80076A10(*frame + 0xE0, D_800C3EA4 + 0x27C8, x, y);
    D_800D2D28[0xA7] = D_800CCB34;
    D_800D2D28[0x9E] = 1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80090C44);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80090E7C);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80091064);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80091604);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800916D4);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_8009187C);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80091B38);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80091D38);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80091EC4);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_8009209C);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80092298);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80092784);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80092B74);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800930AC);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80093578);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_8009382C);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800939CC);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80093B08);
#endif

extern void func_8007FB70(s32);
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
extern u8* D_800D2D28;
#endif

void func_8009413C(u8 idx, u8 flag) {
    if (D_800D2D24[idx] == 4) {
        D_800D2D28[0xB7] = 0;
        D_800D2D28[0xB0] = 0;
        if (flag) {
            func_8007FB70(idx);
        }
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800941A4);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800946F4);
#endif
typedef struct { u8 pad[0x15A]; u8 flags; u8 pad2[0x170 - 0x15B]; } Actor170;
typedef struct {
    Actor170 actors[0x42];
    u8 pad[0x5F54 - 0x42 * 0x170];
    s32 f5F54[3];
    s32 f5F60[3];
    s32 f5F6C[8];
    u8 pad2[0x5FA0 - 0x5F8C];
    u8 f5FA0[8];
} Battle;
void func_80094C78(void) {
    u8 i;
    Battle* b = D_800C34B0;
    u32 cur = D_800C3E50;

    for (i = 0; i < 3; i++) {
        if (b->f5FA0[i] == 0) {
            if (b->actors[i].flags & 0x80) {
                b->f5F60[i] += b->f5F6C[cur];
            } else {
                b->f5F54[i] += b->f5F6C[cur];
            }
        }
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80094D24);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80094EE4);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80095690);
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD3E[];
extern u8* D_800C3E34;
extern u8 D_800D2C88[];
extern s32 D_800D2C54[];
extern u16 D_800D2C9E;
#endif
extern void func_8009AC48(u8 actor, s32 a1);

typedef struct {
    u8 pad0[0x4E];
    u16 unk4E;
    u8 pad50[0x2C];
    s16 unk7C;
    s16 pad7E;
    s16 unk80;
    s16 pad82;
    s16 unk84;
    s16 pad86;
    s16 unk88;
    s16 pad8A;
    s16 unk8C;
} BattleActorC34;

void func_800957D8(void) {
    u8 i;

    if (D_800CCD3E[D_800C3E50 * 0x170] == 2) {
        func_8009AC48(D_800C3E50, 1);
    }
    ((BattleActorC34*)D_800C3E34)->unk7C = 0;
    ((BattleActorC34*)D_800C3E34)->unk80 = 0;
    ((BattleActorC34*)D_800C3E34)->unk84 = 0;
    ((BattleActorC34*)D_800C3E34)->unk88 = 0;
    ((BattleActorC34*)D_800C3E34)->unk8C = 0;
    D_800D2C88[D_800C3E50] = 2;
    i = D_800C3E50;
    *(s32*)((u8*)D_800D2C54 + i * 4) = ((BattleActorC34*)D_800C3E34)->unk4E * ((u8*)D_800C3DFC)[0x11] / 10;
    D_800D2C9E |= 1 << i;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800958D8);
void func_80095A78(void) {
    u8* act = D_800C3DFC;
    s8 r = func_80097964(act[0x1C], act[0x1D], *(u16*)(act + 0x1E));

    if (*(u16*)((u8*)D_800C3DFC + 0xA) & 0x4000) {
        func_800995A0(D_800C3E50, ((u8*)D_800C3DFC)[0x1D], *(u16*)((u8*)D_800C3DFC + 0x1E), 5);
    } else {
        func_800995A0(D_800C3E50, ((u8*)D_800C3DFC)[0x1D], *(u16*)((u8*)D_800C3DFC + 0x1E), ((u8*)D_800C3DFC)[0x11]);
        if (r != 1) {
            u8* b = D_800C34B0;

            (b + D_800C3E50)[0x5FA0] = 6;
        }
    }
}


#ifndef XENO_PC_PORT
void func_80095B44(void) {
    if (func_80097964(*(u8 *)((s8*)(D_800C3E00) + 2), *(u8 *)((s8*)(D_800C3E00) + 3), *(u16 *)((s8*)(D_800C3E00) + 0)) == 1) {
        func_800995A0(D_800C3E50, *(u8 *)((s8*)(D_800C3DFC) + 0x1D), *(u16 *)((s8*)(D_800C3DFC) + 0x1E), 5);
    }
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80095BAC);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80095D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80096018);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80096494);

#ifndef XENO_PC_PORT
void func_80096824(void) {
    s32 temp_s0;

    temp_s0 = *(u8 *)((s8*)(D_800C3E00) + 0x5B) + *(u8 *)((s8*)(D_800C3DFC) + 0x14);
    if ((rand() % 100) >= temp_s0) {
        *(s8 *)((s8*)(D_800C34B0) + 0x5FC7) = 0x38;
        D_800D2DC4 = 1;
    }
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/main51", func_800968C0);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80096AB8);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80096FBC);
INCLUDE_ASM("asm/battle/nonmatchings/main51", func_80097610);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_8009795C.s */
void func_8009795C(void) {
}


/* ==== fo/bat2 port bodies, part A (func_8008FE18..func_80093578) ==== */
/* ==== end part A ==== */

/* ==== fo/bat2 port bodies, part B (func_8009382C..func_80097610) ==== */
/* ==== end part B ==== */

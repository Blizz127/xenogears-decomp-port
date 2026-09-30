#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_80077990);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_800780A8);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_8007819C);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_80078310);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_80078508);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_800785D4);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_80078658);
INCLUDE_ASM("asm/battle/nonmatchings/main8", func_800787E0);
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E8C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DE4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DF0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E61[];
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DDC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3718[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3720[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3726[];
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C400B[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C4022[];
#endif
/* func_8007887C.s: append one 72-byte entry at the D_800C3EAC+0x2DA cursor --
 * colour a0, marker 0xF8 and the halfword D_800C3E8C - 1 -- then bump the
 * cursor.  Skipped entirely while D_800C3E8C is 0. */
void func_8007887C(u8 a0) {
    if (D_800C3E8C != 0) {
        u8* p;

        D_800C400B[D_800C3EAC[0x2DA] * 72] = a0;
        D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF8;
        p = D_800C3EAC;
        *(u16*)(D_800C4022 + p[0x2DA] * 72) = D_800C3E8C - 1;
        p[0x2DA] = p[0x2DA] + 1;
    }
}


#ifndef XENO_PC_PORT
extern u8 D_800D2E5F[];
#endif
extern void func_80078658(u8 a, u8 b);
extern void func_800787E0(u8 a, u8 b);
extern void func_8007887C(u8 a);


/* func_8007893C.s */
void func_8007893C(u8 a, u8 b) {
    if (*(u8*)(D_800D2E5F + ((u32)a << 3)) == 0) {
        return;
    }
    func_80078658(a & 0xFF, b & 0xFF);
    func_800787E0(0x1E, b & 0xFF);
    func_8007887C(b & 0xFF);
}


#ifdef XENO_PC_PORT
/* Port-side bodies transcribed from the retail listings; the matching build
 * still assembles the INCLUDE_ASM bytes above. */
#include "battle_guest_call.h"
extern void* GetStringEntry(void* pTable, s32 index);
extern s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag);
extern void func_800769E8(void* pRect, void* pData);

/* func_80078508.s: reset the eleven per-slot records.  Live slots (non-zero
 * D_800D2DCC byte) take their current value from func_80098AF8(slot, 0) into
 * both halfword rows at D_800D2DF0 and +0x16; empty slots get 0xFF there.
 * Every slot then clears D_800D2DE4[i], the +0x2C halfword row and the
 * caller's byte buffer. */
void func_80078508(void* pBuffer) {
    u8* out = (u8*)pBuffer;
    s32 i;

    for (i = 0; i < 11; i++) {
        u16 v;

        if (D_800D2DCC[i] != 0) {
            /* func_80098AF8 (main53) has no C body yet: run the retail bytes. */
            v = (u16)BATTLE_GUEST_CALL(0x80098AF8u, (u32)i, 0u);
        } else {
            v = 0xFF;
        }
        *(u16*)(D_800D2DF0 + 0x16 + i * 2) = v;
        *(u16*)(D_800D2DF0 + i * 2) = v;
        D_800D2DE4[i] = 0;
        *(u16*)(D_800D2DF0 + 0x2C + i * 2) = 0;
        out[i] = 0;
    }
}

/* func_800785D4.s: append a 72-byte entry at the D_800C3EAC+0x2DA cursor --
 * colour a0 and the D_800D2E60/D_800D2E61 halfword of row b -- then bump the
 * cursor.  Unlike its siblings there is no marker byte. */
void func_800785D4(u8 a, u8 b) {
    u32 row = (u32)b << 3;
    u16 word = D_800D2E60[row] | (D_800D2E61[row] << 8);

    D_800C400B[D_800C3EAC[0x2DA] * 72] = a;
    *(u16*)(D_800C4022 + D_800C3EAC[0x2DA] * 72) = word;
    D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
}

/* func_80078658.s: while fewer than nine strips are in use, render string
 * D_800D2E5F[a*8] of the D_800C3DDC table into strip D_800C3E8C (96-byte
 * records at D_800D3718: rect at +0, work word at +8, width byte at +0xE),
 * upload it, then append a 0xFA entry with colour b and the strip index and
 * advance both the strip count and the entry cursor. */
void func_80078658(u8 a, u8 b) {
    u32 strip;
    u8* p;

    if (D_800C3E8C >= 9) {
        return;
    }
    {
        void* entry = GetStringEntry(D_800C3DDC, D_800D2E5F[(u32)a << 3]);
        s32 width;

        strip = D_800C3E8C;
        width = SystemRenderStringEntry(
            entry, PSX_ADDR(*(u32*)(D_800D3720 + strip * 96)), 0x39, strip & 1);
        D_800D3726[D_800C3E8C * 96] = (u8)width;
    }
    strip = D_800C3E8C;
    func_800769E8(D_800D3718 + strip * 96,
                  PSX_ADDR(*(u32*)(D_800D3720 + strip * 96)));
    D_800C400B[D_800C3EAC[0x2DA] * 72] = b;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFA;
    p = D_800C3EAC;
    strip = D_800C3E8C;
    D_800C3E8C = strip + 1;
    *(u16*)(D_800C4022 + p[0x2DA] * 72) = (u16)strip;
    p[0x2DA] = p[0x2DA] + 1;
}

/* func_800787E0.s: append a 0xF7 entry -- colour b, halfword a -- at the
 * D_800C3EAC+0x2DA cursor and bump the cursor. */
void func_800787E0(u8 a, u8 b) {
    u8* p;

    D_800C400B[D_800C3EAC[0x2DA] * 72] = b;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF7;
    p = D_800C3EAC;
    *(u16*)(D_800C4022 + p[0x2DA] * 72) = a;
    p[0x2DA] = p[0x2DA] + 1;
}
#endif

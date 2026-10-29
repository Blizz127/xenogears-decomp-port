#include "common.h"
#ifndef XENO_PC_PORT
s32 rand();
extern void *D_800C34B0;
extern void * D_800C3DFC;
extern void * D_800C3E00;
extern u8 D_800D2DC4;
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
/* Image pointer as an aggregate element, as in main6.c. */
typedef struct {
    void* data;
} BattleImageRef;
extern void func_80077610(void);
extern void func_80076EA4(void);
extern void func_8008FC1C(s32 x, s32 y, s32 w, s32 h, u8 n);
extern void func_800716D8(void);
extern void* GetItemName(s32 index);
extern u8* D_800D2DB0;
extern BattleImageRef D_800C3E5C[];
extern u8 D_800D2CB0[];
extern u8 D_800D2CC0[];
extern u8 D_800D2CE0[];
extern s32 D_800C3EA4;
extern u8* D_800D2D28;


/* func_8008FE18.s, byte-exact: for init != 0 first run func_80077610,
 * func_80076EA4 and func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE).  Upload a
 * blank 0x39-wide buffer (D_800D2DB0) at (0x3C0, 0, 0x3C, 0xD) and copy the
 * 0x30 ids of D_800D2CE0 and values of D_800D2CB0.  For slot i of 32 render
 * the item names ids[i] (flag 0) and ids[i + 0x10] (flag 1) into one
 * 0x1B-high buffer uploaded at (i % 2 * 30 + 0x380, i / 2 * 13 + 0x100, 0x1E,
 * 0xD), and the two digits of vals[i] and of vals[i + 0x10] (D_800C3E5C
 * images, 6 x 0xD) at x i % 2 * 16 + 0x3C0 / 0x3C2 and + 0x3C4 / 0x3C6: a
 * tens digit only when non-zero (the second one tests D_800D2CC0[i], the
 * global, not the copy), a ones digit only for a non-zero id.  Then free the
 * buffers, run func_800716D8() and set +0x669 of the record pointed to by the
 * word at D_800C3EA4 + 0xA230 to 1 and D_800D2D28[0xB7] to 2.  a and b are
 * unused.  Codegen: the unused rect r6 keeps retail's unused 8-byte frame
 * slot before vals. */
void func_8008FE18(u8 a, u8 b, u8 init) {
    BattleImageRef bufs[32];
    s16 r0[4];
    s16 r1[4];
    s16 r2[4];
    s16 r3[4];
    s16 r4[4];
    s16 r5[4];
    s16 r6[4];
    u8 vals[48];
    u8 ids[48];
    s32 i;
    u8* img;

    if (init) {
        func_80077610();
        func_80076EA4();
        func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
    }
    D_800D2DB0 = func_8008AC00(0x39);
    bzero(D_800D2DB0, 0x618);
    r5[0] = 0x3C0;
    r5[2] = 0x3C;
    r5[1] = 0;
    r5[3] = 0xD;
    func_800769E8(r5, D_800D2DB0);
    for (i = 0; i < 0x30; i++) {
        ids[i] = D_800D2CE0[i];
        vals[i] = D_800D2CB0[i];
    }
    for (i = 0; i < 0x20; i++) {
        bufs[i].data = func_8008AC00(0x1B);
        bzero(bufs[i].data, 0x30C);
        r0[0] = i % 2 * 30 + 0x380;
        r0[1] = i / 2 * 13 + 0x100;
        r0[2] = 0x1E;
        r0[3] = 0xD;
        r1[0] = i % 2 * 16 + 0x3C0;
        r1[1] = i / 2 * 13 + 0x100;
        r1[2] = 6;
        r1[3] = 0xD;
        r2[0] = i % 2 * 16 + 0x3C2;
        r2[1] = i / 2 * 13 + 0x100;
        r2[2] = 6;
        r2[3] = 0xD;
        r3[0] = i % 2 * 16 + 0x3C4;
        r3[1] = i / 2 * 13 + 0x100;
        r3[2] = 6;
        r3[3] = 0xD;
        r4[0] = i % 2 * 16 + 0x3C6;
        r4[1] = i / 2 * 13 + 0x100;
        r4[2] = 6;
        r4[3] = 0xD;
        if (ids[i] != 0) {
            SystemRenderStringEntry(GetItemName(ids[i]), bufs[i].data, 0x1B, 0);
        }
        if (ids[i + 0x10] != 0) {
            SystemRenderStringEntry(GetItemName(ids[i + 0x10]), bufs[i].data, 0x1B, 1);
        }
        func_800769E8(r0, bufs[i].data);
        if ((u8)(vals[i] / 10) != 0) {
            img = D_800C3E5C[(u8)(vals[i] / 10)].data;
        } else {
            img = D_800D2DB0;
        }
        func_800769E8(r1, img);
        if (ids[i] != 0) {
            img = D_800C3E5C[(u8)(vals[i] % 10)].data;
        } else {
            img = D_800D2DB0;
        }
        func_800769E8(r2, img);
        if ((u8)(D_800D2CC0[i] / 10) != 0) {
            img = D_800C3E5C[(u8)(vals[i + 0x10] / 10)].data;
        } else {
            img = D_800D2DB0;
        }
        func_800769E8(r3, img);
        if (ids[i + 0x10] != 0) {
            img = D_800C3E5C[(u8)(vals[i + 0x10] % 10)].data;
        } else {
            img = D_800D2DB0;
        }
        func_800769E8(r4, img);
    }
    for (i = 0; i < 0x20; i++) {
        HeapFree(bufs[i].data);
    }
    HeapFree(D_800D2DB0);
    func_800716D8();
    (*(u8**)(D_800C3EA4 + 0xA230))[0x669] = 1;
    D_800D2D28[0xB7] = 2;
}
extern s32 func_80076A10(s32, s32, s16, s16);
extern s32 D_800C3EA4;
extern u8 D_800CCB34;
extern u8* D_800D2D28;

void func_8009023C(s32 arg0) {
    s32 var_s0;
    s32 var_s1;

    var_s1 = 0;
    var_s0 = 0x68;
    *(s32 *)((s8*)(D_800D2D28) + 0xF8) = 0;
    do {
        *(s32 *)((s8*)(D_800D2D28) + 0xF8) = (s32) (func_80076A10(0x66, D_800C3EA4 + 0xBA8 + (*(s32 *)((s8*)(D_800D2D28) + 0xF8) * 0x50), 0x20, (s16) (var_s0 - 4 + arg0)) + *(s32 *)((s8*)(D_800D2D28) + 0xF8));
        var_s0 += 8;
    } while (++var_s1 < 9);
    *(u8 *)((s8*)(D_800D2D28) + 0xA5) = (u8) D_800CCB34;
    *(s8 *)((s8*)(D_800D2D28) + 0x9C) = 1;
}

extern void func_80076C78(u8* p, s32 x, s32 y, s32 a3, s32 a4, s32 n);
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern s32 D_800CCB34__w[] asm("D_800CCB34");

/* func_80090310.s, byte-exact: with k = b * 2 + a, for k > 16 (then
 * k -= 16, and the second sprite's fourth argument gains 0x10) set the CLUT
 * halfword +0x14E (sprite +0x140) at D_800CCB34 * 40 of the record pointed
 * to by the word at D_800C3EA4 + 0xA230 to g_SystemPalette2, otherwise to
 * g_SystemPalette1; then set up the func_80076C78 sprites at +0x140 (0x18,
 * 0x33, (u8)(k % 2 * 120), (u8)(k / 2 * 13), 0x60) and +0x190 (0x84, 0x33,
 * (u8)(k % 2 << 6 | off), (u8)(k / 2 * 13), 0x10).  CLUT store as in
 * func_8009093C (block-local `rec`, written in each branch). */
void func_80090310(u8 a, u8 b) {
    s32 k = b * 2 + a;
    s32 off = 0;
    s32 q;
    s32 r;

    if (k > 16) {
        k -= 16;
        {
            s32 base = D_800C3EA4;
            u16 pal = g_SystemPalette2;
            {
                u8* rec = *(u8**)(base + 0xA230);
                u8* p = rec + D_800CCB34__w[0] * 40;
                *(u16*)(p + 0x14E) = pal;
            }
        }
        off = 0x10;
    } else {
        {
            s32 base = D_800C3EA4;
            u16 pal = g_SystemPalette1;
            {
                u8* rec = *(u8**)(base + 0xA230);
                u8* p = rec + D_800CCB34__w[0] * 40;
                *(u16*)(p + 0x14E) = pal;
            }
        }
    }
    q = k / 2;
    r = k - q * 2;
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x140 + D_800CCB34__w[0] * 40, 0x18, 0x33, (u8)(r * 120), (u8)(q * 13), 0x60);
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x190 + D_800CCB34__w[0] * 40, 0x84, 0x33, (u8)((r << 6) | off), (u8)(q * 13), 0x10);
}

extern u8 D_800D2CE0[];
extern u8 D_800D2204[];
extern u8 D_800D2F68[][4];

/* func_800904A0.s, byte-exact: func_8009187C's sprite pair for the halfword
 * v of 16-byte entry D_800D2CE0[b * 2 + a] of D_800D2204, at +0x320 (0xA8,
 * 0x33) and +0x370 (0xD0, 0x33).  Its switch has no default: for
 * v & 0xF > 2 retail indexes D_800D2F68 with whatever the caller left in the
 * saved register, and the C reads the unset u8 `j`; that read is undefined
 * in a native build, which must define the case or show it unreachable. */
void func_800904A0(u8 a, u8 b) {
    u16 v;
    s32 i;
    u8 j;

    v = *(u16*)(D_800D2204 + D_800D2CE0[b * 2 + a] * 16);
    if (v & 0x4000) {
        i = 9;
    } else if (v & 0x1000) {
        i = 7;
    } else {
        i = 8;
    }
    switch (v & 0xF) {
    case 0:
        j = 0xC;
        break;
    case 1:
        j = 0xD;
        break;
    case 2:
        j = 0x15;
        break;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x320 + D_800CCB34__w[0] * 40, 0xA8, 0x33, D_800D2F68[i][2], D_800D2F68[i][3], D_800D2F68[i][0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x32E) = D_800D2F68[i][1] ? g_SystemPalette2 : g_SystemPalette1;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x370 + D_800CCB34__w[0] * 40, 0xD0, 0x33, D_800D2F68[j][2], D_800D2F68[j][3], D_800D2F68[j][0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x37E) = D_800D2F68[j][1] ? g_SystemPalette2 : g_SystemPalette1;
    }
}
extern u8 D_800D2CE0[];
extern u8* func_8008AC00(s32 size);
extern void bzero(void* p, s32 n);
extern s32 D_800D329C;
extern u8* GetStringEntry(s32 table, s32 id);
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

extern void func_80076CE8(u8* p, s32 x, s32 y, s32 a3, s32 a4, s32 a5, s32 a6);
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern s32 D_800CCB34__w[] asm("D_800CCB34");

/* func_8009093C.s, byte-exact: for n >= 0x68 (then n -= 0x68, and the last
 * two sprites' fourth argument gains 0x10) set the CLUT halfwords (+0xE,
 * +0x5E) of the first two sprites at D_800CCB34 * 40 of the record pointed
 * to by the word at D_800C3EA4 + 0xA230 to g_SystemPalette2, otherwise to
 * g_SystemPalette1; then set up four func_80076CE8 sprites at +0, +0x50,
 * +0xA0 and +0xF0 with (u8)n as fifth argument and store D_800CCB34 at
 * +0x668.  Codegen: each CLUT store loads the record pointer into its own
 * block-local `rec` (retail adds it as the first operand), and both stores
 * are written in each branch, as a store-pair macro invoked per branch
 * would give (func_80091EC4 and func_8009209C open with the same pair);
 * retail's shared second store is the two copies merged. */
void func_8009093C(s32 n) {
    s32 off = 0;

    if (n >= 0x68) {
        n -= 0x68;
        {
            s32 base = D_800C3EA4;
            u16 pal = g_SystemPalette2;
            {
                u8* rec = *(u8**)(base + 0xA230);
                u8* p = rec + D_800CCB34__w[0] * 40;
                *(u16*)(p + 0xE) = pal;
            }
            {
                u8* rec = *(u8**)(base + 0xA230);
                u8* p = rec + D_800CCB34__w[0] * 40;
                *(u16*)(p + 0x5E) = pal;
            }
        }
        off = 0x10;
    } else {
        s32 base = D_800C3EA4;
        u16 pal = g_SystemPalette1;
        {
            u8* rec = *(u8**)(base + 0xA230);
            u8* p = rec + D_800CCB34__w[0] * 40;
            *(u16*)(p + 0xE) = pal;
        }
        {
            u8* rec = *(u8**)(base + 0xA230);
            u8* p = rec + D_800CCB34__w[0] * 40;
            *(u16*)(p + 0x5E) = pal;
        }
    }
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + D_800CCB34__w[0] * 40, 0x30, 0x60, 0, (u8)n, 0x60, 0x68);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0x50 + D_800CCB34__w[0] * 40, 0xB4, 0x60, 0x78, (u8)n, 0x60, 0x68);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xA0 + D_800CCB34__w[0] * 40, 0x98, 0x60, off, (u8)n, 0x10, 0x68);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xF0 + D_800CCB34__w[0] * 40, 0x11C, 0x60, off | 0x40, (u8)n, 0x10, 0x68);
    (*(u8**)(D_800C3EA4 + 0xA230))[0x668] = *(u8*)D_800CCB34__w;
}

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

extern u16 D_800CCD38__rows[][0xB8] asm("D_800CCD38");

/* func_80090C44.s, byte-exact: two-digit readouts of actor a's halfwords at
 * D_800CCD38 and D_800CCD3A (0x170-byte records): for each value v, a
 * func_80076C78 sprite for the tens digit (only when v / 10 is non-zero) and
 * one for the ones digit, fourth argument (u8)(digit * 8 + 0x78), at
 * +0x460/+0x4B0 and +0x500/+0x550 + D_800CCB34 * 40 of the record pointed to
 * by the word at D_800C3EA4 + 0xA230 (x 0x104/0x10C and 0x11C/0x124,
 * y 0xC6).  Codegen: the halfwords are read through a [][0xB8] row view of
 * D_800CCD38, so the saved register holds only a * 0x170 as in retail
 * (pointer arithmetic on the u8 array keeps the whole address there). */
void func_80090C44(u8 a) {
    if (D_800CCD38__rows[a][0] / 10 != 0) {
        func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x460 + *(s32*)&D_800CCB34 * 40, 0x104, 0xC6, (u8)(D_800CCD38__rows[a][0] / 10 * 8 + 0x78), 0, 8);
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x4B0 + D_800CCB34__w[0] * 40, 0x10C, 0xC6, (u8)(D_800CCD38__rows[a][0] % 10 * 8 + 0x78), 0, 8);
    if (D_800CCD38__rows[a][1] / 10 != 0) {
        func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x500 + D_800CCB34__w[0] * 40, 0x11C, 0xC6, (u8)(D_800CCD38__rows[a][1] / 10 * 8 + 0x78), 0, 8);
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x550 + D_800CCB34__w[0] * 40, 0x124, 0xC6, (u8)(D_800CCD38__rows[a][1] % 10 * 8 + 0x78), 0, 8);
}

extern u8 D_800D2F7C[];
extern u8 D_800D2F80[];

/* func_80090E7C.s, byte-exact: set up the func_80076C78 sprites at +0x3C0
 * (0xA8, 0xA6) and +0x410 (0xEC, 0xC6) + D_800CCB34 * 40 from the 4-byte
 * entries D_800D2F80 and D_800D2F7C (bytes 2, 3 and 0 as arguments), set
 * each sprite's CLUT halfword (+0xE) to g_SystemPalette2 when the entry's
 * byte 1 is set, else g_SystemPalette1, then run func_80090C44(a), set up
 * func_80076A10(0x71, record + 0x5A0, 0x118, 0xD1) and store D_800CCB34 at
 * +0x66C.  CLUT stores as in func_8009093C (block-local `rec`). */
void func_80090E7C(u8 a) {
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x3C0 + D_800CCB34__w[0] * 40, 0xA8, 0xA6, D_800D2F80[2], D_800D2F80[3], D_800D2F80[0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x3CE) = D_800D2F80[1] ? g_SystemPalette2 : g_SystemPalette1;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x410 + D_800CCB34__w[0] * 40, 0xEC, 0xC6, D_800D2F7C[2], D_800D2F7C[3], D_800D2F7C[0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x41E) = D_800D2F7C[1] ? g_SystemPalette2 : g_SystemPalette1;
    }
    func_80090C44(a);
    func_80076A10(0x71, (s32)(*(u8**)(D_800C3EA4 + 0xA230) + 0x5A0), 0x118, 0xD1);
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66C] = D_800CCB34;
}
extern void func_80077610(void);
extern void func_80076EA4(void);
extern void func_8008FC1C(s32 x, s32 y, s32 w, s32 h, u8 n);
extern void* func_80033908(s32 index);
extern void* func_800339FC(s32 index);
extern void func_800716D8(void);
extern u16 func_80089C6C(u16 bits, u8 b);
/* 32-byte per-character records at g_GameState + 0x16C0 (index =
 * D_800D2D24 byte, as in func_80091D38: +0x16C2 / +0x16C6 halfwords);
 * names by offset. */
typedef struct {
    u16 unk0;
    u16 unk2;
    u16 unk4;
    u16 unk6;
    u8 pad8[0x18];
} GameStateChr20;
typedef struct {
    u8 pad[0x16C0];
    GameStateChr20 chr[11];
} GameState16C0;
extern GameState16C0 g_GameState__v asm("g_GameState");
extern u8 D_800D2D24[];
extern u8 D_800D32A1[];
extern u8 D_800CCD3E__rows[][0x170] asm("D_800CCD3E");
extern u8 D_800CCD88__rows[][0x170] asm("D_800CCD88");
extern u8* D_800D2DB0;
extern BattleImageRef D_800C3E5C[];
extern u8 D_800CDD40__b[][38][40] asm("D_800CDD40");
extern u8 D_800CEF10__b[][42][40] asm("D_800CEF10");


/* func_80091064.s, byte-exact: func_80092784's 16-slot form for actor a.
 * After func_80077610, func_80076EA4 and func_8008FC1C(0x20, 0x30, 0x98,
 * 0x38, 0xC) it uploads the blank buffer D_800D2DB0 at (0x3C0, 0, 0x3C, 0xD),
 * then for i 0..15 sets flags[i] / vals[i] from func_80089C6C(the
 * character record's +0x16C2 halfword, i) and byte +0x13 of entry i + 0x16
 * of actor a's D_800CDD40 row (D_800D32A1 byte of a clear) or from
 * +0x16C6 and entry i + 0x15 of its D_800CEF10 row.  Each slot then gets a
 * name (func_80033908(D_800CCD3E byte * 16 + i) or func_800339FC(D_800CCD88
 * byte * 16 + i), only for a set flag) and the two digits of vals[i] as in
 * func_80092784.  Then func_80090E7C(a), the buffers are freed,
 * func_800716D8() runs, the CLUT halfwords +0x14E and +0x176 of the record
 * pointed to by the word at D_800C3EA4 + 0xA230 become g_SystemPalette1,
 * its +0x669 = 1 and D_800D2D28[0xB7] = 1.  Codegen: record and row views
 * keep the symbol in each address (pointer arithmetic hoists it out of the
 * loops), and each name source has its own SystemRenderStringEntry call
 * (gcc merges them). */
void func_80091064(u8 a) {
    s16 r0[4];
    s16 r1[4];
    s16 r2[4];
    s16 r3[4];
    s16 r4[4];
    s16 r5[4];
    BattleImageRef bufs[16];
    u8 flags[16];
    u8 vals[16];
    s32 i;
    u8* img;

    func_80077610();
    func_80076EA4();
    func_8008FC1C(0x20, 0x30, 0x98, 0x38, 0xC);
    D_800D2DB0 = func_8008AC00(0x39);
    bzero(D_800D2DB0, 0x618);
    r5[0] = 0x3C0;
    r5[2] = 0x3C;
    r5[1] = 0;
    r5[3] = 0xD;
    func_800769E8(r5, D_800D2DB0);
    if (D_800D32A1[a * 8] == 0) {
        for (i = 0; i < 16; i++) {
            if (func_80089C6C(g_GameState__v.chr[D_800D2D24[a]].unk2, i) != 0) {
                flags[i] = 1;
                vals[i] = D_800CDD40__b[a][i + 0x16][0x13];
            } else {
                flags[i] = 0;
                vals[i] = 0;
            }
        }
    } else {
        for (i = 0; i < 16; i++) {
            if (func_80089C6C(g_GameState__v.chr[D_800D2D24[a]].unk6, i) != 0) {
                flags[i] = 1;
                vals[i] = D_800CEF10__b[a][i + 0x15][0x13];
            } else {
                flags[i] = 0;
                vals[i] = 0;
            }
        }
    }
    for (i = 0; i < 16; i++) {
        bufs[i].data = func_8008AC00(0x1B);
        bzero(bufs[i].data, 0x30C);
        r3[0] = i % 2 * 30 + 0x380;
        r3[1] = i / 2 * 16 + 0x100;
        r3[2] = 0x1B;
        r3[3] = 0x10;
        func_800769E8(r3, D_800D2DB0);
        if (flags[i] != 0) {
            if (D_800D32A1[a * 8] == 0) {
                SystemRenderStringEntry(func_80033908(D_800CCD3E__rows[a][0] * 16 + i), bufs[i].data, 0x1B, 0);
            } else {
                SystemRenderStringEntry(func_800339FC(D_800CCD88__rows[a][0] * 16 + i), bufs[i].data, 0x1B, 0);
            }
            r0[0] = i % 2 * 30 + 0x380;
            r0[1] = i / 2 * 16 + 0x102;
            r0[2] = 0x1E;
            r0[3] = 0xD;
            func_800769E8(r0, bufs[i].data);
        }
        if (!(i & 1)) {
            r4[0] = 0x3C0;
            r4[1] = i / 2 * 16 + 0x100;
            r4[2] = 0x1B;
            r4[3] = 0x10;
            func_800769E8(r4, D_800D2DB0);
        }
        r1[0] = i % 2 * 16 + 0x3C0;
        r1[1] = i / 2 * 16 + 0x102;
        r1[2] = 6;
        r1[3] = 0xD;
        if ((u8)(vals[i] / 10) != 0) {
            img = D_800C3E5C[(u8)(vals[i] / 10)].data;
        } else {
            img = D_800D2DB0;
        }
        func_800769E8(r1, img);
        r2[0] = i % 2 * 16 + 0x3C2;
        r2[1] = i / 2 * 16 + 0x102;
        r2[2] = 6;
        r2[3] = 0xD;
        if (flags[i] != 0) {
            img = D_800C3E5C[(u8)(vals[i] % 10)].data;
        } else {
            img = D_800D2DB0;
        }
        func_800769E8(r2, img);
    }
    func_80090E7C(a);
    for (i = 0; i < 16; i++) {
        HeapFree(bufs[i].data);
    }
    HeapFree(D_800D2DB0);
    func_800716D8();
    *(u16*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x14E) = g_SystemPalette1;
    *(u16*)(*(u8**)(D_800C3EA4 + 0xA230) + 0x176) = g_SystemPalette1;
    (*(u8**)(D_800C3EA4 + 0xA230))[0x669] = 1;
    D_800D2D28[0xB7] = 1;
}
/* func_80091604.s, byte-exact: func_8009023C's 8-row twin.  The running
 * count at D_800D2D28 +0xF8 starts at 0; each of 8 rows adds
 * func_80076A10(0x66, D_800C3EA4 + 0xBA8 + count * 0x50, 0x20, y + 0x38 +
 * row * 8) to it.  Then +0xA5 = D_800CCB34 and +0x9C = 1. */
void func_80091604(s32 y) {
    s32 dy;
    s32 i;

    i = 0;
    dy = 0x38;
    *(s32*)(D_800D2D28 + 0xF8) = 0;
    do {
        *(s32*)(D_800D2D28 + 0xF8) = func_80076A10(0x66, D_800C3EA4 + 0xBA8 + *(s32*)(D_800D2D28 + 0xF8) * 0x50, 0x20, (s16)(dy + y)) + *(s32*)(D_800D2D28 + 0xF8);
        dy += 8;
    } while (++i < 8);
    D_800D2D28[0xA5] = D_800CCB34;
    D_800D2D28[0x9C] = 1;
}
extern u8 D_800D2D24[];
extern u8 D_800C3CFA[];
extern void func_80076CE8(u8* p, s32 x, s32 y, s32 a3, s32 a4, s32 a5, s32 a6);
extern u32 func_8009A258(s32 a, s32 b);
extern void func_8008AAA0(u32 value);

/* func_800916D4.s, byte-exact: with k = c * 2 + b, set up the
 * func_80076CE8 sprite at +0x140 + D_800CCB34 * 40 of the record pointed to
 * by the word at D_800C3EA4 + 0xA230 (0x20, 0xA4, (u8)(k % 2 * 120),
 * (u8)(k / 2 * 16), 0x60, 0x10) and clear its +0x66E count; for an actor a
 * whose D_800D2D24 byte is 1, pass func_8009A258(a, k + 0x16) to
 * func_8008AAA0 and add one func_80076C78 sprite per D_800C3CFA entry
 * (three, 0xFF = none) at x 0x8C, 0x94, 0x9C, counting them in +0x66E.
 * gcc 2.6.x code; func_8009A258 takes int arguments here (its second one is
 * passed unmasked), and the loop's sprite address reads the record pointer
 * through the D_800C3EA4 word, not a local (retail adds it last). */
void func_800916D4(u8 b, u8 c, u8 a) {
    u32 k = c * 2 + b;
    u32 q = k / 2;
    u32 r = k - q * 2;
    s32 i;
    u16 x;

    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0x140 + *(s32*)&D_800CCB34 * 40, 0x20, 0xA4, (u8)(r * 120), (u8)(q * 16), 0x60, 0x10);
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66E] = 0;
    if (D_800D2D24[a] == 1) {
        func_8008AAA0(func_8009A258(a, k + 0x16));
        for (i = 0, x = 0x8C; i < 3; i++, x += 8) {
            if (D_800C3CFA[i] != 0xFF) {
                func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x190 + ((*(u8**)(D_800C3EA4 + 0xA230))[0x66E] * 2 + *(s32*)&D_800CCB34) * 40, x, 0xA6, (u8)(D_800C3CFA[i] * 8 + 0x78), 0, 8);
                (*(u8**)(D_800C3EA4 + 0xA230))[0x66E]++;
            }
        }
    }
}
extern u16 D_800CDD40__rows[][38][20] asm("D_800CDD40");
extern u16 D_800CEF10__rows[][42][20] asm("D_800CEF10");
extern u8 D_800D2F68[][4];
extern u8 D_800D32A1[];

/* func_8009187C.s, byte-exact: read the halfword v of entry c * 2 + b + 0x16
 * of actor a's D_800CDD40 row (38 40-byte entries per actor) when the
 * D_800D32A1 byte of a is clear, else of entry c * 2 + b + 0x15 of its
 * D_800CEF10 row (42 per actor); pick D_800D2F68 entry 9 (v & 0x4000), 7
 * (v & 0x1000) or 8, and entry 0xD, 0x15 or 0xC for v & 0xF = 1, 2 or other,
 * and draw each as in func_80090E7C at +0x320 (0xDA, 0xAA) and +0x370 (0xFE,
 * 0xAA).  Codegen: row views of the two tables and a [][4] view of
 * D_800D2F68 (index arithmetic on the flat arrays gives another address
 * form and unused stack slots); the switch's explicit case 0 gives retail's
 * extra "< 2" test. */
void func_8009187C(u8 a, u8 b, u8 c) {
    u16 v;
    s32 i;
    s32 j;

    if (D_800D32A1[a * 8] == 0) {
        v = D_800CDD40__rows[a][c * 2 + b + 0x16][0];
    } else {
        v = D_800CEF10__rows[a][c * 2 + b + 0x15][0];
    }
    if (v & 0x4000) {
        i = 9;
    } else if (v & 0x1000) {
        i = 7;
    } else {
        i = 8;
    }
    switch (v & 0xF) {
    case 0:
        j = 0xC;
        break;
    case 1:
        j = 0xD;
        break;
    case 2:
        j = 0x15;
        break;
    default:
        j = 0xC;
        break;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x320 + D_800CCB34__w[0] * 40, 0xDA, 0xAA, D_800D2F68[i][2], D_800D2F68[i][3], D_800D2F68[i][0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x32E) = D_800D2F68[i][1] ? g_SystemPalette2 : g_SystemPalette1;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x370 + D_800CCB34__w[0] * 40, 0xFE, 0xAA, D_800D2F68[j][2], D_800D2F68[j][3], D_800D2F68[j][0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x37E) = D_800D2F68[j][1] ? g_SystemPalette2 : g_SystemPalette1;
    }
}
extern u8 g_GameState[];
extern u8 D_800D2D24[];
extern u8 D_800D32A1[];
extern s32 D_800D367C;
extern s32 D_800CCB34__w[] asm("D_800CCB34");

/* func_80091B38.s, byte-exact: func_8009382C's two-line string sprite for
 * table D_800D367C at x 0x30, with id = D_800D2D24[a] * 32 + (c * 2 + b) * 2
 * (D_800D32A1 byte of a clear) or g_GameState[0x30C + D_800D2D24[a] * 0xA4]
 * * 32 + (c * 2 + b) * 2.  Same codegen as func_8009382C (u32 `str` copy of
 * the u16 id after bzero, word view of D_800CCB34); first draft. */
void func_80091B38(u8 a, u8 b, u8 c) {
    s16 r[4];
    u16 id;
    u8* buf;
    s32 n1;
    s32 n2;
    u32 str;

    if (D_800D32A1[a * 8] == 0) {
        id = D_800D2D24[a] * 32 + (c * 2 + b) * 2;
    } else {
        id = g_GameState[0x30C + D_800D2D24[a] * 0xA4] * 32 + (c * 2 + b) * 2;
    }
    buf = func_8008AC00(0x39);
    bzero(buf, 0x618);
    str = id;
    n1 = SystemRenderStringEntry(GetStringEntry(D_800D367C, str), buf, 0x39, 0);
    n2 = SystemRenderStringEntry(GetStringEntry(D_800D367C, str | 1), buf, 0x39, 1);
    r[0] = 0x3C0;
    r[2] = 0x3C;
    r[1] = 0;
    r[3] = 0xD;
    func_800769E8(r, buf);
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x280 + D_800CCB34__w[0] * 40, 0x30, 0xB6, 0, 0, n1 & 0xFF);
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x2D0 + D_800CCB34__w[0] * 40, 0x30, 0xC6, 0, 0, n2 & 0xFF);
    HeapFree(buf);
}
extern u8 g_GameState[];
extern u8 D_800D2D24[];
extern u8 D_800D32A1[];
extern u16 func_80089C6C(u16 bits, u8 b);
extern void func_800916D4(u8 b, u8 c, u8 a);
extern void func_8009187C(u8 a, u8 b, u8 c);
extern void func_80091B38(u8 a, u8 b, u8 c);

/* func_80091D38.s, byte-exact: when func_80089C6C accepts the halfword at
 * g_GameState + 0x16C2 (D_800D32A1 byte of a clear) or + 0x16C6 (set) +
 * D_800D2D24[a] * 32 for b + c * 2, run func_800916D4(b, c, a),
 * func_8009187C(a, b, c) and func_80091B38(a, b, c) and set bytes +0x66A
 * (D_800CCB34) and +0x66B (1) of the record pointed to by the word at
 * D_800C3EA4 + 0xA230; otherwise clear its +0x66B.  First draft. */
void func_80091D38(u8 a, u8 b, u8 c) {
    u8 ok = 0;

    if (D_800D32A1[a * 8] == 0) {
        ok = func_80089C6C(*(u16*)(g_GameState + 0x16C2 + D_800D2D24[a] * 32), b + c * 2) != 0;
    } else if (func_80089C6C(*(u16*)(g_GameState + 0x16C6 + D_800D2D24[a] * 32), b + c * 2) != 0) {
        ok = 1;
    }
    if (ok) {
        func_800916D4(b, c, a);
        func_8009187C(a, b, c);
        func_80091B38(a, b, c);
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66A] = D_800CCB34;
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66B] = 1;
    } else {
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66B] = 0;
    }
}

/* func_80091EC4.s, byte-exact: set the CLUT halfwords (+0xE, +0x5E) of the
 * first two sprites at D_800CCB34 * 40 of the record pointed to by the word
 * at D_800C3EA4 + 0xA230 to g_SystemPalette1, set up four func_80076CE8
 * sprites at +0, +0x50, +0xA0 and +0xF0 (y 0x34, last argument 0x60) with
 * a as fifth argument and store D_800CCB34 at +0x668.  CLUT stores as in
 * func_8009093C. */
void func_80091EC4(u8 a) {
    s32 base = D_800C3EA4;
    u16 pal = g_SystemPalette1;

    {
        u8* rec = *(u8**)(base + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0xE) = pal;
    }
    {
        u8* rec = *(u8**)(base + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x5E) = pal;
    }
    func_80076CE8(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 40, 0x34, 0x34, 0, a, 0x60, 0x60);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0x50 + D_800CCB34__w[0] * 40, 0xB8, 0x34, 0x78, a, 0x60, 0x60);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xA0 + D_800CCB34__w[0] * 40, 0x9C, 0x34, 0, a, 0x10, 0x60);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xF0 + D_800CCB34__w[0] * 40, 0x120, 0x34, 0x40, a, 0x10, 0x60);
    (*(u8**)(D_800C3EA4 + 0xA230))[0x668] = *(u8*)D_800CCB34__w;
}

/* func_8009209C.s, byte-exact: func_80091EC4's twin at y 0x62 with last
 * argument 0x40 (x 0x36, 0xC2, 0x96 and 0x122, fifth argument 0), then
 * +0x669 = 1 and D_800D2D28[0xB7] = 3. */
void func_8009209C(void) {
    s32 base = D_800C3EA4;
    u16 pal = g_SystemPalette1;

    {
        u8* rec = *(u8**)(base + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0xE) = pal;
    }
    {
        u8* rec = *(u8**)(base + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x5E) = pal;
    }
    func_80076CE8(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 40, 0x36, 0x62, 0, 0, 0x60, 0x40);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0x50 + D_800CCB34__w[0] * 40, 0xC2, 0x62, 0x78, 0, 0x60, 0x40);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xA0 + D_800CCB34__w[0] * 40, 0x96, 0x62, 0, 0, 0x10, 0x40);
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xF0 + D_800CCB34__w[0] * 40, 0x122, 0x62, 0x40, 0, 0x10, 0x40);
    (*(u8**)(D_800C3EA4 + 0xA230))[0x668] = *(u8*)D_800CCB34__w;
    (*(u8**)(D_800C3EA4 + 0xA230))[0x669] = 1;
    D_800D2D28[0xB7] = 3;
}
extern void SetPolyG4(void* p);
extern void SetPolyF4(void* p);
extern u8 D_800C33B4[];
extern s32 D_800C33C4[];
extern s32 D_800C3404[];


/* func_80092298.s, byte-exact: in the record pointed to by the word at
 * D_800C3EA4 + 0xA230 set up the two POLY_G4 at +0x5F0 (0x24 apart; colours
 * (0xFF, 0, 0), (0xFF, 0, 0), (0, 0, 0), (0, 0, 0), all vertices 0) and the
 * two POLY_F4 at +0x638 (0x18 apart; black, vertices 0), then add one
 * func_80076A10 sprite (D_800C33B4[j], D_800C3EA4 + 0x1E68 + count * 0x50,
 * D_800C33C4[j], D_800C3404[j]) for each of the 16 ids bytes that is not
 * 0xFF, counting them at D_800D2D28 + 0xFC, then +0xA6 = D_800CCB34 and
 * +0x9D = 1.  a is unused.  Codegen: the third loop has its own counter j
 * (reusing i swaps s0 and s1). */
void func_80092298(u8 a, u8* ids) {
    s32 i;
    s32 j;

    for (i = 0; i < 2; i++) {
        SetPolyG4(*(u8**)(D_800C3EA4 + 0xA230) + 0x5F0 + i * 0x24);
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x5F4] = 0xFF;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x5F5] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x5F6] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x5FC] = 0xFF;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x5FD] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x5FE] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x604] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x605] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x606] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x60C] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x60D] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24)[0x60E] = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x5F8) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x5FA) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x600) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x602) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x608) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x60A) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x610) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x24 + 0x612) = 0;
    }
    for (i = 0; i < 2; i++) {
        SetPolyF4(*(u8**)(D_800C3EA4 + 0xA230) + 0x638 + i * 0x18);
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18)[0x63C] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18)[0x63D] = 0;
        (*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18)[0x63E] = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x640) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x642) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x644) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x646) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x648) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x64A) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x64C) = 0;
        *(s16*)(*(u8**)(D_800C3EA4 + 0xA230) + i * 0x18 + 0x64E) = 0;
    }
    *(s32*)(D_800D2D28 + 0xFC) = 0;
    for (j = 0; j < 16; j++) {
        if (ids[j] != 0xFF) {
            *(s32*)(D_800D2D28 + 0xFC) = func_80076A10(D_800C33B4[j], D_800C3EA4 + 0x1E68 + *(s32*)(D_800D2D28 + 0xFC) * 0x50, D_800C33C4[j], D_800C3404[j]) + *(s32*)(D_800D2D28 + 0xFC);
        }
    }
    D_800D2D28[0xA6] = D_800CCB34;
    D_800D2D28[0x9D] = 1;
}
extern void func_80077610(void);
extern void func_80076EA4(void);
extern void* func_80033784(s32 table, s32 id);
extern void* func_800338D8(s32 id);
extern void func_80092298(u8 a, u8* ids);
extern u8* D_800D2DB0;
extern BattleImageRef D_800C3E5C[];

/* func_80092784.s, byte-exact: after func_80077610 and func_80076EA4, clear
 * a fresh 0x39-wide buffer (D_800D2DB0) and upload it at (0x3C0, 0, 0x3C,
 * 0xD).  For each slot i of 8: allocate a 0x1B-high buffer, upload the
 * shared buffer at (i % 2 * 30 + 0x380, i / 2 * 16 + 0x100, 0x1B, 0x10),
 * render the slot's name (string ids[i] of table D_800D2D24[a] through
 * func_80033784, skipped for id 0xFF; slot 7 renders func_800338D8(10)) and
 * upload it 30 wide, add a 0x1B x 0x10 strip at x 0x3C0 for even i, then
 * upload the two digits of vals[i] (D_800C3E5C images; the tens only when
 * non-zero, the ones only for a valid id; slot 7 uploads the blank buffer
 * for both) at (i % 2 * 16 + 0x3C0 / 0x3C2, i / 2 * 16 + 0x102).  Finally
 * func_80092298(a, ids), free the buffers and run func_8009209C.
 * Codegen: the buffers are BattleImageRef elements (main6.c's
 * func_80076EA4 idiom; a plain pointer array becomes a pointer walk), the
 * tens image is chosen before its upload call, each ones-digit path has its
 * own upload call (gcc merges them), and the name width 0x1E is a local `w`
 * (retail loads it into t0 at each use; the literal goes to v0). */
void func_80092784(u8 a, u8* ids, u8* vals) {
    s16 r0[4];
    s16 r1[4];
    s16 r2[4];
    s16 r3[4];
    s16 r4[4];
    s16 r5[4];
    BattleImageRef bufs[8];
    s32 i;
    u8* img;
    s32 w = 0x1E;

    func_80077610();
    func_80076EA4();
    D_800D2DB0 = func_8008AC00(0x39);
    bzero(D_800D2DB0, 0x618);
    r5[0] = 0x3C0;
    r5[2] = 0x3C;
    r5[1] = 0;
    r5[3] = 0xD;
    func_800769E8(r5, D_800D2DB0);
    for (i = 0; i < 8; i++) {
        bufs[i].data = func_8008AC00(0x1B);
        bzero(bufs[i].data, 0x30C);
        r3[0] = i % 2 * 30 + 0x380;
        r3[1] = i / 2 * 16 + 0x100;
        r3[2] = 0x1B;
        r3[3] = 0x10;
        func_800769E8(r3, D_800D2DB0);
        if (i != 7) {
            if (ids[i] != 0xFF) {
                SystemRenderStringEntry(func_80033784(D_800D2D24[a], ids[i]), bufs[i].data, 0x1B, 0);
                r0[0] = i % 2 * 30 + 0x380;
                r0[1] = i / 2 * 16 + 0x102;
                r0[2] = w;
                r0[3] = 0xD;
                func_800769E8(r0, bufs[i].data);
            }
        } else {
            SystemRenderStringEntry(func_800338D8(10), bufs[i].data, 0x1B, 0);
            r0[0] = 0x39E;
            r0[1] = 0x132;
            r0[2] = w;
            r0[3] = 0xD;
            func_800769E8(r0, bufs[i].data);
        }
        if (!(i & 1)) {
            r4[0] = 0x3C0;
            r4[1] = i / 2 * 16 + 0x100;
            r4[2] = 0x1B;
            r4[3] = 0x10;
            func_800769E8(r4, D_800D2DB0);
        }
        r1[0] = i % 2 * 16 + 0x3C0;
        r1[1] = i / 2 * 16 + 0x102;
        r1[2] = 6;
        r1[3] = 0xD;
        if (i != 7) {
            if ((u8)(vals[i] / 10) != 0) {
                img = D_800C3E5C[(u8)(vals[i] / 10)].data;
            } else {
                img = D_800D2DB0;
            }
            func_800769E8(r1, img);
            r2[0] = i % 2 * 16 + 0x3C2;
            r2[1] = i / 2 * 16 + 0x102;
            r2[2] = 6;
            r2[3] = 0xD;
            if (ids[i] != 0xFF) {
                func_800769E8(r2, D_800C3E5C[(u8)(vals[i] % 10)].data);
            } else {
                func_800769E8(r2, D_800D2DB0);
            }
        } else {
            func_800769E8(r1, D_800D2DB0);
            r2[0] = 0x3D2;
            r2[1] = 0x132;
            r2[2] = 6;
            r2[3] = 0xD;
            func_800769E8(r2, D_800D2DB0);
        }
    }
    func_80092298(a, ids);
    for (i = 0; i < 8; i++) {
        HeapFree(bufs[i].data);
    }
    HeapFree(D_800D2DB0);
    func_8009209C();
}
/* func_80092B74.s, byte-exact: lay out a value n and a row of glyphs as
 * sprites.  The +0xF8 word of D_800D2D28 counts the 0x50-byte sprite
 * records at D_800C3EA4 + 0xBA8; func_80076A10(glyph, record, x, y) adds
 * the records it writes.  n's tens digit (skipped when 0) and ones digit
 * go to (0x56, 0x38) and (0x5E, 0x38) as glyphs + 0x67.  Then for up to
 * seven entries, stopping at the first D_800C3EAC +0x2C4[i] == 0xFF:
 * glyph D_800C3DE1[i - 1] + 0x39 at (i * 32 + 0x2A, 0x4A) and, for i > 0,
 * glyph 0xA at (i * 32 + 0x1A, 0x4A).  Then D_800D2D28[0xA5] = D_800CCB34,
 * [0x9C] = 1, two quads in the record pointed to by the word at
 * D_800C3EA4 + 0xA230 (x 0x80 to n * 4 + 0x80, then n * 4 + 0x80 to 0xF0;
 * y 0x34 to 0x3C), and that record's +0x66C = D_800CCB34, +0x66F = 1.
 * a is unused (the caller is main48's func_8008C81C).  Codegen: x1 is an
 * int holding (s16)(i * 32 + 0x2A), x2 a short, and j = i - 1 is computed
 * in the loop.  loop.c then inserts the reduced inits as x2, j, x1, sched1
 * hoists only x2's above the ones-digit call, and global.c gives j s2 and
 * x2 s3 as in retail.  With j a separate induction variable, j and x2
 * swap s2/s3; with x1 a short (j still computed in the loop), j and x1
 * swap s1/s2.  The record index is read as D_800CCB34__w[0], an array
 * element, so each quad store forces a reload as in retail (a plain
 * scalar stays in a register: 300 bytes short). */
extern u8 D_800C3DE1[];
typedef struct {
    u8 pad[0x2C4];
    u8 f2C4[7];
} Ids3EAC;
extern u8* D_800C3EAC;

void func_80092B74(u8 a, u8 n) {
    u8 d[2];
    s32 i;
    s32 j;
    s32 x1;
    s16 x2;

    d[1] = n - (d[0] = n / 10) * 10;
    *(s32*)(D_800D2D28 + 0xF8) = 0;
    if (d[0] != 0) {
        *(s32*)(D_800D2D28 + 0xF8) = func_80076A10(d[0] + 0x67, D_800C3EA4 + 0xBA8 + *(s32*)(D_800D2D28 + 0xF8) * 0x50, 0x56, 0x38) + *(s32*)(D_800D2D28 + 0xF8);
    }
    *(s32*)(D_800D2D28 + 0xF8) = func_80076A10(d[1] + 0x67, D_800C3EA4 + 0xBA8 + *(s32*)(D_800D2D28 + 0xF8) * 0x50, 0x5E, 0x38) + *(s32*)(D_800D2D28 + 0xF8);
    for (i = 0; i < 7; i++) {
        x1 = (s16)(i * 32 + 0x2A);
        x2 = i * 32 + 0x1A;
        j = i - 1;
        if (((Ids3EAC*)D_800C3EAC)->f2C4[i] == 0xFF) {
            break;
        }
        *(s32*)(D_800D2D28 + 0xF8) = func_80076A10(D_800C3DE1[j] + 0x39, D_800C3EA4 + 0xBA8 + *(s32*)(D_800D2D28 + 0xF8) * 0x50, x1, 0x4A) + *(s32*)(D_800D2D28 + 0xF8);
        if (i != 0) {
            *(s32*)(D_800D2D28 + 0xF8) = func_80076A10(0xA, D_800C3EA4 + 0xBA8 + *(s32*)(D_800D2D28 + 0xF8) * 0x50, x2, 0x4A) + *(s32*)(D_800D2D28 + 0xF8);
        }
    }
    D_800D2D28[0xA5] = *(u8*)D_800CCB34__w;
    D_800D2D28[0x9C] = 1;
    {
        s32 base = D_800C3EA4;
        s16 x = n * 4 + 0x80;

        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x5F8) = 0x80;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x5FA) = 0x34;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x600) = x;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x602) = 0x34;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x608) = 0x80;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x60A) = 0x3C;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x610) = x;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x24 + 0x612) = 0x3C;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x640) = x;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x642) = 0x34;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x644) = 0xF0;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x646) = 0x34;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x648) = x;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x64A) = 0x3C;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x64C) = 0xF0;
        *(s16*)(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 0x18 + 0x64E) = 0x3C;
    }
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66C] = *(u8*)D_800CCB34__w;
    (*(u8**)(D_800C3EA4 + 0xA230))[0x66F] = 1;
}
extern void* func_80033A8C(s32 index);


/* func_800930AC.s, byte-exact: four rows for actor a.  Upload the blank
 * 0x39-wide buffer D_800D2DB0 at (0x3C0, 0, 0x3C, 0xD); for row i allocate
 * a 0x1B-high buffer, upload blank strips at (0x380, i * 16 + 0x100, 0x1B,
 * 0x10) and (0x3C0, ...), the name func_80033A8C(D_800CCD88 byte * 4 + i)
 * at (0x380, i * 16 + 0x102, 0x1E, 0xD) when ids[i] != 0xFF, and vals[i] as
 * four digits (6 x 0xD): thousands, hundreds and tens at x 0x3C0 + 2k with
 * leading zeros blank, each printed digit's value subtracted from vals[i]
 * (the caller's array is modified), and the ones at 0x3C6 (blank for id
 * 0xFF).  Then free the buffers, set the first sprite's CLUT (+0xE at
 * D_800CCB34 * 40 of the record pointed to by the word at D_800C3EA4 +
 * 0xA230) to g_SystemPalette1, set up func_80076CE8 sprites at +0 (0x94,
 * 0x54, 0, 0, 0x60, 0x40) and +0xA0 (0xFC, 0x54, 0, 0, 0x20, 0x40), store
 * D_800CCB34 at +0x668, +0x669 = 1 and D_800D2D28[0xB7] = 4.  Codegen:
 * `started` is cleared right before the digit loop (cleared before the name
 * it lands in s6 and the row counter in s5); CLUT store as in
 * func_8009093C. */
void func_800930AC(u8 a, u8* ids, u16* vals) {
    s16 r0[4];
    s16 r1[4];
    s16 r2[4];
    s16 r3[4];
    s16 r4[4];
    s16 r5[4];
    BattleImageRef bufs[4];
    u16 div[3];
    s32 i;
    s32 k;
    u8 started;
    u8 d;

    div[0] = 1000;
    div[1] = 100;
    div[2] = 10;
    func_80077610();
    func_80076EA4();
    D_800D2DB0 = func_8008AC00(0x39);
    bzero(D_800D2DB0, 0x618);
    r5[0] = 0x3C0;
    r5[2] = 0x3C;
    r5[1] = 0;
    r5[3] = 0xD;
    func_800769E8(r5, D_800D2DB0);
    for (i = 0; i < 4; i++) {
        bufs[i].data = func_8008AC00(0x1B);
        bzero(bufs[i].data, 0x30C);
        r3[0] = 0x380;
        r3[1] = i * 16 + 0x100;
        r3[2] = 0x1B;
        r3[3] = 0x10;
        func_800769E8(r3, D_800D2DB0);
        if (ids[i] != 0xFF) {
            SystemRenderStringEntry(func_80033A8C(D_800CCD88__rows[a][0] * 4 + i), bufs[i].data, 0x1B, 0);
            r0[0] = 0x380;
            r0[1] = i * 16 + 0x102;
            r0[2] = 0x1E;
            r0[3] = 0xD;
            func_800769E8(r0, bufs[i].data);
        }
        r4[0] = 0x3C0;
        r4[1] = i * 16 + 0x100;
        r4[2] = 0x1B;
        r4[3] = 0x10;
        func_800769E8(r4, D_800D2DB0);
        started = 0;
        for (k = 0; k < 3; k++) {
            r2[0] = k * 2 + 0x3C0;
            r2[1] = i * 16 + 0x102;
            r2[2] = 6;
            r2[3] = 0xD;
            d = vals[i] / div[k];
            if (d != 0 || started != 0) {
                func_800769E8(r2, D_800C3E5C[d].data);
                started = 1;
                vals[i] -= d * div[k];
            } else {
                func_800769E8(r2, D_800D2DB0);
            }
        }
        r1[0] = 0x3C6;
        r1[1] = i * 16 + 0x102;
        r1[2] = 6;
        r1[3] = 0xD;
        if (ids[i] != 0xFF) {
            func_800769E8(r1, D_800C3E5C[(u16)(vals[i] % 10)].data);
        } else {
            func_800769E8(r1, D_800D2DB0);
        }
    }
    for (i = 0; i < 4; i++) {
        HeapFree(bufs[i].data);
    }
    HeapFree(D_800D2DB0);
    {
        s32 base = D_800C3EA4;
        u16 pal = g_SystemPalette1;

        {
            u8* rec = *(u8**)(base + 0xA230);
            u8* p = rec + D_800CCB34__w[0] * 40;
            *(u16*)(p + 0xE) = pal;
        }
        func_80076CE8(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 40, 0x94, 0x54, 0, 0, 0x60, 0x40);
    }
    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xA0 + D_800CCB34__w[0] * 40, 0xFC, 0x54, 0, 0, 0x20, 0x40);
    (*(u8**)(D_800C3EA4 + 0xA230))[0x668] = *(u8*)D_800CCB34__w;
    (*(u8**)(D_800C3EA4 + 0xA230))[0x669] = 1;
    D_800D2D28[0xB7] = 4;
}

/* func_80093578.s, byte-exact: set up the func_80076CE8 sprite at +0x140 +
 * D_800CCB34 * 40 (0x7C, 0xA4, 0, (u8)(b * 16), 0x60, 0x10), then
 * func_8009187C's sprite pair for entry b + 0x25 of actor a's D_800CEF10 row
 * at +0x320 (0x24, 0xA6) and +0x370 (0x48, 0xA6). */
void func_80093578(u8 a, u8 b) {
    u16 v;
    s32 i;
    s32 j;

    func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0x140 + D_800CCB34__w[0] * 40, 0x7C, 0xA4, 0, (u8)(b * 16), 0x60, 0x10);
    v = D_800CEF10__rows[a][b + 0x25][0];
    if (v & 0x4000) {
        i = 9;
    } else if (v & 0x1000) {
        i = 7;
    } else {
        i = 8;
    }
    switch (v & 0xF) {
    case 0:
        j = 0xC;
        break;
    case 1:
        j = 0xD;
        break;
    case 2:
        j = 0x15;
        break;
    default:
        j = 0xC;
        break;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x320 + D_800CCB34__w[0] * 40, 0x24, 0xA6, D_800D2F68[i][2], D_800D2F68[i][3], D_800D2F68[i][0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x32E) = D_800D2F68[i][1] ? g_SystemPalette2 : g_SystemPalette1;
    }
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x370 + D_800CCB34__w[0] * 40, 0x48, 0xA6, D_800D2F68[j][2], D_800D2F68[j][3], D_800D2F68[j][0]);
    {
        u8* rec = *(u8**)(D_800C3EA4 + 0xA230);
        u8* p = rec + D_800CCB34__w[0] * 40;
        *(u16*)(p + 0x37E) = D_800D2F68[j][1] ? g_SystemPalette2 : g_SystemPalette1;
    }
}
extern u8 D_800CCD88[];
extern s32 D_800C3DE8;
extern s32 D_800CCB34__w[] asm("D_800CCB34");

/* func_8009382C.s, byte-exact: render strings id and id | 1 of table
 * D_800C3DE8 (id = (D_800CCD88[a * 0x170] * 4 + b) * 2) into a 0x39-wide
 * buffer, upload it at (0x3C0, 0) and set up two text sprites at
 * +0x280 / +0x2D0 + D_800CCB34 * 40 of the record pointed to by the word at
 * D_800C3EA4 + 0xA230 (func_8009070C's shape with two lines).  Codegen:
 * D_800CCB34 is read as a word through an unsized-array view (retail keeps
 * its address in a saved register across the first func_80076C78 call);
 * the u16 id is copied to a u32 `str` after bzero (retail's late
 * andi 0xFFFF; a decomp-permuter hint), and GetStringEntry takes an s32
 * index, as its definition does. */
void func_8009382C(u8 a, u8 b) {
    s16 r[4];
    u16 id = (D_800CCD88[a * 0x170] * 4 + b) * 2;
    u8* buf = func_8008AC00(0x39);
    s32 n1;
    s32 n2;
    u32 str;

    bzero(buf, 0x618);
    str = id;
    n1 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, str), buf, 0x39, 0);
    n2 = SystemRenderStringEntry(GetStringEntry(D_800C3DE8, str | 1), buf, 0x39, 1);
    r[0] = 0x3C0;
    r[2] = 0x3C;
    r[1] = 0;
    r[3] = 0xD;
    func_800769E8(r, buf);
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x280 + D_800CCB34__w[0] * 40, 0x88, 0xB6, 0, 0, n1 & 0xFF);
    func_80076C78(*(u8**)(D_800C3EA4 + 0xA230) + 0x2D0 + D_800CCB34__w[0] * 40, 0x88, 0xC6, 0, 0, n2 & 0xFF);
    HeapFree(buf);
}
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 g_GameState[];
extern u8 D_800D2D24[];
extern u8 D_800CCD62[];
extern u16 func_80089C6C(u16 bits, u8 b);
extern void func_80093578(u8 a, u8 b);
extern void func_8009382C(u8 a, u8 b);

/* func_800939CC.s, byte-exact: when func_80089C6C accepts the halfword at
 * g_GameState + 0x16DA + D_800D2D24[a] * 32 for b and actor a's D_800CCD62
 * halfword (0x170-byte records) has none of the bits of mask b
 * (D_800C324E, D_800C3250, D_800C3252, D_800C323A), run
 * func_80093578(a, b) and func_8009382C(a, b) and set bytes +0x66A
 * (D_800CCB34) and +0x66B (1) of the record pointed to by the word at
 * D_800C3EA4 + 0xA230; otherwise clear its +0x66B.  gcc 2.6.x code (main51
 * is BattleGcc263); first draft. */
void func_800939CC(u8 a, u8 b) {
    u16 masks[4];

    masks[0] = D_800C324E;
    masks[1] = D_800C3250;
    masks[2] = D_800C3252;
    masks[3] = D_800C323A;
    if (func_80089C6C(*(u16*)(g_GameState + 0x16DA + D_800D2D24[a] * 32), b) != 0
        && !(*(u16*)(D_800CCD62 + a * 0x170) & masks[b])) {
        func_80093578(a, b);
        func_8009382C(a, b);
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66A] = D_800CCB34;
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66B] = 1;
    } else {
        (*(u8**)(D_800C3EA4 + 0xA230))[0x66B] = 0;
    }
}
extern void* GetWeaponName(s32 index);
/* 0xA4-byte records in g_GameState (the actor's equipment slots at +0x2DB,
 * its character index at +0x30C, that character's slots at +0x97C). */
typedef struct {
    u8 b[0xA4];
} M51Rec;
extern M51Rec g_GameState__2DB[] asm("g_GameState+0x2DB");
extern M51Rec g_GameState__30C[] asm("g_GameState+0x30C");
extern M51Rec g_GameState__97C[] asm("g_GameState+0x97C");

extern u8 g_GameState_2286[] asm("g_GameState+0x2286");
extern u8 g_GameState_22B6[] asm("g_GameState+0x22B6");
extern void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh);

/* func_80093B08.s, byte-exact: the two weapon rows for actor a (only when
 * its D_800D2D24 kind is 4 and D_800D2D28[0xB7] is clear).  Takes the two
 * weapon ids from the actor's own g_GameState record (+0x2DB/+0x2DE) or,
 * when D_800D32A1[a * 8] is set, from its character's record (+0x97C/+0x97F
 * via +0x30C), and their counts from the +0x2286 / +0x22B6 tables.  Per row
 * i it uploads blank strips at (0x380, i * 16 + 0x100) and (0x3C0, ...), the
 * name GetWeaponName(id) at (0x380, i * 16 + 0x102, 0x1E, 0xD) when the id
 * is set, and the count as three digits (hundreds and tens at x 0x3C0 + 2k
 * with leading zeros blank, then the ones at 0x3C4; blank for id 0), as
 * func_800930AC does for its four rows.  Then it frees the buffers, sets the
 * first sprite's CLUT to g_SystemPalette1, places the func_80076CE8 sprites
 * at +0 (0xA0, 0xAC, 0x60 x 0x20) and +0xA0 (0x104, 0xAC, 0x18 x 0x20),
 * stores D_800CCB34 at +0x668 with +0x669 = 1, opens the func_8008F8F4
 * window and sets D_800D2D28[0xB7] = 5, [0x8E] = 1.  Codegen: the g_GameState
 * records are read through M51Rec rows (asm aliases at +0x2DB/+0x30C/+0x97C);
 * plain base + offset indexing leaves dead address pseudos that still get
 * four unused stack slots (frame 0xD0 instead of retail's 0xB0); `r5` is
 * retail's untouched rect at sp+0x48. */
void func_80093B08(u8 a) {
    s16 r0[4];
    s16 r1[4];
    s16 r2[4];
    s16 r3[4];
    s16 r4[4];
    s16 r5[4];
    BattleImageRef bufs[2];
    u16 div[2];
    u8 ids[2];
    u16 vals[2];
    s32 i;
    s32 k;
    u8 started;
    u8 d;

    if (D_800D2D24[a] == 4 && D_800D2D28[0xB7] == 0) {
        div[0] = 100;
        div[1] = 10;
        if (D_800D32A1[a * 8] == 0) {
            ids[0] = g_GameState__2DB[D_800D2D24[a]].b[0];
            ids[1] = g_GameState__2DB[D_800D2D24[a]].b[3];
            vals[0] = g_GameState_2286[ids[0]];
            vals[1] = g_GameState_2286[ids[1]];
        } else {
            ids[0] = g_GameState__97C[g_GameState__30C[D_800D2D24[a]].b[0]].b[0];
            ids[1] = g_GameState__97C[g_GameState__30C[D_800D2D24[a]].b[0]].b[3];
            vals[0] = g_GameState_22B6[ids[0]];
            vals[1] = g_GameState_22B6[ids[1]];
        }
        func_80077610();
        func_80076EA4();
        D_800D2DB0 = func_8008AC00(0x39);
        bzero(D_800D2DB0, 0x618);
        for (i = 0; i < 2; i++) {
            bufs[i].data = func_8008AC00(0x1B);
            bzero(bufs[i].data, 0x30C);
            r3[0] = 0x380;
            r3[1] = i * 16 + 0x100;
            r3[2] = 0x1B;
            r3[3] = 0x10;
            func_800769E8(r3, D_800D2DB0);
            if (ids[i] != 0) {
                SystemRenderStringEntry(GetWeaponName(ids[i]), bufs[i].data, 0x1B, 0);
                r0[0] = 0x380;
                r0[1] = i * 16 + 0x102;
                r0[2] = 0x1E;
                r0[3] = 0xD;
                func_800769E8(r0, bufs[i].data);
            }
            r4[0] = 0x3C0;
            r4[1] = i * 16 + 0x100;
            r4[2] = 0x1B;
            r4[3] = 0x10;
            func_800769E8(r4, D_800D2DB0);
            started = 0;
            for (k = 0; k < 2; k++) {
                r2[0] = k * 2 + 0x3C0;
                r2[1] = i * 16 + 0x102;
                r2[2] = 6;
                r2[3] = 0xD;
                d = vals[i] / div[k];
                if (d != 0 || started != 0) {
                    func_800769E8(r2, D_800C3E5C[d].data);
                    started = 1;
                    vals[i] -= d * div[k];
                } else {
                    func_800769E8(r2, D_800D2DB0);
                }
            }
            r1[0] = 0x3C4;
            r1[1] = i * 16 + 0x102;
            r1[2] = 6;
            r1[3] = 0xD;
            if (ids[i] != 0) {
                func_800769E8(r1, D_800C3E5C[(u16)(vals[i] % 10)].data);
            } else {
                func_800769E8(r1, D_800D2DB0);
            }
        }
        for (i = 0; i < 2; i++) {
            HeapFree(bufs[i].data);
        }
        HeapFree(D_800D2DB0);
        {
            s32 base = D_800C3EA4;
            u16 pal = g_SystemPalette1;

            {
                u8* rec = *(u8**)(base + 0xA230);
                u8* p = rec + D_800CCB34__w[0] * 40;
                *(u16*)(p + 0xE) = pal;
            }
            func_80076CE8(*(u8**)(base + 0xA230) + D_800CCB34__w[0] * 40, 0xA0, 0xAC, 0, 0, 0x60, 0x20);
        }
        func_80076CE8(*(u8**)(D_800C3EA4 + 0xA230) + 0xA0 + D_800CCB34__w[0] * 40, 0x104, 0xAC, 0, 0, 0x18, 0x20);
        (*(u8**)(D_800C3EA4 + 0xA230))[0x668] = *(u8*)D_800CCB34__w;
        (*(u8**)(D_800C3EA4 + 0xA230))[0x669] = 1;
        func_8008F8F4(0, 0x98, 0xA8, 0x8C, 0x28, 0, 1);
        D_800D2D28[0xB7] = 5;
        D_800D2D28[0x8E] = 1;
    }
}
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

#ifndef XENO_PC_PORT
extern void func_80097D08(void);
extern void func_8009C198();
extern void func_8009AB38(u8 idx);
extern void func_8009AC48(u8 actor, s32 a1);
extern void func_80099FB0(void);
extern void func_8009AFD8(void);
void func_80094C78(void);
void func_800946F4(void);
void func_800958D8(void);
void func_80095A78(void);
void func_800968C0(void);
extern u8 D_800D2DB8;
extern u8 D_800C34AE;
extern u8 D_800C3E04;
extern u8* D_800C3E34;
extern u8* D_800D2DC8;
extern u8* D_800D2D6C;
extern u8* D_800C3D60;
extern u16 D_800D2C94[];
extern void (*D_800C348C[])(void);

/* +4 of a 0x170-byte actor record: 8-byte rows indexed by command - 3. */
typedef struct {
    u8 pad[4];
    u8 rows[3][8];
} BattleActorRows4;

/* func_800941A4.s, byte-exact: run the queued battle command.
 * func_80097D08(); clear D_800D2DB8 (the result) and D_800C34AE; the
 * acting actor D_800C3E04 = D_800C34B0 +0x5FC1, D_800C3E00 = its 0x170
 * record, D_800D2D6C = record +0xA4.  An actor whose +0x15A has bit 7
 * runs func_8009C198 and func_8009AB38 and returns.  Otherwise
 * D_800C3DFC points at command D_800C34B0 +0x5FC2 (0x28-byte records:
 * party actors < 3 at +0x1058 + actor * 0x5F0, others at +0x35D8) and
 * func_8009AC48(actor, 1) runs.  A command with +0xA bit 0x10 runs
 * func_8009C198, func_8009AB38 and func_80094C78 and returns.  Else
 * D_800D2DC4 = 0; +0xA bit 0x100 with record +0x56 == 1 calls
 * func_80096824; record +0x56 == 4 with command 4 or 5 copies byte 0 of
 * the record's +4 row (command - 3) to command +0x22; record +0x80 bit
 * 0x20 with D_800D2C94 & 7 set and D_800D2C94 < 7 sets D_800D2C94 to
 * 1 << i for each party slot i whose D_800CCD3E byte is 3 (the last one
 * wins).  Then for each actor 0..10 (D_800C3E50) whose bit is set in
 * D_800C34B0 +0x5FAC: publish D_800C3E34 (record), D_800D2DC8 (+0xA4)
 * and D_800C3D60 (+0x148); func_800968C0 for party actors; the handler
 * D_800C348C[command +0x16]; func_800946F4; for outcome 0 one of
 * func_80095B44 / func_80095A78 / func_800958D8 by +0xA bits 0x800 /
 * 0x4000 / 4; D_800C34B0 +0x5FB0 = command +2 (+0xA bit 0) or the
 * command index.  Afterwards func_80099FB0, func_8009AB38, func_8009AFD8
 * when record +0x56 == 4 and D_800C34AE is clear; for a party actor with
 * command < 7 the record's +0x90 halfword [command] grows by +0x55 +
 * +0xA1 while below 65000; func_80094C78; return D_800D2DB8.  The two
 * early exits return whatever the last callee left in $v0, as in retail
 * (the one caller, func_80085CCC, ignores it).
 * Codegen: the command flags are read as ((u16*)D_800C3DFC)[5]; with
 * this array form the reload after the 0x10 test is scheduled before the
 * D_800D2DC4 store, as in retail.  The record offsets are parenthesised
 * (retail adds the constant to the scaled index before the base).  The
 * +4 rows go through a struct member array (retail keeps
 * (command - 3) * 8 unfolded).  D_800D2C94 is declared as an array
 * (retail keeps its address in a register; the scalar differs in 26
 * insns).  `bit = 1` sits at the loop start (reorg copies it into an
 * earlier delay slot). */
u8 func_800941A4(void) {
    s32 bit;
    u8 i;
    u16 f;

    func_80097D08();
    D_800D2DB8 = 0;
    D_800C34AE = 0;
    D_800C3E04 = ((u8*)D_800C34B0)[0x5FC1];
    D_800C3E00 = (u8*)D_800C34B0 + D_800C3E04 * 0x170;
    D_800D2D6C = (u8*)D_800C3E00 + 0xA4;
    if (((Battle*)D_800C34B0)->actors[((u8*)D_800C34B0)[0x5FC1]].flags & 0x80) {
        func_8009C198();
        func_8009AB38(D_800C3E04);
        return;
    }
    if (D_800C3E04 < 3) {
        D_800C3DFC = (u8*)D_800C34B0 + (D_800C3E04 * 0x5F0 + 0x1058) + ((u8*)D_800C34B0)[0x5FC2] * 0x28;
    } else {
        D_800C3DFC = (u8*)D_800C34B0 + (((u8*)D_800C34B0)[0x5FC2] * 0x28 + 0x35D8);
    }
    func_8009AC48(D_800C3E04, 1);
    if (((u16*)D_800C3DFC)[5] & 0x10) {
        func_8009C198();
        func_8009AB38(D_800C3E04);
        func_80094C78();
        return;
    }
    D_800D2DC4 = 0;
    if ((((u16*)D_800C3DFC)[5] & 0x100) && ((u8*)D_800C3E00)[0x56] == 1) {
        func_80096824();
    }
    if (((u8*)D_800C3E00)[0x56] == 4 && (u8)(((u8*)D_800C34B0)[0x5FC2] - 4) < 2) {
        ((u8*)D_800C3DFC)[0x22] = ((BattleActorRows4*)D_800C3E00)->rows[((u8*)D_800C34B0)[0x5FC2] - 3][0];
    }
    if ((*(u16*)((u8*)D_800C3E00 + 0x80) & 0x20) && (D_800D2C94[0] & 7) && D_800D2C94[0] < 7) {
        for (i = 0; i < 3; i++) {
            if (D_800CCD3E__rows[i][0] == 3) {
                D_800D2C94[0] = 1 << i;
            }
        }
    }
    bit = 1;
    for (D_800C3E50 = 0; D_800C3E50 < 11; D_800C3E50++, bit <<= 1) {
        if (bit & *(u16*)((u8*)D_800C34B0 + 0x5FAC)) {
            D_800C3E34 = (u8*)D_800C34B0 + D_800C3E50 * 0x170;
            D_800D2DC8 = D_800C3E34 + 0xA4;
            D_800C3D60 = D_800C3E34 + 0x148;
            if (D_800C3E50 < 3) {
                func_800968C0();
            }
            D_800C348C[((u8*)D_800C3DFC)[0x16]]();
            func_800946F4();
            if (((Battle*)D_800C34B0)->f5FA0[D_800C3E50] == 0) {
                f = ((u16*)D_800C3DFC)[5];
                if (f & 0x800) {
                    func_80095B44();
                } else if (f & 0x4000) {
                    func_80095A78();
                } else if (f & 4) {
                    func_800958D8();
                }
            }
            if (((u16*)D_800C3DFC)[5] & 1) {
                *(u16*)((u8*)D_800C34B0 + 0x5FB0) = *(u16*)((u8*)D_800C3DFC + 2);
            } else {
                *(u16*)((u8*)D_800C34B0 + 0x5FB0) = ((u8*)D_800C34B0)[0x5FC2];
            }
        }
    }
    func_80099FB0();
    func_8009AB38(D_800C3E04);
    if (((u8*)D_800C3E00)[0x56] == 4 && D_800C34AE == 0) {
        func_8009AFD8();
    }
    if (D_800C3E04 < 3 && ((u8*)D_800C34B0)[0x5FC2] < 7) {
        u16* q = (u16*)((u8*)D_800C3E00 + ((u8*)D_800C34B0)[0x5FC2] * 2 + 0x90);

        if (*q < 65000) {
            *q += ((u8*)D_800C3E00)[0x55] + ((u8*)D_800C3E00)[0xA1];
        }
    }
    func_80094C78();
    return D_800D2DB8;
}

extern u16 D_800C3AA4[];
extern s32 D_800D2C54[];
extern u16 D_800CCD68__rows[][0xB8] asm("D_800CCD68");

/* func_800946F4.s, byte-exact: post-roll adjustments for actor D_800C3E50
 * (outcome byte +0x5FA0, result word +0x5F6C) against D_800C3E34.
 * When the outcome is 0:
 * - result word == D_800C3E34 +0x4C and D_800C3E04 < 3: the D_800C3E00
 *   +0x3A counter goes up by one, capped at 65000;
 * - clear bit 0x1000 of D_800C3E34 +0x80; a set bit 0x2000 is cleared
 *   when rand() % 100 < 70;
 * - D_800C3DFC +0xA bit 0x100: outcome 2 when D_800C3E34 +0x36 has
 *   0x4000, or 0x2000 with a zero low nibble in D_800C3DFC +0x22;
 * - D_800C3E34 +0x32 bit 0x80: rand() % 100 below 60 (80 when +0x56 is 0)
 *   halves the result word, otherwise half of it is added (unsigned);
 * - +0x32 bit 0x20: actor D_800C3E04 gets outcome 0 and D_800C3E50's
 *   result word;
 * - D_800C3E04 < 3 and != D_800C3E50: a set bit 2 in +0x7C of actor
 *   D_800C3E50's 0x170 record is cleared and +0x7A set from
 *   D_800C3AA4[D_800C3E50];
 * - +0x56 == 3 and D_800D2C54[D_800C3E50] >= +0x4C (unsigned): clear bit
 *   0x20 in the D_800CCD68 halfword of records 3..10.
 * Then, for any outcome: +0x36 bit 0x8000 turns outcomes 0/5 into 2 and
 * 2 into 0; +0x86 bit 0x80 doubles an outcome-2 result word; +0x8A bit
 * 0x200 zeroes an outcome-1 result word.
 * Codegen: ++/-- on the counter (retail compares the incremented value
 * without a reload); a block-local base pointer for the record address
 * (gcc 2.6.x then adds the base first, as in main56); the record loop
 * through a D_800CCD68 row view, which keeps the symbol in each address
 * (a u8 array plus offset hoists an `la`). */
void func_800946F4(void) {
    s32 thr;
    u8* p;
    u8 i;

    if (((Battle*)D_800C34B0)->f5FA0[D_800C3E50] == 0) {
        if (((Battle*)D_800C34B0)->f5F6C[D_800C3E50] == *(u16*)(D_800C3E34 + 0x4C) && D_800C3E04 < 3) {
            if (++*(u16*)((u8*)D_800C3E00 + 0x3A) > 65000) {
                --*(u16*)((u8*)D_800C3E00 + 0x3A);
            }
        }
        *(u16*)(D_800C3E34 + 0x80) &= 0xEFFF;
        if (*(u16*)(D_800C3E34 + 0x80) & 0x2000) {
            if (rand() % 100 < 70) {
                *(u16*)(D_800C3E34 + 0x80) &= 0xDFFF;
            }
        }
        if (*(u16*)((u8*)D_800C3DFC + 0xA) & 0x100) {
            if (*(u16*)(D_800C3E34 + 0x36) & 0x4000) {
                ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 2;
            }
            if ((*(u16*)(D_800C3E34 + 0x36) & 0x2000) && !(((u8*)D_800C3DFC)[0x22] & 0xF)) {
                ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 2;
            }
        }
        if (*(u16*)(D_800C3E34 + 0x32) & 0x80) {
            thr = 60;
            if (D_800C3E34[0x56] == 0) {
                thr = 80;
            }
            if (rand() % 100 < thr) {
                *(u32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) >>= 1;
            } else {
                *(u32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) += *(u32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) >> 1;
            }
        }
        if (*(u16*)(D_800C3E34 + 0x32) & 0x20) {
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E04] = 0;
            ((Battle*)D_800C34B0)->f5F6C[D_800C3E04] = ((Battle*)D_800C34B0)->f5F6C[D_800C3E50];
        }
        if (D_800C3E04 < 3 && D_800C3E04 != D_800C3E50) {
            u8* base = D_800C34B0;

            p = base + D_800C3E50 * 0x170;
            if (*(u16*)(p + 0x7C) & 2) {
                *(u16*)(p + 0x7C) &= 0xFFFD;
                *(u16*)(p + 0x7A) = D_800C3AA4[D_800C3E50];
            }
        }
        if (D_800C3E34[0x56] == 3 && (u32)D_800D2C54[D_800C3E50] >= *(u16*)(D_800C3E34 + 0x4C)) {
            for (i = 3; i < 11; i++) {
                D_800CCD68__rows[i][0] &= 0xFFDF;
            }
        }
    }
    if (*(u16*)(D_800C3E34 + 0x36) & 0x8000) {
        switch (((Battle*)D_800C34B0)->f5FA0[D_800C3E50]) {
        case 0:
        case 5:
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 2;
            break;
        case 2:
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
            break;
        }
    }
    if ((*(u16*)(D_800C3E34 + 0x86) & 0x80) && ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] == 2) {
        ((Battle*)D_800C34B0)->f5F6C[D_800C3E50] *= 2;
    }
    if ((*(u16*)(D_800C3E34 + 0x8A) & 0x200) && ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] == 1) {
        ((Battle*)D_800C34B0)->f5F6C[D_800C3E50] = 0;
    }
}
#endif
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
extern u16 D_800C3AA4[];

void func_80094D24(void) {
    u8 i;
    u8 cnt1;
    u8 cnt2;
    u16 f;
    u8 *p;

    cnt1 = 0;
    for (i = 0; i < 3; i++) {
        if (*(u16 *)((u8 *)((s32)D_800C34B0 - (-(i * 0x170))) + 0x7C) & 0xC000) {
            cnt1++;
        }
    }
    cnt2 = 0;
    for (i = 0; i < 3; i++) {
        if (*(u16 *)((u8 *)((s32)D_800C34B0 - (-(i * 0x170))) + 0x7C) & 2) {
            cnt2++;
        }
    }
    if (cnt1 == 1 && cnt2 == 2) {
        for (i = 0; i < 3; i++) {
            p = (u8 *)((s32)D_800C34B0 - (-(i * 0x170)));
            f = ((u16 *)p)[0x3E];
            if (f & 2) {
                ((u16 *)p)[0x3E] = f & 0xFFFD;
                ((u16 *)p)[0x3D] = D_800C3AA4[i];
            }
        }
    }
    if (cnt1 == 2 && cnt2 == 1) {
        for (i = 0; i < 3; i++) {
            p = (u8 *)((s32)D_800C34B0 - (-(i * 0x170)));
            f = ((u16 *)p)[0x3E];
            if (f & 2) {
                ((u16 *)p)[0x3E] = f & 0xFFFD;
                ((u16 *)p)[0x3D] = D_800C3AA4[i];
            }
        }
    }
}

s32 func_80096AB8(void);
s16 func_80096FBC(void);
s16 func_80097610(void);
void func_80096494(u16* atk, u16* def, u8* res);
extern u8 D_800D2C88[];

/* func_80094EE4.s, byte-exact: damage for target D_800C3E50 (outcome
 * byte +0x5FA0, result word +0x5F6C).  A target immune to the command
 * (D_800C3E34 +0x34 bit 0x4000 when command +0xA has bit 0x100, else bit
 * 0x8000) only gets outcome 0.  Otherwise res = func_80096AB8() (the hit
 * roll), atk = func_80096FBC() and def = func_80097610(); attacker
 * D_800C3E00 +0x88 | +0x8A bits 8 / 2 / 4 / 1 change atk by +1/5, +1/10,
 * -1/5, -1/10, and the target's bits 4 / 1 / 8 / 2 do the same to def.
 * Command +0x22 bit 0x10 sets target +0x80 bit 0x40 (unless +0x82 has
 * it) and, for attacker +0x8C | +0x8E bits 0x4000 / 0x1000, D_800C3E04's
 * D_800D2C88 byte (3 / 2) and D_800D2C54 word ((u16)(+0x52 / 10) * 2 /
 * (u16)(+0x4E / 10) * 2); +0x22 bit 0x20 sets target +0x80 bit 0x80
 * (unless +0x82 has it).  Target +0x80 bit 0x40 takes a quarter off def
 * and attacker +0x80 bit 0x80 a quarter off atk, clearing the bit; +0xA
 * bit 0x400 makes the rate 20 instead of command +0x11.
 * func_80096494(&atk, &def, &res) applies the element modifier.  Then
 * dmg = 5 * atk - 4 * def (+0xA bit 0x100) or 4 * atk - 3 * def, only
 * the atk term when def is 0; damage types 0 and 1 (+0x1A) scale it by
 * rate / 20; dmg <= 0 becomes 0, below 10 gains rand() % 2, else
 * rand() % (dmg / 10 + 2).  res 1: outcome 0 and at least 1 damage;
 * 2: half damage, outcome 5; 3: no damage, outcome 4; 4: outcome 2;
 * 5: outcome 7.  With D_800D2DC4 set, a +0xA bit 0x100 command divides a
 * nonzero dmg by 3; the result is clamped to 0..9999.
 * The switch on res is a jump table (jtbl_80070370, placed by the main51
 * .rodata split in config/battle.yaml).  Codegen: record fields are read
 * in array form (((u16*)D_800C3E00)[k]); as in-struct loads they may be
 * scheduled before the stack stores of atk/def, as in retail.  The
 * immunity mask goes through an if/else temp.  The type test is two
 * nested ifs on an int: a switch tests the upper bound first and `&&`
 * folds into one sltiu, while retail tests >= 0 first.  The switch takes
 * res as (s8): retail sign-extends res - 1 (sll/sra 24), which a u8
 * switch omits.  Case 1 writes the outcome in both arms (retail repeats
 * the address computation and shares the store). */
void func_80094EE4(void) {
    u16 atk;
    u16 def;
    u8 res;
    u8 rate;
    s32 m;
    s32 n;
    s32 dmg;
    s32 t;
    s32 type;

    rate = ((u8*)D_800C3DFC)[0x11];
    if (((u16*)D_800C3DFC)[5] & 0x100) {
        t = ((u16*)D_800C3E34)[0x1A] & 0x4000;
    } else {
        t = ((u16*)D_800C3E34)[0x1A] & 0x8000;
    }
    if (t) {
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
        return;
    }
    res = func_80096AB8();
    atk = func_80096FBC();
    def = func_80097610();
    if ((((u16*)D_800C3E00)[0x44] | ((u16*)D_800C3E00)[0x45]) & 8) {
        atk += atk / 5;
    }
    if ((((u16*)D_800C3E00)[0x44] | ((u16*)D_800C3E00)[0x45]) & 2) {
        atk += atk / 10;
    }
    if ((((u16*)D_800C3E00)[0x44] | ((u16*)D_800C3E00)[0x45]) & 4) {
        atk -= atk / 5;
    }
    if ((((u16*)D_800C3E00)[0x44] | ((u16*)D_800C3E00)[0x45]) & 1) {
        atk -= atk / 10;
    }
    if ((((u16*)D_800C3E34)[0x44] | ((u16*)D_800C3E34)[0x45]) & 4) {
        def += def / 5;
    }
    if ((((u16*)D_800C3E34)[0x44] | ((u16*)D_800C3E34)[0x45]) & 1) {
        def += def / 10;
    }
    if ((((u16*)D_800C3E34)[0x44] | ((u16*)D_800C3E34)[0x45]) & 8) {
        def -= def / 5;
    }
    if ((((u16*)D_800C3E34)[0x44] | ((u16*)D_800C3E34)[0x45]) & 2) {
        def -= def / 10;
    }
    if (((u8*)D_800C3DFC)[0x22] & 0x10) {
        if (!(((u16*)D_800C3E34)[0x41] & 0x40)) {
            ((u16*)D_800C3E34)[0x40] |= 0x40;
        }
        if ((((u16*)D_800C3E00)[0x46] | ((u16*)D_800C3E00)[0x47]) & 0x4000) {
            D_800D2C88[D_800C3E04] = 3;
            D_800D2C54[D_800C3E04] = (u16)(((u16*)D_800C3E00)[0x29] / 10) * 2;
        }
        if ((((u16*)D_800C3E00)[0x46] | ((u16*)D_800C3E00)[0x47]) & 0x1000) {
            D_800D2C88[D_800C3E04] = 2;
            D_800D2C54[D_800C3E04] = (u16)(((u16*)D_800C3E00)[0x27] / 10) * 2;
        }
    }
    if ((((u8*)D_800C3DFC)[0x22] & 0x20) && !(((u16*)D_800C3E34)[0x41] & 0x80)) {
        ((u16*)D_800C3E34)[0x40] |= 0x80;
    }
    if (((u16*)D_800C3E34)[0x40] & 0x40) {
        def -= def >> 2;
        ((u16*)D_800C3E34)[0x40] &= 0xFFBF;
    }
    if (((u16*)D_800C3E00)[0x40] & 0x80) {
        atk -= atk >> 2;
        ((u16*)D_800C3E00)[0x40] &= 0xFF7F;
    }
    if (((u16*)D_800C3DFC)[5] & 0x400) {
        rate = 20;
    }
    func_80096494(&atk, &def, &res);
    if (((u16*)D_800C3DFC)[5] & 0x100) {
        m = 5;
        n = 4;
    } else {
        m = 4;
        n = 3;
    }
    if (def != 0) {
        dmg = m * atk - n * def;
    } else {
        dmg = m * atk;
    }
    type = ((u8*)D_800C3DFC)[0x1A];
    if (type >= 0) {
        if (type < 2) {
            dmg = rate * dmg / 20;
        }
    }
    if (dmg <= 0) {
        dmg = 0;
    } else if (dmg < 10) {
        dmg += rand() % 2;
    } else {
        dmg += rand() % (dmg / 10 + 2);
    }
    switch ((s8)res) {
    case 1:
        if (dmg > 0) {
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
        } else {
            dmg = 1;
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
        }
        break;
    case 2:
        dmg /= 2;
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 5;
        break;
    case 3:
        dmg = 0;
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 4;
        break;
    case 4:
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 2;
        break;
    case 5:
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 7;
        break;
    }
    if (D_800D2DC4 != 0 && (((u16*)D_800C3DFC)[5] & 0x100) && dmg != 0) {
        dmg /= 3;
    }
    if (dmg >= 10000) {
        dmg = 9999;
    }
    if (dmg < 0) {
        dmg = 0;
    }
    ((Battle*)D_800C34B0)->f5F6C[D_800C3E50] = dmg;
}
extern u8* D_800C3E34;
extern u8 D_800CCE42[];

void func_80095690(void) {
    s16 temp_lo;
    s16 var_a1;
    u16 temp_a2;

    temp_lo = *(u8 *)((s8*)(D_800C3E00) + 0x5B) * *(u8 *)((s8*)(D_800C3DFC) + 0x11);
    var_a1 = temp_lo;
    if (*(u16 *)((s8*)(D_800C3E00) + 0x8A) & 0x2000) {
        var_a1 = temp_lo * 2;
    }
    temp_a2 = *(u16 *)((s8*)(D_800C3E34) + 0x8C) | *(u16 *)((s8*)(D_800C3E34) + 0x8E);
    if (temp_a2 & 0x100) {
        var_a1 = (var_a1 * 7) / 10;
    }
    if (temp_a2 & 0x200) {
        var_a1 = (var_a1 * 0xD) / 10;
    }
    if (D_800CCE42[D_800C3E50 * 0x170] & 0x80) {
        var_a1 = 0;
    }
    *(s8 *)((s8*)((D_800C34B0 + D_800C3E50)) + 0x5FA0) = 2;
    *(s32 *)((s8*)(((D_800C3E50 * 4) + D_800C34B0)) + 0x5F6C) = (s32) var_a1;
}

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
/* func_800958D8.s, byte-exact: when rand() % 100 is at most byte +0x1C of
 * D_800C3DFC and its byte +0x1D is 0x6E, clear each halfword +0x84 ... +0x8E
 * of D_800C3E34 whose bit (0x8000 down to 0x400) is set in D_800C3DFC's
 * halfword +0x1E, and set byte +0x5FC7 of D_800C34B0 to 0x3A. */
void func_800958D8(void) {
    if (rand() % 100 <= *(u8*)((s8*)D_800C3DFC + 0x1C) && *(u8*)((s8*)D_800C3DFC + 0x1D) == 0x6E) {
        if (*(u16*)((s8*)D_800C3DFC + 0x1E) & 0x8000) {
            *(s16*)((s8*)D_800C3E34 + 0x84) = 0;
        }
        if (*(u16*)((s8*)D_800C3DFC + 0x1E) & 0x4000) {
            *(s16*)((s8*)D_800C3E34 + 0x86) = 0;
        }
        if (*(u16*)((s8*)D_800C3DFC + 0x1E) & 0x2000) {
            *(s16*)((s8*)D_800C3E34 + 0x88) = 0;
        }
        if (*(u16*)((s8*)D_800C3DFC + 0x1E) & 0x1000) {
            *(s16*)((s8*)D_800C3E34 + 0x8A) = 0;
        }
        if (*(u16*)((s8*)D_800C3DFC + 0x1E) & 0x800) {
            *(s16*)((s8*)D_800C3E34 + 0x8C) = 0;
        }
        if (*(u16*)((s8*)D_800C3DFC + 0x1E) & 0x400) {
            *(s16*)((s8*)D_800C3E34 + 0x8E) = 0;
        }
        ((u8*)D_800C34B0)[0x5FC7] = 0x3A;
    }
}
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

#ifndef XENO_PC_PORT

void func_80095BAC(void) {
    u8 temp_s0;

    temp_s0 = *(u8 *)((s8*)(D_800C3DFC) + 0x1C);
    if ((s32) temp_s0 < (rand() % 100)) {
        u8* b = D_800C34B0;

        (b + D_800C3E50)[0x5FA0] = 6;
        return;
    }
    if (*(u8 *)((s8*)(D_800C3DFC) + 0x1D) & 0x80) {
        *(u16 *)((s8*)(D_800C3E34) + 0x7C) = (u16) (*(u16 *)((s8*)(D_800C3E34) + 0x7C) & 0xC000);
    }
    if (*(u8 *)((s8*)(D_800C3DFC) + 0x1D) & 0x40) {
        *(s16 *)((s8*)(D_800C3E34) + 0x80) = 0;
        *(u16 *)((s8*)(D_800C3E34) + 0x7A) = (u16) (*(u16 *)((s8*)(D_800C3E34) + 0x7A) & 0xFFDF);
    }
    if (*(u8 *)((s8*)(D_800C3DFC) + 0x1D) & 0x20) {
        *(s16 *)((s8*)(D_800C3E34) + 0x84) = 0;
    }
    if (*(u8 *)((s8*)(D_800C3DFC) + 0x1D) & 0x10) {
        *(s16 *)((s8*)(D_800C3E34) + 0x88) = 0;
    }
    if (*(u8 *)((s8*)(D_800C3DFC) + 0x1D) & 8) {
        *(s16 *)((s8*)(D_800C3E34) + 0x8C) = 0;
    }
}
#endif

extern u8 D_800C3E04;

/* func_80095D4C.s, byte-exact: rate check (D_800C3DFC mode +0x18: 0 =
 * D_800C3E00 byte +0x60, 1 = own byte +0x1C; unset for other modes, as in
 * retail); on a miss (rate < rand() % 100) actor D_800C3E50's +0x5FA0
 * outcome byte is 6.  Otherwise D_800C3DFC +0x1A picks a halfword (0/2 =
 * D_800C3E00 +0x4E/+0x52, 1/3 = D_800C3E34 +0x4E/+0x52), scaled to
 * amt = val * byte +0x11 / 20; mode 5 takes D_800C3E34 +0x4C - 1 instead
 * (the scaled value of the unset val is discarded, as in retail).  Modes
 * 0, 1 and 5 set outcomes D_800C3E04 = 2 / D_800C3E50 = 0; modes 2 and 3
 * set outcome 6 on D_800C3E50 when D_800C3E34 +0x88 | +0x8A has bit 0x200,
 * else 3 / 1; both write amt to the two actors' +0x5F6C result words.
 * The switch is a jump table (jtbl_80070388, placed by the main51 .rodata
 * split in config/battle.yaml).  Codegen: the result words go through
 * Battle's f5F6C array.  In-struct stores do not force reloads of
 * D_800C34B0 or D_800C3E50, so both addresses are formed before the two
 * stores as in retail (the raw *(s32*) form reloads between them), and amt
 * is stored directly (an s32 copy moves it from $a2 to $a1). */
void func_80095D4C(void) {
    u8 rate;
    u16 val;
    u16 amt;

    switch (((u8*)D_800C3DFC)[0x18]) {
    case 0:
        rate = ((u8*)D_800C3E00)[0x60];
        break;
    case 1:
        rate = ((u8*)D_800C3DFC)[0x1C];
        break;
    }
    if (rate < rand() % 100) {
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 6;
        return;
    }
    switch (((u8*)D_800C3DFC)[0x1A]) {
    case 0:
        val = *(u16*)((u8*)D_800C3E00 + 0x4E);
        break;
    case 1:
        val = *(u16*)(D_800C3E34 + 0x4E);
        break;
    case 2:
        val = *(u16*)((u8*)D_800C3E00 + 0x52);
        break;
    case 3:
        val = *(u16*)(D_800C3E34 + 0x52);
        break;
    }
    amt = val * ((u8*)D_800C3DFC)[0x11] / 20;
    if (((u8*)D_800C3DFC)[0x1A] == 5) {
        amt = *(u16*)(D_800C3E34 + 0x4C) - 1;
    }
    switch (((u8*)D_800C3DFC)[0x1A]) {
    case 0:
    case 1:
    case 5:
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E04] = 2;
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
        ((Battle*)D_800C34B0)->f5F6C[D_800C3E04] = amt;
        ((Battle*)D_800C34B0)->f5F6C[D_800C3E50] = amt;
        break;
    case 2:
    case 3:
        if ((*(u16*)(D_800C3E34 + 0x88) | *(u16*)(D_800C3E34 + 0x8A)) & 0x200) {
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 6;
            break;
        }
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E04] = 3;
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 1;
        ((Battle*)D_800C34B0)->f5F6C[D_800C3E04] = amt;
        ((Battle*)D_800C34B0)->f5F6C[D_800C3E50] = amt;
        break;
    }
}

extern u8* D_800D2DC8;
extern u8* D_800D2D6C;
extern u8 D_800CCE42[];


/* func_80096018.s, byte-exact: rate check as in func_80095D4C (D_800C3DFC
 * mode +0x18: 0 = D_800C3E00 byte +0x60, 1 = own byte +0x1C; unset for
 * other modes, as in retail); on a miss (rate < rand() % 100) the acting
 * actor's +0x5FA0 outcome byte is 6, else 0 and D_800C3DFC +0x1A picks the
 * result word at +0x5F6C (0x170-record actor D_800C3E50): 0 = D_800D2DC8
 * word +0x60 (bit-7 actor, unsigned) or D_800C3E34 halfword +0x4C, divided
 * by +0x11; 1 = the same value - 1; 2 = +0x64 - +0x60 of D_800D2D6C (actor
 * D_800C3E04 bit 7) or +0x4E - +0x4C of D_800C3E00; 3 = D_800C3E34 +0x50
 * * 10; 4 = 1; 5 = D_800C3E34 +0x4C; 6 = outcome 0 and D_800D2DC8 word +0x64
 * (D_800CCE42 bit 7) or D_800C3E34 halfword +0x4E, clamped in place to 9999;
 * 7 = outcome 6 for a bit-7 actor, else bit 0 of D_800C3E34 +0x7C and +0x80.
 * The switch is a jump table (jtbl_800703A0, placed by the main51 .rodata
 * split in config/battle.yaml).  Codegen: outcome bytes and actor flags
 * through Battle (base-first), result words by index arithmetic. */
void func_80096018(void) {
    u8 rate;

    switch (((u8*)D_800C3DFC)[0x18]) {
    case 0:
        rate = ((u8*)D_800C3E00)[0x60];
        break;
    case 1:
        rate = ((u8*)D_800C3DFC)[0x1C];
        break;
    }
    if (rate < rand() % 100) {
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 6;
        return;
    }
    ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
    switch (((u8*)D_800C3DFC)[0x1A]) {
    case 0:
        if (((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 0x80) {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u32*)(D_800D2DC8 + 0x60) / ((u8*)D_800C3DFC)[0x11];
        } else {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u16*)(D_800C3E34 + 0x4C) / ((u8*)D_800C3DFC)[0x11];
        }
        break;
    case 1:
        if (((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 0x80) {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(s32*)(D_800D2DC8 + 0x60) - 1;
        } else {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u16*)(D_800C3E34 + 0x4C) - 1;
        }
        break;
    case 2:
        if (((Battle*)D_800C34B0)->actors[D_800C3E04].flags & 0x80) {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(s32*)(D_800D2D6C + 0x64) - *(s32*)(D_800D2D6C + 0x60);
        } else {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u16*)((u8*)D_800C3E00 + 0x4E) - *(u16*)((u8*)D_800C3E00 + 0x4C);
        }
        break;
    case 3:
        *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u16*)(D_800C3E34 + 0x50) * 10;
        break;
    case 4:
        *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = 1;
        break;
    case 5:
        *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u16*)(D_800C3E34 + 0x4C);
        break;
    case 6:
        ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 0;
        if (D_800CCE42[D_800C3E50 * 0x170] & 0x80) {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(s32*)(D_800D2DC8 + 0x64);
            if (*(u32*)(D_800D2DC8 + 0x64) >= 10000) {
                *(s32*)(D_800D2DC8 + 0x64) = 9999;
            }
        } else {
            *(s32*)((u8*)D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = *(u16*)(D_800C3E34 + 0x4E);
            if (*(u16*)(D_800C3E34 + 0x4E) >= 10000) {
                *(u16*)(D_800C3E34 + 0x4E) = 9999;
            }
        }
        break;
    case 7:
        if (((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 0x80) {
            ((Battle*)D_800C34B0)->f5FA0[D_800C3E50] = 6;
        } else {
            *(u16*)(D_800C3E34 + 0x7C) |= 1;
            *(u16*)(D_800C3E34 + 0x80) |= 1;
        }
        break;
    }
}
extern u8* D_800D2D6C;
extern u8 D_800C3E04;
extern u8* D_800D2DC8;

/* func_80096494.s, byte-exact: apply the element modifier to atk/def/hit (see
 * the caller notes near the top of this block).
 * CONSTRUCTS (counted separately):
 * - The u8 flag `weak` is reused to hold the second status word, so the
 *   first `if (weak)` keeps retail's andi 0xFF.
 * - The inherited permuter cast `(u8)(elem & st1)`.
 * - `if (st & 8) *hit = 4;` is duplicated into both arms of each `if (st & 4)`
 *   (the duplicated-statement allocation lever: st reaches 8 refs and gets
 *   v1).  jump2 cross-jumps the copies back.
 * Natural: the block-local u32 `sv` per arm, and `st1 = (...) >> 12` in each
 * arm (both small sets, so combine drops elem's mask in `elem & st1`). */
void func_80096494(u16* atk, u16* def, u8* hit) {
    u8* base;
    u8* p;
    u8* q;
    u16 st1;
    u8 self;
    u8 elem;
    u8 res;
    u8 weak;
    s16 weak2;
    s16 boost;
    u16 st;
    s8 mul;
    s32 r8;
    u8 dmul;

    boost = 0;
    weak = 0;
    base = D_800C34B0;
    q = base + D_800C3E50 * 0x170;
    self = q[0x15A] >> 7;
    elem = ((u8*)D_800C3DFC)[0x22] & 0x3F;
    res = D_800C3E34[0x38] & 0x3F;
    p = base + D_800C3E04 * 0x170;
    if (!(p[0x15A] >> 7)) {
        st1 = (((u16*)D_800C3E00)[0x46] | ((u16*)D_800C3E00)[0x47]) >> 12;
    } else {
        st1 = (((u16*)D_800D2D6C)[0x42] | ((u16*)D_800D2D6C)[0x43]) >> 12;
    }
    if (((u16*)D_800C3DFC)[0x5] & 0x100) {
        boost = 1;
    }
    if (elem == 0 && st1 != 0) {
        elem = st1;
    }
    if ((u8)(elem & st1)) {
        weak = 1;
    }
    mul = 10;
    dmul = 10;
    if (elem & res) {
        mul = (((u16*)D_800C3E34)[0x1C] & 0x40) ? 0x12 : 0xF;
        if (weak) {
            mul += 2;
        }
    }
    weak2 = 0;
    if (!self) {
        u32 sv = ((u16*)D_800C3E34)[0x46] | ((u16*)D_800C3E34)[0x47];

        res = (sv & 0xF00) >> 8;
        st = sv;
        weak2 = (sv >> 1) & 1;
    } else {
        u32 sv = ((u16*)D_800D2DC8)[0x42] | ((u16*)D_800D2DC8)[0x43];

        res = (sv & 0xF00) >> 8;
        st = sv;
        weak = sv;
        if (weak & 2) {
            weak2 = 1;
        }
    }
    if (elem & res) {
        if (boost) {
            mul -= 3;
            if (st & 4) {
                mul -= 3;
                if (st & 8) {
                    *hit = 4;
                }
            } else {
                if (st & 8) {
                    *hit = 4;
                }
            }
        }
        if ((elem & res) && weak2) {
            mul -= 3;
            if (st & 4) {
                mul -= 3;
                if (st & 8) {
                    *hit = 4;
                }
            } else {
                if (st & 8) {
                    *hit = 4;
                }
            }
        }
    }
    r8 = res << 8;
    switch (elem) {
    case 8:
        if (r8 & 0x400) {
            mul += 3;
        }
        break;
    case 4:
        if (r8 & 0x800) {
            mul += 3;
        }
        break;
    case 2:
        if (r8 & 0x100) {
            mul += 3;
        }
        break;
    case 1:
        if (r8 & 0x200) {
            mul += 3;
        }
        break;
    }
    if (mul <= 0) {
        mul = 1;
    }
    if (dmul <= 0) {
        dmul = 1;
    }
    *atk = *atk * mul / 10;
    *def = dmul * *def / 10;
    if (((u16*)D_800C3E00)[0x19] & 0x10) {
        *atk += *atk / 5;
    }
    if (((u16*)D_800C3E34)[0x19] & 0x10) {
        *def += *def / 5;
    }
}


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

#ifndef XENO_PC_PORT
extern u8 D_800C3E04;

void func_800968C0(void) {
    s32 var_s0;
    u16 temp_v1;
    u16 temp_v1_2;
    u8 temp_a1;
    void *temp_v0;

    if (!(*(u16 *)((s8*)(D_800C3E34) + 0x7C) & 0xA000)) {
        temp_v1 = *(u16 *)((s8*)(D_800C3E34) + 0x80);
        if (!(temp_v1 & 0x1000) && (((u8) D_800C3E04 >= 3U) || (temp_v1 & 0x2000)) && (*(u16 *)((s8*)(D_800C3E34) + 0x88) & 0x1000)) {
            temp_v1_2 = *(u16 *)((s8*)(D_800C3DFC) + 0xA);
            if (!(temp_v1_2 & 0x2000) && !(temp_v1_2 & 0x100) && !(((Battle *)D_800C34B0)->actors[D_800C3E04].flags & 0x80)) {
                var_s0 = 0x32;
                if (*(u8 *)((s8*)(D_800C3E34) + 0x56) == 0) {
                    var_s0 = 0x3C;
                }
                if ((var_s0 >= (rand() % 100)) && (*(u8 *)((s8*)(D_800C3DFC) + 0x16) != 2)) {
                    temp_a1 = D_800C3E04;
                    D_800C3E04 = D_800C3E50;
                    temp_v0 = D_800C3E00;
                    D_800C3E00 = D_800C3E34;
                    D_800C3E50 = temp_a1;
                    D_800C3E34 = temp_v0;
                    D_800C3DFC = (D_800C3E04 * 0x5F0) + D_800C34B0 + 0x1378;
                    *(s8 *)((s8*)((D_800C34B0 + D_800C3E04)) + 0x5FA0) = 7;
                    *(s32 *)((s8*)(((D_800C3E04 * 4) + D_800C34B0)) + 0x5F6C) = 0;
                    *(s8 *)((s8*)(D_800C34B0) + 0x5FC7) = 0x34;
                }
            }
        }
    }
}
#endif

/* func_80096AB8.s, byte-exact: the hit roll for func_80094EE4.  It returns
 * the outcome code.
 * - 3 when: command +0xA bit 0x40 is set with the target's D_800CCE42
 *   bit 7; either D_800C3DFC +0x10 bit 0x80 / 0x10 check finds
 *   g_GameState[0x2286 + attacker +0x6F / +0x72] == 0 (these two also set
 *   D_800C34AE = 1); command +0xA bit 0x200 is set with target +0x34 bit
 *   8 or D_800CCE42 bit 7; command flag 0x1000 is set; or target status
 *   0x100 is set.
 * - 1 for the target's +0x7C 0x2000 / +0x80 0x1000 bits or command flag
 *   0x8000, and 5 for command flag 2.
 * - Otherwise roll = rand() % 100 - diff, with diff = (s8) command +0x15
 *   + attacker +0x5E - target +0x5F.  Bit 0 of the target's actor-record
 *   flags (actors[D_800C3E50]) gives 2 at 95%, else 1.  Target status
 *   0x20 gives 1 below 50, else 3; status 0x40 gives 1 below 50, else 2.
 *   Then 3 at or above bonus - (penalty - 90), 2 at or above
 *   bonus - (penalty - 85), else 1.
 * Codegen (gcc 2.6.3):
 * - The four roll-vs-limit picks (status 0x20, status 0x40 and the two
 *   final limits) are written with the test inverted
 *   (`if (roll >= 50) return 3; return 1;`).  Each then keeps its own two
 *   returns, as in retail.  Written the other way, jump.c turns them into
 *   `v0 = 3; if (c) v0 = 1` and cross-jumps the sites together.  The 95%
 *   actor-flag pick keeps its `<` form.
 * - The last pick comes last, so it falls into the epilogue.
 * - The inner (s16) in (s16)(penalty - 90) keeps the subtraction from
 *   being reassociated into (bonus + 90) - penalty.
 * - diff is written (s8)cmd[0x15] + atk - def, which gives retail's add
 *   operand order. */
extern u8 D_800C34AE;

s32 func_80096AB8(void) {
    s16 bonus = 0;
    s16 penalty = 0;
    u8 atk = ((u8*)D_800C3E00)[0x5E];
    u8 def = D_800C3E34[0x5F];
    u16 flags;
    u16 st;
    s16 diff;

    if ((*(u16*)((u8*)D_800C3DFC + 0xA) & 0x40) && (D_800CCE42[D_800C3E50 * 0x170] & 0x80)) {
        return 3;
    }
    if (((u8*)D_800C3E00)[0x56] == 4) {
        if ((((u8*)D_800C3DFC)[0x10] & 0x80) && g_GameState[0x2286 + ((u8*)D_800C3E00)[0x6F]] == 0) {
            D_800C34AE = 1;
            return 3;
        }
        if ((((u8*)D_800C3DFC)[0x10] & 0x10) && g_GameState[0x2286 + ((u8*)D_800C3E00)[0x72]] == 0) {
            D_800C34AE = 1;
            return 3;
        }
    }
    if (*(u16*)((u8*)D_800C3DFC + 0xA) & 0x200) {
        if (*(u16*)(D_800C3E34 + 0x34) & 8) {
            return 3;
        }
        if (D_800CCE42[D_800C3E50 * 0x170] & 0x80) {
            return 3;
        }
    }
    flags = *(u16*)((u8*)D_800C3DFC + 0xA);
    if (flags & 0x1000) {
        return 3;
    }
    st = *(u16*)(D_800C3E34 + 0x84) | *(u16*)(D_800C3E34 + 0x86);
    if (st & 0x100) {
        return 3;
    }
    if (*(u16*)(D_800C3E34 + 0x7C) & 0x2000) {
        return 1;
    }
    if (*(u16*)(D_800C3E34 + 0x80) & 0x1000) {
        return 1;
    }
    if (flags & 0x8000) {
        return 1;
    }
    if (flags & 2) {
        return 5;
    }
    if (*(u16*)((u8*)D_800C3E00 + 0x7C) & 0x400) {
        bonus -= 50;
    }
    if ((*(u16*)((u8*)D_800C3E00 + 0x84) | *(u16*)((u8*)D_800C3E00 + 0x86)) & 0x1000) {
        bonus += 30;
    }
    if (st & 0x800) {
        penalty += 50;
    }
    diff = (s8)((u8*)D_800C3DFC)[0x15] + atk - def;
    if (((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 1) {
        if (rand() % 100 < 95) {
            return 2;
        }
        return 1;
    }
    if (st & 0x20) {
        if ((s16)(rand() % 100 - diff) >= 50) {
            return 3;
        }
        return 1;
    }
    if ((*(u16*)(D_800C3E34 + 0x84) | *(u16*)(D_800C3E34 + 0x86)) & 0x40) {
        if ((s16)(rand() % 100 - diff) >= 50) {
            return 2;
        }
        return 1;
    }
    if ((s16)(rand() % 100 - diff) >= (s16)(bonus - (s16)(penalty - 90))) {
        return 3;
    }
    if ((s16)(rand() % 100 - diff) >= (s16)(bonus - (s16)(penalty - 85))) {
        return 2;
    }
    return 1;
}
extern u8 D_800CCE2A[];
extern void func_8009B46C(u16* p);

/* func_80096FBC.s, byte-exact: attack value of actor D_800C3E04 for
 * command D_800C3DFC.  An actor whose D_800C34B0 record +0x15A has bit 7
 * takes three bytes from D_800D2D6C (+0x12 + i * 8) and power = +0x3C *
 * +0x3F (plus 2 * +0x3C with +0x80 | +0x82 bit 0x1000); any other takes
 * four bytes from D_800C3E00 (+4 + i * 8) and power = +0x58.  Command
 * +0x10 bits 0x80 / 0x40 / 0x20 / 0x10 / 8 add bytes 0..4 to the sum
 * (byte 4 is never filled, nor byte 3 in the first case, as in retail);
 * attacker +0x8C | +0x8E bit 0 adds half the sum.  For +0xA bit 0x100
 * the D_800C3E00 byte +0x5B is scaled by k / 4 (k = 4, 5 with +0x88 |
 * +0x8A bit 0x8000, one less with +0x80 bit 0x400) and doubled with
 * +0x88 | +0x8A bit 0x2000; otherwise, unless +0x15A has bit 7, power is
 * scaled by k / 4 (k = 4, 5 with +0x84 | +0x86 bit 0x2000, one less with
 * +0x7C bit 0x200, plus 10 - +0x4C / (+0x4E / 10) with bit 0x400).  +0xA
 * bit 0x20 drops the power.  +0x1A picks the result: 0 = sum + power,
 * 1 = the scaled +0x5B byte, 2 = that byte (bit 0x100) or sum + power
 * (the sum cut to 6/10 when +0x56 == 4) times +0x11 / 20; other types
 * leave it unset, as in retail.  For party actors (< 3) it is then
 * adjusted: func_8009B46C for +0x56 == 7; +1/4 for target D_800C3E34
 * +0x38 bit 0x20 and again for bits 0x60 (with +0x56 == 4 or command
 * +0x22 bit 0x20); * D_800CCE2A / 4 for +0x56 == 8 with D_800CCE42 bit 7
 * clear and bit 0x100; +1/5 for +0x56 == 10; +1/4 twice for +0x22 bit
 * 0x10 with target +0x38 bit 0x10.
 * Codegen: the table loops address each row through its own pointer from
 * a block-local base (gcc 2.6.x then adds the base first, as in
 * func_8009DA04); `k += 10 - ...` keeps retail's (k + 10) - q, where
 * `k = k + 10 - ...` folds the constant into the quotient; record fields
 * are read in array form, as in func_800941A4. */
s16 func_80096FBC(void) {
    u8 w[5];
    u16 res;
    u8 i;
    u16 pow;
    u16 sum;
    u16 ether;
    u8 attr;
    u8 k;
    u16 f;

    if (((Battle*)D_800C34B0)->actors[D_800C3E04].flags & 0x80) {
        for (i = 0; i < 3; i++) {
            u8* base = D_800D2D6C;
            u8* p = base + i * 8;

            w[i] = p[0x12];
        }
        pow = D_800D2D6C[0x3C] * D_800D2D6C[0x3F];
        if ((((u16*)D_800D2D6C)[0x40] | ((u16*)D_800D2D6C)[0x41]) & 0x1000) {
            pow += D_800D2D6C[0x3C] * 2;
        }
    } else {
        for (i = 0; i < 4; i++) {
            u8* base = D_800C3E00;
            u8* p = base + i * 8;

            w[i] = p[4];
        }
        pow = ((u8*)D_800C3E00)[0x58];
    }
    attr = ((u8*)D_800C3DFC)[0x10];
    ether = ((u8*)D_800C3E00)[0x5B];
    sum = 0;
    if (attr & 0x80) {
        sum = w[0];
    }
    if (attr & 0x40) {
        sum += w[1];
    }
    if (attr & 0x20) {
        sum += w[2];
    }
    if (attr & 0x10) {
        sum += w[3];
    }
    if (attr & 8) {
        sum += w[4];
    }
    if ((((u16*)D_800C3E00)[0x46] | ((u16*)D_800C3E00)[0x47]) & 1) {
        sum += sum >> 1;
    }
    if (((u16*)D_800C3DFC)[5] & 0x100) {
        k = 4;
        if ((((u16*)D_800C3E00)[0x44] | ((u16*)D_800C3E00)[0x45]) & 0x8000) {
            k = 5;
        }
        if (((u16*)D_800C3E00)[0x40] & 0x400) {
            k--;
        }
        ether = ether * k / 4;
        if ((((u16*)D_800C3E00)[0x44] | ((u16*)D_800C3E00)[0x45]) & 0x2000) {
            ether *= 2;
        }
    } else if (!(((Battle*)D_800C34B0)->actors[D_800C3E04].flags & 0x80)) {
        f = ((u16*)D_800C3E00)[0x42] | ((u16*)D_800C3E00)[0x43];
        k = 4;
        if (f & 0x2000) {
            k = 5;
        }
        if (((u16*)D_800C3E00)[0x3E] & 0x200) {
            k--;
        }
        if (f & 0x400) {
            k += 10 - ((u16*)D_800C3E00)[0x26] / (u16)(((u16*)D_800C3E00)[0x27] / 10);
        }
        pow = pow * k / 4;
    }
    if (((u16*)D_800C3DFC)[5] & 0x20) {
        pow = 0;
    }
    switch (((u8*)D_800C3DFC)[0x1A]) {
    case 0:
        res = sum + pow;
        break;
    case 1:
        res = ether;
        break;
    case 2:
        if (((u8*)D_800C3E00)[0x56] == 4) {
            sum = sum * 6 / 10;
        }
        if (((u16*)D_800C3DFC)[5] & 0x100) {
            res = ether * ((u8*)D_800C3DFC)[0x11] / 20;
        } else {
            res = (sum + pow) * ((u8*)D_800C3DFC)[0x11] / 20;
        }
        break;
    }
    if (D_800C3E04 < 3) {
        if (((u8*)D_800C3E00)[0x56] == 7) {
            func_8009B46C(&res);
        }
        if (((u8*)D_800C3E00)[0x56] == 4 || (((u8*)D_800C3DFC)[0x22] & 0x20)) {
            if (((u16*)D_800C3E34)[0x1C] & 0x20) {
                res += res >> 2;
            }
            if ((((u32*)D_800C3E34)[0xE] & 0x60) == 0x60) {
                res += res >> 2;
            }
        }
        if (((u8*)D_800C3E00)[0x56] == 8 && !(D_800CCE42[D_800C3E04 * 0x170] & 0x80) && (((u16*)D_800C3DFC)[5] & 0x100)) {
            res = res * D_800CCE2A[D_800C3E04 * 0x170] / 4;
        }
        if (((u8*)D_800C3E00)[0x56] == 10) {
            res += res / 5;
        }
        if ((((u8*)D_800C3DFC)[0x22] & 0x10) && (((u16*)D_800C3E34)[0x1C] & 0x10)) {
            res += res >> 2;
            if (((u16*)D_800C3E34)[0x1C] & 0x10) {
                res += res >> 2;
            }
        }
    }
    return res;
}
extern u8* D_800D2DC8;

/* func_80097610.s, byte-exact: a stat value for the acting actor
 * (D_800C3E50, record D_800C3E34) picked by D_800C3DFC's mode byte +0x1B:
 * 0 = c + b, 1 = a, 2 = c.  For a +0x15A bit-7 actor c is D_800D2DC8's
 * halfword +0x70 and b = 0; otherwise c and b are D_800C3E34 bytes +0x2D
 * and +0x59, b times 3 / 2 when D_800C3DFC halfword +0xA bit 8 is clear and
 * D_800C3E34 halfword +0x84 | +0x86 has bit 8.  a is byte +0x5C, times 3 / 2
 * when +0xA bit 8 is set and +0x88 | +0x8A has bit 14.  For a non-bit-7
 * actor, with n = party actors 0..2 whose halfword +0x7C has bit 15 and
 * n != 0: D_800C3E34 halfword +0x32 bit 2 (with +0xA bit 8 clear) scales the
 * value by (4 - n) / 4, and bit 1 (likewise) and bit 0 each by (n + 2) / 2.
 * Returns it as s16.  The mode switch has no default: for a mode byte > 2
 * the value is never set (retail returns the caller's a3; undefined in a
 * native build).  Codegen: the first three bit-7 tests read Battle's
 * actors[] (with named pointer locals there gcc does not thread the second
 * test's set-bit branch straight to b = 0 as retail does); the last test's
 * base local also serves the party loop (base-first sums); a's 3 / 2 is
 * spelled `a += a * 2; a >>= 1` (retail adds a first, `a * 3` adds the
 * shifted copy first). */
s16 func_80097610(void) {
    u16 a;
    u16 b;
    u16 c;
    u16 v;
    u8 n;
    u8 i;

    if (((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 0x80) {
        c = *(u16*)(D_800D2DC8 + 0x70);
    } else {
        b = D_800C3E34[0x59];
        c = D_800C3E34[0x2D];
    }
    a = D_800C3E34[0x5C];
    if (*(u16*)((u8*)D_800C3DFC + 0xA) & 0x100) {
        if ((*(u16*)(D_800C3E34 + 0x88) | *(u16*)(D_800C3E34 + 0x8A)) & 0x4000) {
            a += a * 2;
            a >>= 1;
        }
    } else {
        if (!(((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 0x80)) {
            if ((*(u16*)(D_800C3E34 + 0x84) | *(u16*)(D_800C3E34 + 0x86)) & 0x100) {
                b = b * 3 >> 1;
            }
        }
    }
    if (((Battle*)D_800C34B0)->actors[D_800C3E50].flags & 0x80) {
        b = 0;
    }
    switch (((u8*)D_800C3DFC)[0x1B]) {
    case 0:
        v = c + b;
        break;
    case 1:
        v = a;
        break;
    case 2:
        v = c;
        break;
    }
    {
        u8* base4 = D_800C34B0;
        u8* p4 = base4 + D_800C3E50 * 0x170;

        if (!(p4[0x15A] & 0x80)) {
            n = 0;
            for (i = 0; i < 3; i++) {
                u8* q = base4 + i * 0x170;

                if (*(u16*)(q + 0x7C) & 0x8000) {
                    n++;
                }
            }
            if ((*(u16*)(D_800C3E34 + 0x32) & 4) && n != 0 && !(*(u16*)((u8*)D_800C3DFC + 0xA) & 0x100)) {
                v = v * (4 - n) / 4;
            }
            if ((*(u16*)(D_800C3E34 + 0x32) & 2) && n != 0 && !(*(u16*)((u8*)D_800C3DFC + 0xA) & 0x100)) {
                v = v * (n + 2) >> 1;
            }
            if ((*(u16*)(D_800C3E34 + 0x32) & 1) && n != 0) {
                v = v * (n + 2) >> 1;
            }
        }
    }
    return v;
}
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

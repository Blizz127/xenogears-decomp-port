/* Retail TU part 2 of main40.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
u32 func_80076A10(u32 sel, u8 *p, s32 m, s32 n);
u32 func_80089C6C(u32 mask, u8 index);
extern u8 D_800CCB34;
extern u16 D_800D2C30;
extern u8 * D_800D2DB4;
void func_80089038(void);
#endif

#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
extern u8 D_800C207C;
/* D_800C2054 is read as two rows of five words: func_8008887C(x0, y0, x1, y1)
 * gets a random target x from row 0 and a random target y from row 1.
 * Indexing row 1 of this 2-D view gives retail's base-first add for `pb`;
 * widening `r` where it is assigned and loading the row-0 base once into
 * `tbl` give retail's andi/la/sll order for `pa`. */
extern s32 D_800C2054__2d[2][5] asm("D_800C2054");
extern s32 D_800C2080;
extern s32 D_800C2084;
extern s32 D_800C3A7C;
extern s32 D_800C3A80;
extern s32 D_800CCB34__w asm("D_800CCB34");
extern u8 D_800CCB34__b[] asm("D_800CCB34");
extern u8 func_8001BD40(u8 min, u8 max);
extern void func_8008887C(u16 x, u16 y, s32 a2, s32 a3);
extern void func_80088990(void);
extern void func_80076B68(u8* p);
extern void func_80076BF0(u8* p);

void func_80088B80(void) {
    s32 i;
    s32 j;
    s32 k;
    s32* pa;
    s32* pb;

    if (D_800D2D28[0xAD] == 0) {
        return;
    }
    if (D_800C207C != 0) {
        if (func_8001BD40(0, 0x63) >= 0x60) {
            s32 r = func_8001BD40(0, 4);
            s32* tbl = D_800C2054__2d[0];

            pa = &tbl[r];
            pb = &D_800C2054__2d[1][func_8001BD40(0, 4)];
            func_8008887C(*(u16*)(D_800D2DB4 + 0x5D9C), *(u16*)(D_800D2DB4 + 0x5D9E), *pa, *pb);
        }
        if (D_800C207C != 0) {
            goto draw;
        }
    }
    func_80088990();
    ((u16*)D_800D2DB4)[0x5D9C / 2] = D_800C3A7C + D_800C2080 / 256;
    ((u16*)D_800D2DB4)[0x5D9E / 2] = D_800C3A80 + D_800C2084 / 256;
draw:
    D_800D2DB4[0x5D72] =
        func_80076A10(0xB9, D_800D2DB4 + 0xD20, *(s16*)(D_800D2DB4 + 0x5D9C), *(s16*)(D_800D2DB4 + 0x5D9E));
    D_800D2DB4[0x5D94] = D_800CCB34__b[0];
    D_800D2DB4[0x5D71] = func_80076A10((*(u16*)(D_800D2DB4 + 0x5D9C) & 0xF) + 0xA9, D_800D2DB4 + 0x3C0, 0xA0, 0x64);
    D_800D2DB4[0x5D93] = D_800CCB34__b[0];
    D_800D2DB4[0x5D73] =
        func_80076A10(0x82, D_800D2DB4 + 0xFA0, *(s16*)(D_800D2DB4 + 0x5D9C), *(s16*)(D_800D2DB4 + 0x5D9E));
    D_800D2DB4[0x5D95] = D_800CCB34__b[0];
    for (i = 0; i < D_800D2DB4[0x5D72]; i++) {
        func_80076B68(D_800D2DB4 + 0xD20 + (i * 2 + D_800D2DB4[0x5D94]) * 0x28);
    }
    for (i = 0; i < D_800D2DB4[0x5D73]; i++) {
        func_80076B68(D_800D2DB4 + 0xFA0 + (i * 2 + D_800D2DB4[0x5D95]) * 0x28);
    }
    for (i = 0; i < D_800D2DB4[0x5D71]; i++) {
        func_80076B68(D_800D2DB4 + 0x3C0 + (i * 2 + D_800D2DB4[0x5D93]) * 0x28);
    }
    D_800D2DB4[0x5D96] = func_80076A10((*(u16*)(D_800D2DB4 + 0x5D9E) & 0xF) + 0xC9, D_800D2DB4 + 0x32A0,
                                       *(s16*)(D_800D2DB4 + 0x5D9C), *(s16*)(D_800D2DB4 + 0x5D9E));
    D_800D2DB4[0x5D97] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D96]; i++) {
        func_80076BF0(D_800D2DB4 + 0x32A0 + (i * 2 + D_800D2DB4[0x5D97]) * 0x28);
    }
    if (D_800C207C == 0) {
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 10; j++) {
                k = i * 10 + j;
                func_80076A10(func_8001BD40(0, 9) + 0xBA, D_800D2DB4 + 0x46A0 + k * 0x50, (s16)(0x82 + j * 6),
                              (s16)(0xA + i * 0xBD));
                func_80076B68(D_800D2DB4 + 0x46A0 + (k * 2 + D_800CCB34__w) * 0x28);
            }
        }
        D_800D2DB4[0x5D98] = D_800CCB34;
    }
}

#ifndef XENO_PC_PORT
void func_80089038(void) {
    s32 temp_v0;
    s32 var_s0;
    s32 var_s1;

    var_s0 = 0;
    var_s1 = 0x6E0000;
    *(u8 *)((s8*)((*(void **)&D_800D2DB4)) + 0x5DA1) = 0U;
    do {
        if (func_80089C6C(D_800D2C30, var_s0 & 0xFF) & 0xFFFF) {
            temp_v0 = func_80076A10(var_s0 + 0xC4, (*(u8 *)((s8*)((*(void **)&D_800D2DB4)) + 0x5DA1) * 0x50) + 0x4CE0 + (*(void **)&D_800D2DB4), 0xE0, var_s1 >> 0x10);
            var_s1 += 0xA0000;
            *(u8 *)((s8*)((*(void **)&D_800D2DB4)) + 0x5DA1) = (u8) (*(u8 *)((s8*)((*(void **)&D_800D2DB4)) + 0x5DA1) + temp_v0);
        }
        var_s0 += 1;
    } while (var_s0 < 5);
    *(u8 *)((s8*)((*(void **)&D_800D2DB4)) + 0x5DA0) = (u8) D_800CCB34;
    *(s16 *)((s8*)((*(void **)&D_800D2DB4)) + 0x5DA2) = 0;
}
#endif

extern u8 D_800D2C38;

void func_80089110(void) {
    u8 pad[1];
    s32 i;
    s32 id = 0xA0;

    if (D_800D2C38 != 0) {
        id = 0xA1;
    }
    D_800D2DB4[0x5D75] = func_80076A10(id, D_800D2DB4 + 0x3AC0, 0xA0, 0x64);
    D_800D2DB4[0x5D84] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D75]; i++) {
        func_80076B68(D_800D2DB4 + 0x3AC0 + (i * 2 + D_800D2DB4[0x5D84]) * 0x28);
    }
}

extern u8 D_800D2C34;

/* Both selectors are u8 locals computed before the first call: retail
 * forms D_800D2C34 - 0x25 in s0 ahead of the first jal and masks it after. */
void func_800891E4(void) {
    s32 i;
    u8 a = D_800D2C34 - 0x5D;
    u8 b = D_800D2C34 - 0x25;

    D_800D2DB4[0x5D77] = func_80076A10(a, D_800D2DB4 + 0x3E80, 0xA0, 0x64);
    D_800D2DB4[0x5D86] = D_800CCB34;
    D_800D2DB4[0x5D7F] = func_80076A10(b, D_800D2DB4 + 0x43D0, 0xA0, 0x64);
    D_800D2DB4[0x5D8E] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D77]; i++) {
        func_80076B68(D_800D2DB4 + 0x3E80 + (i * 2 + D_800D2DB4[0x5D86]) * 0x28);
    }
    for (i = 0; i < D_800D2DB4[0x5D7F]; i++) {
        func_80076BF0(D_800D2DB4 + 0x43D0 + (i * 2 + D_800D2DB4[0x5D8E]) * 0x28);
    }
}

extern u16 D_800D2C2A;
extern void func_8008AAA0(u32 value);
extern u8 D_800C3CF8[];

/* Lay out the (up to five) D_800C3CF8 item names from y = 0x11A in steps of
 * 6 (kept as y << 16), counting sprites in +0x5D78, then register those sprites. */
void func_80089348(void) {
    u8 pad[1];
    s32 i;
    s32 y;

    func_8008AAA0(D_800D2C2A);
    for (i = 0, y = 0x11A << 16; i < 5; i++) {
        if (D_800C3CF8[i] != 0xFF) {
            D_800D2DB4[0x5D78] += func_80076A10(D_800C3CF8[i] + 0x92, D_800D2DB4 + 0x4E70 + D_800D2DB4[0x5D78] * 0x50, y >> 16, 0x46);
            y += 6 << 16;
        }
    }
    D_800D2DB4[0x5D87] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D78]; i++) {
        func_80076B68(D_800D2DB4 + 0x4E70 + (i * 2 + D_800D2DB4[0x5D87]) * 0x28);
    }
}

extern u16 D_800D2C3A;
extern u8 D_800D2C36;
extern u8 D_800C3CFA[];
extern u8 D_800C3CFB[];

/* D_800D2C34 < 4: the last two digits of D_800D2C3A (0xFF = suppressed zero)
 * from 0x11A in steps of 6 per drawn digit (kept << 16), then selector 0x9D
 * after them; otherwise selector 0xA2 alone.  Counted in +0x5D79, latched in
 * +0x5D88, then registered. */
void func_8008946C(void) {
    u8 pad[1];
    s32 i;
    s32 y;
    s32 drawn;

    if (D_800D2C34 < 4) {
        func_8008AAA0(D_800D2C3A);
        for (i = 0, drawn = 0, y = 0x11A << 16; i < 2; i++) {
            if (D_800C3CFB[i] != 0xFF) {
                D_800D2DB4[0x5D79] += func_80076A10(D_800C3CFB[i] + 0x92, D_800D2DB4 + 0x4FB0 + D_800D2DB4[0x5D79] * 0x50, y >> 16, 0x4E);
                drawn++;
                y += 6 << 16;
            }
        }
        D_800D2DB4[0x5D79] += func_80076A10(0x9D, D_800D2DB4 + 0x4FB0 + D_800D2DB4[0x5D79] * 0x50, (s16)(drawn * 6 + 0x11A), 0x4E);
    } else {
        D_800D2DB4[0x5D79] = func_80076A10(0xA2, D_800D2DB4 + 0x4FB0, 0x11A, 0x4E);
    }
    D_800D2DB4[0x5D88] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D79]; i++) {
        func_80076B68(D_800D2DB4 + 0x4FB0 + (i * 2 + D_800D2DB4[0x5D88]) * 0x28);
    }
}

/* The last three digits of D_800D2C36, then selector 0x9D, laid out as in
 * func_8008946C; counted in +0x5D7A, latched in +0x5D89, then registered. */
void func_8008963C(void) {
    u8 pad[1];
    s32 i;
    s32 y;
    s32 drawn;

    func_8008AAA0(D_800D2C36);
    for (i = 0, drawn = 0, y = 0x11A << 16; i < 3; i++) {
        if (D_800C3CFA[i] != 0xFF) {
            D_800D2DB4[0x5D7A] += func_80076A10(D_800C3CFA[i] + 0x92, D_800D2DB4 + 0x50A0 + D_800D2DB4[0x5D7A] * 0x50, y >> 16, 0x56);
            drawn++;
            y += 6 << 16;
        }
    }
    D_800D2DB4[0x5D7A] += func_80076A10(0x9D, D_800D2DB4 + 0x50A0 + D_800D2DB4[0x5D7A] * 0x50, (s16)(drawn * 6 + 0x11A), 0x56);
    D_800D2DB4[0x5D89] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D7A]; i++) {
        func_80076B68(D_800D2DB4 + 0x50A0 + (i * 2 + D_800D2DB4[0x5D89]) * 0x28);
    }
}

extern u8 D_800D2C35;
extern void func_8008AAA0(u32 value);

/* Lay out the (up to two) D_800C3CFB item names from y = 0x11A in steps of
 * 6 (kept as y << 16), counting sprites in +0x5D7B, then register those sprites. */
void func_800897CC(void) {
    u8 pad[1];
    s32 i;
    s32 y;

    func_8008AAA0(D_800D2C35);
    for (i = 0, y = 0x11A << 16; i < 2; i++) {
        if (D_800C3CFB[i] != 0xFF) {
            D_800D2DB4[0x5D7B] += func_80076A10(D_800C3CFB[i] + 0x92, D_800D2DB4 + 0x51E0 + D_800D2DB4[0x5D7B] * 0x50, y >> 16, 0x5E);
            y += 6 << 16;
        }
    }
    D_800D2DB4[0x5D8A] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D7B]; i++) {
        func_80076B68(D_800D2DB4 + 0x51E0 + (i * 2 + D_800D2DB4[0x5D8A]) * 0x28);
    }
}

extern u8 D_800CCDC4[];
extern u8 D_800CCDC6[];
extern u8 D_800C3CF9[];

/* Two four-digit readouts of the 0x170-byte record `index` (halfwords at +0
 * and +2 of D_800CCDC4): the first column steps x by 8 for every digit
 * position, the second only for drawn digits and ends with selector 0x9C at
 * 0x41.  Counts +0x5D7C/+0x5D7D, latches +0x5D8B/+0x5D8C; nothing registered. */
void func_800898F0(u8 index) {
    s32 i;
    s32 x;

    D_800D2DB4[0x5D7C] = 0;
    func_8008AAA0(*(u16*)(D_800CCDC4 + index * 0x170));
    for (i = 0, x = 0x20 << 16; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            D_800D2DB4[0x5D7C] += func_80076A10(D_800C3CF9[i] + 0x92, D_800D2DB4 + 0x5280 + D_800D2DB4[0x5D7C] * 0x50, x >> 16, 0xCC);
        }
        x += 8 << 16;
    }
    D_800D2DB4[0x5D8B] = D_800CCB34;
    D_800D2DB4[0x5D7D] = 0;
    func_8008AAA0(*(u16*)(D_800CCDC6 + index * 0x170));
    for (i = 0, x = 0x48 << 16; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            D_800D2DB4[0x5D7D] += func_80076A10(D_800C3CF9[i] + 0x92, D_800D2DB4 + 0x53C0 + D_800D2DB4[0x5D7D] * 0x50, x >> 16, 0xCC);
            x += 8 << 16;
        }
    }
    D_800D2DB4[0x5D7D] += func_80076A10(0x9C, D_800D2DB4 + 0x53C0 + D_800D2DB4[0x5D7D] * 0x50, 0x41, 0xCC);
    D_800D2DB4[0x5D8C] = D_800CCB34;
}
#endif
#include "main40.c"

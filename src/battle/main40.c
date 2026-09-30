#include "common.h"
/* Retail TU split: gcc 2.7.2 cc1 emits every file-scope INCLUDE_ASM block
 * before the first compiled body, so a mixed TU whose retail layout has a C
 * function ahead of an assembly function cannot be one object.  Each
 * contiguous [asm run][C run] of the retail layout is therefore its own
 * matching TU: src/battle/main40_p<N>.c defines BATTLE_TU_PART=<N> and
 * includes this file.  Host/port builds compile this file whole (part 0). */
#ifndef BATTLE_TU_PART
#define BATTLE_TU_PART 0
#endif
#define BATTLE_PART(n) (BATTLE_TU_PART == 0 || BATTLE_TU_PART == (n))


#ifndef XENO_PC_PORT
extern u8 D_800C33B0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCB34;
#endif
#ifndef XENO_PC_PORT
extern u8 *D_800D2DB4;
#endif
/* Retail passes the glyph selector in $a0 without masking it to a byte. */
extern u32 func_80076A10(u32 sel, u8 *p, s32 m, s32 n);
extern void func_80076B68(u8 *p);
extern void func_80076BF0(u8 *p);
/* The real definition is `u8 func_8001BD40(u8 min, u8 max)` in
 * src/slus_006.64/system/temp3.c -- a rand()-based pick in [min, max]. The
 * old four-u32 prototype here was wrong: retail sets only $a0=1 and $a1=8
 * before the jal, and $a2/$a3 still hold this function's own x1/y1, which the
 * two-parameter callee simply ignores. Calling through a mismatched prototype
 * is undefined behaviour, so declare it as it is actually defined. */
extern u8 func_8001BD40(u8 min, u8 max);
#ifndef XENO_PC_PORT
extern u8 D_800C3A94;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3A98;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C207C;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A7C;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A80;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A84;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A88;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A8C;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A90;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3A9C;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C2080;
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C2084;
#endif


#if BATTLE_PART(1)
/* func_8008860C.s */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8008860C(void) {
    s32 i;
    s32 j;
    u8 *base;
    for (i = 0; i < 2; i++) {
        u8 v0;
        u8 r;
        base = D_800D2DB4;
        v0 = base[0x5D74];
        r = (u8)func_80076A10(D_800C33B0[i], base + (u32)v0 * 80u + 0x1720u, 0xA0u, 0x64u);
        base = D_800D2DB4;
        base[0x5D74] = (u8)(base[0x5D74] + r);
    }
    base = D_800D2DB4;
    base[0x5D83] = D_800CCB34;
    {
        u8 v0 = base[0x5D70];
        u8 r = (u8)func_80076A10(0xA8u, base + (u32)v0 * 80u, 0xA0u, 0x64u);
        base = D_800D2DB4;
        base[0x5D70] = r;
        base[0x5D92] = D_800CCB34;
    }
    base = D_800D2DB4;
    if (base[0x5D74] > 0) {
        j = 0;
        do {
            u8 v1 = base[0x5D83];
            func_80076B68(base + ((u32)j * 2u + (u32)v1) * 40u + 0x1720u);
            base = D_800D2DB4;
            j++;
        } while (j < base[0x5D74]);
    }
    base = D_800D2DB4;
    if (base[0x5D70] > 0) {
        j = 0;
        do {
            u8 v1 = base[0x5D92];
            func_80076B68(base + ((u32)j * 2u + (u32)v1) * 40u + 0x1720u);
            base = D_800D2DB4;
            j++;
        } while (j < base[0x5D70]);
    }
    for (i = 2; i < 4; i++) {
        u8 v0;
        u8 r;
        base = D_800D2DB4;
        v0 = base[0x5D7E];
        r = (u8)func_80076A10(D_800C33B0[i], base + (u32)v0 * 80u + 0x2530u, 0xA0u, 0x64u);
        base = D_800D2DB4;
        base[0x5D7E] = (u8)(base[0x5D7E] + r);
    }
    base = D_800D2DB4;
    base[0x5D8D] = D_800CCB34;
    base = D_800D2DB4;
    if (base[0x5D7E] > 0) {
        j = 0;
        do {
            u8 v1 = base[0x5D8D];
            func_80076BF0(base + ((u32)j * 2u + (u32)v1) * 40u + 0x2530u);
            base = D_800D2DB4;
            j++;
        } while (j < base[0x5D7E]);
    }
}
#endif /* XENO_PC_PORT */

/* func_80088990.s: retail evaluates the rounding shift separately inside each
 * direction arm rather than hoisting it above the flag test. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_80088990(void) {
    s32 n = D_800C3A9C;
    if (n > 0) {
        u8 dirx = D_800C3A94;
        s32 stepx = D_800C3A8C;
        u8 diry = D_800C3A98;
        s32 stepy = D_800C3A90;
        s32 i = 0;
        do {
            if (dirx != 0)
                D_800C2080 = (s32)((u32)D_800C2080 - (u32)stepx);
            else
                D_800C2080 = (s32)((u32)D_800C2080 + (u32)stepx);
            if (diry != 0)
                D_800C2084 = (s32)((u32)D_800C2084 - (u32)stepy);
            else
                D_800C2084 = (s32)((u32)D_800C2084 + (u32)stepy);
            i++;
        } while (i < n);
    }
    if (D_800C3A8C == 0x100) {
        if (D_800C3A94 != 0) {
            s32 v = D_800C2080;

            if (v < 0)
                v += 0xFF;
            v >>= 8;
            if ((s32)((u32)v + (u32)D_800C3A7C) < D_800C3A84)
                D_800C207C = 1;
        } else {
            s32 v = D_800C2080;

            if (v < 0)
                v += 0xFF;
            v >>= 8;
            if (D_800C3A84 < (s32)((u32)v + (u32)D_800C3A7C))
                D_800C207C = 1;
        }
    } else {
        if (D_800C3A98 != 0) {
            s32 v = D_800C2084;

            if (v < 0)
                v += 0xFF;
            v >>= 8;
            if ((s32)((u32)v + (u32)D_800C3A80) < D_800C3A88)
                D_800C207C = 1;
        } else {
            s32 v = D_800C2084;

            if (v < 0)
                v += 0xFF;
            v >>= 8;
            if (D_800C3A88 < (s32)((u32)v + (u32)D_800C3A80))
                D_800C207C = 1;
        }
    }
}
#endif /* XENO_PC_PORT */

#endif /* BATTLE_PART(1) */
#if BATTLE_PART(2)
#ifndef XENO_PC_PORT
extern s32 D_800C2054[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C2A;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C30;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C34;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C35;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C36;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C38;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C3A;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3CF4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDC4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCDC6[];
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
extern void func_8008AAA0(u32 value);
extern u32 func_80089C6C(u32 mask, u8 index);

/* The functions below are still assembled from retail bytes by the matching
 * build (main40_p2.c); these bodies are for the port and the differential
 * harnesses only.  Every one draws through the same sprite-list helpers:
 * func_80076A10 builds `count` sprites at D_800D2DB4 + base + slot * 80 and
 * returns how many it wrote, then each sprite of the current buffer half
 * (D_800CCB34) is finished by func_80076B68 / func_80076BF0. */
#ifdef XENO_PC_PORT
/* Byte counter at `count_off`, buffer latch at `latch_off`: finish every sprite
 * written into the list at `base`. */
static void func_80088B80_finish(u32 base, u32 count_off, u32 latch_off,
                                 void (*finish)(u8 *))
{
    u8 *d = D_800D2DB4;
    s32 j;

    if (d[count_off] > 0) {
        j = 0;
        do {
            u32 v1 = d[latch_off];
            finish(d + ((u32)j * 2u + v1) * 40u + base);
            d = D_800D2DB4;
            j++;
        } while (j < d[count_off]);
    }
}

/* func_80088B80.s */
void func_80088B80(void) {
    u8 *d;
    s32 i;
    s32 k;

    if (D_800D2D28[0xAD] == 0)
        return;
    /* D_800C207C set: the previous line has finished; one time in 25 start
     * a new one between two random points of the D_800C2054 table. */
    if (D_800C207C != 0 && func_8001BD40(0, 0x63) >= 0x60) {
        s32 *a = &D_800C2054[func_8001BD40(0, 4)];
        s32 *b = &D_800C2054[func_8001BD40(0, 4)];

        d = D_800D2DB4;
        func_8008887C(*(u16 *)(d + 0x5D9C), *(u16 *)(d + 0x5D9E), a[0], b[5]);
    }
    if (D_800C207C == 0) {
        s32 v;

        func_80088990();
        d = D_800D2DB4;
        v = D_800C2080;
        if (v < 0)
            v += 0xFF;
        *(u16 *)(d + 0x5D9C) = (u16)((u16)D_800C3A7C + (v >> 8));
        v = D_800C2084;
        if (v < 0)
            v += 0xFF;
        *(u16 *)(d + 0x5D9E) = (u16)((u16)D_800C3A80 + (v >> 8));
    }

    d = D_800D2DB4;
    d[0x5D72] = (u8)func_80076A10(0xB9, d + 0xD20, *(s16 *)(d + 0x5D9C),
                                  *(s16 *)(d + 0x5D9E));
    D_800D2DB4[0x5D94] = D_800CCB34;
    d = D_800D2DB4;
    d[0x5D71] = (u8)func_80076A10((*(u16 *)(d + 0x5D9C) & 0xF) + 0xA9,
                                  d + 0x3C0, 0xA0, 0x64);
    D_800D2DB4[0x5D93] = D_800CCB34;
    d = D_800D2DB4;
    d[0x5D73] = (u8)func_80076A10(0x82, d + 0xFA0, *(s16 *)(d + 0x5D9C),
                                  *(s16 *)(d + 0x5D9E));
    D_800D2DB4[0x5D95] = D_800CCB34;
    func_80088B80_finish(0xD20, 0x5D72, 0x5D94, func_80076B68);
    func_80088B80_finish(0xFA0, 0x5D73, 0x5D95, func_80076B68);
    func_80088B80_finish(0x3C0, 0x5D71, 0x5D93, func_80076B68);

    d = D_800D2DB4;
    {
        u16 y = *(u16 *)(d + 0x5D9E);

        d[0x5D96] = (u8)func_80076A10((y & 0xF) + 0xC9, d + 0x32A0,
                                      *(s16 *)(d + 0x5D9C), (s16)y);
    }
    D_800D2DB4[0x5D97] = D_800CCB34;
    func_80088B80_finish(0x32A0, 0x5D96, 0x5D97, func_80076BF0);

    if (D_800C207C != 0)
        return;
    for (i = 0; i < 2; i++) {
        for (k = 0; k < 10; k++) {
            u32 slot = (u32)(i * 10 + k);
            u8 r = func_8001BD40(0, 9);

            func_80076A10((u32)r + 0xBA, D_800D2DB4 + slot * 80u + 0x46A0,
                          0x82 + k * 6, 0xA + i * 0xBD);
            /* Retail reads the buffer latch as a whole word here. */
            func_80076B68(D_800D2DB4 +
                          (slot * 2u + *(u32 *)&D_800CCB34) * 40u + 0x46A0);
        }
    }
    D_800D2DB4[0x5D98] = D_800CCB34;
}

/* func_80089038.s */
void func_80089038(void) {
    s32 i;
    s32 y = 0x6E;

    D_800D2DB4[0x5DA1] = 0;
    for (i = 0; i < 5; i++) {
        if ((u16)func_80089C6C(D_800D2C30, (u8)i) != 0) {
            u8 *d = D_800D2DB4;
            u32 r = func_80076A10((u32)i + 0xC4,
                                  d + (u32)d[0x5DA1] * 80u + 0x4CE0, 0xE0, y);

            d = D_800D2DB4;
            y += 0xA;
            d[0x5DA1] = (u8)(d[0x5DA1] + r);
        }
    }
    D_800D2DB4[0x5DA0] = D_800CCB34;
    *(u16 *)(D_800D2DB4 + 0x5DA2) = 0;
}

/* func_80089110.s */
void func_80089110(void) {
    u32 sel = (D_800D2C38 != 0) ? 0xA1 : 0xA0;

    D_800D2DB4[0x5D75] = (u8)func_80076A10(sel, D_800D2DB4 + 0x3AC0, 0xA0, 0x64);
    D_800D2DB4[0x5D84] = D_800CCB34;
    func_80088B80_finish(0x3AC0, 0x5D75, 0x5D84, func_80076B68);
}

/* func_800891E4.s */
void func_800891E4(void) {
    u8 v = D_800D2C34;

    D_800D2DB4[0x5D77] = (u8)func_80076A10((u8)(v - 0x5D), D_800D2DB4 + 0x3E80,
                                           0xA0, 0x64);
    D_800D2DB4[0x5D86] = D_800CCB34;
    D_800D2DB4[0x5D7F] = (u8)func_80076A10((u8)(v - 0x25), D_800D2DB4 + 0x43D0,
                                           0xA0, 0x64);
    D_800D2DB4[0x5D8E] = D_800CCB34;
    func_80088B80_finish(0x3E80, 0x5D77, 0x5D86, func_80076B68);
    func_80088B80_finish(0x43D0, 0x5D7F, 0x5D8E, func_80076BF0);
}

/* Digits of the last func_8008AAA0 conversion, starting at digit `first`
 * (0xFF = suppressed leading zero), drawn left to right from x = 0x11A.
 * Returns how many digits were drawn. */
static s32 func_80089348_digits(u32 first, u32 digits, u32 base,
                                u32 count_off, s32 y)
{
    s32 i = 0;
    s32 drawn = 0;
    s32 x = 0x11A;

    do {
        u32 c = D_800C3CF4[first + (u32)i];

        i++;
        if (c != 0xFF) {
            u8 *d = D_800D2DB4;
            u32 r = func_80076A10(c + 0x92,
                                  d + (u32)d[count_off] * 80u + base, x, y);

            drawn++;
            d = D_800D2DB4;
            x += 6;
            d[count_off] = (u8)(d[count_off] + r);
        }
    } while (i < (s32)digits);
    return drawn;
}

/* func_80089348.s */
void func_80089348(void) {
    func_8008AAA0(D_800D2C2A);
    func_80089348_digits(4, 5, 0x4E70, 0x5D78, 0x46);
    D_800D2DB4[0x5D87] = D_800CCB34;
    func_80088B80_finish(0x4E70, 0x5D78, 0x5D87, func_80076B68);
}

/* Draw a trailing glyph (0x9D) just right of the digits func_80089348_digits
 * drew. */
static void func_8008946C_suffix(s32 drawn, u32 base, u32 count_off, s32 y)
{
    u8 *d = D_800D2DB4;
    u32 r = func_80076A10(0x9D, d + (u32)d[count_off] * 80u + base,
                          (s16)(drawn * 6 + 0x11A), y);

    d = D_800D2DB4;
    d[count_off] = (u8)(d[count_off] + r);
}

/* func_8008946C.s */
void func_8008946C(void) {
    if (D_800D2C34 < 4) {
        s32 drawn;

        func_8008AAA0(D_800D2C3A);
        drawn = func_80089348_digits(7, 2, 0x4FB0, 0x5D79, 0x4E);
        func_8008946C_suffix(drawn, 0x4FB0, 0x5D79, 0x4E);
    } else {
        D_800D2DB4[0x5D79] = (u8)func_80076A10(0xA2, D_800D2DB4 + 0x4FB0,
                                               0x11A, 0x4E);
    }
    D_800D2DB4[0x5D88] = D_800CCB34;
    func_80088B80_finish(0x4FB0, 0x5D79, 0x5D88, func_80076B68);
}

/* func_8008963C.s */
void func_8008963C(void) {
    s32 drawn;

    func_8008AAA0(D_800D2C36);
    drawn = func_80089348_digits(6, 3, 0x50A0, 0x5D7A, 0x56);
    func_8008946C_suffix(drawn, 0x50A0, 0x5D7A, 0x56);
    D_800D2DB4[0x5D89] = D_800CCB34;
    func_80088B80_finish(0x50A0, 0x5D7A, 0x5D89, func_80076B68);
}

/* func_800897CC.s */
void func_800897CC(void) {
    func_8008AAA0(D_800D2C35);
    func_80089348_digits(7, 2, 0x51E0, 0x5D7B, 0x5E);
    D_800D2DB4[0x5D8A] = D_800CCB34;
    func_80088B80_finish(0x51E0, 0x5D7B, 0x5D8A, func_80076B68);
}

/* func_800898F0.s: two four-digit readouts of the 0x170-byte record `index`
 * (halfwords at +0 and +2 of D_800CCDC4).  The first column advances x for
 * every digit position, the second only for drawn digits, and is followed by
 * a separator glyph at x = 0x41. */
void func_800898F0(u8 index) {
    u32 rec = (u32)index * 0x170u;
    s32 i;
    s32 x;

    D_800D2DB4[0x5D7C] = 0;
    func_8008AAA0(*(u16 *)(D_800CCDC4 + rec));
    x = 0x20;
    for (i = 0; i < 4; i++) {
        u32 c = D_800C3CF4[5 + i];

        if (c != 0xFF) {
            u8 *d = D_800D2DB4;
            u32 r = func_80076A10(c + 0x92, d + (u32)d[0x5D7C] * 80u + 0x5280,
                                  x, 0xCC);

            d = D_800D2DB4;
            d[0x5D7C] = (u8)(d[0x5D7C] + r);
        }
        x += 8;
    }
    D_800D2DB4[0x5D8B] = D_800CCB34;
    D_800D2DB4[0x5D7D] = 0;
    func_8008AAA0(*(u16 *)(D_800CCDC6 + rec));
    x = 0x48;
    for (i = 0; i < 4; i++) {
        u32 c = D_800C3CF4[5 + i];

        if (c != 0xFF) {
            u8 *d = D_800D2DB4;
            u32 r = func_80076A10(c + 0x92, d + (u32)d[0x5D7D] * 80u + 0x53C0,
                                  x, 0xCC);

            d = D_800D2DB4;
            x += 8;
            d[0x5D7D] = (u8)(d[0x5D7D] + r);
        }
    }
    {
        u8 *d = D_800D2DB4;
        u32 r = func_80076A10(0x9C, d + (u32)d[0x5D7D] * 80u + 0x53C0, 0x41,
                              0xCC);

        d = D_800D2DB4;
        d[0x5D7D] = (u8)(d[0x5D7D] + r);
    }
    D_800D2DB4[0x5D8C] = D_800CCB34;
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(2) */


extern void func_8008860C(void);
extern void func_80089038(void);
extern void func_80089110(void);
extern void func_800891E4(void);
extern void func_80089348(void);
extern void func_8008946C(void);
extern void func_8008963C(void);
extern void func_800897CC(void);


#if BATTLE_PART(2)
/* func_80089AF8.s */
void func_80089AF8(void) {
    func_8008860C();
    func_80089038();
    func_80089110();
    func_800891E4();
    func_80089348();
    func_8008946C();
    func_8008963C();
    func_800897CC();
}
#endif /* BATTLE_PART(2) */

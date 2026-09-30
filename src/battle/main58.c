#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009B104);
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009B1E4);
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009B46C);
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009B684);
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009BAC4);
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009BD94);
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009BE0C);
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCCE8[];
#endif
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_8009C050.s: status flags for actor idx (368-byte stride over
 * D_800CCCE8): bit0 = +0x120 & 0x400, bit1 = +0x104 < (+0x108 >> 3) with
 * +0x36 bit0 clear, bit2 = +0x148 == 4. */
u8 func_8009C050(u8 idx) {
    u32 off = idx * 368;
    u8* base = D_800CCCE8;
    u8* a = off + (base + 0xA4);
    u8* b = off + base;
    u8* c = off + (base + 0x148);
    u32 t = (*(u16*)(a + 0x7C) & 0x400) != 0;
    u32 r = t;

    if (*(u32*)(a + 0x60) < (*(u32*)(a + 0x64) >> 3) && !(*(u16*)(b + 0x36) & 1)) {
        r = t | 2;
    }
    if (c[0] == 4) {
        r |= 4;
    }
    return r;
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/main58", func_8009C050);
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE30[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE4A[];
#endif
extern u8 g_GameState[];


/* func_8009C0E0.s */
void func_8009C0E0(u8 index) {
    u32 off = (u32)index * 368;
    u16* p = (u16*)(g_GameState + 0x16DA);

    D_800CCE30[off] = 4;
    D_800CCE4A[off] = 3;
    *p |= 0x4000;
}


/* ---- Port bodies (fo/bat3).  Not byte-matching: the matching build keeps
 * the retail bytes from the INCLUDE_ASM block above. ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3E00;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3DFC;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E50;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE42[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
#endif
#ifdef XENO_PC_PORT
extern int rand(void);

/* func_8009B104.s: seed actor idx's HP words (+0x104 current, +0x108 max)
 * from the record at a1 (+0x4C/+0x4E halfwords, x50), clamping to 99999 when
 * the max overflows it. */
void func_8009B104(u32 idx, u8* a1) {
    u8* a0 = D_800C34B0 + (idx & 0xFF) * 0x170;
    s32 cap = 0x1869F;
    s32 max = *(u16*)(a1 + 0x4E) * 50;

    if (cap < max) {
        s32 cur = *(u16*)(a1 + 0x4C) * 50;

        *(u32*)(a0 + 0x104) = (cap < cur) ? cap : cur;
        *(u32*)(a0 + 0x108) = 0x1869F;
    } else {
        *(u32*)(a0 + 0x104) = *(u16*)(a1 + 0x4C) * 50;
        *(u32*)(a0 + 0x108) = *(u16*)(a1 + 0x4E) * 50;
    }
}

/* func_8009B1E4.s: reset the g_GameState battle tables: ids 1..47 into
 * +0x2026[i] with 0x63 at +0x1F90[i], then the fixed per-character mask
 * words (retail store order). */
void func_8009B1E4(void) {
    u32 i;

    for (i = 1; i < 0x30; i++) {
        g_GameState[0x2026 + i] = i;
        g_GameState[0x1F90 + i] = 0x63;
    }
    *(u16*)(g_GameState + 0x16C0) = 0xFFF8;
    *(u16*)(g_GameState + 0x16C6) = 0xFE00;
    *(u16*)(g_GameState + 0x16E2) = 0xFFF0;
    *(u16*)(g_GameState + 0x16E6) = 0xFFF0;
    *(u16*)(g_GameState + 0x1746) = 0xFC00;
    *(u16*)(g_GameState + 0x16C2) = 0xFF00;
    *(u16*)(g_GameState + 0x16C4) = 0xFFFF;
    *(u16*)(g_GameState + 0x16DA) = 0xF000;
    *(u8*)(g_GameState + 0x16D7) = 0x7;
    *(u16*)(g_GameState + 0x16E0) = 0xFFE0;
    *(u16*)(g_GameState + 0x16E4) = 0xFFFF;
    *(u16*)(g_GameState + 0x16FA) = 0xC000;
    *(u8*)(g_GameState + 0x16F7) = 0x7;
    *(u16*)(g_GameState + 0x1700) = 0xFFE0;
    *(u16*)(g_GameState + 0x1702) = 0xFFE0;
    *(u16*)(g_GameState + 0x1704) = 0xFFFF;
    *(u16*)(g_GameState + 0x1706) = 0xFF00;
    *(u16*)(g_GameState + 0x171A) = 0x8000;
    *(u8*)(g_GameState + 0x1717) = 0x7;
    *(u16*)(g_GameState + 0x1720) = 0xFFE0;
    *(u16*)(g_GameState + 0x1722) = 0xFFC0;
    *(u16*)(g_GameState + 0x1724) = 0xFFFF;
    *(u16*)(g_GameState + 0x1726) = 0xFF00;
    *(u16*)(g_GameState + 0x173A) = 0xF000;
    *(u8*)(g_GameState + 0x1737) = 0x7;
    *(u16*)(g_GameState + 0x1740) = 0xFFC0;
    *(u16*)(g_GameState + 0x1742) = 0xFFC0;
    *(u16*)(g_GameState + 0x1744) = 0xFFFF;
    *(u16*)(g_GameState + 0x175A) = 0xE000;
    *(u8*)(g_GameState + 0x1757) = 0x7;
    *(u16*)(g_GameState + 0x1760) = 0xFFC0;
    *(u16*)(g_GameState + 0x1762) = 0xF000;
    *(u16*)(g_GameState + 0x1764) = 0xFFFF;
    *(u16*)(g_GameState + 0x1766) = 0xF000;
    *(u16*)(g_GameState + 0x177A) = 0x8000;
    *(u8*)(g_GameState + 0x1777) = 0x7;
    *(u16*)(g_GameState + 0x1780) = 0xFFC0;
    *(u16*)(g_GameState + 0x1782) = 0xFF00;
    *(u16*)(g_GameState + 0x1784) = 0xFFFF;
    *(u16*)(g_GameState + 0x1786) = 0xFF00;
    *(u16*)(g_GameState + 0x179A) = 0x8000;
    *(u8*)(g_GameState + 0x1797) = 0x7;
    *(u16*)(g_GameState + 0x17A0) = 0x0;
    *(u16*)(g_GameState + 0x17A2) = 0xFF00;
    *(u16*)(g_GameState + 0x17A4) = 0x0;
    *(u16*)(g_GameState + 0x17A6) = 0xFF00;
    *(u16*)(g_GameState + 0x17BA) = 0x0;
    *(u8*)(g_GameState + 0x17B7) = 0x7;
    *(u16*)(g_GameState + 0x17C0) = 0x0;
    *(u16*)(g_GameState + 0x17C2) = 0xF800;
    *(u16*)(g_GameState + 0x17C4) = 0xFFFF;
    *(u16*)(g_GameState + 0x17C6) = 0x0;
    *(u16*)(g_GameState + 0x17DA) = 0xE000;
    *(u8*)(g_GameState + 0x17D7) = 0x7;
    *(u16*)(g_GameState + 0x17E0) = 0xFFE0;
    *(u16*)(g_GameState + 0x17E2) = 0xFFE0;
    *(u16*)(g_GameState + 0x17E4) = 0xFFFF;
    *(u16*)(g_GameState + 0x17E6) = 0xFFE0;
    *(u16*)(g_GameState + 0x17FA) = 0x8000;
    *(u8*)(g_GameState + 0x17F7) = 0x7;
    *(u16*)(g_GameState + 0x1800) = 0xFFC0;
    *(u16*)(g_GameState + 0x1802) = 0xFF00;
    *(u16*)(g_GameState + 0x1804) = 0xFFFF;
    *(u16*)(g_GameState + 0x1806) = 0xFF00;
    *(u16*)(g_GameState + 0x181A) = 0xC000;
    *(u8*)(g_GameState + 0x1817) = 0x7;
}

/* HP-fraction score for one actor: +1 below half, +1 more below a quarter.
 * A +0x162 bit7 actor uses the 32-bit words at +0x104/+0x108. */
static u32 bat3_low_hp_score(u8* a2, u32 off) {
    u32 cur;
    u32 max;
    u32 n = 0;

    if (D_800CCE42[off] & 0x80) {
        max = *(u32*)(a2 + 0x108);
        cur = *(u32*)(a2 + 0x104);
    } else {
        max = *(u16*)(a2 + 0x4E);
        cur = *(u16*)(a2 + 0x4C);
    }
    if (cur < (max >> 1)) n++;
    if (cur < (max >> 2)) n++;
    return n;
}

/* func_8009B46C.s: scale the halfword at s2 up by the low-HP score of the
 * type-0 and type-3 actors among the first three, then on a rand() roll
 * (10%, or 60% with D_800C3E00+0x32 bit9) multiply it by 3/2 (4/2 with bit10). */
void func_8009B46C(u16* s2) {
    u32 n = 0;
    u32 i;
    u32 f;
    s32 mul;
    s32 pct;

    for (i = 0; i < 3; i++) {
        u32 off = i * 0x170;
        u8* a2 = D_800C34B0 + off;

        if (a2[0x56] == 0) {
            n += bat3_low_hp_score(a2, off);
        }
        if (a2[0x56] == 3) {
            n += bat3_low_hp_score(a2, off);
        }
    }
    if ((n & 0xFF) != 0) {
        *s2 = *s2 + (n & 0xFF) * (*s2 >> 1);
    }
    f = *(u16*)(D_800C3E00 + 0x32);
    mul = (f & 0x400) ? 4 : 3;
    pct = (f & 0x200) ? 0x3C : 0xA;
    if ((s32)(rand() % 100) < pct) {
        *s2 = (mul * *s2) >> 1;
    }
}

/* func_8009B684.s: map (menu a0, bit a1) to the D_800C34B0+0x5FC7 command
 * code (jtbl_80070478 over menus 0/2/5/7/9); unknown pairs store nothing. */
void func_8009B684(u8 a0, u16 a1) {
    u32 code = 0;

    switch (a0) {
    case 0:
        switch (a1) {
        case 0x2000: code = 0x01; break;
        case 0x1000: code = 0x02; break;
        case 0x0800: code = 0x03; break;
        case 0x0400: code = 0x04; break;
        case 0x0200: code = 0x05; break;
        case 0x0001: code = 0x08; break;
        }
        break;
    case 2:
        switch (a1) {
        case 0x2000: code = 0x09; break;
        case 0x1000: code = 0x0A; break;
        case 0x0800: code = 0x0B; break;
        case 0x0400: code = 0x0C; break;
        case 0x0001: code = 0x0D; break;
        case 0x0020: code = 0x07; break;
        }
        break;
    case 5:
        switch (a1) {
        case 0x8000: code = 0x0E; break;
        case 0x4000: code = 0x0F; break;
        case 0x2000: code = 0x10; break;
        case 0x1000: code = 0x11; break;
        case 0x0800: code = 0x12; break;
        case 0x1800: code = 0x13; break;
        }
        break;
    case 7:
        switch (a1) {
        case 0x8000: code = 0x15; break;
        case 0x4000: code = 0x16; break;
        case 0x1000: code = 0x18; break;
        case 0x0008:
        case 0x0002: code = 0x19; break;
        case 0x0001:
        case 0x0004: code = 0x1A; break;
        }
        break;
    case 9:
        switch (a1) {
        case 0x8000: code = 0x1B; break;
        case 0x4000: code = 0x1C; break;
        case 0x2000: code = 0x1D; break;
        case 0x1000: code = 0x1E; break;
        case 0x0400: code = 0x1F; break;
        case 0x0800: code = 0x20; break;
        case 0x0100: code = 0x21; break;
        case 0x0200: code = 0x22; break;
        }
        break;
    }
    if (code != 0) {
        D_800C34B0[0x5FC7] = code;
    }
}

/* func_8009BAC4.s: pick an enemy action for actor idx into out[0] (kind) and
 * out[1] (argument).  Kinds: 4 (10% when +0x7A bit8 clear and *a2 == 0), 2
 * with a random slot allowed by the type's mask word at
 * g_GameState+0x16C2[type*32] (25% when +0x7A bit5 clear), 0/0 for type 8,
 * else 1 with 0/1/2 weighted 48/32/20.  Retail leaves $s0/$s1 unset for a
 * type >= 11; the host uses 0 there, and a zero modulus follows the R3000
 * rule (remainder = dividend). */
void func_8009BAC4(u8 idx, u8* out, s16* a2) {
    u32 off = idx * 0x170;
    u8* s3 = D_800CCCE8 + off;
    u32 s0 = 0;
    u32 s1 = 0;

    if (!(D_800CCE42[off] & 0x80)) {
        if ((s32)(rand() % 100) < 0xA && !(*(u16*)(s3 + 0x7A) & 0x100) && *a2 == 0) {
            out[0] = 4;
            return;
        }
        switch (s3[0x56]) {
        case 0: case 8: s1 = 0xC000; s0 = 2; break;
        case 1: case 6: case 10: s1 = 0xF000; s0 = 4; break;
        case 2: case 9: s1 = 0xBFE0; s0 = 0xB; break;
        case 3: s1 = 0xC3C0; s0 = 0xA; break;
        case 4: s1 = 0xDF80; s0 = 0xA; break;
        case 5: s1 = 0x1000; s0 = 4; break;
        case 7: s1 = 0xE000; s0 = 3; break;
        }
        if ((s32)(rand() % 100) < 0x19 && !(*(u16*)(s3 + 0x7A) & 0x20)) {
            s32 v;
            s32 d = s0 & 0xFF;
            s32 r;

            out[0] = 2;
            v = rand();
            r = (d == 0) ? v : v % d;
            if (*(u16*)(g_GameState + 0x16C2 + s3[0x56] * 32) &
                ((0x8000 >> (r & 31)) & (s1 & 0xFFFF))) {
                out[1] = r;
                return;
            }
        }
        if (s3[0x56] == 8) {
            out[0] = 0;
            out[1] = 0;
            return;
        }
    }
    out[0] = 1;
    if ((s32)(rand() % 100) < 0x50) {
        if ((s32)(rand() % 100) < 0x3C) {
            out[1] = 0;
        } else {
            out[1] = 1;
        }
    } else {
        out[1] = 2;
    }
}

/* func_8009BD94.s: mark the current actor (D_800C3E50) with state 2 and
 * store D_800C3DFC+0x11 x D_800D2DC8+0x64 / 20 as its +0x5F6C word. */
void func_8009BD94(void) {
    u32 prod;

    D_800C34B0[D_800C3E50 + 0x5FA0] = 2;
    prod = D_800C3DFC[0x11] * *(u32*)(D_800D2DC8 + 0x64);
    *(u32*)(D_800C34B0 + D_800C3E50 * 4 + 0x5F6C) = prod / 20;
}

/* func_8009BE0C.s: copy the live battle stats of the first three actors back
 * into their g_GameState character records (0xA4 stride from +0x26C, indexed
 * by +0x56; the Gear record by +0xA0 from +0x978), clamping to the maxima.
 * Slots whose D_800D2D24 byte is 0x7F are skipped. */
void func_8009BE0C(void) {
    u32 i;

    for (i = 0; i < 3; i++) {
        u32 off;
        u8* a2;
        u8* a1;
        u8* t0;
        u8* t1;
        u32 j;
        u32 g;

        if (D_800D2D24[i] == 0x7F) {
            continue;
        }
        off = i * 0x170;
        a2 = D_800CCCE8 + off;
        a1 = g_GameState + 0x26C + a2[0x56] * 0xA4;
        t0 = g_GameState + 0x26C + 0x70C + a2[0xA0] * 0xA4;
        t1 = a2 + 0xA4;
        if (a2[0x56] == 7 && (D_800CCE42[off] & 0x80)) {
            *(u16*)(a2 + 0x4C) = (*(u32*)(t1 + 0x60) + 1) / 50;
            if (*(u16*)(a2 + 0x4C) == 0) {
                *(u16*)(a2 + 0x4C) = 1;
            }
        }
        *(u16*)(a1 + 0x4C) = *(u16*)(a2 + 0x4C);
        {
            u16 mp = *(u16*)(a2 + 0x50);

            if (*(u16*)(a1 + 0x4E) < *(u16*)(a1 + 0x4C)) {
                *(u16*)(a1 + 0x50) = mp;
                *(u16*)(a1 + 0x4C) = *(u16*)(a1 + 0x4E);
            } else {
                *(u16*)(a1 + 0x50) = mp;
            }
        }
        if (*(u16*)(a1 + 0x52) < *(u16*)(a1 + 0x50)) {
            *(u16*)(a1 + 0x50) = *(u16*)(a1 + 0x52);
        }
        for (j = 0; j < 7; j++) {
            *(u16*)(a1 + 0x90 + j * 2) = *(u16*)(a2 + 0x90 + j * 2);
        }
        *(u16*)(a1 + 0x3A) = *(u16*)(a2 + 0x3A);
        if (*(u16*)(a2 + 0x7C) & 0xC000) {
            *(u16*)(a1 + 0x4C) = 1;
        }
        g = a2[0xA0];
        if (g < 7 || (g >= 8 && g < 0x11)) {
            u32 cap = *(u32*)(t0 + 0x64);
            u16 hp = *(u16*)(t1 + 0x38);

            *(u32*)(t0 + 0x60) = *(u32*)(t1 + 0x60);
            *(u16*)(t0 + 0x38) = hp;
            if (cap < *(u32*)(t0 + 0x60)) {
                *(u32*)(t0 + 0x60) = cap;
            }
            if (*(u16*)(t0 + 0x3A) < *(u16*)(t0 + 0x38)) {
                *(u16*)(t0 + 0x38) = *(u16*)(t0 + 0x3A);
            }
            if (*(u16*)(t1 + 0x7C) & 0x8000) {
                *(u32*)(t0 + 0x60) = *(u32*)(t0 + 0x64) / 10;
            }
        }
    }
}
#endif /* XENO_PC_PORT */

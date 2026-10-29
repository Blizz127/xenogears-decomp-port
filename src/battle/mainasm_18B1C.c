/* battle mainasm_18B1C: the retail-asm run at file offset 0x18B1C, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif

#ifndef XENO_BATTLE_OVERLAY_HOST_BODIES
extern s8 D_800C207C;
extern s32 D_800C2080;
extern s32 D_800C2084;
extern s32 D_800C3A7C;
extern s32 D_800C3A80;
extern s32 D_800C3A84;
extern s32 D_800C3A88;
extern s32 D_800C3A8C;
extern s32 D_800C3A90;
extern u8 D_800C3A94;
extern u8 D_800C3A98;
extern s32 D_800C3A9C;
#endif
#ifdef XENO_PC_PORT
extern u8 func_8001BD40(u8, u8);
#else
extern s32 func_8001BD40(s32, s32);
#endif

#ifndef XENO_PC_PORT
extern u8* D_800D2DB4;
extern u8 D_800C33B0[];
extern u8 D_800CCB34__b[] asm("D_800CCB34");
extern s32 func_80076A10(s32 glyph, u8* prims, s16 x, s16 y);
extern void func_80076B68(u8* p);
extern void func_80076BF0(u8* p);

/* Draw the battle message glyph rows: two command-name glyph runs from
 * D_800C33B0[0..1] appended to the +0x5D74 POLY_FT4 run at +0x1720 (80
 * bytes each), the 0xA8 cursor glyph at +0x5D70 (prims at +0), their
 * current D_800CCB34 buffers recorded at +0x5D83 / +0x5D92, the glyphs
 * submitted through func_80076B68 for both runs, then D_800C33B0[2..3] at
 * +0x5D7E (+0x2530, buffer +0x5D8D) through func_80076BF0.  gcc 2.6.x.
 * Codegen: D_800D2DB4 is re-read in every statement and the prim offset
 * is added before the base, as retail; D_800CCB34 through an unsized array
 * view, so its address stays in s0 across the calls; the draw loops are
 * for loops: their duplicated `j < count` tests leave three dead compare
 * temporaries, which still get retail's three spare frame slots. */
void func_8008860C(void) {
    s32 i;
    s32 j;

    for (i = 0; i < 2; i++) {
        D_800D2DB4[0x5D74] += func_80076A10(D_800C33B0[i], D_800D2DB4 + (D_800D2DB4[0x5D74] * 80 + 0x1720), 0xA0, 0x64);
    }
    D_800D2DB4[0x5D83] = D_800CCB34__b[0];
    D_800D2DB4[0x5D70] = func_80076A10(0xA8, D_800D2DB4 + D_800D2DB4[0x5D70] * 80, 0xA0, 0x64);
    D_800D2DB4[0x5D92] = D_800CCB34__b[0];
    for (j = 0; j < D_800D2DB4[0x5D74]; j++) {
        func_80076B68(D_800D2DB4 + ((j * 2 + D_800D2DB4[0x5D83]) * 40 + 0x1720));
    }
    for (j = 0; j < D_800D2DB4[0x5D70]; j++) {
        func_80076B68(D_800D2DB4 + (j * 2 + D_800D2DB4[0x5D92]) * 40);
    }
    for (i = 2; i < 4; i++) {
        D_800D2DB4[0x5D7E] += func_80076A10(D_800C33B0[i], D_800D2DB4 + (D_800D2DB4[0x5D7E] * 80 + 0x2530), 0xA0, 0x64);
    }
    D_800D2DB4[0x5D8D] = D_800CCB34__b[0];
    for (j = 0; j < D_800D2DB4[0x5D7E]; j++) {
        func_80076BF0(D_800D2DB4 + ((j * 2 + D_800D2DB4[0x5D8D]) * 40 + 0x2530));
    }
}
#endif

#ifdef XENO_PC_PORT
/* R3000 signed DIV quotient after a wrapping SLL. Host C division traps
 * on INT32_MIN / -1, whereas the original returns INT32_MIN. */
static s32 battle_line_slope_quotient(s32 delta, s32 divisor)
{
    s32 numerator = (s32)((u32)delta << 8);
    if (divisor == 0)
        return numerator >= 0 ? -1 : 1;
    if ((u32)numerator == 0x80000000u && divisor == -1)
        return numerator;
    return numerator / divisor;
}
#endif

void func_8008887C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 var_a0;
    s32 var_a1;

    D_800C3A7C = arg0;
    D_800C3A80 = arg1;
    D_800C3A84 = arg2;
    D_800C3A88 = arg3;
    if ((arg2 != arg0) && (arg3 != arg1)) {
        if (arg2 < arg0) {
            D_800C3A94 = 1;

#ifdef XENO_PC_PORT
            var_a0 = (s32)((u32)arg0 - (u32)arg2);
#else
            var_a0 = arg0 - arg2;
#endif
        } else {

#ifdef XENO_PC_PORT
            var_a0 = (s32)((u32)arg2 - (u32)arg0);
#else
            var_a0 = arg2 - arg0;
#endif
            D_800C3A94 = 0;
        }
        if (arg3 < arg1) {
            D_800C3A98 = 1;

#ifdef XENO_PC_PORT
            var_a1 = (s32)((u32)arg1 - (u32)arg3);
#else
            var_a1 = arg1 - arg3;
#endif
        } else {

#ifdef XENO_PC_PORT
            var_a1 = (s32)((u32)arg3 - (u32)arg1);
#else
            var_a1 = arg3 - arg1;
#endif
            D_800C3A98 = 0;
        }
        if (var_a0 >= var_a1) {
            D_800C3A8C = 0x100;

#ifdef XENO_PC_PORT
            D_800C3A90 = battle_line_slope_quotient(var_a1, var_a0);
#else
            D_800C3A90 = (s32) (var_a1 << 8) / var_a0;
#endif
        } else {
            D_800C3A90 = 0x100;

#ifdef XENO_PC_PORT
            D_800C3A8C = battle_line_slope_quotient(var_a0, var_a1);
#else
            D_800C3A8C = (s32) (var_a0 << 8) / var_a1;
#endif
        }
        D_800C2080 = 0;
        D_800C2084 = 0;
        D_800C3A9C = func_8001BD40(1, 8) & 0xFF;
        D_800C207C = 0;
    }
}

#ifndef XENO_PC_PORT
/* func_80088990.s, byte-exact: step the D_800C2080 / D_800C2084 position
 * D_800C3A9C times (by D_800C3A8C / D_800C3A90, down when the D_800C3A94 /
 * D_800C3A98 flag is set, else up), then set D_800C207C when the position
 * (/ 256, plus D_800C3A7C / D_800C3A80) has passed the D_800C3A84 /
 * D_800C3A88 target in the stepping direction: the x axis when D_800C3A8C
 * is 0x100, else the y axis.  The port arm is the
 * XENO_PC_PORT body in main40.c.  loop.c hoists the step globals as in
 * retail; the two flags are u8 (retail reads them with lbu).  First draft;
 * matches under gcc 2.7.2 (main40 already declares the flags u8). */
void func_80088990(void) {
    s32 i;

    for (i = 0; i < D_800C3A9C; i++) {
        if (D_800C3A94 != 0) {
            D_800C2080 -= D_800C3A8C;
        } else {
            D_800C2080 += D_800C3A8C;
        }
        if (D_800C3A98 != 0) {
            D_800C2084 -= D_800C3A90;
        } else {
            D_800C2084 += D_800C3A90;
        }
    }
    if (D_800C3A8C == 0x100) {
        if (D_800C3A94 != 0) {
            if (D_800C2080 / 256 + D_800C3A7C < D_800C3A84) {
                D_800C207C = 1;
            }
        } else if (D_800C3A84 < D_800C2080 / 256 + D_800C3A7C) {
            D_800C207C = 1;
        }
    } else {
        if (D_800C3A98 != 0) {
            if (D_800C2084 / 256 + D_800C3A80 < D_800C3A88) {
                D_800C207C = 1;
            }
        } else if (D_800C3A88 < D_800C2084 / 256 + D_800C3A80) {
            D_800C207C = 1;
        }
    }
}
#endif

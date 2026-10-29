/*
 * Small retail game functions whose original implementation is either a
 * direct memory write, a tail-call alias, or a GTE loop that cannot be
 * assembled for the x86-64 port.  Each body below follows the retail game
 * instructions in disc/SLUS_006.64; no game state is synthesized here.
 * (No Psy-Q SDK functions: those are in psyq_compat_leaf.inc.)
 */
#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/font.h"

#include <stdarg.h>
#include <string.h>

extern s32 D_80050618;
extern void func_80036718(int mode, char* format, va_list args);
extern void MTC2(unsigned int value, int reg);
extern unsigned int MFC2(int reg);
extern unsigned int CFC2(int reg);
extern void CTC2(unsigned int value, int reg);
extern int doCOP2(int op);

/* Psy-Q SDK entry points (ReadGeomScreen, SetMulMatrix, SetPolyFT3/G3/GT3,
 * SetLineF2/G2, RotMatrixZYX) live in the compat layer, written from the
 * documented interface; included here so the tests that link this TU keep
 * resolving them. */
#include "psyq_compat_leaf.inc"

/* Retail 0x800379B4-0x800379C4: one word store and return. */
void func_800379B4(s32 value)
{
    D_80050618 = value;
}

/* Retail 0x800379C8 is a tail jump to FontPrintf.  A C variadic wrapper cannot
 * express a machine-level tail alias portably, so forward the same va_list to
 * FontPrintf's formatter while retaining FontPrintf's null-font guard. */
void func_800379C8(char* format, ...)
{
    va_list args;

    if (g_Font != NULL) {
        va_start(args, format);
        func_80036718(0, format, args);
        va_end(args);
    }
}

/* Retail 80025D4C..80025FA8, SHA-256
 * 8beea0886035c1f0694c1503bae9c336ace1eea3ecafe6ded8adf06fc13a5e67 (the
 * handwritten GTE loop temp1f.c keeps as INCLUDE_ASM).  For `count` BGR555
 * pixels: take source[i]'s fields (kind 0 as is, 1 halved, 2 quartered, 3 the
 * grey mean by retail's divide-by-three), add red / green << 5 / blue << 10,
 * clamp each field with retail's 16-bit signed compares, then step towards
 * target[i] by IR0 = min(factor, 32) << 7 with GPF12 and store
 * field + (IR & field mask) | 0x8000 into destination[i] (a carry out of a
 * field ORs into its neighbour, as in retail).  A zero source pixel keeps
 * its destination; the GTE step still runs.  Every retail register is a u32
 * here and the GTE moves and command are retail's own.  A kind outside 0..3
 * reuses the previous pixel's fields, as retail's t2..t4 do; the first such
 * pixel starts from 0 where retail starts from the caller's leftover t2..t4
 * (no caller does this: main70.c's 800A6AE8 passes kinds 0..3 here and sends
 * kind 4 to func_80026F44).  Retail returns with v0 = 1 and HI/LO from the
 * grey divide; its one caller (800A6CFC) rewrites v0 before reading it and
 * reads neither v1 nor HI/LO. */
void func_80025D4C(s32 count, u16* source, u16* target, u16* destination,
                   s32 red, s32 green, s32 blue, s32 kind, s32 factor)
{
    u32 greenOffset = (u32)green << 5;
    u32 blueOffset = (u32)blue << 10;
    u32 r = 0, g = 0, b = 0;     /* retail t4, t3, t2 */
    u32 remaining = (u32)count;

    if (factor > 32) factor = 32;
    MTC2((u32)factor << 7, 8);
    while (remaining-- != 0) {
        u32 pixel = *source;
        u32 outR, outG, outB, to, stepR, stepG, stepB;

        switch (kind) {
        case 0:
            r = pixel & 0x001F;
            g = pixel & 0x03E0;
            b = pixel & 0x7C00;
            break;
        case 1:
            r = (pixel & 0x001E) >> 1;
            g = (pixel & 0x03C0) >> 1;
            b = (pixel & 0x7800) >> 1;
            break;
        case 2:
            r = (pixel & 0x001C) >> 2;
            g = (pixel & 0x0380) >> 2;
            b = (pixel & 0x7000) >> 2;
            break;
        case 3: {
            u32 sum = (pixel & 0x001F) + ((pixel & 0x03E0) >> 5) + ((pixel & 0x7C00) >> 10);
            u32 grey = sum / 3u;   /* mult 0x55555556, mfhi: exact for 0..93 */
            r = grey;
            g = grey << 5;
            b = grey << 10;
            break;
        }
        default:
            break;
        }
        outR = r + (u32)red;
        if ((s16)outR < 0) outR = 0;
        outG = g + greenOffset;
        if ((s16)outG < 0) outG = 0;
        outB = b + blueOffset;
        if ((s16)outB < 0) outB = 0;
        if ((s16)outR >= 0x20) outR = 0x1F;
        if ((s16)outG >= 0x3E1) outG = 0x3E0;
        if ((s16)outB >= 0x7C01) outB = 0x7C00;
        to = *target;
        MTC2((to & 0x001Fu) - (u32)(s32)(s16)outR, 9);
        MTC2((to & 0x03E0u) - (u32)(s32)(s16)outG, 10);
        MTC2((to & 0x7C00u) - (u32)(s32)(s16)outB, 11);
        doCOP2(0x0198003D);
        stepR = MFC2(9);        /* swc2 IR1..IR3: read before the zero test */
        stepG = MFC2(10);
        stepB = MFC2(11);
        if (pixel != 0)
            *destination = (u16)(((outR + (stepR & 0x001F)) | 0x8000) |
                                 (outG + (stepG & 0x03E0)) | (outB + (stepB & 0x7C00)));
        ++source;
        ++target;
        ++destination;
    }
}

/* Retail 80026FE8..8002709C, SHA-256
 * df91dc9c5881d3488381d84c56ca367b3f5a46aa7c77877ae6e87450814eabaa.
 * Keep GPF12 and the final sourceB read in the branch delay slot. */
void func_80026FE8(s32 count, s32 factor, u16* destination,
                   const u16* sourceA, const u16* sourceB)
{
    if (factor > 32) factor = 32;
    MTC2((u32)factor << 7, 8);
    u32 remaining = (u32)count;
    for (;;) {
        u16 inputB = *(const volatile u16*)sourceB;
        if (remaining == 0) break;
        --remaining;
        u16 inputA = *sourceA;
        ++sourceB;
        u32 red = inputA & 0x001F;
        u32 green = inputA & 0x03E0;
        u32 blue = inputA & 0x7C00;
        MTC2((inputB & 0x001Fu) - red, 9);
        MTC2((inputB & 0x03E0u) - green, 10);
        MTC2((inputB & 0x7C00u) - blue, 11);
        doCOP2(0x0198003D);
        red += MFC2(9) & 0x001F;
        green += MFC2(10) & 0x03E0;
        blue += MFC2(11) & 0x7C00;
        *destination = (u16)(red | green | blue);
        ++sourceA;
        ++destination;
    }
}

/* Retail 0x80026F44-0x80026FE4.  The original writes IR0/IR1/IR2/IR3 and runs
 * GPF12 for every BGR555 pixel.  Keep that exact hardware boundary and let
 * PsyCross execute the GTE operation; only the MIPS loop mechanics are C. */
void func_80026F44(s32 count, s32 factor, u16* destination,
                   const u16* source)
{
    if (factor >= 0x20) {
        factor = 0x20;
    }
    MTC2((u32)factor << 7, 8);

    while (count-- != 0) {
        u16 input = *source++;
        u16 output;

        MTC2(input & 0x001F, 9);
        MTC2(input & 0x03E0, 10);
        MTC2(input & 0x7C00, 11);
        doCOP2(0x0198003D); /* GPF12 */

        output = (u16)((MFC2(9) & 0x001F) |
                       (MFC2(10) & 0x03E0) |
                       (MFC2(11) & 0x7C00));
        if (input != 0 && output == 0) {
            output = 1;
        }
        *destination++ = (u16)((input & 0x8000) | output);
    }
}

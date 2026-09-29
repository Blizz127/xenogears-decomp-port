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

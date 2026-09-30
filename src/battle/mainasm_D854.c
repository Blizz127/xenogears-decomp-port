/* battle mainasm_D854: the retail-asm run at file offset 0xD854, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainasm_D854", func_8007D344);
INCLUDE_ASM("asm/battle/nonmatchings/mainasm_D854", func_8007D478);
extern u8 D_800CCD64[];
extern u8 D_800D3430[];

/* count i in 0..2 with (D_800CCD64[i * 0x170] & 0xC000) == 0 and store it
 * to D_800D3430[(index << 6) + p[1]] (p = *ppBoard). */
void func_8007D5B0(u8** ppBoard, u8 index) {
    s32 count = 0;
    s32 i = 0;
    s32 off = 0;
    u16 flags;
    u8* p;
    u8* row;

loop:
    flags = *(u16*)(D_800CCD64 + off);
    off += 0x170;
    if ((flags & 0xC000) == 0) {
        count++;
    }
    i++;
    if (i < 3) goto loop;

    p = *ppBoard;
    row = D_800D3430 + (index << 6);
    row[p[1]] = count;
}

extern u8 D_800C3EB7[];
extern u8 D_800CCD64[];
extern u8 D_800D2DCC[];
extern u8 D_800D3430[];

/* count i in 3..10 with D_800D2DCC[i] != 0,
 * (D_800CCD64[0x450 + (i - 3) * 0x170] & 0xC000) == 0 and
 * D_800C3EB7[0x54 + (i - 3) * 0x1C] == 0; store it to
 * D_800D3430[(index << 6) + p[1]] (p = *ppBoard). */
void func_8007D610(u8** ppBoard, u8 index) {
    s32 count = 0;
    s32 i = 3;
    s32 off2 = 0x54;
    s32 off = 0x450;
    u8 alive;
    u8* p;
    u8* row;

loop:
    alive = D_800D2DCC[i];
    i++;
    if (alive != 0 && (*(u16*)(D_800CCD64 + off) & 0xC000) == 0 && D_800C3EB7[off2] == 0) {
        count++;
    }
    off2 += 0x1C;
    off += 0x170;
    if (i < 11) goto loop;

    p = *ppBoard;
    row = D_800D3430 + (index << 6);
    row[p[1]] = count;
}

#endif

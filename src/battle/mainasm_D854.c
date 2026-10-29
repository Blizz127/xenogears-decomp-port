/* battle mainasm_D854: the retail run at file offset 0xD854, in retail order
 * (gcc 2.6.x code: BattleGcc263).  The whole file is retail-build only; the
 * port bodies for these functions live in main27.c. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
extern u8 D_800C3EB7__rows[][28] asm("D_800C3EB7");
extern u32 func_8007A6C8(u8 index, u8 kind);
extern u32 func_8001BD40(u32 lo, u8 hi);
extern u16 func_80089C08(u8 index);

/* func_8007D344.s, byte-exact.  Same shape as main26's func_8007D1DC, but
 * over slots 3..10 with another filter: collect the slots that
 * func_8007A6C8(i, p[2]) accepts and whose 28-byte row byte 7 (D_800C3EB7)
 * has bit 7 set, and store func_80089C08 of a random one in halfword p[1]
 * of row `index` of D_800D3420 (cleared first).  gcc 2.6.x code
 * (mainasm_D854 is BattleGcc263). */
void func_8007D344(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 3;
    s32 n = 0;
    u16* row;
    u16* dst;
    u16 v;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[1]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A6C8(i, (*ppBoard)[2]) != 0 && (D_800C3EB7__rows[i][0] & 0x80)) {
            list[n] = i;
            n++;
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[1]] = v;
    }
}

extern u16 D_800CCD64__h[][184] asm("D_800CCD64");

/* func_8007D478.s, byte-exact: clear halfword p[1] of row `index` of
 * D_800D3420, then pick slots r = func_8001BD40(0, 2) not tried yet:
 * when the u16 at the start of r's 0x170-byte record (D_800CCD64) has
 * bit 15 set and bits 0x4002 clear, store func_80089C08(r) in that
 * halfword and return; otherwise mark r tried, until all three are.  As in
 * func_8007D1DC, the second row pointer is set after the call, from a
 * separate `v` (set once before the loop, it is one insn long). */
void func_8007D478(u8** ppBoard, u8 index) {
    u8 tried[3];
    u16* row;
    u16* dst;
    u16 v;
    u32 r;
    u16 flags;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    tried[0] = tried[1] = tried[2] = 0;
    row[(*ppBoard)[1]] = 0;
    while (!(tried[0] & tried[1] & tried[2])) {
        r = func_8001BD40(0, 2);
        if (tried[(u8)r] == 0) {
            flags = D_800CCD64__h[(u8)r][0];
            if ((flags & 0x8000) && !(flags & 0x4002)) {
                v = func_80089C08(r);
                dst = (u16*)(D_800D3420 + ((u32)index << 6));
                dst[(*ppBoard)[1]] = v;
                return;
            }
            tried[(u8)r] = 1;
        }
    }
}
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

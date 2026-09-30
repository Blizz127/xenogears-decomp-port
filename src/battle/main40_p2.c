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
INCLUDE_ASM("asm/battle/nonmatchings/main40_p2", func_80088B80);

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

INCLUDE_ASM("asm/battle/nonmatchings/main40_p2", func_800891E4);
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

INCLUDE_ASM("asm/battle/nonmatchings/main40_p2", func_8008946C);
INCLUDE_ASM("asm/battle/nonmatchings/main40_p2", func_8008963C);
extern u8 D_800D2C35;
extern void func_8008AAA0(u32 value);
extern u8 D_800C3CFB[];

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

INCLUDE_ASM("asm/battle/nonmatchings/main40_p2", func_800898F0);
#endif
#include "main40.c"

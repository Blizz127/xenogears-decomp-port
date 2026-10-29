/* Retail run 1 of main35_p3: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in main35.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#define BATTLE_TU_PART 3
#ifndef XENO_PC_PORT
extern u8 D_800C3E90[];
extern u8 D_800D3274;
extern u16 D_800C3D64;
extern u32 func_80084108(u32 arg0, u32 arg1);
extern u16 func_80089C08(u8);

/* func_80084548.s, byte-exact: build the D_800C3E90 target list from one
 * or two target ranges (the two-range form of func_80084750).  Fill the
 * 12 entries with 0xFF from the end, then pick the ranges by mode: 0
 * targets 3..10; 1 targets 0..2; 2 both, 3..10 then 0..2 when flag is 0,
 * else 0..2 then 3..10.  Clear D_800D3274 and D_800C3D64 and append each
 * target that func_80084108(target, actor) accepts, OR-ing
 * func_80089C08(target) into D_800C3D64 and counting it in D_800D3274.
 * Returns the first list entry.  Other modes leave the first range unset
 * (retail runs on whatever the registers hold).  The u8 mode is promoted
 * to int for the switch (retail's signed range test).  Matches under gcc
 * 2.6.x only (2.7.2 fills the loop delay slots differently), so this TU
 * builds with BattleGcc263. */
u32 func_80084548(u8 mode, u8 actor, u8 flag) {
    s32 count2 = 0;
    s32 fill = 0xFF;
    s32 i = 11;
    s32 output;
    u8 start1;
    s32 count1;
    u8 start2;
    u8* clear = &D_800C3E90[11];

    for (; i >= 0; i--) {
        *clear = fill;
        clear--;
    }
    switch (mode) {
    case 0:
        start1 = 3;
        count1 = 8;
        break;
    case 1:
        start1 = 0;
        count1 = 3;
        break;
    case 2:
        if (flag == 0) {
            start1 = 3;
            count1 = 8;
            start2 = 0;
            count2 = 3;
        } else {
            start1 = 0;
            count1 = 3;
            start2 = 3;
            count2 = 8;
        }
        break;
    }
    output = 0;
    count1--;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = start1; count1 >= 0; count1--) {
        u8 target = i;
        u32 eligible = func_80084108(target, actor);

        if (eligible & 0xFF) {
            u32 mask;

            D_800C3E90[output] = target;
            mask = func_80089C08(target);
            output++;
            D_800C3D64 |= mask;
            D_800D3274++;
        }
        i++;
    }
    count2--;
    for (i = start2; count2 >= 0; count2--) {
        u8 target = i;
        u32 eligible = func_80084108(target, actor);

        if (eligible & 0xFF) {
            u32 mask;

            D_800C3E90[output] = target;
            mask = func_80089C08(target);
            output++;
            D_800C3D64 |= mask;
            D_800D3274++;
        }
        i++;
    }
    return D_800C3E90[0];
}
#endif
#include "main35.c"

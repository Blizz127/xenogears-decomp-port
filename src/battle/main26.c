#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
extern u8 D_800CCD3E__rows[][0x170] asm("D_800CCD3E");
extern u32 func_8007A744(u8 index);
extern u32 func_8001BD40(u32 lo, u8 hi);
extern u16 func_80089C08(u8 index);

/* func_8007D1DC.s, byte-exact: battle-script op on p = *ppBoard: clear
 * halfword p[1] of row `index` of D_800D3420, collect the combatants 0..10
 * that func_8007A744 accepts and whose 0x170-byte record byte (D_800CCD3E)
 * equals p[2], and when there are any, store func_80089C08 of a random one
 * (func_8001BD40(0, count - 1)) in that halfword.  gcc 2.6.x code (main26
 * is BattleGcc263; 2.7.2 is 2 insns short).  The records are a 2D view
 * (symbol + offset), the
 * stores go through row pointers computed after the calls, and the list is
 * indexed by the count. */
void func_8007D1DC(u8** ppBoard, u8 index) {
    u8 list[16];
    s32 i = 0;
    s32 n = 0;
    u16* row;
    u16* dst;
    u16 v;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[1]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A744(i) != 0 && D_800CCD3E__rows[i][0] == (*ppBoard)[2]) {
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
#endif


/* func_8007D30C.s */
#ifndef XENO_PC_PORT
extern u8 D_800D3410[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C8B[];
#endif
void func_8007D30C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 row = (u32)D_800D3410 + ((u32)index << 6);

    *(u32*)((p[1] << 2) + row) = D_800D2C8B[index];
}

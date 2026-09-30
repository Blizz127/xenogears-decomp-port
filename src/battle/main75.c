#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main75", func_800B14CC);
#endif
typedef struct { u32 w[7]; } BattleOtRecord;
#ifndef XENO_PC_PORT
extern BattleOtRecord D_800C3BD0;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3BEC;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3BF0;
#endif
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_800B15D8.s: copy entry idx of the 28-byte record array at a0 + 0xC into
 * D_800C3BD0; unless bit0 of a0[+4] is set, relocate the record's three
 * pointer words by the entry address.  Publishes the +0x10 word in D_800C3BEC,
 * clears D_800C3BF0 and returns the entry's +0x14 word. */
u32 func_800B15D8(u8* a0, s32 idx) {
    u8* e = a0 + idx * 28 + 0xC;

    D_800C3BD0 = *(BattleOtRecord*)e;
    if ((*(u32*)(a0 + 4) & 1) == 0) {
        D_800C3BD0.w[0] += (u32)e;
        D_800C3BD0.w[2] += (u32)e;
        D_800C3BD0.w[4] += (u32)e;
    }
    D_800C3BF0[0] = 0;
    D_800C3BEC[0] = D_800C3BD0.w[4];
    return *(u32*)(e + 0x14);
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/main75", func_800B15D8);
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D40;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D48;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3BCC[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);


/* func_800B168C.s */
u8* func_800B168C(u8* pArg, u32 iValue) {
    return pArg + (iValue * 0x1C + 0xC);
}


#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* func_800B14CC.s: for each of the three party slots other than `self`,
 * run func_800AA934(actor, actor, D_800C3D0C, 5) on its D_800D3368 record;
 * then wait (func_800BE790 per pass) until none of those records has its
 * +0x34 or +0x38 busy byte set, wait one more frame and run func_800A9FF0
 * on each of those slots. */
extern void func_800AA934(u8* a0, u8* a1, u8* a2, u32 a3);
extern void func_800BE790(void);
extern void func_800A9FF0(u32 v);
void func_800B14CC(u32 self) {
    u32 i;
    s32 busy;

    for (i = 0; (s32)i < 3; i++) {
        if (i != self) {
            u32 ent = BG_U32(0x800D3368 + i * 4); /* D_800D3368 */

            func_800AA934(BG_PTR(ent), BG_PTR(ent), BG_PTR(0x800C3D0C), 5); /* D_800C3D0C */
        }
    }
    do {
        busy = 0;
        for (i = 0; (s32)i < 3; i++) {
            u32 ent;

            if (i == self) {
                continue;
            }
            ent = BG_U32(0x800D3368 + i * 4); /* D_800D3368 */
            if (ent != 0 && (BG_U8(ent + 0x34) != 0 || BG_U8(ent + 0x38) != 0)) {
                busy = 1;
            }
        }
        func_800BE790();
    } while (busy != 0);
    func_800BE790();
    for (i = 0; (s32)i < 3; i++) {
        if (i != self) {
            func_800A9FF0(i);
        }
    }
}
#endif

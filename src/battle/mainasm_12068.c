/* battle mainasm_12068: the retail run at file offset 0x12068 (func_80081B58),
 * split out of main35_p2 because its C is gcc 2.6.3 code (BattleGcc263;
 * main35_p2 and mainasm_11828 around it are 2.7.2).  The whole file is
 * retail-build only; any port body for this function lives in the port's
 * own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
/* main35_p2 context the body relies on (declared as there). */
u32 func_80084854(u32 arg0, u32 arg1);
extern struct BattleCommandContext * D_800C3EAC;
void func_8008189C(s32 arg0);
extern void func_80077980(void);
extern void func_8009413C(u8 idx, u8 flag);
extern void func_800819A4(u8 actor);
extern u8 D_800D366C;
extern u8* D_800D2D28;
extern void func_80080B64(u8 v);
extern u8 D_800D3014;
extern u16 D_800D2E06[];
extern u8 D_800C31D4__r[][8] asm("D_800C31D4");
extern void func_800B8DA4(void);
extern void func_8008AA40(u8 v);
extern void func_800879A8(u8 a, u8 b);
extern u32 func_800877E0(u8 a, u8 b);
extern u8 func_80085EB4(u8 a, u8 b);
extern void func_80085D34(u8 n);
extern void func_800861D0(u8 a, u8 b);
extern u32 func_80087AF0(u8 actor, u8 column);

/* func_80081B58.s, byte-exact under gcc 2.6.3 (2.7.2 differs in the
 * * 100 / 56 quotient region).
 * CONSTRUCT (counted separately): the +0x2D4 update is a set-twice block,
 * `{ u8 left = ...; hits = ...; left -= hits; ... = left; }`.  With `left`
 * set twice, sched1 no longer boosts the subtraction as a once-set temp, so
 * the 2D4/2DF loads keep source order as in retail. */
s32 func_80081B58(u8 actor) {
    u32 hits;

    D_800D366C = 0;
    if (((u8*)D_800C3EAC)[0x2E1] != 0) {
        return;
    }
    ((u8*)D_800C3EAC)[0x2DF] = 0;
    func_8008189C(actor);
    switch (D_800D3014) {
    case 5:
        if (((u8*)D_800C3EAC)[0x2D4] != ((u8*)D_800C3EAC)[0x2D5]) {
            func_80080B64(actor);
            ((u8*)D_800C3EAC)[0x2E1] = 1;
        } else {
            func_800B8DA4();
            D_800D366C = 1;
            D_800D2D28[0x7B] = 0;
            D_800D2D28[0xAF] = 0;
            ((u8*)D_800C3EAC)[0x2DD] = 1;
            func_80077980();
            func_8009413C(actor, 1);
        }
        break;
    case 4:
        if (*(u16*)((u8*)D_800C3EAC + (actor << 6) + 0x20) != 0) {
            D_800D3014 = 5;
            func_8008AA40(0x4F);
        }
        break;
    case 7:
        if (*(u16*)((u8*)D_800C3EAC + (actor << 6) + 0x1E) != 0) {
            D_800D3014 = 5;
            func_8008AA40(0x4F);
        }
        break;
    case 6:
        if (*(u16*)((u8*)D_800C3EAC + (actor << 6) + 0x1C) != 0) {
            D_800D3014 = 5;
            func_8008AA40(0x4F);
        }
        break;
    }
    if (((u8*)D_800C3EAC)[0x2E9] == 0) {
        switch (D_800D3014) {
        case 0:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 0);
            func_800879A8(((u8*)D_800C3EAC)[0x2D3], ((u8*)D_800C3EAC)[0x2E8]);
            func_8008AA40(0x4C);
            break;
        case 1:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 1);
            func_800879A8(((u8*)D_800C3EAC)[0x2D3], ((u8*)D_800C3EAC)[0x2E8]);
            func_8008AA40(0x4C);
            break;
        case 2:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 2);
            func_800879A8(((u8*)D_800C3EAC)[0x2D3], ((u8*)D_800C3EAC)[0x2E8]);
            func_8008AA40(0x4C);
            break;
        case 3:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 3);
            func_800879A8(((u8*)D_800C3EAC)[0x2D3], ((u8*)D_800C3EAC)[0x2E8]);
            func_8008AA40(0x4C);
            break;
        }
        func_800877E0(actor, ((u8*)D_800C3EAC)[0x2E8]);
    }
    D_800D2D28[0xAF] = 1;
    switch (D_800D3014) {
    case 5:
        break;
    case 4:
        if (func_80085EB4(4, actor) && ((u8*)D_800C3EAC)[0x2D4] - 3 >= 0) {
            ((u8*)D_800C3EAC)[0x2E4]++;
        }
        ((u8*)D_800C3EAC)[0x2DF]++;
    case 7:
        ((u8*)D_800C3EAC)[0x2DF]++;
    case 6:
        ((u8*)D_800C3EAC)[0x2DF]++;
        if (((u8*)D_800C3EAC)[0x2D4] - ((u8*)D_800C3EAC)[0x2DF] >= 0) {
            func_8008AA40(0x4D);
            func_800819A4(actor);
            if (D_800D2D28[0xCB] != 0) {
                ((u8*)D_800C3EAC)[0x2DD] = 0x64;
                ((u8*)D_800C3EAC)[0x2E2] = 0xFF;
                ((u8*)D_800C3EAC)[0x2E0] = 1;
            } else {
                ((u8*)D_800C3EAC)[0x2DD] = 5;
                ((u8*)D_800C3EAC)[0x2E2] = 5;
            }
            { u8 left = ((u8*)D_800C3EAC)[0x2D4]; hits = ((u8*)D_800C3EAC)[0x2DF]; left -= hits; ((u8*)D_800C3EAC)[0x2D4] = left; }
            func_80085D34(hits);
            func_800861D0(D_800D3014, actor);
            ((u8*)D_800C3EAC)[0x2E5] = func_80087AF0(actor, ((u8*)D_800C3EAC)[0x2DF]);
            D_800D2E06[actor] = D_800C31D4__r[((u8*)D_800C3EAC)[0x2D5]][((u8*)D_800C3EAC)[0x2D4]] * 100 / 56;
            ((u8*)D_800C3EAC)[0x2E3] = 1;
        } else {
            func_8008AA40(0x4F);
        }
        if (((u8*)D_800C3EAC)[0x2D4] == 0) {
            func_80080B64(actor);
            ((u8*)D_800C3EAC)[0x2E1] = 1;
        }
        if (((u8*)D_800C3EAC)[0x2E5] != 0) {
            func_80080B64(actor);
            ((u8*)D_800C3EAC)[0x2E1] = 1;
        }
        break;
    }
}
#endif

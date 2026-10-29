/* Retail TU part 1 of main35.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-1 C bodies. */
#define BATTLE_TU_PART 1
#include "common.h"
#ifndef XENO_PC_PORT
typedef struct {
    u8 pad[0x3C];
    u8 target;
    u8 pad3D[3];
} A80Row;
typedef struct {
    A80Row rows[11];
} A80Ctx;
extern A80Ctx* D_800C3EAC__c asm("D_800C3EAC");
extern u8 D_800C3E2C;
extern u8 D_800C3E18;
extern u8 D_800C204C;
extern u8 D_800C48EA;
extern u8 D_800D2D24[];
extern u8 D_800D32A1[][8];
extern u8 D_800C400B__r[][0x48] asm("D_800C400B");
extern u8 D_800C402F__r[][0x48] asm("D_800C402F");
extern u16 D_800C4022__r[][0x24] asm("D_800C4022");
extern u16 D_800C3FFE__r[][0x24] asm("D_800C3FFE");
extern u8 D_800CDD53__b[][38][40] asm("D_800CDD53");
extern u16 D_800CCD38__r[][0xB8] asm("D_800CCD38");
extern void func_800716D8(void);
extern void func_8009AB00(u8 actor);
extern void func_8009BAC4(u8 actor, u8* info, u8* rec);
extern u32 func_80083FF4(u32, u32);
extern s32 func_8001BD40(s32 lo, s32 hi);
extern void func_800879A8(u8 actor, u8 target);
extern u16 func_80089C08(u8 actor);
extern void func_800BC404(u32 mask);
extern s32 func_800877E0(u8 actor, u8 target);
extern s32 func_80080AE4(u8 actor);
extern void func_800B89FC(s32 a0, u8 actor, u8 target, s32 a3);
extern void func_80087AF0(u8 actor, u8 n);
extern void func_80080B64(u8 actor);
extern void func_8008ADD0(u8 actor, u16 cur, u8 cost);
extern void func_800826CC(u8 actor);
extern void func_8009AA44(u8 actor);

/* func_80080C94.s, byte-exact under gcc 2.6.3 (main35_p1 is BattleGcc263).
 * Codegen notes:
 * - The fill is a plain up-counting `for (i = 0; i < 11; i++)`: loop.c's
 *   dbra reversal emits the count-down init after the hoisted 0xFF, as in
 *   retail.  A hand-written count-down puts `i = 10` first.
 * - The pick loop is a single do/while, so list and 0xFF sit at depth 2 and
 *   the actor copy gets s0.
 * - The local D_800C3EAC__c struct view gives the base-first addu
 *   (->rows[actor].target), the same idiom as func_8008115C.
 * CONSTRUCTS (counted separately):
 * - func_8008ADD0 is declared with 3 arguments (main45 defines it with
 *   one); the extra arguments reproduce retail's a1/a2.
 * - The asm-alias views D_800C3EAC__c and D_800CDD53__b ([][38][40], as in
 *   main46). */
void func_80080C94(u8 actor) {
    u8 info[2];
    u8 list[11];
    s32 i;
    u8* p;
    u8 result = 1;
    u32 cost;
    u32 cur;

    func_800716D8();
    for (i = 0; i < 11; i++) {
        list[i] = 0xFF;
    }
    ((u8*)D_800C3EAC__c)[0x2EA] = 1;
    ((u8*)D_800C3EAC__c)[0x2DE] = 0;
    func_8009AB00(actor);
    func_8009BAC4(actor, info, ((u8*)D_800C3EAC__c) + actor * 64 + 0x2A);
    i = 0;
    if (info[0] == 1 || info[0] == 2) {
        p = list;
        for (; i < 11; i++) {
            if ((u8)func_80083FF4(actor, (u8)i)) {
                *p++ = i;
            }
        }
        do {
        } while ((D_800C3E2C = list[(u8)func_8001BD40(0, 10)]) == 0xFF || D_800C3E2C == actor);
    }
    func_800879A8(actor, D_800C3E2C);
    D_800C3EAC__c->rows[actor].target = D_800C3E2C;
    func_800BC404((u16)(func_80089C08(actor) | func_80089C08(D_800C3EAC__c->rows[actor].target)));
    func_800716D8();
    D_800C204C = 1;
    switch (info[0]) {
    case 1:
        D_800C3E18 = 0;
        if (D_800D2D24[actor] != 4) {
            D_800C400B__r[((u8*)D_800C3EAC__c)[0x2DA]][0] = actor;
            D_800C402F__r[((u8*)D_800C3EAC__c)[0x2DA]][0] = 0xFD;
            D_800C4022__r[((u8*)D_800C3EAC__c)[0x2DA]][0] = 0;
            D_800C3FFE__r[((u8*)D_800C3EAC__c)[0x2DA]][0] = func_80089C08(D_800C3EAC__c->rows[actor].target);
            ((u8*)D_800C3EAC__c)[0x2DA]++;
        }
        if (D_800D32A1[actor][0] == 0) {
            ((u8*)D_800C3EAC__c)[0x2DC] = 0;
        } else {
            ((u8*)D_800C3EAC__c)[0x2DC] = info[1];
        }
        func_800B89FC(func_800877E0(actor, D_800C3EAC__c->rows[actor].target), actor,
                      D_800C3EAC__c->rows[actor].target, func_80080AE4(actor));
        func_80087AF0(actor, info[1] + 1);
        func_80080B64(actor);
        result = 0;
        break;
    case 2:
        cost = D_800CDD53__b[actor][info[1] + 0x16][0];
        cur = D_800CCD38__r[actor][0];
        if (cur >= cost) {
            D_800CCD38__r[actor][0] = cur - cost;
            ((u8*)D_800C3EAC__c)[0x2E6] = info[1];
            func_8008ADD0(actor, cur, cost);
            result = 0;
        }
        break;
    case 4:
        func_800826CC(actor);
        ((u8*)D_800C3EAC__c)[0x2DE] = 1;
        result = 0;
        break;
    }
    if (result) {
        func_8009AA44(actor);
        ((u8*)D_800C3EAC__c)[0x2EA] = 0;
        ((u8*)D_800C3EAC__c)[0x2DE] = 1;
    }
    while (((u8*)D_800C3EAC__c)[0x2DE] == 0 && D_800C48EA == 0 && ((u8*)D_800C3EAC__c)[0x2DB] == 0) {
        func_800716D8();
    }
    D_800C204C = 0;
}

#endif
#include "main35.c"

#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3610[];
typedef union BattleSlotRow { u8 b[0x1C]; u16 h[0xE]; } BattleSlotRow;
extern BattleSlotRow D_800C3EB0__slots[] asm("D_800C3EB0");
extern void func_800BEFF4(u32 arg0);
extern void func_800BEE2C(u32 arg0, u32 arg1, u32 arg2);
extern void func_800BA8F4(u8* s);
extern void func_800BEDE8(void);

/* func_800B8DA4.s, byte-exact: with a command task (D_800C3610), after
 * func_800BEFF4 of the task's +0x20 word, when the actor's 4-bit
 * class (+0xAC bits 0..1 above +0xA8 bits 30..31) has its 28-byte slot row
 * byte 8 set, func_800BEE2C(class, 0, 0x1F); otherwise the actor's 16.16
 * x/z (+0/+8) come from slot row (task +0x24) halfwords +0xE/+0x10, then
 * func_800BA8F4, y (+4) from its +0x84, func_800245D8(actor, +0xB0) and
 * func_80021BF8(actor, 0).  Finally func_800BEDE8.  The slot table pointer
 * is taken before the class (retail schedules its address into the load
 * delays); the class uses a separate +0xA8 temporary. */
void func_800B8DA4(void) {
    u8* task;
    u8* actor;
    u32 lo;
    u32 cls;
    BattleSlotRow* slots;

    if (D_800C3610[0] == NULL) {
        return;
    }
    func_800BEFF4(*(u32*)(D_800C3610[0] + 0x20));
    task = D_800C3610[0];
    slots = D_800C3EB0__slots;
    actor = *(u8**)(task + 4);
    lo = *(u32*)(actor + 0xA8) >> 30;
    cls = ((*(u32*)(actor + 0xAC) & 3) << 2) | lo;
    if (slots[cls].b[8] != 0) {
        func_800BEE2C(cls, 0, 0x1F);
    } else {
        *(s32*)(actor + 0) = slots[*(s32*)(task + 0x24)].h[0xE / 2] << 16;
        *(s32*)(actor + 8) = slots[*(s32*)(task + 0x24)].h[0x10 / 2] << 16;
        func_800BA8F4(actor);
        *(s32*)(actor + 4) = *(s16*)(actor + 0x84) << 16;
        func_800245D8((u32)actor, *(s8*)(actor + 0xB0));
        func_80021BF8((u32)actor, 0);
    }
    func_800BEDE8();
}

extern u8 D_800C3624[];
extern u16 D_800C3DF0[];
extern u8 D_800C3622[];
extern u32 D_80059464[];
extern u8 D_800C3688[];
extern u8 D_800C35D4[];
extern s32 D_800C3E54[];
extern u8 D_800C3EB0[];
extern void func_80080BD0(void);
extern void DrawSync(s32 mode);
extern void func_80021BF8(u32 p, u32 v);
extern u32 func_800C0314(void);
extern u32 func_800BF720(void);
extern void func_800BE790(void);
extern void func_800C0564(void);
extern void func_800B8D7C(void);
extern void func_800BC3F8(u32 v);
extern void func_800BE0DC(void);
extern void func_800BF9EC(void);
extern void func_800BFA9C(void);
extern void func_800BC454(u16 v);
extern void func_800BA4E0(s32 arg0);
extern void func_8003A89C(s32 a0, s32 a1, s32 a2);

/* func_800B8EBC.s, byte-exact: with a command task (D_800C3610), clears
 * D_800C3624/D_800C3DF0/D_800C3622, runs func_80080BD0 and DrawSync(0),
 * func_80021BF8(actor, 0) and func_800C0314, waits (func_800BE790) until
 * func_800BF720() equals D_80059464, runs func_800C0564 ... func_800BEDE8
 * and func_800BA4E0 (the task's +0x40 word), calls
 * func_8003A89C(D_800C3E54, 0x7F, 0x50) when D_800C35D4 is set, clears it,
 * and clears bit 6 of +0xAC on each of the 11 records at
 * D_800C3EB0 +0x8C8C.  The actor is read before the task NULL test, as in
 * retail. */
void func_800B8EBC(void) {
    u8* task;
    u8* actor;
    s32 arg;
    s32 i;
    u8* base;

    task = D_800C3610[0];
    actor = *(u8**)(task + 4);
    if (task == NULL) {
        return;
    }
    D_800C3624[0] = 0;
    D_800C3DF0[0] = 0;
    D_800C3622[0] = 0;
    func_80080BD0();
    DrawSync(0);
    arg = *(s32*)(D_800C3610[0] + 0x40);
    func_80021BF8((u32)actor, 0);
    func_800C0314();
    while (D_80059464[0] != func_800BF720()) {
        func_800BE790();
    }
    func_800C0564();
    func_800B8D7C();
    func_800BC3F8(1);
    func_800BE0DC();
    func_800BF9EC();
    func_800BFA9C();
    D_800C3688[0] = 0;
    func_800BC454(0xC0);
    func_800BEDE8();
    func_800BA4E0(arg);
    if (D_800C35D4[0] != 0) {
        func_8003A89C(D_800C3E54[0], 0x7F, 0x50);
    }
    D_800C35D4[0] = 0;
    base = D_800C3EB0;
    for (i = 0; i != 11; i++) {
        u8* rec = *(u8**)(base + 0x8C8C + i * 4);

        if (rec != NULL) {
            *(u32*)(rec + 0xAC) &= ~0x40;
        }
    }
}
#endif


extern void func_800245D8(u32 p, s32 v);
extern void func_80021BF8(u32 p, u32 v);


/* func_800B9020.s */
void func_800B9020(u8* p) {
    func_800245D8((u32)p, *(s8*)(p + 0xB0));
    func_80021BF8((u32)p, 0);
}

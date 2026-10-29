#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3610[];
extern s32 D_800C360C[];
extern u32 D_800D363C[];
extern u8 D_800D3634__b[] asm("D_800D3634");
extern s16 D_800D3678[];
typedef union BattleStepRow { u8 b[0x48]; u16 h[0x24]; } BattleStepRow;
extern BattleStepRow D_800C3EB0__steps[] asm("D_800C3EB0");
extern u32 func_800BEEB4(u32 mask, u32* out, u32 value);
extern u32 func_800BEF24(u8* a, u8* b);
extern void func_800223B0(u8* obj, s16 angle);

/* func_800BF3E8.s, byte-exact: pair `actor` with its command target.  The
 * current step's u16 at +0x14E (D_800C3EB0 row D_800C360C) goes to
 * D_800D3634 and to func_800BEEB4, which looks the target up into
 * D_800D363C; its result goes to D_800D3678, and with no target the actor
 * targets itself.  Actor and target then point at each other (+0x74), the
 * command task (D_800C3610) gets the target (+0x4C) and its 4-bit class
 * (+0x28: +0xAC bits 0..1 above +0xA8 bits 30..31), and both turn to face
 * each other (func_800223B0 of func_800BEF24), the target only when its
 * state +0xAF is not 0x15.
 * Codegen: the D_800D3634 store goes through a byte-array alias (not an
 * array element, which is in-struct and keeps the step field's second load
 * after the store); the class is built from a separate +0xA8 temporary
 * (retail ORs the +0xAC part first but loads +0xA8 first). */
void func_800BF3E8(u8* actor) {
    u8* target;
    u8* task;
    u32 flags;
    u32 lo;

    *(u16*)D_800D3634__b = D_800C3EB0__steps[D_800C360C[0]].h[0x14E / 2];
    D_800D3678[0] = func_800BEEB4(D_800C3EB0__steps[D_800C360C[0]].h[0x14E / 2], D_800D363C, (u32)actor);
    if (D_800D3678[0] == 0) {
        D_800D363C[0] = (u32)actor;
    }
    target = (u8*)D_800D363C[0];
    task = D_800C3610[0];
    *(u8**)(actor + 0x74) = target;
    *(u8**)(target + 0x74) = actor;
    lo = *(u32*)(target + 0xA8) >> 30;
    flags = ((*(u32*)(target + 0xAC) & 3) << 2) | lo;
    *(u32*)(task + 0x4C) = D_800D363C[0];
    *(u32*)(task + 0x28) = flags;
    func_800223B0(actor, func_800BEF24(actor, *(u8**)(actor + 0x74)));
    if (*(s8*)(target + 0xAF) != 0x15) {
        func_800223B0(target, func_800BEF24(*(u8**)(actor + 0x74), actor));
    }
}
extern u8* D_800C3610[];
typedef union BattleRow6 { u8 b[6]; u16 h[3]; } BattleRow6;
extern BattleRow6 D_800C3EB0__r6[] asm("D_800C3EB0");
extern void func_800B9C00(u8* rec, u8* other);
extern void func_800BF1EC(u8* a, s32 b);
extern void func_800BF0B4(u32 value);
extern void func_800BF0C4(u8* a);

/* func_800BF4F0.s, byte-exact: when the current command's step (the 6-byte
 * D_800C3EB0 row of the task's +0x2C index) has no explicit target (both
 * halfwords at +0xA3C/+0xA3E are 0xFFFF), object `a` restarts its approach
 * to `b`: its 16.16 position (+0/+8) is set from its old target (+0xA0/+0xA4,
 * << 16), and the new target is b's x (high half of b +0) 0x50 short on a's
 * side, y 0 and b's z (high half of b +8).  When that target is where `a`
 * already stands, func_800B9C00(a, b); otherwise func_800BF1EC(a, 3) and
 * func_800BF0B4(2).  With an explicit target, func_800BF0C4(a).
 * Codegen: the row is indexed directly in both tests (a row pointer computes
 * the index before the table address); b's x is an s32 (one sign-extended
 * load for the compare and both sides of the select); the +0xA4 store comes
 * before the +0xA2 one (the other order keeps `a` in a0). */
void func_800BF4F0(u8* a, u8* b) {
    s32 bx;

    if (D_800C3EB0__r6[*(u32*)(D_800C3610[0] + 0x2C)].h[0xA3C / 2] == 0xFFFF
        && D_800C3EB0__r6[*(u32*)(D_800C3610[0] + 0x2C)].h[0xA3E / 2] == 0xFFFF) {
        *(s32*)(a + 0) = *(s16*)(a + 0xA0) << 16;
        *(s32*)(a + 8) = *(s16*)(a + 0xA4) << 16;
        bx = *(s32*)(b + 0) >> 16;
        *(s16*)(a + 0xA0) = (*(s16*)(a + 2) < bx) ? bx - 0x50 : bx + 0x50;
        *(s16*)(a + 0xA4) = *(s32*)(b + 8) >> 16;
        *(s16*)(a + 0xA2) = 0;
        if (*(s16*)(a + 0xA0) == *(s16*)(a + 2) && *(s16*)(a + 0xA4) == *(s16*)(a + 0xA)) {
            func_800B9C00(a, b);
        } else {
            func_800BF1EC(a, 3);
            func_800BF0B4(2);
        }
    } else {
        func_800BF0C4(a);
    }
}
#endif


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(u32 a0, u32 a1);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
#ifndef XENO_PC_PORT
extern u32 D_800C3548[];
#endif


/* func_800BF5E8.s */
void func_800BF5E8(void) {
    D_800C3CE8[0]++;
}
/* func_800BF600.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void func_800B7C34(u8* p);
extern void func_80021BF8(u8* p, void* cb);
extern void func_800245D8(u32 a0, u32 a1);
extern void func_800BF5E8(void);
extern void func_800BE790(void);
void func_800BF600(u8* a0, u8* a1) {
    u8* s0 = a1;

    if (*(u32*)(s0 + 0x48) == 0) {
        func_800B7C34(a0);
        return;
    }
    D_800C3CE8[0] = 0;
    if (*(s8*)(s0 + 0xAF) != 0) {
        func_80021BF8(s0, func_800BF5E8);
        func_800245D8((u32)s0, (u32)*(s8*)(s0 + 0xAF));
    }
    func_800B7C34(a0);
    if (*(s8*)(s0 + 0xAF) != 0) {
        while (D_800C3CE8[0] == 0) {
            func_800BE790();
        }
        func_80021BF8(s0, 0);
    }
}

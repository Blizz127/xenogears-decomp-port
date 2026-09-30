#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc84", func_800B572C);
#include "psyq/libgte.h"

u32 func_800B57E4(u8* s) {
    VECTOR d;
    VECTOR sq;

    d.vx = *(s16*)(s + 0xA0) - *(s16*)(s + 0x2);
    d.vy = *(s16*)(s + 0xA2) - *(s16*)(s + 0x6);
    d.vz = *(s16*)(s + 0xA4) - *(s16*)(s + 0xA);
    Square0(&d, &sq);
    return SquareRoot0(sq.vx + sq.vz + sq.vy);
}

#endif
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_800B5854.s: refresh the task's +0x28 tick from func_800B57E4(+0x1C).
 * A rise past the old value, a drop below the +0x2C floor, a cleared +0x9E
 * marker on the target, or a +0xAF state that no longer matches +0x24 all
 * mark the task dirty and fire its +0xC callback; the first case also arms
 * +0x9E and copies +0x30 into the target's +0x64.  The dirty flag is live
 * across the call, so retail keeps it in a callee-saved register. */
void func_800B5854(u8* task) {
    u8* tgt = *(u8**)(task + 0x1C);
    s32 prev = *(s32*)(task + 0x28);
    u32 dirty = 0;
    s32 cur;

    cur = func_800B57E4(tgt);
    *(s32*)(task + 0x28) = cur;
    if (prev < cur || cur < *(s32*)(task + 0x2C)) {
        dirty = 1;
        *(s16*)(tgt + 0x9E) = 1;
        *(u32*)(tgt + 0x64) = *(u32*)(task + 0x30);
    }
    if (*(s16*)(tgt + 0x9E) == 0) {
        dirty = 1;
    }
    if (*(s8*)(tgt + 0xAF) != *(s32*)(task + 0x24)) {
        dirty = 1;
    }
    if (dirty & 0xFF) {
        (*(void (**)(u8*))(task + 0xC))(task);
    }
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/mainc84", func_800B5854);
#endif
extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(u8* task);
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


/* func_800B5924.s */
u8* func_800B5924(u8* a0, u32 a1, u32 a2) {
    u8* p = (u8*)TimerWorkListAllocateTask(*(u32*)(a0 + 0x6C), 0x18);

    TimerWorkListSetTaskCallback(p, func_800B5854);
    *(u32*)(p + 0x1C) = (u32)(unsigned long)a0;
    *(u32*)(p + 0x28) = func_800B57E4(a0);
    *(u32*)(p + 0x2C) = a1;
    *(u32*)(p + 0x30) = a2;
    *(u32*)(p + 0x24) = (u32)(*(s8*)(a0 + 0xAF));
    *(u32*)(a0 + 0xAC) |= 0x20;
    return p;
}

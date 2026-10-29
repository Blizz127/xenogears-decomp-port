#include "common.h"


#ifndef XENO_PC_PORT
extern int rcos(int a);
extern int rsin(int a);
extern void func_8001F6B0(u8* obj);

/* func_800BCBB4.s: timer callback installed by func_800BCC60: from the
 * task's +0x20 tick, set bytes +0x28 (cos) and +0x29/+0x2A (sin) of the
 * object at task +0x1C to (trig + 0x1000) / 64 + 0x80, capped at
 * 0xFF, call func_8001F6B0 on the object and advance the tick.  The local
 * copy t of task gives retail's double copy ($a0 -> $v0 -> $s0), with the
 * first read through task; c is updated in place step by step, which keeps
 * it in $v1 (the one-expression form is 10/43). */
void func_800BCBB4(u8* task) {
    u8* t = task;
    s32 ang = *(s32*)(task + 0x20) << 6;
    u8* obj = *(u8**)(t + 0x1C);
    s32 c;

    c = rcos(ang) + 0x1000;
    c >>= 6;
    c += 0x80;
    if (c > 0xFF) {
        c = 0xFF;
    }
    obj[0x28] = c;
    c = rsin(ang) + 0x1000;
    c >>= 6;
    c += 0x80;
    if (c > 0xFF) {
        c = 0xFF;
    }
    obj[0x29] = c;
    obj[0x2A] = c;
    func_8001F6B0(obj);
    (*(s32*)(t + 0x20))++;
}

extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800BCB54(u8* task);
extern u8 D_800C3EB0[];
extern u8* D_800C3748[];
extern u8 D_800C4922[];
extern u8 D_800591AC[];
extern u32 D_80059464[];

/* func_800BCC60.s: (re)start the timer task that drives func_800BCBB4.  An
 * existing task (D_800C3748) is kept when its +0x24 slot already equals
 * D_800C4922; otherwise func_800BCB54(0) is called.  Then, for slot
 * D_800C3EB0[0xA72], allocate a 0xC-byte timer task for the owner word at
 * +0x8CB8 + slot * 4 (none: return), install func_800BCBB4 and the
 * func_800BCB54 on-free callback, step D_80059464 down when D_800591AC is
 * set, clear bit 31 of +0x14, point +0x1C at the slot's object (+0x8C8C +
 * slot * 4), store the slot at +0x24 and clear bit 0 of the object's +0x2B.
 * First draft. */
void func_800BCC60(void) {
    u8* base;
    u8* task;
    u8* obj;
    u32 owner;

    if (D_800C3748[0] != 0) {
        if (*(u32*)(D_800C3748[0] + 0x24) == D_800C4922[0]) {
            return;
        }
        func_800BCB54(0);
    }
    base = D_800C3EB0;
    owner = *(u32*)(base + base[0xA72] * 4 + 0x8CB8);
    if (owner == 0) {
        return;
    }
    task = TimerWorkListAllocateTask(owner, 0xC);
    D_800C3748[0] = task;
    TimerWorkListSetTaskCallback(task, func_800BCBB4);
    WorkListTaskSetOnFreeCallback(task, func_800BCB54);
    if (D_800591AC[0] != 0) {
        D_80059464[0]--;
    }
    *(u32*)(task + 0x14) &= 0x7FFFFFFF;
    obj = *(u8**)(base + base[0xA72] * 4 + 0x8C8C);
    *(u8**)(task + 0x1C) = obj;
    *(u32*)(task + 0x24) = base[0xA72];
    obj[0x2B] &= 0xFE;
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


/* func_800BCD8C.s */
#ifndef XENO_PC_PORT
extern u16 D_800C3D14[];
#endif
void func_800BCD8C(void) {
    D_800C3D14[0] = 0;
}

#include "common.h"


#ifndef XENO_PC_PORT
/* func_800B3B94.s, byte-exact: the timer callback of func_800B3CD4's
 * task.  While the +0x3C countdown runs, +0x46 moves from +0x40 towards
 * +0x42: n = +0x3C - 1 (stored back), f = (n << 5) / +0x38, and
 * +0x46 = +0x42 - (+0x42 - +0x40) * f / 32.  Once the countdown is 0 and
 * +0x46 is 0, it calls the task's +0xC function with the task.
 * Codegen (counted natural, flagged):
 * - x carries the whole chain (the +0x40 value, the product, the quotient,
 *   the result).  That gives retail's rounding copy, which reads the
 *   product, and retail's registers.  The +0x40 read must stay its own
 *   statement: folded into the product the registers come out mirrored
 *   (v0/v1, 14/38).  With separate expressions the copy is coalesced (one
 *   insn short).
 * - p is a second name for t, used only in the else branch; it gives
 *   retail's a1 (then) / a0 (else) split (t alone is one insn short). */
void func_800B3B94(u8* t) {
    u8* p = t;
    s32 n;
    s32 f;
    s32 a;
    s32 x;

    if (*(s32*)(t + 0x3C) != 0) {
        n = *(s32*)(t + 0x3C) - 1;
        f = (n << 5) / *(s32*)(t + 0x38);
        a = *(s16*)(t + 0x42);
        *(s32*)(t + 0x3C) = n;
        x = *(s16*)(t + 0x40);
        x = (a - x) * f;
        x /= 32;
        x = a - x;
        *(s16*)(t + 0x46) = x;
    } else if (*(s16*)(p + 0x46) == 0) {
        (*(void (**)(u8*))(p + 0xC))(p);
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


/* func_800B3C2C.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3560[];
#endif
extern void WorkListRemoveTask(u32 p);
extern void TimerWorkListRemoveTask(u32 p);
extern void HeapFree(u32 p);
extern void func_800A6F98(void);
void func_800B3C2C(u8* p) {
    WorkListRemoveTask((u32)(p + 0x1C));
    TimerWorkListRemoveTask((u32)p);
    HeapFree((u32)p);
    D_800C3560[0] = 0;
    func_800A6F98();
}

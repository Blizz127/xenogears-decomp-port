#include "common.h"


/* func_800BF73C.s: refresh the task's +0x24 value from func_800B57E4(+0x1C);
 * if it rose above the previous value or is below the +0x28 floor, fire the
 * +0x2C callback on +0x1C and then the +0xC callback on the task. */
void func_800BF73C(u8* task) {
    u8* t = task;
    s32 prev = *(s32*)(task + 0x24);
    s32 cur = func_800B57E4(*(u8**)(task + 0x1C));

    *(s32*)(task + 0x24) = cur;
    if (prev < cur || cur < *(s32*)(task + 0x28)) {
        (*(void (**)(u8*))(t + 0x2C))(*(u8**)(t + 0x1C));
        (*(void (**)(u8*))(t + 0xC))(t);
    }
}


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(u8* task);
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


/* func_800BF7C8.s */
void func_800BF7C8(u8* a0, u32 a1, u32 a2) {
    u8* p = (u8*)TimerWorkListAllocateTask(*(u32*)(a0 + 0x6C), 0x14);

    TimerWorkListSetTaskCallback(p, func_800BF73C);
    *(u32*)(p + 0x2C) = a2;
    *(u32*)(p + 0x1C) = (u32)(unsigned long)a0;
    *(u32*)(p + 0x20) = (u32)(*(s8*)(a0 + 0xAF));
    *(u32*)(p + 0x24) = func_800B57E4(a0);
    *(u32*)(p + 0x28) = a1;
    *(u32*)(a0 + 0xAC) |= 0x20;
}

#include "common.h"


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


/* func_800B16A4.s */
s32 func_800B16A4(u8* p) {
    s32 count = 0;
    s32 total = 0;
    u32 off = *(u32*)(p + 0x10);
    u32 n = *(u32*)(p + 0x14);
    u8* q = (u8*)(off + (u32)p);

    if (n != 0) {
        do {
            count++;
            total += (q[0] + 1) * 4;
            q += (q[1] + 1) * 4;
        } while (count != n);
    }
    return total;
}
/* func_800B16F0.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3BEC[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3BF0[];
#endif
void func_800B16F0(void) {
    u8* p = (u8*)(u32)D_800C3BEC[0];
    u8 n = p[1];

    D_800C3BF0[0] = D_800C3BF0[0] + 1;
    D_800C3BEC[0] = (u32)(p + ((n + 1) << 2));
}

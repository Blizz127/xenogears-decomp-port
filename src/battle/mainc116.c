#include "common.h"


extern int rcos(int a);
extern void func_8001F6B0(u8* p);
/* func_800BCAFC.s: grey level 0x80 + (rcos(a1 * 64) + 0x1000) / 64, saturated
 * to 0xFF, written to the three colour bytes at +0x28, then func_8001F6B0. */
void func_800BCAFC(u8* a0, s32 a1) {
    s32 c = rcos(a1 << 6);
    s32 v = c + 0x1000;

    v >>= 6;
    v += 0x80;
    if (v >= 0x100) {
        v = 0xFF;
    }
    a0[0x28] = v;
    a0[0x29] = v;
    a0[0x2A] = v;
    func_8001F6B0(a0);
}

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


/* func_800BCB54.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3748[];
#endif
extern void TimerWorkListRemoveTask(u32 p);
extern void HeapFree(u32 p);
void func_800BCB54(void) {
    u8* s0 = (u8*)(u32)D_800C3748[0];

    if (s0 != 0) {
        u8* v1 = (u8*)(u32)*(u32*)(s0 + 0x1C);

        *(u8*)(v1 + 0x2B) |= 1;
        TimerWorkListRemoveTask((u32)s0);
        HeapFree((u32)s0);
        D_800C3748[0] = 0;
    }
}

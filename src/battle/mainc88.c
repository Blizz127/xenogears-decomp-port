#include "common.h"


extern void func_8002DDE4(void* a0, u32 a1, u32 a2, u32 a3, u32 s0, u32 s1, u32 s2);
/* func_800B63F0.s: run func_8002DDE4 on the record at a1 + its signed 16-bit
 * little-endian offset (a1[0] low, (s8)a1[1] high); a0 is unused. */
void func_800B63F0(u32 a0, u8* a1) {
    func_8002DDE4(a1 + (((s32)(s8)a1[1] << 8) | a1[0]), 0, 0, 0, 0, 0, 0);
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


/* func_800B6438.s */
void func_800B6438(u8* p) {
    WorkListSetTaskCallback((u8*)(u32)*(u32*)(p + 0x6C) + 0x1C, D_80025A88);
}

#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc108", func_800B9284);
INCLUDE_ASM("asm/battle/nonmatchings/mainc108", func_800B9508);
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


/* func_800B9B30.s */
#ifndef XENO_PC_PORT
extern u8* D_800C3610[];
#endif
void func_800B9B30(void) {
#ifdef XENO_PC_PORT
    /* main125.c/main68.c read D_800C3610 as a u8*, so the guest-RAM alias is
     * that pointer and the retail array's [0] element is the alias itself.
     * Indexing it a second time would subscript a u8. */
    D_800C3610[0x48] = 1;
    D_800C3610[0x49] = D_800C3610[0x1C];
#else
    D_800C3610[0][0x48] = 1;
    D_800C3610[0][0x49] = D_800C3610[0][0x1C];
#endif
}

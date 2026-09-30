#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc102", func_800B838C);
INCLUDE_ASM("asm/battle/nonmatchings/mainc102", func_800B853C);
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


/* func_800B8774.s */
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 D_8005919C[];
extern u8 D_800591AD[];
#ifndef XENO_PC_PORT
extern u32 D_800D2D54[];
#endif
extern void func_800BE790(void);
extern void DrawSync(u32 v);
extern void func_800BADD4(u32 i);
extern void GfxFreeWorkBuffers(void);
extern void WorkListsFreeAllEntries(void);
extern void func_8003852C(u32 p);
extern void HeapFree(u32 p);
extern void func_800A9F94(void);
extern void func_800A4820(void);
void func_800B8774(void) {
    u8* base = (u8*)(u32)D_800C3EB0;
    u32 i;
    u32 n;

    if (*(u32*)(base + 0x8000 + 0xC84) == 0) {
        func_800BE790();
    }
    DrawSync(0);
    i = 0;
    n = 0xB;
    for (; i != n; i++) {
        func_800BADD4(i);
    }
    GfxFreeWorkBuffers();
    WorkListsFreeAllEntries();
    func_8003852C(D_8005919C[0]);
    HeapFree(D_8005919C[0]);
    D_800591AD[0] = 0;
    DrawSync(0);
    func_800A9F94();
    func_800A4820();
    HeapFree(D_800D2D54[0]);
}
/* func_800B8840.s */
extern u8 D_800591AD[];
extern u32 D_80059464[];
extern u8 D_800591AC[];
extern u32 D_800591A8[];
extern u32 D_80050104[];
extern void func_800BED30(void);
extern void func_800BE108(void);
extern void WorkListsReset(void);
extern void func_800BB7F8(void);
extern void GfxAllocateWorkBuffers(u32 a0, u32 a1);
extern void func_800BCD8C(void);
extern void func_800B7C28(void);
extern void func_800B89F4(void);
void func_800B8840(void) {
    D_800591AD[0] = 1;
    D_80059464[0] = 0;
    D_800591AC[0] = 0;
    D_800591A8[0] = 0x2000;
    func_800BED30();
    func_800BE108();
    WorkListsReset();
    func_800BB7F8();
    GfxAllocateWorkBuffers(0x5000, 0);
    func_800BCD8C();
    func_800B7C28();
    func_800B89F4();
    D_80050104[0] = 0;
}

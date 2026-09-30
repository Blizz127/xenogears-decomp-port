#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc132", func_800BFA9C);
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


/* func_800BFBA0.s */
#ifndef XENO_PC_PORT
extern u8 D_800C3620[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3622[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3A6C[];
#endif
extern void func_800B8354(void);
extern u32 ArchiveSetIndex(u32 dir, u32 entry);
extern u32 ArchiveDecodeAlignedSize(u32 index);
extern u32 HeapAlloc(u32 size, u32 flag);
extern void ArchiveReadFileToBuffer(u32 a0, void* buf, u32 a2, u32 a3);
extern u32 SoundFindWdsEntry(u32 id);
extern void func_800C0F70(void);
extern u32 SoundLoadWdsFile(void* p, u32 a1);
extern u32 func_8003BDFC(u32 v);
extern void func_800BE790(void);
extern void HeapFree(u32 p);
void func_800BFBA0(void) {
    u8* s0;

    if (D_800C3620[0] == 0) {
        func_800B8354();
        ArchiveSetIndex(0x2C, 0);
        s0 = (u8*)(u32)HeapAlloc(ArchiveDecodeAlignedSize(5), 0);
        ArchiveReadFileToBuffer(5, s0, 0, 0x80);
        func_800B8354();
        if (SoundFindWdsEntry(*(u16*)(s0 + 0x20)) == 0) {
            func_800C0F70();
            D_800C3A6C[0] = (u32)SoundLoadWdsFile(s0, 0);
            while ((func_8003BDFC(0) << 16) != 0) {
                func_800BE790();
            }
            D_800C3620[0] = 1;
            D_800C3622[0] = 0;
        }
        HeapFree((u32)s0);
    }
}

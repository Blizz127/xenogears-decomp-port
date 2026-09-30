#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc126", func_800BF0C4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc126", func_800BF1EC);
#endif
extern void func_800B8D7C(u8* task);
extern void ArchiveSetIndex(u32 a0, u32 a1);
extern u32 ArchiveDecodeAlignedSize(u32 file);
extern void ArchiveReadFileToBuffer(u32 file, u8* buf, u32 a2, u32 a3);
#ifndef XENO_PC_PORT
extern u32 D_800C3618[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C361C;
#endif
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_800BF2B8.s: read the task's archive file (id at **(task + 0x7C)) into a
 * fresh heap buffer recorded in D_800C3618, then pack the task's +0xAC low two
 * bits above the +0xA8 top two bits into D_800C361C. */
void func_800BF2B8(u8* task) {
    u32 file;
    u8* buf;

    func_800B8D7C(task);
    ArchiveSetIndex(0x2C, 1);
    file = **(u32**)(task + 0x7C);
    buf = HeapAlloc(ArchiveDecodeAlignedSize(file), 1);
    ArchiveReadFileToBuffer(file, buf, 0, 0x80);
    D_800C3618[0] = (u32)buf;
    D_800C361C = ((*(u32*)(task + 0xAC) & 3) << 2) | (*(u32*)(task + 0xA8) >> 30);
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/mainc126", func_800BF2B8);
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


/* func_800BF354.s */
#ifndef XENO_PC_PORT
extern u8 D_800D3350[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3618[];
#endif
extern u32 func_800C0FAC(u32 p);
u32 func_800BF354(void) {
    u32 r;

    if (D_800D3350[0] == 0) {
        r = func_800C0FAC(D_800C3618[0]);
        D_800D3350[0] = 1;
    }
    return r;
}
/* func_800BF3A4.s */
#ifndef XENO_PC_PORT
extern u8 D_800D3350[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3618[];
#endif
extern void func_800C1140(u32 p);
void func_800BF3A4(void) {
    if (D_800D3350[0] != 0) {
        func_800C1140(D_800C3618[0]);
        D_800D3350[0] = 0;
    }
}

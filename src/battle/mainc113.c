#include "common.h"


#ifndef XENO_PC_PORT
extern s32 ArchiveDataSync(void);
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern void func_800BB690(u8* task);
extern u8 D_800C3CB8[];

/* once the archive is idle and D_800C3CB8 is clear, run func_800BB690(task)
 * on a HeapAlloc(0x1000, 1) stack (caller $sp saved at +0xF00). */
void func_800BB6E0(u8* task) {
    u8* stack;

    if (ArchiveDataSync() == 0 && D_800C3CB8[0] == 0) {
        stack = HeapAlloc(0x1000, 1);
        __asm__ volatile(
            "\taddu  $t0, %0, $zero\n"
            "\tsw    $sp, 0($t0)\n"
            "\taddiu $t0, $t0, -4\n"
            "\taddu  $sp, $t0, $zero\n"
            :
            : "r"(stack + 0xF00)
            : "t0", "memory");
        func_800BB690(task);
        __asm__ volatile(
            "\taddiu $sp, $sp, 4\n"
            "\tlw    $sp, 0($sp)\n"
            :
            :
            : "memory");
        HeapFree(stack);
    }
}

extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern void func_800BB6E0(u8* task);
extern u8 D_800591AC[];
extern u8 D_800591AF[];
extern s32 D_800C35D8[];

/* queue a 4-byte timer task (callback func_800BB6E0) for `slot` with
 * D_800591AC cleared and D_800591AF set around the allocation, and count it
 * in D_800C35D8. */
void func_800BB760(u32 slot) {
    u8 saved = D_800591AC[0];
    u8* task;
    s32 count;

    D_800591AC[0] = 0;
    D_800591AF[0] = 1;
    task = TimerWorkListAllocateTask(0, 4);
    TimerWorkListSetTaskCallback(task, func_800BB6E0);
    count = D_800C35D8[0];
    *(u32*)(task + 0x1C) = slot;
    D_800591AF[0] = 0;
    D_800C35D8[0] = count + 1;
    D_800591AC[0] = saved;
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


/* func_800BB7F8.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3674[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3678[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3CC4[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CBC[];
#endif
extern void func_800BC2F0(u32 v);
void func_800BB7F8(void) {
    D_800C3674[0] = 0x200;
    D_800C3678[0] = 0xFFFFFFFF;
    D_800C3CC4[0] = 0;
    D_800C3CBC[0] = 1;
    func_800BC2F0(0);
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native owners: src/slus_006.64/system/{memory,libarchive}.c, sdata. */
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern s32 ArchiveDataSync(void);
extern u8 D_800591AC;
extern u8 D_800591AF;
extern void func_800BB690(u8* p);

/* func_800BB6E0.s: timer callback: once the archive is idle and the
 * D_800C3CB8 counter is 0, run func_800BB690(task) on a private 0x1000-byte
 * heap stack (retail saves $sp at block + 0xF00 and switches to it; the
 * native body just calls through) and free it. */
void func_800BB6E0(u8* task) {
    if (ArchiveDataSync() == 0 && BG_U8(0x800C3CB8u) == 0) { /* D_800C3CB8 */
        void* stack = HeapAlloc(0x1000, 1);

        func_800BB690(task);
        HeapFree(stack);
    }
}

/* func_800BB760.s: queue a 4-byte timer task (callback func_800BB6E0) that
 * carries `slot` at +0x1C, with the heap flags D_800591AC forced to 0 and
 * D_800591AF to 1 around the allocation, and bump D_800C35D8. */
void func_800BB760(u32 slot) {
    u8 saved = D_800591AC;
    u8* task;
    u32 pending;

    D_800591AC = 0;
    D_800591AF = 1;
    task = TimerWorkListAllocateTask(0, 4);
    TimerWorkListSetTaskCallback(task, (void*)(uintptr_t)0x800BB6E0u);
    pending = BG_U32(0x800C35D8u); /* D_800C35D8 */
    *(u32*)(task + 0x1C) = slot;
    D_800591AF = 0;
    BG_U32(0x800C35D8u) = pending + 1;
    D_800591AC = saved;
}
#endif

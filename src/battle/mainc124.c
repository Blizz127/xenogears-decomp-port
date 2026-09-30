#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc124", func_800BED4C);
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


#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#endif
/* func_800BEDE8.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3610[];
#endif
extern u32 D_80059464[];
extern u8 D_800591AC[];
extern void HeapFree(u32 p);
void func_800BEDE8(void) {
#ifdef XENO_PC_PORT
    /* This TU declares D_800C3610 as `u32[]`, but another declares it `u8*`,
     * so the shared guest-RAM alias is pointer-typed: `D_800C3610[0]` reads
     * the byte at the address held in the word, and assigning to it writes
     * that byte instead of the word. Retail does `lw`/`sw` at 0x800C3610
     * itself, which is why the prover saw 0x800C3610..13 left unzeroed.
     * Go through the word directly, as main59.c does for D_800C3DFC. */
    /* The native HeapFree takes a host pointer: translate the guest word the
     * way the bridge's translate_argument() does for the interpreted call
     * (KSEG0 -> g_PsxRam, 0 -> NULL, a raw host address unchanged).  The
     * file-scope extern above is the matching build's u32 view. */
    ((u_int (*)(void*))(void*)HeapFree)(
        bg_arg(*(u32*)PSX_ADDR(0x800C3610)));
    D_80059464[0] = 0;
    *(u32*)PSX_ADDR(0x800C3610) = 0;
    D_800591AC[0] = 0;
#else
    HeapFree(D_800C3610[0]);
    D_80059464[0] = 0;
    D_800C3610[0] = 0;
    D_800591AC[0] = 0;
#endif
}

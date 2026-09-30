#include "common.h"


#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_800B6464.s: for each of (a0[0x40] >> 2) 0x18-byte primitives starting
 * at (*(a0+0x20))->+0x30 + 0xC, add a1[0] to the 6-bit u field and a1[1] to
 * the 9-bit v field packed in the halfword. The counter is a halfword; retail
 * keeps the limit in its own register and masks both fields before adding. */
void func_800B6464(u8* a0, u8* a1) {
    s32 c = a0[0x40] >> 2;
    u16* p = (u16*)(*(u8**)(*(u8**)(a0 + 0x20) + 0x30) + 0xC);

    if (c != 0) {
        s32 n = c;
        s16 i = 0;
        do {
            u32 v;
            u32 u;
            u32 w;

            i++;
            v = *p;
            u = v & 0x3F;
            w = (v >> 6) & 0x1FF;
            *p = (u + a1[0]) | ((w + a1[1]) << 6);
            p += 0xC;
        } while (i != n);
    }
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/mainc89", func_800B6464);
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


/* func_800B64D4.s */
void func_800B64D4(u8* a0, u8* a1) {
    u8* base = (u8*)(u32)D_800C3EB0;
    u8* p = base + (a1[1] << 2) + 0x8C8C;

    func_800245D8(*(u32*)p, a1[0]);
}

#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc85", func_800B59BC);
typedef struct {
    s16 x;
    s16 y;
} Point16;

extern Point16 func_800B59BC(void);

/* func_800B59BC() offset by the object's (+2, +6). */
Point16 func_800B5AC4(s16* obj) {
    Point16 p = func_800B59BC();
    s32 d;

    d = obj[1];
    p.x += d;
    d = obj[3];
    p.y += d;
    return p;
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc85", func_800B5B3C);
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


/* func_800B5C18.s */
u8* func_800B5C18(u8* a0, u8* a1) {
    u8* s0;
    u8* s1 = a0;
    u8* s2 = a1;

    s0 = TimerWorkListAllocateTask(*(s32*)(s1 + 0x6C), 0x18);
    TimerWorkListSetTaskCallback(s0, func_800B5B3C);
    *(u32*)(s0 + 0x1C) = (u32)s1;
    *(s32*)(s0 + 0x20) = *(s32*)(s1 + 0x74);
    *(s32*)(s0 + 0x2C) = *(u16*)(s1 + 0x34);
    *(s32*)(s0 + 0x30) = *(s8*)(s1 + 0xAF);
    *(s32*)(s0 + 0x24) = *(u8*)(s2 + 0) & 0xF;
    *(s32*)(s0 + 0x28) = *(u8*)(s2 + 0) >> 4;
    func_800B5B3C(s0);
    return s0;
}

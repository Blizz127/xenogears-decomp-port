#include "common.h"


#ifndef XENO_PC_PORT
typedef struct {
    s16 x;
    s16 y;
} Point16;

/* func_800B59BC.s, byte-exact: the object's point `index`: entry `index`
 * (8 bytes, s8 x at +0, s8 y at +1) of the table at its +0x20 data's +0x34,
 * x negated when bit 2 of +0xAC is set, both scaled by the s16 at +0x2C
 * (/ 4096).  When the data or the table is missing, or the +0x3C mode
 * (bits 0..1) is not 1, retail returns without writing the result: the
 * body falls off the end there, so the caller's buffer keeps its old value.
 * That is undefined behaviour for a host compiler: a native version needs an
 * explicit form for those paths (e.g. an out-pointer left untouched). */
Point16 func_800B59BC(s16* obj, s32 index) {
    Point16 p;

    if (*(u8**)((u8*)obj + 0x20) != NULL) {
        if ((*(u32*)((u8*)obj + 0x3C) & 3) == 1) {
            if (*(s8**)(*(u8**)((u8*)obj + 0x20) + 0x34) != NULL) {
                p.y = (*(s8**)(*(u8**)((u8*)obj + 0x20) + 0x34))[index * 8 + 1];
                p.x = (*(s8**)(*(u8**)((u8*)obj + 0x20) + 0x34))[index * 8];
                if ((*(u32*)((u8*)obj + 0xAC) >> 2) & 1) {
                    p.x = -p.x;
                }
                p.y = p.y * *(s16*)((u8*)obj + 0x2C) / 4096;
                p.x = p.x * *(s16*)((u8*)obj + 0x2C) / 4096;
                return p;
            }
        }
    }
}

/* func_800B59BC(obj, index) offset by the object's (+2, +6). */
Point16 func_800B5AC4(s16* obj, s32 index) {
    Point16 p = func_800B59BC(obj, index);
    s32 d;

    d = obj[1];
    p.x += d;
    d = obj[3];
    p.y += d;
    return p;
}

/* func_800B5B3C.s, byte-exact: per-frame step of a follow task.  It
 * moves the target (the object's +0x74) by the difference between the
 * object's point func_800B5AC4(obj, task +0x24) and the target's
 * func_800B5AC4(target, task +0x28): x and y as 16.16 adds to the target's
 * +0 and +4.  The task's +0xC callback runs unless the object's state +0xAF
 * still equals task +0x30 and its +0xA8 mode (bits 28..29) is 2 or 3.  The
 * two mode tests are nested ifs; `mode == 0 || mode == 1` folds into one
 * sltiu. */
void func_800B5B3C(u8* task) {
    u8* tgt;
    Point16 a;
    Point16 b;
    Point16 d;
    u32 mode;

    tgt = *(u8**)(*(u8**)(task + 0x1C) + 0x74);
    a = func_800B5AC4(*(s16**)(task + 0x1C), *(s32*)(task + 0x24));
    b = func_800B5AC4((s16*)tgt, *(s32*)(task + 0x28));
    d.x = a.x - b.x;
    d.y = a.y - b.y;
    *(s32*)(tgt + 0) += d.x << 16;
    *(s32*)(tgt + 4) += d.y << 16;
    if (*(s32*)(task + 0x30) == *(s8*)(*(u8**)(task + 0x1C) + 0xAF)) {
        mode = (*(u32*)(*(u8**)(task + 0x1C) + 0xA8) >> 28) & 3;
        if (mode != 0) {
            if (mode != 1) {
                return;
            }
        }
    }
    (*(void (**)(u8*))(task + 0xC))(task);
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

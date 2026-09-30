#include "common.h"


#ifndef XENO_PC_PORT
typedef struct {
    u8 pad[0x44];
    s16 cur;
    s16 target;
    s16 a;
    s16 b;
    s16 c;
    s16 d;
} Obj;

extern void func_800A6444(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5);

/* task->0x04 object: when cur (+0x44) != target (+0x46), cur = target and
 * func_800A6444(0, +0x48, 0x20 - target, +0x4A, +0x4C, +0x4E). */
void func_800B3C74(Obj** task) {
    Obj* w = task[1];

    if (w->cur != w->target) {
        w->cur = w->target;
        func_800A6444(0, w->a, 0x20 - w->target, w->b, w->c, w->d);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B3CD4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B3E04);
INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B3F04);
#include "psyq/libgpu.h"

/* copy an 8x8 texture cell (u = +4 >> 2, v = +5, page from tpage +0xA) to
 * VRAM (0x3F0, 0x1F0) and its 16-entry CLUT (+0xC) to (0x3F0, 0x1EE). */
void func_800B4EDC(u8* task) {
    RECT rect;
    u8* spr = *(u8**)(*(u8**)(task + 0x20) + 0x30);
    u16 tpage = *(u16*)(spr + 0xA);
    u16 clut;

    rect.x = (spr[4] >> 2) + ((tpage & 0xF) << 6);
    rect.y = spr[5] + ((tpage << 4) & 0x100);
    rect.w = 8;
    rect.h = 8;
    MoveImage(&rect, 0x3F0, 0x1F0);
    clut = *(u16*)(spr + 0xC);
    rect.w = 16;
    rect.h = 1;
    rect.x = clut & 0x3F;
    rect.y = (clut >> 6) & 0x1FF;
    MoveImage(&rect, 0x3F0, 0x1EE);
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B4F88);
INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B50D4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B51B0);
INCLUDE_ASM("asm/battle/nonmatchings/mainc83", func_800B5588);
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


/* func_800B56E4.s */
void func_800B56E4(u8* p) {
    u8* q = (u8*)(u32)*(u32*)(p + 4);

    func_8001E148(*(u32*)(q + 0x38));
    func_800C08CC(5, q + 0x78, func_800B51B0);
}

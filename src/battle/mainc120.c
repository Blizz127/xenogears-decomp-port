#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgte.h"

extern u8* D_800D2F5C[];
extern u_long* g_GfxCurOT;
extern void func_80026BA4(u8* p, s32 mode, s16 x, s16 y, u_long* ot);
extern MATRIX D_800C3760;
extern void func_800BD810(u8* e, s32 a1);

/* Draw the task's 0x18-byte entries (+0x68, count +0x64) through
 * func_800BD810 in a matrix built from its rotation (+0x38) and scale
 * (+0x40), screen-centred at (160, 70) * 2 and the projection distance. */
void func_800BDD3C(u8* task) {
    MATRIX m;
    u8 pad[8];
    VECTOR t;
    long ofx;
    long ofy;
    u8* o = *(u8**)(task + 4);
    s32 i;
    u8* e;

    ReadGeomOffset(&ofx, &ofy);
    func_80026BA4(D_800D2F5C[0], 0x81, *(s16*)(o + 0x50), *(s16*)(o + 0x52), g_GfxCurOT);
    i = 0;
    t.vx = (0xA0 - ofx) * 2;
    t.vy = (0x46 - ofy) * 2;
    t.vz = ReadGeomScreen();
    D_800C3760.t[2] = ReadGeomScreen();
    RotMatrix((SVECTOR*)(o + 0x38), &m);
    TransMatrix(&m, &t);
    CompMatrix(&D_800C3760, &m, &m);
    ScaleMatrix(&m, (VECTOR*)(o + 0x40));
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    for (e = o + 0x68; i != *(s32*)(o + 0x64); i++, e += 0x18) {
        func_800BD810(e, *(s32*)(o + 0x60));
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc120", func_800BDE58);
INCLUDE_ASM("asm/battle/nonmatchings/mainc120", func_800BDF1C);
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


/* func_800BE0DC.s */
#ifndef XENO_PC_PORT
extern u32 D_800D2D68[];
#endif
extern void func_800BDCF8(u8* p);
void func_800BE0DC(void) {
    u8* p = (u8*)(u32)D_800D2D68[0];

    if (p != 0) {
        func_800BDCF8(p);
    }
}
/* func_800BE108.s */
#ifndef XENO_PC_PORT
extern u32 D_800D2D68[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C374C[];
#endif
void func_800BE108(void) {
    D_800D2D68[0] = 0;
    D_800C374C[0] = 0;
}

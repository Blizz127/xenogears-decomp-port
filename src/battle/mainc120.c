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

extern u32 D_800D2D68[];
extern u8 D_800C3780;
extern u8 D_800D30EC[];
extern s32 D_800D3680__a[] asm("D_800D3680");
extern void TimerWorkListAddTask(u32 owner, void* pTask);
extern void WorkListAddTask(void* pOwner, void* pTask);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800BDD34(void);
extern void func_800BDCF8(u8* p);

/* func_800BDE58.s: unless the popup task already exists (D_800D2D68) or
 * D_800C3780 is set, install the static task block D_800D30EC: a timer task
 * (func_800BDD34) with a work task at +0x1C (func_800BDD3C) and on-free
 * func_800BDCF8, screen position (0x90, 0x2A), both tasks pointing at the
 * block, no glyphs (+0x64 = 0), and D_800D3680 = -1. */
void func_800BDE58(void) {
    if (D_800D2D68[0] == 0 && D_800C3780 == 0) {
        D_800D2D68[0] = (u32)D_800D30EC;
        TimerWorkListAddTask(0, D_800D30EC);
        WorkListAddTask(D_800D30EC, D_800D30EC + 0x1C);
        TimerWorkListSetTaskCallback(D_800D30EC, func_800BDD34);
        WorkListSetTaskCallback(D_800D30EC + 0x1C, func_800BDD3C);
        WorkListTaskSetOnFreeCallback(D_800D30EC, func_800BDCF8);
        *(s16*)(D_800D30EC + 0x50) = 0x90;
        *(s16*)(D_800D30EC + 0x52) = 0x2A;
        *(u8**)(D_800D30EC + 4) = D_800D30EC;
        *(u8**)(D_800D30EC + 0x20) = D_800D30EC;
        *(s32*)(D_800D30EC + 0x64) = 0;
        D_800D3680__a[0] = -1;
    }
}
extern s32 D_800C3D38[];
extern s32 D_800D3630[];
extern s16 D_800C3752[];
extern void func_800BE330(s32 value);
extern void func_800BE6E8(s32 value, u8* out, s32 digits, u32 lead, u32 bias);
extern s32 func_80026DCC(u8* pTable, s32 index, u8* pPrimBuffer, s16 ofsX, s16 ofsY);

/* func_800BDF1C.s, byte-exact.  Codegen notes:
 * - The +0x63 `&= 0xFE` comes after the three +0x40/+0x44/+0x48 stores.  Those
 *   varying stores drop cse.c's memory entries, so the code byte is re-read
 *   instead of folded into the 0x2D store (retail's sb / lbu / andi / sb).
 * - D_800C3D38 / D_800D3630 are unsized arrays read through [0]: a sized
 *   scalar extern compiles to maspsx's adjacent lui macro, while retail
 *   splits %hi/%lo across registers (the gcc-split form).
 * - The nested if puts the once-set `t` copy in the block that tests
 *   D_800D3680, where it fills the beq delay slot as in retail. */
void func_800BDF1C(void) {
    u8* t;
    s32 v = D_800C3D38[0];
    u8 digits[8];
    s32 x;
    s32 i;
    s32 mode;

    if (D_800D2D68[0] != 0 && D_800C3780 == 0) {
        t = (u8*)D_800D2D68[0];
        if (v != D_800D3680__a[0]) {
            D_800D3680__a[0] = v;
            func_800BE330(v);
            t[0x63] = 0x2D;
            *(s32*)(t + 0x40) = 0x2000;
            *(s32*)(t + 0x44) = 0x2000;
            *(s32*)(t + 0x48) = 0x2000;
            t[0x63] &= 0xFE;
            mode = D_800D3630[0];
            *(s16*)(t + 0x38) = 0;
            *(s16*)(t + 0x3A) = 0;
            *(s16*)(t + 0x3C) = 0;
            switch (mode) {
            case 0xB:
                t[0x60] = 0;
                t[0x61] = 0;
                t[0x62] = 0x80;
                break;
            case 3:
                t[0x60] = 0x80;
                t[0x61] = 0;
                t[0x62] = 0x80;
                break;
            case 2:
                t[0x60] = 0;
                t[0x61] = 0x80;
                t[0x62] = 0;
                break;
            default:
                t[0x60] = 0x80;
                t[0x61] = 0x80;
                t[0x62] = 0x80;
                break;
            }
            func_800BE6E8(v, digits, 5, 0, 0);
            x = D_800C3752[digits[0]];
            *(s32*)(t + 0x64) = 0;
            for (i = 0; i != digits[0]; i++, x += 10) {
                *(s32*)(t + 0x64) += func_80026DCC(D_800D2F5C[0], digits[i + 1] + 0x72,
                                                   t + (*(s32*)(t + 0x64) * 0x18 + 0x68), x, -8);
            }
        }
    }
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

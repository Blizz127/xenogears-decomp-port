#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif


/* func_8007ACDC.s: divide the row cell by the operand (quotient). */
void func_8007ACDC(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);

    row[p[1]] = row[p[1]] / ((p[3] << 8) + p[2]);
}


/* func_8007AD24.s: divide the row cell by the operand (remainder). */
void func_8007AD24(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);

    row[p[1]] = row[p[1]] % ((p[3] << 8) + p[2]);
}


/* func_8007AD6C.s: AND the operand into the row cell. */
void func_8007AD6C(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);

    row[p[1]] &= p[2] + (p[3] << 8);
}


/* func_8007ADB0.s: OR the board's 16-bit operand into the D_800D3420 row cell. */
void func_8007ADB0(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);

    row[p[1]] |= p[2] + (p[3] << 8);
}

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


/* func_8007ADF4.s */
void func_8007ADF4(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));

    row[p[1]] ^= (u16)(p[2] + (p[3] << 8));
}

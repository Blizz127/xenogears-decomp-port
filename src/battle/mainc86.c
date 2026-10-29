#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgte.h"

extern void AnimScriptTick(void* obj);
extern s16 func_80022CAC(u8* obj, s32 a1);

/* func_800B5CC0.s: timer callback -- tick the object's animation script,
 * rotate (func_80022CAC(obj, +0xC >> 13), 0, 0) by the angles
 * (0, +0x10 >> 13, +0x14 >> 13), add the s16 offset at +0xA0, and store the
 * result as 16.16 at +0/+4/+8; when +0x64 is 0, call the task's +0xC
 * callback.  The angle stores go vy, vz, vx; the other five orders are
 * 2-6 instructions off or one instruction longer. */
void func_800B5CC0(u8* task) {
    u8* s = *(u8**)(task + 4);
    MATRIX m;
    SVECTOR v;
    SVECTOR rot;
    VECTOR out;

    AnimScriptTick(s);
    rot.vy = *(s32*)(s + 0x10) >> 13;
    rot.vz = *(s32*)(s + 0x14) >> 13;
    rot.vx = 0;
    v.vx = func_80022CAC(s, *(s32*)(s + 0xC) >> 13);
    v.vy = 0;
    v.vz = 0;
    RotMatrix(&rot, &m);
    ApplyMatrix(&m, &v, &out);
    out.vx += *(s16*)(s + 0xA0);
    out.vy += *(s16*)(s + 0xA2);
    out.vz += *(s16*)(s + 0xA4);
    *(s32*)(s + 0) = out.vx << 16;
    *(s32*)(s + 4) = out.vy << 16;
    *(s32*)(s + 8) = out.vz << 16;
    if (*(s32*)(s + 0x64) == 0) {
        (*(void (**)(u8*))(task + 0xC))(task);
    }
}
#endif


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(u8* task);
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


/* func_800B5DC4.s */
void func_800B5DC4(u8* a0) {
    *(u16*)(a0 + 0x34) = 1;
    TimerWorkListSetTaskCallback((void*)(u32)*(u32*)(a0 + 0x6C), func_800B5CC0);
}

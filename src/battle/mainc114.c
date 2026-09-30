#include "common.h"
/* Retail TU split: gcc 2.7.2 cc1 emits every file-scope INCLUDE_ASM block
 * before the first compiled body, so a mixed TU whose retail layout has a C
 * function ahead of an assembly function cannot be one object.  Each
 * contiguous [asm run][C run] of the retail layout is therefore its own
 * matching TU: src/battle/mainc114_p<N>.c defines BATTLE_TU_PART=<N> and
 * includes this file.  Host/port builds compile this file whole (part 0). */
#ifndef BATTLE_TU_PART
#define BATTLE_TU_PART 0
#endif
#define BATTLE_PART(n) (BATTLE_TU_PART == 0 || BATTLE_TU_PART == (n))


#if BATTLE_PART(1)
#if defined(XENO_PC_PORT) && !defined(XENO_BATTLE_VIEW_MATRIX_STAGED)
/* Native SDK composition is tested only via the explicit staged build;
 * runtime guest-pointer admission and BC460 integration remain pending. */
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p1", func_800BB844);
#else
#include "psyq/libgte.h"
extern void func_80021B14(void*, s32, s32, s32);
#ifdef XENO_PC_PORT
extern long VectorNormal(VECTOR*, VECTOR*);
extern void OuterProduct12(VECTOR*, VECTOR*, VECTOR*);
#endif
/* Retail BB844: right/up/forward rows, then negative transformed eye.
 * Unsigned subtraction preserves NEGU for every 32-bit ApplyMatrix result. */
void func_800BB844(MATRIX* matrix, SVECTOR* eye, SVECTOR* target, SVECTOR* up) {
    VECTOR temp, forward, right, vertical;
    func_80021B14(&temp, target->vx-eye->vx, target->vy-eye->vy,
                 target->vz-eye->vz);
    vertical.vx = up->vx;
    vertical.vy = up->vy;
    vertical.vz = up->vz;
    VectorNormal(&temp, &forward);
    OuterProduct12(&vertical, &forward, &temp);
    VectorNormal(&temp, &right);
    OuterProduct12(&forward, &right, &temp);
    VectorNormal(&temp, &vertical);
    matrix->m[0][0] = right.vx;
    matrix->m[0][1] = right.vy;
    matrix->m[0][2] = right.vz;
    matrix->m[1][0] = vertical.vx;
    matrix->m[1][1] = vertical.vy;
    matrix->m[1][2] = vertical.vz;
    matrix->m[2][0] = forward.vx;
    matrix->m[2][1] = forward.vy;
    matrix->m[2][2] = forward.vz;
    PushMatrix();
    ApplyMatrix(matrix, eye, &temp);
    matrix->t[0] = 0u - (u32)temp.vx;
    matrix->t[1] = 0u - (u32)temp.vy;
    matrix->t[2] = 0u - (u32)temp.vz;
    PopMatrix();
}
#endif
#endif /* BATTLE_PART(1) */
#if BATTLE_PART(2)
#endif /* BATTLE_PART(2) */
#if BATTLE_PART(2)
#ifdef XENO_PC_PORT
/* PC adoption uses the explicit resolver/frame API in battle_target_setup.c;
 * retail task pointers below must not become unchecked native pointers. */
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc114_p2", func_800BC2F0);
#endif
#else
/* Only the callback-bearing prefix of the retail 0x1C task is accessed. */
typedef struct BattleSetupTask {
    u32 owner, payload, tick;
    void (*on_free)(struct BattleSetupTask*);
} BattleSetupTask;
typedef struct BattleSetupShortVector { s16 x, y, z, pad; } BattleSetupShortVector;
typedef struct BattleSetupVector { s32 x, y, z, pad; } BattleSetupVector;
STATIC_ASSERT_SIZEOF(BattleSetupTask, 0x10);
STATIC_ASSERT_SIZEOF(BattleSetupShortVector, 8);
STATIC_ASSERT_SIZEOF(BattleSetupVector, 0x10);
static inline void BattleSetupExpandVector(BattleSetupShortVector* src,
                                          BattleSetupVector* dst) {
    dst->x = (u32)src->x << 16;
    dst->y = (u32)src->y << 16;
    dst->z = (u32)src->z << 16;
}
#ifndef XENO_PC_PORT
extern u32 D_800C3CC0[];
#endif
#ifndef XENO_PC_PORT
extern s32 D_800C3CBC[];
#endif
#ifndef XENO_PC_PORT
extern BattleSetupTask* D_800C3680[];
#endif
#ifndef XENO_PC_PORT
extern BattleSetupTask* D_800C3684[];
#endif
#ifndef XENO_PC_PORT
extern BattleSetupShortVector D_800D30A0[];
#endif
extern BattleSetupVector D_8006F99C[], D_8006F9AC[];
void func_800BC2F0(u32 mode) {
    D_800C3CC0[0] = mode;
    D_800C3CBC[0] = 1;
    switch (mode) {
    case 4:
        D_800C3CBC[0] = 5;
        break;
    case 2: {
        BattleSetupExpandVector(D_800D30A0, D_8006F99C);
        BattleSetupExpandVector(D_800D30A0 + 1, D_8006F9AC);
        break;
    }
    default: {
        BattleSetupTask* task = D_800C3680[0];
        if (task != 0) {
            task->on_free(task);
            D_800C3680[0] = 0;
        }
        task = D_800C3684[0];
        if (task != 0) {
            task->on_free(task);
            D_800C3684[0] = 0;
        }
        break;
    }
    }
}
#endif
#endif /* BATTLE_PART(2) */


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


#if BATTLE_PART(2)
/* func_800BC3F8.s */
#ifndef XENO_PC_PORT
extern u32 D_800C367C[];
#endif
void func_800BC3F8(u32 v) {
    D_800C367C[0] = v;
}
#endif /* BATTLE_PART(2) */
#if BATTLE_PART(2)
/* func_800BC404.s */
#ifndef XENO_PC_PORT
extern u8 D_800C37C8[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C3CDC[];
#endif
extern u16 D_80059454[];
extern void func_800BC2F0(u32 v);
extern void func_800BC460(u32 mask);
void func_800BC404(u32 mask) {
    if (D_800C37C8[0] == 0) {
        func_800BC2F0(1);
        func_800BC460(mask);
    }
    D_80059454[0] = D_800C3CDC[0];
}
#endif /* BATTLE_PART(2) */
#if BATTLE_PART(2)
/* func_800BC454.s */
#ifndef XENO_PC_PORT
extern u16 D_800C3740[];
#endif
void func_800BC454(u16 v) {
    D_800C3740[0] = v;
}
#endif /* BATTLE_PART(2) */

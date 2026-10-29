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

#if defined(XENO_PC_PORT) && !defined(XENO_BATTLE_VIEW_MATRIX_STAGED) /* h-hotpath lifted bodies */
/* Instruction-faithful port bodies for the hybrid hot path, generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N --void --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own jal
 * path, so each callee gets the binding an interpreted jal gets).  The
 * retail matched C stays the matching build's: 800BB844 here (part 1),
 * 800BD810 in mainc118.c.  In the native-only build (and the hybrid with
 * XENO_BATTLE_NATIVE_HOSTS=1) the identity-checked target-camera host owns
 * 800BB844 and 800BC460 first (target== hooks), so there these lifts are
 * shadowed; they serve the hybrid default (the shipped build).  Batch 4's
 * main2 bodies (80075168, 80075938, 800745EC and its leaves 800743A4,
 * 800744BC, 80074554) have no host claim: the
 * phase-chain host reaches 80075938/800745EC through the bridge, so they
 * serve both builds.
 * XENO_BATTLE_VIEW_MATRIX_STAGED (run_battle_view_matrix_native_test.sh and
 * run_battle_target_camera_test.sh, -Wall -Wextra -Werror, no runtime link)
 * compiles the part-1 matched C of 800BB844 instead and none of this block.
 * No port binary is built with it: its 800BB844 takes host pointers, which
 * the W32 thunk row in battle_guest_abi_thunks.inc does not describe. */
#include "battle_lift.h"

/* func_800BB844.s (100 instructions; --args 4 --void), hot path batch 2: the
 * battle view matrix (right/up/forward rows from eye, target and up, then the
 * negated transformed eye).  Callees through the jal service: ApplyMatrix,
 * OuterProduct12, PopMatrix, PushMatrix, VectorNormal, func_80021B14. */
void func_800BB844(u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    static const char FN[] = "func_800BB844";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x68u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    r_sp = r_sp + 0xFFFFFF98u; /* 800BB844 addiu $sp, $sp, -0x68 */
    bl_sw(r_sp + 0x54u, r_s1); /* 800BB848 sw $s1, 0x54($sp) */
    r_s1 = r_a0 + 0u; /* 800BB84C addu $s1, $a0, $zero */
    bl_sw(r_sp + 0x60u, r_s4); /* 800BB850 sw $s4, 0x60($sp) */
    r_s4 = r_a1 + 0u; /* 800BB854 addu $s4, $a1, $zero */
    bl_sw(r_sp + 0x50u, r_s0); /* 800BB858 sw $s0, 0x50($sp) */
    r_s0 = r_a3 + 0u; /* 800BB85C addu $s0, $a3, $zero */
    r_a0 = r_sp + 0x10u; /* 800BB860 addiu $a0, $sp, 0x10 */
    bl_sw(r_sp + 0x64u, r_ra); /* 800BB864 sw $ra, 0x64($sp) */
    bl_sw(r_sp + 0x5Cu, r_s3); /* 800BB868 sw $s3, 0x5C($sp) */
    bl_sw(r_sp + 0x58u, r_s2); /* 800BB86C sw $s2, 0x58($sp) */
    r_v0 = bl_lh(r_a2); /* 800BB870 lh $v0, 0x0($a2) */
    r_a1 = bl_lh(r_s4); /* 800BB874 lh $a1, 0x0($s4) */
    r_t0 = bl_lh(r_a2 + 0x2u); /* 800BB878 lh $t0, 0x2($a2) */
    r_v1 = bl_lh(r_a2 + 0x4u); /* 800BB87C lh $v1, 0x4($a2) */
    r_a3 = bl_lh(r_s4 + 0x4u); /* 800BB880 lh $a3, 0x4($s4) */
    r_a1 = r_v0 - r_a1; /* 800BB884 subu $a1, $v0, $a1 */
    r_v0 = bl_lh(r_s4 + 0x2u); /* 800BB888 lh $v0, 0x2($s4) */
    r_a3 = r_v1 - r_a3; /* 800BB88C subu $a3, $v1, $a3 */
    /* 800BB890 jal func_80021B14 */
    r_a2 = r_t0 - r_v0;
    bl_jal(FN, 0x80021B14u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80021B14 */
    r_v0 = bl_lh(r_s0); /* 800BB898 lh $v0, 0x0($s0) */
    r_a0 = r_sp + 0x10u; /* 800BB89C addiu $a0, $sp, 0x10 */
    bl_sw(r_sp + 0x40u, r_v0); /* 800BB8A0 sw $v0, 0x40($sp) */
    r_v0 = bl_lh(r_s0 + 0x2u); /* 800BB8A4 lh $v0, 0x2($s0) */
    r_s2 = r_sp + 0x20u; /* 800BB8A8 addiu $s2, $sp, 0x20 */
    bl_sw(r_sp + 0x44u, r_v0); /* 800BB8AC sw $v0, 0x44($sp) */
    r_v0 = bl_lh(r_s0 + 0x4u); /* 800BB8B0 lh $v0, 0x4($s0) */
    r_a1 = r_s2 + 0u; /* 800BB8B4 addu $a1, $s2, $zero */
    /* 800BB8B8 jal VectorNormal */
    bl_sw(r_sp + 0x48u, r_v0);
    bl_jal(FN, 0x80048D7Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* VectorNormal */
    r_s3 = r_sp + 0x40u; /* 800BB8C0 addiu $s3, $sp, 0x40 */
    r_a0 = r_s3 + 0u; /* 800BB8C4 addu $a0, $s3, $zero */
    r_a1 = r_s2 + 0u; /* 800BB8C8 addu $a1, $s2, $zero */
    /* 800BB8CC jal OuterProduct12 */
    r_a2 = r_sp + 0x10u;
    bl_jal(FN, 0x8004A480u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* OuterProduct12 */
    r_a0 = r_sp + 0x10u; /* 800BB8D4 addiu $a0, $sp, 0x10 */
    r_s0 = r_sp + 0x30u; /* 800BB8D8 addiu $s0, $sp, 0x30 */
    /* 800BB8DC jal VectorNormal */
    r_a1 = r_s0 + 0u;
    bl_jal(FN, 0x80048D7Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* VectorNormal */
    r_a0 = r_s2 + 0u; /* 800BB8E4 addu $a0, $s2, $zero */
    r_a1 = r_s0 + 0u; /* 800BB8E8 addu $a1, $s0, $zero */
    /* 800BB8EC jal OuterProduct12 */
    r_a2 = r_sp + 0x10u;
    bl_jal(FN, 0x8004A480u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* OuterProduct12 */
    r_a0 = r_sp + 0x10u; /* 800BB8F4 addiu $a0, $sp, 0x10 */
    /* 800BB8F8 jal VectorNormal */
    r_a1 = r_s3 + 0u;
    bl_jal(FN, 0x80048D7Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* VectorNormal */
    r_v0 = bl_lhu(r_sp + 0x30u); /* 800BB900 lhu $v0, 0x30($sp) */
    ; /* 800BB904 nop  */
    bl_sh(r_s1, r_v0); /* 800BB908 sh $v0, 0x0($s1) */
    r_v0 = bl_lhu(r_sp + 0x34u); /* 800BB90C lhu $v0, 0x34($sp) */
    ; /* 800BB910 nop  */
    bl_sh(r_s1 + 0x2u, r_v0); /* 800BB914 sh $v0, 0x2($s1) */
    r_v0 = bl_lhu(r_sp + 0x38u); /* 800BB918 lhu $v0, 0x38($sp) */
    ; /* 800BB91C nop  */
    bl_sh(r_s1 + 0x4u, r_v0); /* 800BB920 sh $v0, 0x4($s1) */
    r_v0 = bl_lhu(r_sp + 0x40u); /* 800BB924 lhu $v0, 0x40($sp) */
    ; /* 800BB928 nop  */
    bl_sh(r_s1 + 0x6u, r_v0); /* 800BB92C sh $v0, 0x6($s1) */
    r_v0 = bl_lhu(r_sp + 0x44u); /* 800BB930 lhu $v0, 0x44($sp) */
    ; /* 800BB934 nop  */
    bl_sh(r_s1 + 0x8u, r_v0); /* 800BB938 sh $v0, 0x8($s1) */
    r_v0 = bl_lhu(r_sp + 0x48u); /* 800BB93C lhu $v0, 0x48($sp) */
    ; /* 800BB940 nop  */
    bl_sh(r_s1 + 0xAu, r_v0); /* 800BB944 sh $v0, 0xA($s1) */
    r_v0 = bl_lhu(r_sp + 0x20u); /* 800BB948 lhu $v0, 0x20($sp) */
    ; /* 800BB94C nop  */
    bl_sh(r_s1 + 0xCu, r_v0); /* 800BB950 sh $v0, 0xC($s1) */
    r_v0 = bl_lhu(r_sp + 0x24u); /* 800BB954 lhu $v0, 0x24($sp) */
    ; /* 800BB958 nop  */
    bl_sh(r_s1 + 0xEu, r_v0); /* 800BB95C sh $v0, 0xE($s1) */
    r_v0 = bl_lhu(r_sp + 0x28u); /* 800BB960 lhu $v0, 0x28($sp) */
    /* 800BB964 jal PushMatrix */
    bl_sh(r_s1 + 0x10u, r_v0);
    bl_jal(FN, 0x8004960Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* PushMatrix */
    r_a0 = r_s1 + 0u; /* 800BB96C addu $a0, $s1, $zero */
    r_a1 = r_s4 + 0u; /* 800BB970 addu $a1, $s4, $zero */
    /* 800BB974 jal ApplyMatrix */
    r_a2 = r_sp + 0x10u;
    bl_jal(FN, 0x80049CECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ApplyMatrix */
    r_v0 = bl_lw(r_sp + 0x10u); /* 800BB97C lw $v0, 0x10($sp) */
    ; /* 800BB980 nop  */
    r_v0 = 0u - r_v0; /* 800BB984 negu $v0, $v0 */
    bl_sw(r_s1 + 0x14u, r_v0); /* 800BB988 sw $v0, 0x14($s1) */
    r_v0 = bl_lw(r_sp + 0x14u); /* 800BB98C lw $v0, 0x14($sp) */
    ; /* 800BB990 nop  */
    r_v0 = 0u - r_v0; /* 800BB994 negu $v0, $v0 */
    bl_sw(r_s1 + 0x18u, r_v0); /* 800BB998 sw $v0, 0x18($s1) */
    r_v0 = bl_lw(r_sp + 0x18u); /* 800BB99C lw $v0, 0x18($sp) */
    ; /* 800BB9A0 nop  */
    r_v0 = 0u - r_v0; /* 800BB9A4 negu $v0, $v0 */
    /* 800BB9A8 jal PopMatrix */
    bl_sw(r_s1 + 0x1Cu, r_v0);
    bl_jal(FN, 0x800496ACu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* PopMatrix */
    r_ra = bl_lw(r_sp + 0x64u); /* 800BB9B0 lw $ra, 0x64($sp) */
    r_s4 = bl_lw(r_sp + 0x60u); /* 800BB9B4 lw $s4, 0x60($sp) */
    r_s3 = bl_lw(r_sp + 0x5Cu); /* 800BB9B8 lw $s3, 0x5C($sp) */
    r_s2 = bl_lw(r_sp + 0x58u); /* 800BB9BC lw $s2, 0x58($sp) */
    r_s1 = bl_lw(r_sp + 0x54u); /* 800BB9C0 lw $s1, 0x54($sp) */
    r_s0 = bl_lw(r_sp + 0x50u); /* 800BB9C4 lw $s0, 0x50($sp) */
    r_sp = r_sp + 0x68u; /* 800BB9C8 addiu $sp, $sp, 0x68 */
    /* 800BB9CC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800BD810.s (89 instructions; --args 2 --void), hot path batch 2: emit
 * one textured sprite quad (POLY_FT4, 9 words) for a glyph record in an RGB
 * word, projected through the current GTE matrices, linked into the current
 * OT.  Callee through the jal service: RotTransPers4. */
void func_800BD810(u32 arg0, u32 arg1) {
    static const char FN[] = "func_800BD810";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x40u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_sp = r_sp + 0xFFFFFFC0u; /* 800BD810 addiu $sp, $sp, -0x40 */
    bl_sw(r_sp + 0x34u, r_s1); /* 800BD814 sw $s1, 0x34($sp) */
    r_s1 = r_a0 + 0u; /* 800BD818 addu $s1, $a0, $zero */
    r_a0 = 0x80060000u; /* 800BD81C lui $a0, %hi(g_GfxCurWorkBuffer) */
    r_v0 = 0x80060000u; /* 800BD820 lui $v0, %hi(g_GfxCurWorkBufferEnd) */
    bl_sw(r_sp + 0x30u, r_s0); /* 800BD824 sw $s0, 0x30($sp) */
    r_s0 = bl_lw(r_a0 + 0xFFFF9580u); /* 800BD828 lw $s0, %lo(g_GfxCurWorkBuffer)($a0) */
    r_v0 = bl_lw(r_v0 + 0xFFFF9534u); /* 800BD82C lw $v0, %lo(g_GfxCurWorkBufferEnd)($v0) */
    r_v1 = r_s0 + 0x28u; /* 800BD830 addiu $v1, $s0, 0x28 */
    r_v0 = (u32)(r_v1 < r_v0); /* 800BD834 sltu $v0, $v1, $v0 */
    /* 800BD838 beqz $v0, .L800BD95C */
    { int c_ = (r_v0 == 0); bl_sw(r_sp + 0x38u, r_ra); if (c_) goto L_800BD95C; }
    r_v0 = 0u + 0x9u; /* 800BD840 addiu $v0, $zero, 0x9 */
    bl_sw(r_a0 + 0xFFFF9580u, r_v1); /* 800BD844 sw $v1, %lo(g_GfxCurWorkBuffer)($a0) */
    bl_sb(r_s0 + 0x3u, r_v0); /* 800BD848 sb $v0, 0x3($s0) */
    bl_sw(r_s0 + 0x4u, r_a1); /* 800BD84C sw $a1, 0x4($s0) */
    r_v0 = bl_lhu(r_s1 + 0xAu); /* 800BD850 lhu $v0, 0xA($s1) */
    ; /* 800BD854 nop  */
    bl_sh(r_s0 + 0x16u, r_v0); /* 800BD858 sh $v0, 0x16($s0) */
    r_v0 = bl_lhu(r_s1 + 0xCu); /* 800BD85C lhu $v0, 0xC($s1) */
    r_a0 = 0x80050000u; /* 800BD860 lui $a0, %hi(D_8004FB98) */
    bl_sh(r_s0 + 0xEu, r_v0); /* 800BD864 sh $v0, 0xE($s0) */
    r_a1 = bl_lbu(r_s1 + 0x6u); /* 800BD868 lbu $a1, 0x6($s1) */
    r_a3 = bl_lbu(r_s1 + 0x7u); /* 800BD86C lbu $a3, 0x7($s1) */
    r_a2 = bl_lhu(r_s1); /* 800BD870 lhu $a2, 0x0($s1) */
    r_v1 = bl_lhu(r_s1 + 0x2u); /* 800BD874 lhu $v1, 0x2($s1) */
    r_v0 = r_s0 + 0x8u; /* 800BD878 addiu $v0, $s0, 0x8 */
    bl_sw(r_sp + 0x10u, r_v0); /* 800BD87C sw $v0, 0x10($sp) */
    r_v0 = r_s0 + 0x10u; /* 800BD880 addiu $v0, $s0, 0x10 */
    bl_sw(r_sp + 0x14u, r_v0); /* 800BD884 sw $v0, 0x14($sp) */
    r_v0 = r_s0 + 0x20u; /* 800BD888 addiu $v0, $s0, 0x20 */
    bl_sw(r_sp + 0x18u, r_v0); /* 800BD88C sw $v0, 0x18($sp) */
    r_v0 = r_s0 + 0x18u; /* 800BD890 addiu $v0, $s0, 0x18 */
    bl_sw(r_sp + 0x1Cu, r_v0); /* 800BD894 sw $v0, 0x1C($sp) */
    r_v0 = r_sp + 0x28u; /* 800BD898 addiu $v0, $sp, 0x28 */
    bl_sw(r_sp + 0x20u, r_v0); /* 800BD89C sw $v0, 0x20($sp) */
    r_v0 = r_sp + 0x2Cu; /* 800BD8A0 addiu $v0, $sp, 0x2C */
    bl_sw(r_sp + 0x24u, r_v0); /* 800BD8A4 sw $v0, 0x24($sp) */
    bl_sh(r_a0 + 0xFFFFFB98u, r_a2); /* 800BD8A8 sh $a2, %lo(D_8004FB98)($a0) */
    r_a0 = r_a0 + 0xFFFFFB98u; /* 800BD8AC addiu $a0, $a0, %lo(D_8004FB98) */
    r_a1 = r_a2 + r_a1; /* 800BD8B0 addu $a1, $a2, $a1 */
    bl_sh(r_a0 + 0x2u, r_v1); /* 800BD8B4 sh $v1, 0x2($a0) */
    bl_sh(r_a0 + 0xAu, r_v1); /* 800BD8B8 sh $v1, 0xA($a0) */
    r_v1 = r_v1 + r_a3; /* 800BD8BC addu $v1, $v1, $a3 */
    bl_sh(r_a0 + 0x8u, r_a1); /* 800BD8C0 sh $a1, 0x8($a0) */
    bl_sh(r_a0 + 0x10u, r_a1); /* 800BD8C4 sh $a1, 0x10($a0) */
    r_a1 = r_a0 + 0x8u; /* 800BD8C8 addiu $a1, $a0, 0x8 */
    bl_sh(r_a0 + 0x18u, r_a2); /* 800BD8CC sh $a2, 0x18($a0) */
    r_a2 = r_a0 + 0x10u; /* 800BD8D0 addiu $a2, $a0, 0x10 */
    r_a3 = r_a0 + 0x18u; /* 800BD8D4 addiu $a3, $a0, 0x18 */
    bl_sh(r_a0 + 0x12u, r_v1); /* 800BD8D8 sh $v1, 0x12($a0) */
    /* 800BD8DC jal RotTransPers4 */
    bl_sh(r_a0 + 0x1Au, r_v1);
    bl_jal(FN, 0x8004A73Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotTransPers4 */
    r_a1 = 0xFF0000u; /* 800BD8E4 lui $a1, (0xFFFFFF >> 16) */
    r_a1 = r_a1 | 0xFFFFu; /* 800BD8E8 ori $a1, $a1, (0xFFFFFF & 0xFFFF) */
    r_a0 = bl_lbu(r_s1 + 0x4u); /* 800BD8EC lbu $a0, 0x4($s1) */
    r_v0 = bl_lbu(r_s1 + 0x5u); /* 800BD8F0 lbu $v0, 0x5($s1) */
    r_v1 = bl_lbu(r_s1 + 0x6u); /* 800BD8F4 lbu $v1, 0x6($s1) */
    r_a2 = bl_lbu(r_s1 + 0x7u); /* 800BD8F8 lbu $a2, 0x7($s1) */
    r_v1 = r_a0 + r_v1; /* 800BD8FC addu $v1, $a0, $v1 */
    bl_sb(r_s0 + 0xDu, r_v0); /* 800BD900 sb $v0, 0xD($s0) */
    bl_sb(r_s0 + 0x15u, r_v0); /* 800BD904 sb $v0, 0x15($s0) */
    r_v0 = r_v0 + r_a2; /* 800BD908 addu $v0, $v0, $a2 */
    bl_sb(r_s0 + 0x1Du, r_v0); /* 800BD90C sb $v0, 0x1D($s0) */
    bl_sb(r_s0 + 0x25u, r_v0); /* 800BD910 sb $v0, 0x25($s0) */
    r_v0 = 0x80060000u; /* 800BD914 lui $v0, %hi(g_GfxCurOT) */
    r_a2 = 0xFF000000u; /* 800BD918 lui $a2, (0xFF000000 >> 16) */
    bl_sb(r_s0 + 0xCu, r_a0); /* 800BD91C sb $a0, 0xC($s0) */
    bl_sb(r_s0 + 0x14u, r_v1); /* 800BD920 sb $v1, 0x14($s0) */
    bl_sb(r_s0 + 0x1Cu, r_a0); /* 800BD924 sb $a0, 0x1C($s0) */
    bl_sb(r_s0 + 0x24u, r_v1); /* 800BD928 sb $v1, 0x24($s0) */
    r_a0 = bl_lw(r_v0 + 0xFFFF956Cu); /* 800BD92C lw $a0, %lo(g_GfxCurOT)($v0) */
    r_v1 = bl_lw(r_s0); /* 800BD930 lw $v1, 0x0($s0) */
    r_v0 = bl_lw(r_a0); /* 800BD934 lw $v0, 0x0($a0) */
    r_v1 = r_v1 & r_a2; /* 800BD938 and $v1, $v1, $a2 */
    r_v0 = r_v0 & r_a1; /* 800BD93C and $v0, $v0, $a1 */
    r_v1 = r_v1 | r_v0; /* 800BD940 or $v1, $v1, $v0 */
    bl_sw(r_s0, r_v1); /* 800BD944 sw $v1, 0x0($s0) */
    r_v0 = bl_lw(r_a0); /* 800BD948 lw $v0, 0x0($a0) */
    r_a1 = r_s0 & r_a1; /* 800BD94C and $a1, $s0, $a1 */
    r_v0 = r_v0 & r_a2; /* 800BD950 and $v0, $v0, $a2 */
    r_v0 = r_v0 | r_a1; /* 800BD954 or $v0, $v0, $a1 */
    bl_sw(r_a0, r_v0); /* 800BD958 sw $v0, 0x0($a0) */
L_800BD95C:
    r_ra = bl_lw(r_sp + 0x38u); /* 800BD95C lw $ra, 0x38($sp) */
    r_s1 = bl_lw(r_sp + 0x34u); /* 800BD960 lw $s1, 0x34($sp) */
    r_s0 = bl_lw(r_sp + 0x30u); /* 800BD964 lw $s0, 0x30($sp) */
    r_sp = r_sp + 0x40u; /* 800BD968 addiu $sp, $sp, 0x40 */
    /* 800BD96C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
/* func_800BC460.s (401 instructions; --args 1 --void), hot path batch 3:
 * the battle target-chain camera.  For the targets in the mask (D_800C3EB0
 * rows/positions) it centres the view between them, builds the view matrix
 * (800BB844), projects every target -- and its head when flagged -- and
 * sets the camera distance (D_800C3CDC) and eye/target (D_800D30A0) from the
 * widest projection.  Callees through the jal service: memset (the bridge's
 * typed C-library path), RotMatrixZYX, ReadGeomScreen, ApplyMatrix,
 * func_800BB844, SetRotMatrix, SetTransMatrix, RotTransPers, SquareRoot0.
 * The retail matched C stays mainc115.c's (matching build only). */
void func_800BC460(u32 arg0) {
    static const char FN[] = "func_800BC460";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x120u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFEE0u; /* 800BC460 addiu $sp, $sp, -0x120 */
    bl_sw(r_sp + 0x104u, r_s3); /* 800BC464 sw $s3, 0x104($sp) */
    r_s3 = r_a0 + 0u; /* 800BC468 addu $s3, $a0, $zero */
    r_a0 = r_sp + 0x10u; /* 800BC46C addiu $a0, $sp, 0x10 */
    r_a1 = 0u + 0u; /* 800BC470 addu $a1, $zero, $zero */
    r_a2 = 0u + 0x10u; /* 800BC474 addiu $a2, $zero, 0x10 */
    bl_sw(r_sp + 0x11Cu, r_ra); /* 800BC478 sw $ra, 0x11C($sp) */
    bl_sw(r_sp + 0x118u, r_fp); /* 800BC47C sw $fp, 0x118($sp) */
    bl_sw(r_sp + 0x114u, r_s7); /* 800BC480 sw $s7, 0x114($sp) */
    bl_sw(r_sp + 0x110u, r_s6); /* 800BC484 sw $s6, 0x110($sp) */
    bl_sw(r_sp + 0x10Cu, r_s5); /* 800BC488 sw $s5, 0x10C($sp) */
    bl_sw(r_sp + 0x108u, r_s4); /* 800BC48C sw $s4, 0x108($sp) */
    bl_sw(r_sp + 0x100u, r_s2); /* 800BC490 sw $s2, 0x100($sp) */
    bl_sw(r_sp + 0xFCu, r_s1); /* 800BC494 sw $s1, 0xFC($sp) */
    /* 800BC498 jal memset */
    bl_sw(r_sp + 0xF8u, r_s0);
    bl_jal(FN, 0x8003FA08u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* memset */
    r_s5 = 0u + 0u; /* 800BC4A0 addu $s5, $zero, $zero */
    r_v0 = 0x800C0000u; /* 800BC4A4 lui $v0, %hi(D_800C3678) */
    r_s2 = r_s5 + 0u; /* 800BC4A8 addu $s2, $s5, $zero */
    r_a2 = r_s2 + 0u; /* 800BC4AC addu $a2, $s2, $zero */
    r_s1 = r_s3 + 0u; /* 800BC4B0 addu $s1, $s3, $zero */
    r_t0 = 0u | 0x8C8Cu; /* 800BC4B4 ori $t0, $zero, 0x8C8C */
    r_a3 = 0u + 0xBu; /* 800BC4B8 addiu $a3, $zero, 0xB */
    bl_sw(r_v0 + 0x3678u, r_s3); /* 800BC4BC sw $s3, %lo(D_800C3678)($v0) */
    r_v0 = 0x800C0000u; /* 800BC4C0 lui $v0, %hi(D_800C3EB0) */
    r_a0 = r_v0 + 0x3EB0u; /* 800BC4C4 addiu $a0, $v0, %lo(D_800C3EB0) */
    r_a1 = r_a0 + 0u; /* 800BC4C8 addu $a1, $a0, $zero */
L_800BC4CC:
    r_v0 = r_s1 & 0x1u; /* 800BC4CC andi $v0, $s1, 0x1 */
    /* 800BC4D0 beqz $v0, .L800BC538 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800BC538; }
    r_v0 = bl_lbu(r_a0 + 0x7u); /* 800BC4D8 lbu $v0, 0x7($a0) */
    ; /* 800BC4DC nop  */
    /* 800BC4E0 bnez $v0, .L800BC538 */
    { int c_ = (r_v0 != 0); r_v0 = r_a1 + r_t0; if (c_) goto L_800BC538; }
    r_s0 = bl_lw(r_v0); /* 800BC4E8 lw $s0, 0x0($v0) */
    ; /* 800BC4EC nop  */
    /* 800BC4F0 beqz $s0, .L800BC538 */
    { int c_ = (r_s0 == 0); ; if (c_) goto L_800BC538; }
    r_a2 = r_a2 + 0x1u; /* 800BC4F8 addiu $a2, $a2, 0x1 */
    r_v0 = bl_lw(r_s0); /* 800BC4FC lw $v0, 0x0($s0) */
    r_v1 = bl_lw(r_sp + 0x10u); /* 800BC500 lw $v1, 0x10($sp) */
    r_v0 = BL_SRA(r_v0, 1); /* 800BC504 sra $v0, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 800BC508 addu $v1, $v1, $v0 */
    bl_sw(r_sp + 0x10u, r_v1); /* 800BC50C sw $v1, 0x10($sp) */
    r_v0 = bl_lw(r_s0 + 0x4u); /* 800BC510 lw $v0, 0x4($s0) */
    r_v1 = bl_lw(r_sp + 0x14u); /* 800BC514 lw $v1, 0x14($sp) */
    r_v0 = BL_SRA(r_v0, 1); /* 800BC518 sra $v0, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 800BC51C addu $v1, $v1, $v0 */
    bl_sw(r_sp + 0x14u, r_v1); /* 800BC520 sw $v1, 0x14($sp) */
    r_v0 = bl_lw(r_s0 + 0x8u); /* 800BC524 lw $v0, 0x8($s0) */
    r_v1 = bl_lw(r_sp + 0x18u); /* 800BC528 lw $v1, 0x18($sp) */
    r_v0 = BL_SRA(r_v0, 1); /* 800BC52C sra $v0, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 800BC530 addu $v1, $v1, $v0 */
    bl_sw(r_sp + 0x18u, r_v1); /* 800BC534 sw $v1, 0x18($sp) */
L_800BC538:
    r_a1 = r_a1 + 0x4u; /* 800BC538 addiu $a1, $a1, 0x4 */
    r_a0 = r_a0 + 0x1Cu; /* 800BC53C addiu $a0, $a0, 0x1C */
    r_s2 = r_s2 + 0x1u; /* 800BC540 addiu $s2, $s2, 0x1 */
    r_v0 = r_s1 & 0xFFFFu; /* 800BC544 andi $v0, $s1, 0xFFFF */
    /* 800BC548 bne $s2, $a3, .L800BC4CC */
    { int c_ = (r_s2 != r_a3); r_s1 = r_v0 >> 1; if (c_) goto L_800BC4CC; }
    /* 800BC550 beqz $a2, .L800BCA70 */
    { int c_ = (r_a2 == 0); r_s2 = 0u + 0u; if (c_) goto L_800BCA70; }
    r_v1 = bl_lw(r_sp + 0x10u); /* 800BC558 lw $v1, 0x10($sp) */
    ; /* 800BC55C nop  */
    BL_DIV(r_v1, r_a2); /* 800BC560 div $zero, $v1, $a2 */
    r_v1 = lo; /* 800BC564 mflo $v1 */
    r_a0 = bl_lw(r_sp + 0x18u); /* 800BC568 lw $a0, 0x18($sp) */
    ; /* 800BC56C nop  */
    BL_DIV(r_a0, r_a2); /* 800BC570 div $zero, $a0, $a2 */
    r_a0 = lo; /* 800BC574 mflo $a0 */
    r_a1 = bl_lw(r_sp + 0x14u); /* 800BC578 lw $a1, 0x14($sp) */
    ; /* 800BC57C nop  */
    BL_DIV(r_a1, r_a2); /* 800BC580 div $zero, $a1, $a2 */
    r_a1 = lo; /* 800BC584 mflo $a1 */
    r_s1 = r_s3 + 0u; /* 800BC588 addu $s1, $s3, $zero */
    r_t6 = 0u | 0x8C8Cu; /* 800BC58C ori $t6, $zero, 0x8C8C */
    r_t5 = 0u + 0xBu; /* 800BC590 addiu $t5, $zero, 0xB */
    r_v0 = 0x800C0000u; /* 800BC594 lui $v0, %hi(D_800C3EB0) */
    r_t0 = r_v0 + 0x3EB0u; /* 800BC598 addiu $t0, $v0, %lo(D_800C3EB0) */
    r_a3 = r_t0 + 0u; /* 800BC59C addu $a3, $t0, $zero */
    r_v1 = r_v1 << 1; /* 800BC5A0 sll $v1, $v1, 1 */
    r_t3 = r_v1 + 0u; /* 800BC5A4 addu $t3, $v1, $zero */
    r_a0 = r_a0 << 1; /* 800BC5A8 sll $a0, $a0, 1 */
    r_t2 = r_a0 + 0u; /* 800BC5AC addu $t2, $a0, $zero */
    r_t4 = r_t2 + 0u; /* 800BC5B0 addu $t4, $t2, $zero */
    r_a1 = r_a1 << 1; /* 800BC5B4 sll $a1, $a1, 1 */
    r_t1 = r_a1 + 0u; /* 800BC5B8 addu $t1, $a1, $zero */
    r_a2 = r_t1 + 0u; /* 800BC5BC addu $a2, $t1, $zero */
    bl_sw(r_sp + 0x10u, r_v1); /* 800BC5C0 sw $v1, 0x10($sp) */
    bl_sw(r_sp + 0x14u, r_t1); /* 800BC5C4 sw $t1, 0x14($sp) */
    bl_sw(r_sp + 0x18u, r_t2); /* 800BC5C8 sw $t2, 0x18($sp) */
L_800BC5CC:
    r_v0 = r_s1 & 0x1u; /* 800BC5CC andi $v0, $s1, 0x1 */
    /* 800BC5D0 beqz $v0, .L800BC664 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800BC664; }
    r_v0 = bl_lbu(r_t0 + 0x7u); /* 800BC5D8 lbu $v0, 0x7($t0) */
    ; /* 800BC5DC nop  */
    /* 800BC5E0 bnez $v0, .L800BC664 */
    { int c_ = (r_v0 != 0); r_v0 = r_a3 + r_t6; if (c_) goto L_800BC664; }
    r_s0 = bl_lw(r_v0); /* 800BC5E8 lw $s0, 0x0($v0) */
    ; /* 800BC5EC nop  */
    /* 800BC5F0 beqz $s0, .L800BC664 */
    { int c_ = (r_s0 == 0); ; if (c_) goto L_800BC664; }
    r_a0 = bl_lw(r_s0); /* 800BC5F8 lw $a0, 0x0($s0) */
    ; /* 800BC5FC nop  */
    r_v0 = (u32)((s32)r_t3 < (s32)r_a0); /* 800BC600 slt $v0, $t3, $a0 */
    /* 800BC604 beqz $v0, .L800BC610 */
    { int c_ = (r_v0 == 0); r_v0 = (u32)((s32)r_a0 < (s32)r_v1); if (c_) goto L_800BC610; }
    r_t3 = r_a0 + 0u; /* 800BC60C addu $t3, $a0, $zero */
L_800BC610:
    /* 800BC610 beqz $v0, .L800BC61C */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800BC61C; }
    r_v1 = r_a0 + 0u; /* 800BC618 addu $v1, $a0, $zero */
L_800BC61C:
    r_a0 = bl_lw(r_s0 + 0x8u); /* 800BC61C lw $a0, 0x8($s0) */
    ; /* 800BC620 nop  */
    r_v0 = (u32)((s32)r_t4 < (s32)r_a0); /* 800BC624 slt $v0, $t4, $a0 */
    /* 800BC628 beqz $v0, .L800BC634 */
    { int c_ = (r_v0 == 0); r_v0 = (u32)((s32)r_a0 < (s32)r_t2); if (c_) goto L_800BC634; }
    r_t4 = r_a0 + 0u; /* 800BC630 addu $t4, $a0, $zero */
L_800BC634:
    /* 800BC634 beqz $v0, .L800BC640 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800BC640; }
    r_t2 = r_a0 + 0u; /* 800BC63C addu $t2, $a0, $zero */
L_800BC640:
    r_a0 = bl_lw(r_s0 + 0x4u); /* 800BC640 lw $a0, 0x4($s0) */
    ; /* 800BC644 nop  */
    r_v0 = (u32)((s32)r_a2 < (s32)r_a0); /* 800BC648 slt $v0, $a2, $a0 */
    /* 800BC64C beqz $v0, .L800BC658 */
    { int c_ = (r_v0 == 0); r_v0 = (u32)((s32)r_a0 < (s32)r_t1); if (c_) goto L_800BC658; }
    r_a2 = r_a0 + 0u; /* 800BC654 addu $a2, $a0, $zero */
L_800BC658:
    /* 800BC658 beqz $v0, .L800BC664 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800BC664; }
    r_t1 = r_a0 + 0u; /* 800BC660 addu $t1, $a0, $zero */
L_800BC664:
    r_a3 = r_a3 + 0x4u; /* 800BC664 addiu $a3, $a3, 0x4 */
    r_t0 = r_t0 + 0x1Cu; /* 800BC668 addiu $t0, $t0, 0x1C */
    r_s2 = r_s2 + 0x1u; /* 800BC66C addiu $s2, $s2, 0x1 */
    r_v0 = r_s1 & 0xFFFFu; /* 800BC670 andi $v0, $s1, 0xFFFF */
    /* 800BC674 bne $s2, $t5, .L800BC5CC */
    { int c_ = (r_s2 != r_t5); r_s1 = r_v0 >> 1; if (c_) goto L_800BC5CC; }
    r_a0 = 0x800C0000u; /* 800BC67C lui $a0, %hi(D_800C3740) */
    r_a0 = r_a0 + 0x3740u; /* 800BC680 addiu $a0, $a0, %lo(D_800C3740) */
    r_s0 = r_sp + 0x30u; /* 800BC684 addiu $s0, $sp, 0x30 */
    r_a1 = r_s0 + 0u; /* 800BC688 addu $a1, $s0, $zero */
    r_a3 = r_v1 + r_t3; /* 800BC68C addu $a3, $v1, $t3 */
    r_v0 = r_a3 >> 31; /* 800BC690 srl $v0, $a3, 31 */
    r_a3 = r_a3 + r_v0; /* 800BC694 addu $a3, $a3, $v0 */
    r_v0 = BL_SRA(r_a3, 1); /* 800BC698 sra $v0, $a3, 1 */
    r_a2 = r_a2 + r_t1; /* 800BC69C addu $a2, $a2, $t1 */
    bl_sw(r_sp + 0x10u, r_v0); /* 800BC6A0 sw $v0, 0x10($sp) */
    r_v0 = r_a2 >> 31; /* 800BC6A4 srl $v0, $a2, 31 */
    r_a2 = r_a2 + r_v0; /* 800BC6A8 addu $a2, $a2, $v0 */
    r_v0 = BL_SRA(r_a2, 1); /* 800BC6AC sra $v0, $a2, 1 */
    bl_sw(r_sp + 0x14u, r_v0); /* 800BC6B0 sw $v0, 0x14($sp) */
    r_v0 = r_t2 + r_t4; /* 800BC6B4 addu $v0, $t2, $t4 */
    r_v1 = r_v0 >> 31; /* 800BC6B8 srl $v1, $v0, 31 */
    r_v0 = r_v0 + r_v1; /* 800BC6BC addu $v0, $v0, $v1 */
    r_v1 = BL_SRA(r_v0, 1); /* 800BC6C0 sra $v1, $v0, 1 */
    r_a3 = BL_SRA(r_a3, 17); /* 800BC6C4 sra $a3, $a3, 17 */
    r_a2 = BL_SRA(r_a2, 17); /* 800BC6C8 sra $a2, $a2, 17 */
    r_v0 = BL_SRA(r_v0, 17); /* 800BC6CC sra $v0, $v0, 17 */
    bl_sw(r_sp + 0x18u, r_v1); /* 800BC6D0 sw $v1, 0x18($sp) */
    bl_sw(r_sp + 0x10u, r_a3); /* 800BC6D4 sw $a3, 0x10($sp) */
    bl_sw(r_sp + 0x14u, r_a2); /* 800BC6D8 sw $a2, 0x14($sp) */
    /* 800BC6DC jal RotMatrixZYX */
    bl_sw(r_sp + 0x18u, r_v0);
    bl_jal(FN, 0x8004ABBCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixZYX */
    bl_sh(r_sp + 0x70u, 0u); /* 800BC6E4 sh $zero, 0x70($sp) */
    /* 800BC6E8 jal ReadGeomScreen */
    bl_sh(r_sp + 0x72u, 0u);
    bl_jal(FN, 0x8004A0DCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ReadGeomScreen */
    r_a0 = r_s0 + 0u; /* 800BC6F0 addu $a0, $s0, $zero */
    r_a1 = r_sp + 0x70u; /* 800BC6F4 addiu $a1, $sp, 0x70 */
    r_a2 = r_sp + 0x50u; /* 800BC6F8 addiu $a2, $sp, 0x50 */
    r_v0 = r_v0 << 3; /* 800BC6FC sll $v0, $v0, 3 */
    /* 800BC700 jal ApplyMatrix */
    bl_sh(r_sp + 0x74u, r_v0);
    bl_jal(FN, 0x80049CECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ApplyMatrix */
    r_a0 = r_s0 + 0u; /* 800BC708 addu $a0, $s0, $zero */
    r_a1 = r_sp + 0x20u; /* 800BC70C addiu $a1, $sp, 0x20 */
    r_a2 = r_sp + 0x28u; /* 800BC710 addiu $a2, $sp, 0x28 */
    r_a3 = 0x800C0000u; /* 800BC714 lui $a3, %hi(D_800C3730) */
    r_a3 = r_a3 + 0x3730u; /* 800BC718 addiu $a3, $a3, %lo(D_800C3730) */
    r_s2 = 0u + 0u; /* 800BC71C addu $s2, $zero, $zero */
    r_t7 = r_sp + 0x60u; /* 800BC720 addiu $t7, $sp, 0x60 */
    r_fp = r_sp + 0x68u; /* 800BC724 addiu $fp, $sp, 0x68 */
    r_v0 = bl_lhu(r_sp + 0x10u); /* 800BC728 lhu $v0, 0x10($sp) */
    r_v1 = bl_lhu(r_sp + 0x14u); /* 800BC72C lhu $v1, 0x14($sp) */
    r_t0 = bl_lhu(r_sp + 0x18u); /* 800BC730 lhu $t0, 0x18($sp) */
    r_t1 = bl_lhu(r_sp + 0x50u); /* 800BC734 lhu $t1, 0x50($sp) */
    r_s7 = r_sp + 0x78u; /* 800BC738 addiu $s7, $sp, 0x78 */
    bl_sh(r_sp + 0x20u, r_v0); /* 800BC73C sh $v0, 0x20($sp) */
    bl_sh(r_sp + 0x28u, r_v0); /* 800BC740 sh $v0, 0x28($sp) */
    r_v0 = r_v0 - r_t1; /* 800BC744 subu $v0, $v0, $t1 */
    bl_sh(r_sp + 0x20u, r_v0); /* 800BC748 sh $v0, 0x20($sp) */
    r_v0 = bl_lhu(r_sp + 0x54u); /* 800BC74C lhu $v0, 0x54($sp) */
    r_t1 = bl_lhu(r_sp + 0x58u); /* 800BC750 lhu $t1, 0x58($sp) */
    r_s6 = r_sp + 0x7Cu; /* 800BC754 addiu $s6, $sp, 0x7C */
    bl_sw(r_sp + 0xF0u, r_t7); /* 800BC758 sw $t7, 0xF0($sp) */
    bl_sh(r_sp + 0x22u, r_v1); /* 800BC75C sh $v1, 0x22($sp) */
    bl_sh(r_sp + 0x24u, r_t0); /* 800BC760 sh $t0, 0x24($sp) */
    bl_sh(r_sp + 0x2Au, r_v1); /* 800BC764 sh $v1, 0x2A($sp) */
    bl_sh(r_sp + 0x2Cu, r_t0); /* 800BC768 sh $t0, 0x2C($sp) */
    r_v1 = r_v1 + r_v0; /* 800BC76C addu $v1, $v1, $v0 */
    r_t0 = r_t0 - r_t1; /* 800BC770 subu $t0, $t0, $t1 */
    bl_sh(r_sp + 0x22u, r_v1); /* 800BC774 sh $v1, 0x22($sp) */
    /* 800BC778 jal func_800BB844 */
    bl_sh(r_sp + 0x24u, r_t0);
    bl_jal(FN, 0x800BB844u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BB844 */
    /* 800BC780 jal SetRotMatrix */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80049EFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetRotMatrix */
    /* 800BC788 jal SetTransMatrix */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80049F8Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetTransMatrix */
    r_s1 = r_s3 + 0u; /* 800BC790 addu $s1, $s3, $zero */
    r_v0 = 0x800C0000u; /* 800BC794 lui $v0, %hi(D_800C3EB0) */
    r_s3 = r_v0 + 0x3EB0u; /* 800BC798 addiu $s3, $v0, %lo(D_800C3EB0) */
    r_s4 = r_s3 + 0u; /* 800BC79C addu $s4, $s3, $zero */
L_800BC7A0:
    r_v0 = r_s1 & 0x1u; /* 800BC7A0 andi $v0, $s1, 0x1 */
    /* 800BC7A4 beqz $v0, .L800BC8FC */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800BC8FC; }
    r_v0 = bl_lbu(r_s3 + 0x7u); /* 800BC7AC lbu $v0, 0x7($s3) */
    ; /* 800BC7B0 nop  */
    /* 800BC7B4 bnez $v0, .L800BC8FC */
    { int c_ = (r_v0 != 0); r_v0 = 0u | 0x8C8Cu; if (c_) goto L_800BC8FC; }
    r_v0 = r_s4 + r_v0; /* 800BC7BC addu $v0, $s4, $v0 */
    r_s0 = bl_lw(r_v0); /* 800BC7C0 lw $s0, 0x0($v0) */
    ; /* 800BC7C4 nop  */
    /* 800BC7C8 beqz $s0, .L800BC8FC */
    { int c_ = (r_s0 == 0); r_a1 = r_fp + 0u; if (c_) goto L_800BC8FC; }
    r_a0 = bl_lw(r_sp + 0xF0u); /* 800BC7D0 lw $a0, 0xF0($sp) */
    r_v0 = bl_lh(r_s0 + 0x2u); /* 800BC7D4 lh $v0, 0x2($s0) */
    ; /* 800BC7D8 nop  */
    bl_sh(r_sp + 0x60u, r_v0); /* 800BC7DC sh $v0, 0x60($sp) */
    r_v0 = bl_lh(r_s0 + 0x6u); /* 800BC7E0 lh $v0, 0x6($s0) */
    r_a2 = r_s7 + 0u; /* 800BC7E4 addu $a2, $s7, $zero */
    bl_sh(r_sp + 0x62u, r_v0); /* 800BC7E8 sh $v0, 0x62($sp) */
    r_v0 = bl_lh(r_s0 + 0xAu); /* 800BC7EC lh $v0, 0xA($s0) */
    r_a3 = r_s6 + 0u; /* 800BC7F0 addu $a3, $s6, $zero */
    /* 800BC7F4 jal RotTransPers */
    bl_sh(r_sp + 0x64u, r_v0);
    bl_jal(FN, 0x8004A64Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotTransPers */
    r_a0 = bl_lhu(r_sp + 0x68u); /* 800BC7FC lhu $a0, 0x68($sp) */
    ; /* 800BC800 nop  */
    r_a0 = r_a0 + 0xFFFFFF60u; /* 800BC804 addiu $a0, $a0, -0xA0 */
    r_v0 = r_a0 << 18; /* 800BC808 sll $v0, $a0, 18 */
    r_v0 = BL_SRA(r_v0, 16); /* 800BC80C sra $v0, $v0, 16 */
    BL_MULT(r_v0, r_v0); /* 800BC810 mult $v0, $v0 */
    r_v1 = bl_lhu(r_sp + 0x6Au); /* 800BC814 lhu $v1, 0x6A($sp) */
    ; /* 800BC818 nop  */
    r_v1 = r_v1 + 0xFFFFFF5Cu; /* 800BC81C addiu $v1, $v1, -0xA4 */
    r_a1 = lo; /* 800BC820 mflo $a1 */
    r_v0 = r_v1 << 18; /* 800BC824 sll $v0, $v1, 18 */
    r_v0 = BL_SRA(r_v0, 16); /* 800BC828 sra $v0, $v0, 16 */
    BL_MULT(r_v0, r_v0); /* 800BC82C mult $v0, $v0 */
    bl_sh(r_sp + 0x68u, r_a0); /* 800BC830 sh $a0, 0x68($sp) */
    r_a0 = r_a0 << 2; /* 800BC834 sll $a0, $a0, 2 */
    bl_sh(r_sp + 0x6Au, r_v1); /* 800BC838 sh $v1, 0x6A($sp) */
    r_v1 = r_v1 << 2; /* 800BC83C sll $v1, $v1, 2 */
    bl_sh(r_sp + 0x68u, r_a0); /* 800BC840 sh $a0, 0x68($sp) */
    r_t0 = lo; /* 800BC844 mflo $t0 */
    r_a1 = r_a1 + r_t0; /* 800BC848 addu $a1, $a1, $t0 */
    r_v0 = (u32)((s32)r_s5 < (s32)r_a1); /* 800BC84C slt $v0, $s5, $a1 */
    /* 800BC850 beqz $v0, .L800BC85C */
    { int c_ = (r_v0 == 0); bl_sh(r_sp + 0x6Au, r_v1); if (c_) goto L_800BC85C; }
    r_s5 = r_a1 + 0u; /* 800BC858 addu $s5, $a1, $zero */
L_800BC85C:
    r_v0 = bl_lbu(r_s3 + 0x8u); /* 800BC85C lbu $v0, 0x8($s3) */
    ; /* 800BC860 nop  */
    /* 800BC864 beqz $v0, .L800BC8FC */
    { int c_ = (r_v0 == 0); r_v0 = 0x800C0000u; if (c_) goto L_800BC8FC; }
    r_v0 = bl_lbu(r_v0 + 0x3688u); /* 800BC86C lbu $v0, %lo(D_800C3688)($v0) */
    ; /* 800BC870 nop  */
    /* 800BC874 bnez $v0, .L800BC8FC */
    { int c_ = (r_v0 != 0); r_a1 = r_fp + 0u; if (c_) goto L_800BC8FC; }
    r_a2 = r_s7 + 0u; /* 800BC87C addu $a2, $s7, $zero */
    r_a3 = r_s6 + 0u; /* 800BC880 addu $a3, $s6, $zero */
    r_v0 = bl_lhu(r_sp + 0x62u); /* 800BC884 lhu $v0, 0x62($sp) */
    r_v1 = bl_lhu(r_s0 + 0x36u); /* 800BC888 lhu $v1, 0x36($s0) */
    r_a0 = bl_lw(r_sp + 0xF0u); /* 800BC88C lw $a0, 0xF0($sp) */
    r_v0 = r_v0 - r_v1; /* 800BC890 subu $v0, $v0, $v1 */
    /* 800BC894 jal RotTransPers */
    bl_sh(r_sp + 0x62u, r_v0);
    bl_jal(FN, 0x8004A64Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotTransPers */
    r_a0 = bl_lhu(r_sp + 0x68u); /* 800BC89C lhu $a0, 0x68($sp) */
    ; /* 800BC8A0 nop  */
    r_a0 = r_a0 + 0xFFFFFF60u; /* 800BC8A4 addiu $a0, $a0, -0xA0 */
    r_v0 = r_a0 << 18; /* 800BC8A8 sll $v0, $a0, 18 */
    r_v0 = BL_SRA(r_v0, 16); /* 800BC8AC sra $v0, $v0, 16 */
    BL_MULT(r_v0, r_v0); /* 800BC8B0 mult $v0, $v0 */
    r_v1 = bl_lhu(r_sp + 0x6Au); /* 800BC8B4 lhu $v1, 0x6A($sp) */
    ; /* 800BC8B8 nop  */
    r_v1 = r_v1 + 0xFFFFFF5Cu; /* 800BC8BC addiu $v1, $v1, -0xA4 */
    r_a1 = lo; /* 800BC8C0 mflo $a1 */
    r_v0 = r_v1 << 18; /* 800BC8C4 sll $v0, $v1, 18 */
    r_v0 = BL_SRA(r_v0, 16); /* 800BC8C8 sra $v0, $v0, 16 */
    BL_MULT(r_v0, r_v0); /* 800BC8CC mult $v0, $v0 */
    bl_sh(r_sp + 0x68u, r_a0); /* 800BC8D0 sh $a0, 0x68($sp) */
    r_a0 = r_a0 << 2; /* 800BC8D4 sll $a0, $a0, 2 */
    bl_sh(r_sp + 0x6Au, r_v1); /* 800BC8D8 sh $v1, 0x6A($sp) */
    r_v1 = r_v1 << 2; /* 800BC8DC sll $v1, $v1, 2 */
    bl_sh(r_sp + 0x68u, r_a0); /* 800BC8E0 sh $a0, 0x68($sp) */
    r_t0 = lo; /* 800BC8E4 mflo $t0 */
    r_a1 = r_a1 + r_t0; /* 800BC8E8 addu $a1, $a1, $t0 */
    r_v0 = (u32)((s32)r_s5 < (s32)r_a1); /* 800BC8EC slt $v0, $s5, $a1 */
    /* 800BC8F0 beqz $v0, .L800BC8FC */
    { int c_ = (r_v0 == 0); bl_sh(r_sp + 0x6Au, r_v1); if (c_) goto L_800BC8FC; }
    r_s5 = r_a1 + 0u; /* 800BC8F8 addu $s5, $a1, $zero */
L_800BC8FC:
    r_s4 = r_s4 + 0x4u; /* 800BC8FC addiu $s4, $s4, 0x4 */
    r_s2 = r_s2 + 0x1u; /* 800BC900 addiu $s2, $s2, 0x1 */
    r_v0 = r_s1 & 0xFFFFu; /* 800BC904 andi $v0, $s1, 0xFFFF */
    r_s1 = r_v0 >> 1; /* 800BC908 srl $s1, $v0, 1 */
    r_v0 = 0u + 0xBu; /* 800BC90C addiu $v0, $zero, 0xB */
    /* 800BC910 bne $s2, $v0, .L800BC7A0 */
    { int c_ = (r_s2 != r_v0); r_s3 = r_s3 + 0x1Cu; if (c_) goto L_800BC7A0; }
    /* 800BC918 jal SquareRoot0 */
    r_a0 = r_s5 + 0u;
    bl_jal(FN, 0x80048C4Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SquareRoot0 */
    r_s5 = r_v0 + 0u; /* 800BC920 addu $s5, $v0, $zero */
    r_v0 = (u32)((s32)r_s5 < 120); /* 800BC924 slti $v0, $s5, 0x78 */
    /* 800BC928 beqz $v0, .L800BC994 */
    { int c_ = (r_v0 == 0); r_a0 = 0x800C0000u; if (c_) goto L_800BC994; }
    r_a0 = r_a0 + 0x3740u; /* 800BC930 addiu $a0, $a0, %lo(D_800C3740) */
    r_s0 = r_sp + 0x80u; /* 800BC934 addiu $s0, $sp, 0x80 */
    /* 800BC938 jal RotMatrixZYX */
    r_a1 = r_s0 + 0u;
    bl_jal(FN, 0x8004ABBCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixZYX */
    bl_sh(r_sp + 0xB0u, 0u); /* 800BC940 sh $zero, 0xB0($sp) */
    /* 800BC944 jal ReadGeomScreen */
    bl_sh(r_sp + 0xB2u, 0u);
    bl_jal(FN, 0x8004A0DCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ReadGeomScreen */
    r_v0 = r_v0 << 1; /* 800BC94C sll $v0, $v0, 1 */
    /* 800BC950 jal ReadGeomScreen */
    bl_sh(r_sp + 0xB4u, r_v0);
    bl_jal(FN, 0x8004A0DCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ReadGeomScreen */
    r_a0 = r_s0 + 0u; /* 800BC958 addu $a0, $s0, $zero */
    r_a1 = r_sp + 0xB0u; /* 800BC95C addiu $a1, $sp, 0xB0 */
    r_a2 = r_sp + 0x60u; /* 800BC960 addiu $a2, $sp, 0x60 */
    r_v1 = 0x800C0000u; /* 800BC964 lui $v1, %hi(D_800C3CDC) */
    r_v0 = r_v0 << 1; /* 800BC968 sll $v0, $v0, 1 */
    /* 800BC96C jal ApplyMatrix */
    bl_sw(r_v1 + 0x3CDCu, r_v0);
    bl_jal(FN, 0x80049CECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ApplyMatrix */
    r_a2 = bl_lhu(r_sp + 0x10u); /* 800BC974 lhu $a2, 0x10($sp) */
    r_a3 = bl_lhu(r_sp + 0x14u); /* 800BC978 lhu $a3, 0x14($sp) */
    r_t0 = bl_lhu(r_sp + 0x18u); /* 800BC97C lhu $t0, 0x18($sp) */
    r_a1 = bl_lhu(r_sp + 0x60u); /* 800BC980 lhu $a1, 0x60($sp) */
    r_v1 = bl_lhu(r_sp + 0x64u); /* 800BC984 lhu $v1, 0x64($sp) */
    r_a0 = bl_lhu(r_sp + 0x68u); /* 800BC988 lhu $a0, 0x68($sp) */
    /* 800BC98C j .L800BCA20 */
    r_v0 = 0x800D0000u; goto L_800BCA20;
L_800BC994:
    r_v0 = 0x88880000u; /* 800BC994 lui $v0, (0x88888889 >> 16) */
    r_v0 = r_v0 | 0x8889u; /* 800BC998 ori $v0, $v0, (0x88888889 & 0xFFFF) */
    r_v1 = r_s5 << 14; /* 800BC99C sll $v1, $s5, 14 */
    BL_MULT(r_v1, r_v0); /* 800BC9A0 mult $v1, $v0 */
    r_t7 = hi; /* 800BC9A4 mfhi $t7 */
    r_v0 = r_t7 + r_v1; /* 800BC9A8 addu $v0, $t7, $v1 */
    r_v0 = BL_SRA(r_v0, 6); /* 800BC9AC sra $v0, $v0, 6 */
    r_v1 = BL_SRA(r_v1, 31); /* 800BC9B0 sra $v1, $v1, 31 */
    /* 800BC9B4 jal ReadGeomScreen */
    r_s1 = r_v0 - r_v1;
    bl_jal(FN, 0x8004A0DCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ReadGeomScreen */
    r_v1 = r_s1 << 1; /* 800BC9BC sll $v1, $s1, 1 */
    BL_MULT(r_v1, r_v0); /* 800BC9C0 mult $v1, $v0 */
    r_a0 = 0x800C0000u; /* 800BC9C4 lui $a0, %hi(D_800C3740) */
    r_a0 = r_a0 + 0x3740u; /* 800BC9C8 addiu $a0, $a0, %lo(D_800C3740) */
    r_s0 = r_sp + 0xB8u; /* 800BC9CC addiu $s0, $sp, 0xB8 */
    r_a1 = r_s0 + 0u; /* 800BC9D0 addu $a1, $s0, $zero */
    r_v0 = 0x800C0000u; /* 800BC9D4 lui $v0, %hi(D_800C3CDC) */
    r_s1 = lo; /* 800BC9D8 mflo $s1 */
    r_s1 = BL_SRA(r_s1, 14); /* 800BC9DC sra $s1, $s1, 14 */
    /* 800BC9E0 jal RotMatrixZYX */
    bl_sw(r_v0 + 0x3CDCu, r_s1);
    bl_jal(FN, 0x8004ABBCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixZYX */
    r_a0 = r_s0 + 0u; /* 800BC9E8 addu $a0, $s0, $zero */
    r_a1 = r_sp + 0xE8u; /* 800BC9EC addiu $a1, $sp, 0xE8 */
    r_a2 = r_sp + 0xA0u; /* 800BC9F0 addiu $a2, $sp, 0xA0 */
    bl_sh(r_sp + 0xE8u, 0u); /* 800BC9F4 sh $zero, 0xE8($sp) */
    bl_sh(r_sp + 0xEAu, 0u); /* 800BC9F8 sh $zero, 0xEA($sp) */
    /* 800BC9FC jal ApplyMatrix */
    bl_sh(r_sp + 0xECu, r_s1);
    bl_jal(FN, 0x80049CECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ApplyMatrix */
    r_v0 = 0x800D0000u; /* 800BCA04 lui $v0, %hi(D_800D30A0) */
    r_a2 = bl_lhu(r_sp + 0x10u); /* 800BCA08 lhu $a2, 0x10($sp) */
    r_a3 = bl_lhu(r_sp + 0x14u); /* 800BCA0C lhu $a3, 0x14($sp) */
    r_t0 = bl_lhu(r_sp + 0x18u); /* 800BCA10 lhu $t0, 0x18($sp) */
    r_a1 = bl_lhu(r_sp + 0xA0u); /* 800BCA14 lhu $a1, 0xA0($sp) */
    r_v1 = bl_lhu(r_sp + 0xA4u); /* 800BCA18 lhu $v1, 0xA4($sp) */
    r_a0 = bl_lhu(r_sp + 0xA8u); /* 800BCA1C lhu $a0, 0xA8($sp) */
L_800BCA20:
    r_a1 = r_a2 - r_a1; /* 800BCA20 subu $a1, $a2, $a1 */
    r_v1 = r_a3 + r_v1; /* 800BCA24 addu $v1, $a3, $v1 */
    r_a0 = r_t0 - r_a0; /* 800BCA28 subu $a0, $t0, $a0 */
    bl_sh(r_v0 + 0x30A0u, r_a1); /* 800BCA2C sh $a1, %lo(D_800D30A0)($v0) */
    r_v0 = r_v0 + 0x30A0u; /* 800BCA30 addiu $v0, $v0, %lo(D_800D30A0) */
    bl_sh(r_sp + 0x20u, r_a2); /* 800BCA34 sh $a2, 0x20($sp) */
    bl_sh(r_sp + 0x22u, r_a3); /* 800BCA38 sh $a3, 0x22($sp) */
    bl_sh(r_sp + 0x24u, r_t0); /* 800BCA3C sh $t0, 0x24($sp) */
    bl_sh(r_sp + 0x28u, r_a2); /* 800BCA40 sh $a2, 0x28($sp) */
    bl_sh(r_sp + 0x2Au, r_a3); /* 800BCA44 sh $a3, 0x2A($sp) */
    bl_sh(r_sp + 0x2Cu, r_t0); /* 800BCA48 sh $t0, 0x2C($sp) */
    bl_sh(r_sp + 0x20u, r_a1); /* 800BCA4C sh $a1, 0x20($sp) */
    bl_sh(r_sp + 0x22u, r_v1); /* 800BCA50 sh $v1, 0x22($sp) */
    bl_sh(r_sp + 0x24u, r_a0); /* 800BCA54 sh $a0, 0x24($sp) */
    bl_sh(r_v0 + 0x2u, r_v1); /* 800BCA58 sh $v1, 0x2($v0) */
    bl_sh(r_v0 + 0x4u, r_a0); /* 800BCA5C sh $a0, 0x4($v0) */
    bl_sh(r_v0 + 0x8u, r_a2); /* 800BCA60 sh $a2, 0x8($v0) */
    r_v0 = r_v0 + 0x8u; /* 800BCA64 addiu $v0, $v0, 0x8 */
    bl_sh(r_v0 + 0x2u, r_a3); /* 800BCA68 sh $a3, 0x2($v0) */
    bl_sh(r_v0 + 0x4u, r_t0); /* 800BCA6C sh $t0, 0x4($v0) */
L_800BCA70:
    r_ra = bl_lw(r_sp + 0x11Cu); /* 800BCA70 lw $ra, 0x11C($sp) */
    r_fp = bl_lw(r_sp + 0x118u); /* 800BCA74 lw $fp, 0x118($sp) */
    r_s7 = bl_lw(r_sp + 0x114u); /* 800BCA78 lw $s7, 0x114($sp) */
    r_s6 = bl_lw(r_sp + 0x110u); /* 800BCA7C lw $s6, 0x110($sp) */
    r_s5 = bl_lw(r_sp + 0x10Cu); /* 800BCA80 lw $s5, 0x10C($sp) */
    r_s4 = bl_lw(r_sp + 0x108u); /* 800BCA84 lw $s4, 0x108($sp) */
    r_s3 = bl_lw(r_sp + 0x104u); /* 800BCA88 lw $s3, 0x104($sp) */
    r_s2 = bl_lw(r_sp + 0x100u); /* 800BCA8C lw $s2, 0x100($sp) */
    r_s1 = bl_lw(r_sp + 0xFCu); /* 800BCA90 lw $s1, 0xFC($sp) */
    r_s0 = bl_lw(r_sp + 0xF8u); /* 800BCA94 lw $s0, 0xF8($sp) */
    r_sp = r_sp + 0x120u; /* 800BCA98 addiu $sp, $sp, 0x120 */
    /* 800BCA9C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_80075168.s (500 instructions; --args 0 --void), hot path batch 4:
 * the party-HUD layout for the three party slots (main2 retail, matched C
 * retail-only there).  For each slot it reads the state byte at
 * *(D_800C3EA4) + 0x853D + slot*0x1E4 and writes the slot's HUD element
 * positions and sizes.  No callees. */
void func_80075168(void) {
    static const char FN[] = "func_80075168";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x10u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFF0u; /* 80075168 addiu $sp, $sp, -0x10 */
    r_t4 = 0u + 0u; /* 8007516C addu $t4, $zero, $zero */
    bl_sw(r_sp + 0x8u, r_s2); /* 80075170 sw $s2, 0x8($sp) */
    r_s2 = 0x800C0000u; /* 80075174 lui $s2, %hi(D_800C3254) */
    r_s2 = r_s2 + 0x3254u; /* 80075178 addiu $s2, $s2, %lo(D_800C3254) */
    bl_sw(r_sp + 0x4u, r_s1); /* 8007517C sw $s1, 0x4($sp) */
    r_s1 = 0u | 0x22u; /* 80075180 ori $s1, $zero, 0x22 */
    bl_sw(r_sp, r_s0); /* 80075184 sw $s0, 0x0($sp) */
    r_s0 = 0u | 0x26u; /* 80075188 ori $s0, $zero, 0x26 */
    r_t3 = 0u | 0xFFu; /* 8007518C ori $t3, $zero, 0xFF */
    r_t9 = 0u | 0x7Fu; /* 80075190 ori $t9, $zero, 0x7F */
    r_t7 = 0u | 0x28u; /* 80075194 ori $t7, $zero, 0x28 */
    r_t2 = 0u + 0u; /* 80075198 addu $t2, $zero, $zero */
    r_t6 = 0u + 0u; /* 8007519C addu $t6, $zero, $zero */
    r_a2 = 0u + 0u; /* 800751A0 addu $a2, $zero, $zero */
    r_t5 = 0x800D0000u; /* 800751A4 lui $t5, %hi(D_800D32A0) */
    r_t5 = r_t5 + 0x32A0u; /* 800751A8 addiu $t5, $t5, %lo(D_800D32A0) */
    r_t8 = 0u + 0u; /* 800751AC addu $t8, $zero, $zero */
L_800751B0:
    r_a3 = 0x800C0000u; /* 800751B0 lui $a3, %hi(D_800C3EA4) */
    r_a3 = bl_lw(r_a3 + 0x3EA4u); /* 800751B4 lw $a3, %lo(D_800C3EA4)($a3) */
    ; /* 800751B8 nop  */
    r_v0 = r_a3 + r_t8; /* 800751BC addu $v0, $a3, $t8 */
    r_at = 0x10000u; /* 800751C0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800751C4 addu $at, $v0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFF853Du); /* 800751C8 lbu $v1, -0x7AC3($at) */
    r_v0 = 0u | 0x1u; /* 800751CC ori $v0, $zero, 0x1 */
    /* 800751D0 beq $v1, $v0, .L800751E8 */
    { int c_ = (r_v1 == r_v0); r_v0 = 0u | 0x2u; if (c_) goto L_800751E8; }
    /* 800751D8 beq $v1, $v0, .L80075410 */
    { int c_ = (r_v1 == r_v0); r_a0 = 0xD1B70000u; if (c_) goto L_80075410; }
    /* 800751E0 j .L80075900 */
    r_t7 = r_t7 + 0x60u; goto L_80075900;
L_800751E8:
    r_v0 = 0x800D0000u; /* 800751E8 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 800751EC lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 800751F0 nop  */
    r_v0 = r_a2 + r_v0; /* 800751F4 addu $v0, $a2, $v0 */
    r_v1 = r_v0 << 3; /* 800751F8 sll $v1, $v0, 3 */
    r_v1 = r_v1 + r_v0; /* 800751FC addu $v1, $v1, $v0 */
    r_v0 = 0x800D0000u; /* 80075200 lui $v0, %hi(D_800D3280) */
    r_v0 = bl_lbu(r_v0 + 0x3280u); /* 80075204 lbu $v0, %lo(D_800D3280)($v0) */
    r_v1 = r_v1 << 2; /* 80075208 sll $v1, $v1, 2 */
    r_a1 = r_v0 << 1; /* 8007520C sll $a1, $v0, 1 */
    r_a1 = r_a1 + r_v0; /* 80075210 addu $a1, $a1, $v0 */
    r_a1 = r_a1 + r_t4; /* 80075214 addu $a1, $a1, $t4 */
    r_a1 = r_a1 << 1; /* 80075218 sll $a1, $a1, 1 */
    r_a1 = r_a1 + r_s2; /* 8007521C addu $a1, $a1, $s2 */
    r_v0 = bl_lhu(r_a1); /* 80075220 lhu $v0, 0x0($a1) */
    r_v1 = r_v1 + r_a3; /* 80075224 addu $v1, $v1, $a3 */
    r_v0 = r_v0 + 0x28u; /* 80075228 addiu $v0, $v0, 0x28 */
    r_v0 = r_t2 + r_v0; /* 8007522C addu $v0, $t2, $v0 */
    bl_sh(r_v1 + 0x748u, r_v0); /* 80075230 sh $v0, 0x748($v1) */
    r_v1 = 0x800D0000u; /* 80075234 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075238 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8007523C nop  */
    r_v1 = r_a2 + r_v1; /* 80075240 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075244 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075248 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8007524C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075250 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x74Au, r_s1); /* 80075254 sh $s1, 0x74A($v0) */
    r_v0 = 0x800D0000u; /* 80075258 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8007525C lw $v0, %lo(D_800CCB34)($v0) */
    r_v1 = bl_lbu(r_t5); /* 80075260 lbu $v1, 0x0($t5) */
    r_v0 = r_a2 + r_v0; /* 80075264 addu $v0, $a2, $v0 */
    r_a0 = r_v0 << 3; /* 80075268 sll $a0, $v0, 3 */
    r_a0 = r_a0 + r_v0; /* 8007526C addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 2; /* 80075270 sll $a0, $a0, 2 */
    r_a0 = r_a0 + r_a3; /* 80075274 addu $a0, $a0, $a3 */
    r_v0 = bl_lhu(r_a1); /* 80075278 lhu $v0, 0x0($a1) */
    r_v1 = r_v1 << 1; /* 8007527C sll $v1, $v1, 1 */
    r_v0 = r_v0 + 0x28u; /* 80075280 addiu $v0, $v0, 0x28 */
    r_v0 = r_t2 + r_v0; /* 80075284 addu $v0, $t2, $v0 */
    r_v0 = r_v0 + r_v1; /* 80075288 addu $v0, $v0, $v1 */
    bl_sh(r_a0 + 0x750u, r_v0); /* 8007528C sh $v0, 0x750($a0) */
    r_v1 = 0x800D0000u; /* 80075290 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075294 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075298 nop  */
    r_v1 = r_a2 + r_v1; /* 8007529C addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800752A0 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800752A4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800752A8 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 800752AC addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x752u, r_s1); /* 800752B0 sh $s1, 0x752($v0) */
    r_v0 = 0x800D0000u; /* 800752B4 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 800752B8 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 800752BC nop  */
    r_v0 = r_a2 + r_v0; /* 800752C0 addu $v0, $a2, $v0 */
    r_v1 = r_v0 << 3; /* 800752C4 sll $v1, $v0, 3 */
    r_v1 = r_v1 + r_v0; /* 800752C8 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 800752CC sll $v1, $v1, 2 */
    r_v0 = bl_lhu(r_a1); /* 800752D0 lhu $v0, 0x0($a1) */
    r_v1 = r_v1 + r_a3; /* 800752D4 addu $v1, $v1, $a3 */
    r_v0 = r_v0 + 0x28u; /* 800752D8 addiu $v0, $v0, 0x28 */
    r_v0 = r_t2 + r_v0; /* 800752DC addu $v0, $t2, $v0 */
    bl_sh(r_v1 + 0x758u, r_v0); /* 800752E0 sh $v0, 0x758($v1) */
    r_v1 = 0x800D0000u; /* 800752E4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800752E8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800752EC nop  */
    r_v1 = r_a2 + r_v1; /* 800752F0 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800752F4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800752F8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800752FC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075300 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x75Au, r_s0); /* 80075304 sh $s0, 0x75A($v0) */
    r_v0 = 0x800D0000u; /* 80075308 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8007530C lw $v0, %lo(D_800CCB34)($v0) */
    r_v1 = bl_lbu(r_t5); /* 80075310 lbu $v1, 0x0($t5) */
    r_v0 = r_a2 + r_v0; /* 80075314 addu $v0, $a2, $v0 */
    r_a0 = r_v0 << 3; /* 80075318 sll $a0, $v0, 3 */
    r_a0 = r_a0 + r_v0; /* 8007531C addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 2; /* 80075320 sll $a0, $a0, 2 */
    r_a0 = r_a0 + r_a3; /* 80075324 addu $a0, $a0, $a3 */
    r_v0 = bl_lhu(r_a1); /* 80075328 lhu $v0, 0x0($a1) */
    r_v1 = r_v1 << 1; /* 8007532C sll $v1, $v1, 1 */
    r_v0 = r_v0 + 0x28u; /* 80075330 addiu $v0, $v0, 0x28 */
    r_v0 = r_t2 + r_v0; /* 80075334 addu $v0, $t2, $v0 */
    r_v0 = r_v0 + r_v1; /* 80075338 addu $v0, $v0, $v1 */
    bl_sh(r_a0 + 0x760u, r_v0); /* 8007533C sh $v0, 0x760($a0) */
    r_v1 = 0x800D0000u; /* 80075340 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075344 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075348 nop  */
    r_v1 = r_a2 + r_v1; /* 8007534C addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075350 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075354 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075358 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 8007535C addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x762u, r_s0); /* 80075360 sh $s0, 0x762($v0) */
    r_v1 = 0x800D0000u; /* 80075364 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075368 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8007536C nop  */
    r_v1 = r_a2 + r_v1; /* 80075370 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075374 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075378 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8007537C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075380 addu $v0, $v0, $a3 */
    bl_sb(r_v0 + 0x744u, 0u); /* 80075384 sb $zero, 0x744($v0) */
    r_v1 = 0x800D0000u; /* 80075388 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007538C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075390 nop  */
    r_v1 = r_a2 + r_v1; /* 80075394 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075398 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8007539C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800753A0 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800753A4 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800753A8 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800753AC addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x745u, r_t3); /* 800753B0 sb $t3, 0x745($v0) */
    r_v1 = 0x800D0000u; /* 800753B4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800753B8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800753BC nop  */
    r_v1 = r_a2 + r_v1; /* 800753C0 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800753C4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800753C8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800753CC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800753D0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800753D4 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800753D8 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x746u, 0u); /* 800753DC sb $zero, 0x746($v0) */
    r_v1 = 0x800D0000u; /* 800753E0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800753E4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800753E8 nop  */
    r_v1 = r_a2 + r_v1; /* 800753EC addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800753F0 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800753F4 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800753F8 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800753FC lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075400 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075404 addu $v0, $v0, $v1 */
    /* 80075408 j .L800757A0 */
    bl_sb(r_v0 + 0x74Cu, 0u); goto L_800757A0;
L_80075410:
    r_at = 0x800D0000u; /* 80075410 lui $at, %hi(D_800CCDC4) */
    r_at = r_at + r_t6; /* 80075414 addu $at, $at, $t6 */
    r_t0 = bl_lhu(r_at + 0xFFFFCDC4u); /* 80075418 lhu $t0, %lo(D_800CCDC4)($at) */
    r_at = 0x800D0000u; /* 8007541C lui $at, %hi(D_800CCDC6) */
    r_at = r_at + r_t6; /* 80075420 addu $at, $at, $t6 */
    r_t1 = bl_lhu(r_at + 0xFFFFCDC6u); /* 80075424 lhu $t1, %lo(D_800CCDC6)($at) */
    r_v1 = r_t0 << 1; /* 80075428 sll $v1, $t0, 1 */
    r_v1 = r_v1 + r_t0; /* 8007542C addu $v1, $v1, $t0 */
    r_v1 = r_v1 << 3; /* 80075430 sll $v1, $v1, 3 */
    r_v1 = r_v1 + r_t0; /* 80075434 addu $v1, $v1, $t0 */
    r_v1 = r_v1 << 2; /* 80075438 sll $v1, $v1, 2 */
    BL_DIVU(r_v1, r_t1); /* 8007543C divu $zero, $v1, $t1 */
    r_v1 = lo; /* 80075440 mflo $v1 */
    r_a0 = r_a0 | 0x1759u; /* 80075444 ori $a0, $a0, (0xD1B71759 & 0xFFFF) */
    r_v0 = r_v1 << 1; /* 80075448 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 8007544C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075450 sll $v0, $v0, 2 */
    r_v0 = r_v0 - r_v1; /* 80075454 subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 4; /* 80075458 sll $v0, $v0, 4 */
    r_v0 = r_v0 - r_v1; /* 8007545C subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 5; /* 80075460 sll $v0, $v0, 5 */
    BL_MULTU(r_v0, r_a0); /* 80075464 multu $v0, $a0 */
    r_v1 = 0x800D0000u; /* 80075468 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007546C lw $v1, %lo(D_800CCB34)($v1) */
    r_v0 = 0x800D0000u; /* 80075470 lui $v0, %hi(D_800D3280) */
    r_v0 = bl_lbu(r_v0 + 0x3280u); /* 80075474 lbu $v0, %lo(D_800D3280)($v0) */
    r_v1 = r_a2 + r_v1; /* 80075478 addu $v1, $a2, $v1 */
    r_a0 = r_v1 << 3; /* 8007547C sll $a0, $v1, 3 */
    r_a0 = r_a0 + r_v1; /* 80075480 addu $a0, $a0, $v1 */
    r_a0 = r_a0 << 2; /* 80075484 sll $a0, $a0, 2 */
    r_a1 = r_v0 << 1; /* 80075488 sll $a1, $v0, 1 */
    r_a1 = r_a1 + r_v0; /* 8007548C addu $a1, $a1, $v0 */
    r_a1 = r_a1 + r_t4; /* 80075490 addu $a1, $a1, $t4 */
    r_a1 = r_a1 << 1; /* 80075494 sll $a1, $a1, 1 */
    r_a1 = r_a1 + r_s2; /* 80075498 addu $a1, $a1, $s2 */
    r_v0 = bl_lhu(r_a1); /* 8007549C lhu $v0, 0x0($a1) */
    r_a0 = r_a0 + r_a3; /* 800754A0 addu $a0, $a0, $a3 */
    r_v0 = r_v0 + 0x28u; /* 800754A4 addiu $v0, $v0, 0x28 */
    r_v0 = r_t2 + r_v0; /* 800754A8 addu $v0, $t2, $v0 */
    bl_sh(r_a0 + 0x748u, r_v0); /* 800754AC sh $v0, 0x748($a0) */
    r_v1 = 0x800D0000u; /* 800754B0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800754B4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800754B8 nop  */
    r_v1 = r_a2 + r_v1; /* 800754BC addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800754C0 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800754C4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800754C8 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 800754CC addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x74Au, r_s1); /* 800754D0 sh $s1, 0x74A($v0) */
    r_v1 = 0x800D0000u; /* 800754D4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800754D8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800754DC nop  */
    r_v1 = r_a2 + r_v1; /* 800754E0 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800754E4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800754E8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800754EC sll $v0, $v0, 2 */
    r_v1 = bl_lhu(r_a1); /* 800754F0 lhu $v1, 0x0($a1) */
    r_v0 = r_v0 + r_a3; /* 800754F4 addu $v0, $v0, $a3 */
    r_v1 = r_t7 + r_v1; /* 800754F8 addu $v1, $t7, $v1 */
    r_a0 = hi; /* 800754FC mfhi $a0 */
    r_a0 = r_a0 >> 13; /* 80075500 srl $a0, $a0, 13 */
    r_v1 = r_v1 + r_a0; /* 80075504 addu $v1, $v1, $a0 */
    bl_sh(r_v0 + 0x750u, r_v1); /* 80075508 sh $v1, 0x750($v0) */
    r_v1 = 0x800D0000u; /* 8007550C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075510 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075514 nop  */
    r_v1 = r_a2 + r_v1; /* 80075518 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 8007551C sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075520 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075524 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075528 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x752u, r_s1); /* 8007552C sh $s1, 0x752($v0) */
    r_v0 = 0x800D0000u; /* 80075530 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075534 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80075538 nop  */
    r_v0 = r_a2 + r_v0; /* 8007553C addu $v0, $a2, $v0 */
    r_v1 = r_v0 << 3; /* 80075540 sll $v1, $v0, 3 */
    r_v1 = r_v1 + r_v0; /* 80075544 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075548 sll $v1, $v1, 2 */
    r_v0 = bl_lhu(r_a1); /* 8007554C lhu $v0, 0x0($a1) */
    r_v1 = r_v1 + r_a3; /* 80075550 addu $v1, $v1, $a3 */
    r_v0 = r_v0 + 0x28u; /* 80075554 addiu $v0, $v0, 0x28 */
    r_v0 = r_t2 + r_v0; /* 80075558 addu $v0, $t2, $v0 */
    bl_sh(r_v1 + 0x758u, r_v0); /* 8007555C sh $v0, 0x758($v1) */
    r_v1 = 0x800D0000u; /* 80075560 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075564 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075568 nop  */
    r_v1 = r_a2 + r_v1; /* 8007556C addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075570 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075574 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075578 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 8007557C addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x75Au, r_s0); /* 80075580 sh $s0, 0x75A($v0) */
    r_v0 = 0x800D0000u; /* 80075584 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075588 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 8007558C nop  */
    r_v0 = r_a2 + r_v0; /* 80075590 addu $v0, $a2, $v0 */
    r_v1 = r_v0 << 3; /* 80075594 sll $v1, $v0, 3 */
    r_v1 = r_v1 + r_v0; /* 80075598 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 8007559C sll $v1, $v1, 2 */
    r_v0 = bl_lhu(r_a1); /* 800755A0 lhu $v0, 0x0($a1) */
    r_v1 = r_v1 + r_a3; /* 800755A4 addu $v1, $v1, $a3 */
    r_v0 = r_t7 + r_v0; /* 800755A8 addu $v0, $t7, $v0 */
    r_v0 = r_v0 + r_a0; /* 800755AC addu $v0, $v0, $a0 */
    bl_sh(r_v1 + 0x760u, r_v0); /* 800755B0 sh $v0, 0x760($v1) */
    r_v1 = 0x800D0000u; /* 800755B4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800755B8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800755BC nop  */
    r_v1 = r_a2 + r_v1; /* 800755C0 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800755C4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800755C8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800755CC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 800755D0 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x762u, r_s0); /* 800755D4 sh $s0, 0x762($v0) */
    r_v0 = r_t1 >> 2; /* 800755D8 srl $v0, $t1, 2 */
    r_v0 = (u32)(r_t0 < r_v0); /* 800755DC sltu $v0, $t0, $v0 */
    /* 800755E0 bnez $v0, .L800756EC */
    { int c_ = (r_v0 != 0); r_v0 = r_t1 >> 3; if (c_) goto L_800756EC; }
    r_v1 = 0x800D0000u; /* 800755E8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800755EC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800755F0 nop  */
    r_v1 = r_a2 + r_v1; /* 800755F4 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800755F8 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800755FC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075600 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075604 addu $v0, $v0, $a3 */
    bl_sb(r_v0 + 0x744u, 0u); /* 80075608 sb $zero, 0x744($v0) */
    r_v1 = 0x800D0000u; /* 8007560C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075610 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075614 nop  */
    r_v1 = r_a2 + r_v1; /* 80075618 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 8007561C sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075620 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075624 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075628 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 8007562C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075630 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x745u, 0u); /* 80075634 sb $zero, 0x745($v0) */
    r_v1 = 0x800D0000u; /* 80075638 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007563C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075640 nop  */
    r_v1 = r_a2 + r_v1; /* 80075644 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075648 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8007564C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075650 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075654 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075658 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 8007565C addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x746u, r_t3); /* 80075660 sb $t3, 0x746($v0) */
    r_v1 = 0x800D0000u; /* 80075664 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075668 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8007566C nop  */
    r_v1 = r_a2 + r_v1; /* 80075670 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075674 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075678 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8007567C lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075680 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075684 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075688 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Cu, 0u); /* 8007568C sb $zero, 0x74C($v0) */
    r_v1 = 0x800D0000u; /* 80075690 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075694 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075698 nop  */
    r_v1 = r_a2 + r_v1; /* 8007569C addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800756A0 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800756A4 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800756A8 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800756AC lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800756B0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800756B4 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Du, 0u); /* 800756B8 sb $zero, 0x74D($v0) */
    r_v1 = 0x800D0000u; /* 800756BC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800756C0 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800756C4 nop  */
    r_v1 = r_a2 + r_v1; /* 800756C8 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800756CC sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800756D0 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800756D4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800756D8 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800756DC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800756E0 addu $v0, $v0, $v1 */
    /* 800756E4 j .L800758FC */
    bl_sb(r_v0 + 0x74Eu, r_t3); goto L_800758FC;
L_800756EC:
    r_v0 = (u32)(r_t0 < r_v0); /* 800756EC sltu $v0, $t0, $v0 */
    /* 800756F0 bnez $v0, .L800757FC */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_800757FC; }
    r_v1 = 0x800D0000u; /* 800756F8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800756FC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075700 nop  */
    r_v1 = r_a2 + r_v1; /* 80075704 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075708 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8007570C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075710 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075714 addu $v0, $v0, $a3 */
    bl_sb(r_v0 + 0x744u, r_t3); /* 80075718 sb $t3, 0x744($v0) */
    r_v1 = 0x800D0000u; /* 8007571C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075720 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075724 nop  */
    r_v1 = r_a2 + r_v1; /* 80075728 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 8007572C sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075730 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075734 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075738 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 8007573C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075740 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x745u, r_t3); /* 80075744 sb $t3, 0x745($v0) */
    r_v1 = 0x800D0000u; /* 80075748 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007574C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075750 nop  */
    r_v1 = r_a2 + r_v1; /* 80075754 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075758 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8007575C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075760 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075764 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075768 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 8007576C addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x746u, 0u); /* 80075770 sb $zero, 0x746($v0) */
    r_v1 = 0x800D0000u; /* 80075774 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075778 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8007577C nop  */
    r_v1 = r_a2 + r_v1; /* 80075780 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075784 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075788 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 8007578C lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075790 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075794 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075798 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Cu, r_t3); /* 8007579C sb $t3, 0x74C($v0) */
L_800757A0:
    r_v1 = 0x800D0000u; /* 800757A0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800757A4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800757A8 nop  */
    r_v1 = r_a2 + r_v1; /* 800757AC addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800757B0 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800757B4 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800757B8 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800757BC lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800757C0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800757C4 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Du, r_t3); /* 800757C8 sb $t3, 0x74D($v0) */
    r_v1 = 0x800D0000u; /* 800757CC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800757D0 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800757D4 nop  */
    r_v1 = r_a2 + r_v1; /* 800757D8 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800757DC sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800757E0 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800757E4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800757E8 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800757EC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800757F0 addu $v0, $v0, $v1 */
    /* 800757F4 j .L800758FC */
    bl_sb(r_v0 + 0x74Eu, 0u); goto L_800758FC;
L_800757FC:
    r_v1 = 0x800D0000u; /* 800757FC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075800 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075804 nop  */
    r_v1 = r_a2 + r_v1; /* 80075808 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 8007580C sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075810 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075814 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a3; /* 80075818 addu $v0, $v0, $a3 */
    bl_sb(r_v0 + 0x744u, r_t3); /* 8007581C sb $t3, 0x744($v0) */
    r_v1 = 0x800D0000u; /* 80075820 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075824 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075828 nop  */
    r_v1 = r_a2 + r_v1; /* 8007582C addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075830 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075834 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075838 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8007583C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075840 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075844 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x745u, r_t9); /* 80075848 sb $t9, 0x745($v0) */
    r_v1 = 0x800D0000u; /* 8007584C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075850 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075854 nop  */
    r_v1 = r_a2 + r_v1; /* 80075858 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 8007585C sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 80075860 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075864 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075868 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 8007586C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075870 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x746u, r_t9); /* 80075874 sb $t9, 0x746($v0) */
    r_v1 = 0x800D0000u; /* 80075878 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007587C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075880 nop  */
    r_v1 = r_a2 + r_v1; /* 80075884 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 80075888 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 8007588C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075890 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075894 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075898 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 8007589C addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Cu, r_t3); /* 800758A0 sb $t3, 0x74C($v0) */
    r_v1 = 0x800D0000u; /* 800758A4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800758A8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800758AC nop  */
    r_v1 = r_a2 + r_v1; /* 800758B0 addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800758B4 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800758B8 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800758BC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800758C0 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800758C4 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800758C8 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Du, r_t9); /* 800758CC sb $t9, 0x74D($v0) */
    r_v1 = 0x800D0000u; /* 800758D0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800758D4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800758D8 nop  */
    r_v1 = r_a2 + r_v1; /* 800758DC addu $v1, $a2, $v1 */
    r_v0 = r_v1 << 3; /* 800758E0 sll $v0, $v1, 3 */
    r_v0 = r_v0 + r_v1; /* 800758E4 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800758E8 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800758EC lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800758F0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800758F4 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x74Eu, r_t9); /* 800758F8 sb $t9, 0x74E($v0) */
L_800758FC:
    r_t7 = r_t7 + 0x60u; /* 800758FC addiu $t7, $t7, 0x60 */
L_80075900:
    r_t2 = r_t2 + 0x60u; /* 80075900 addiu $t2, $t2, 0x60 */
    r_t6 = r_t6 + 0x170u; /* 80075904 addiu $t6, $t6, 0x170 */
    r_a2 = r_a2 + 0x2u; /* 80075908 addiu $a2, $a2, 0x2 */
    r_t5 = r_t5 + 0x8u; /* 8007590C addiu $t5, $t5, 0x8 */
    r_t4 = r_t4 + 0x1u; /* 80075910 addiu $t4, $t4, 0x1 */
    r_v0 = (u32)((s32)r_t4 < 3); /* 80075914 slti $v0, $t4, 0x3 */
    /* 80075918 bnez $v0, .L800751B0 */
    { int c_ = (r_v0 != 0); r_t8 = r_t8 + 0x1E4u; if (c_) goto L_800751B0; }
    r_s2 = bl_lw(r_sp + 0x8u); /* 80075920 lw $s2, 0x8($sp) */
    r_s1 = bl_lw(r_sp + 0x4u); /* 80075924 lw $s1, 0x4($sp) */
    r_s0 = bl_lw(r_sp); /* 80075928 lw $s0, 0x0($sp) */
    r_sp = r_sp + 0x10u; /* 8007592C addiu $sp, $sp, 0x10 */
    /* 80075930 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_80075938.s (696 instructions; --args 0 --void), hot path batch 4:
 * the party-HUD bars (main2 retail).  It runs 80075168 (through the jal
 * service: the adopted lift), then per slot sets the HP/limit bar quads from
 * D_800D2E06 and the D_800CCDC4/6 rows.  Retail reads w[slot] (sp+0x10+4i)
 * before writing it (800759BC) and clamps it at 0 (800759CC); both following
 * branches assign w[slot] before any use, so that read is dead. */
void func_80075938(void) {
    static const char FN[] = "func_80075938";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x30u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFD0u; /* 80075938 addiu $sp, $sp, -0x30 */
    bl_sw(r_sp + 0x2Cu, r_ra); /* 8007593C sw $ra, 0x2C($sp) */
    bl_sw(r_sp + 0x28u, r_s2); /* 80075940 sw $s2, 0x28($sp) */
    bl_sw(r_sp + 0x24u, r_s1); /* 80075944 sw $s1, 0x24($sp) */
    /* 80075948 jal func_80075168 */
    bl_sw(r_sp + 0x20u, r_s0);
    bl_jal(FN, 0x80075168u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80075168 */
    r_t1 = 0u + 0u; /* 80075950 addu $t1, $zero, $zero */
    r_t8 = 0x68DB0000u; /* 80075954 lui $t8, (0x68DB8BAD >> 16) */
    r_t8 = r_t8 | 0x8BADu; /* 80075958 ori $t8, $t8, (0x68DB8BAD & 0xFFFF) */
    r_s0 = 0u | 0x1Au; /* 8007595C ori $s0, $zero, 0x1A */
    r_s1 = 0u | 0x1Eu; /* 80075960 ori $s1, $zero, 0x1E */
    r_s2 = 0u | 0xCu; /* 80075964 ori $s2, $zero, 0xC */
    r_t9 = 0u | 0xCEu; /* 80075968 ori $t9, $zero, 0xCE */
    r_t2 = 0u + 0u; /* 8007596C addu $t2, $zero, $zero */
    r_v0 = 0x800D0000u; /* 80075970 lui $v0, %hi(D_800D2E06) */
    r_v0 = r_v0 + 0x2E06u; /* 80075974 addiu $v0, $v0, %lo(D_800D2E06) */
    r_t4 = r_v0 + 0u; /* 80075978 addu $t4, $v0, $zero */
    r_a3 = 0u + 0u; /* 8007597C addu $a3, $zero, $zero */
    r_t3 = 0u + 0u; /* 80075980 addu $t3, $zero, $zero */
    r_t7 = r_t4 + 0xFFFFFFEAu; /* 80075984 addiu $t7, $t4, -0x16 */
    r_t6 = 0u + 0u; /* 80075988 addu $t6, $zero, $zero */
    r_v0 = 0x800D0000u; /* 8007598C lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 80075990 lw $v0, %lo(D_800D2D28)($v0) */
    r_t5 = 0u + 0u; /* 80075994 addu $t5, $zero, $zero */
    bl_sb(r_v0 + 0x82u, 0u); /* 80075998 sb $zero, 0x82($v0) */
L_8007599C:
    r_at = 0x800C0000u; /* 8007599C lui $at, %hi(D_800C3EB6) */
    r_at = r_at + r_t6; /* 800759A0 addu $at, $at, $t6 */
    r_v1 = bl_lbu(r_at + 0x3EB6u); /* 800759A4 lbu $v1, %lo(D_800C3EB6)($at) */
    r_v0 = 0u | 0x7Fu; /* 800759A8 ori $v0, $zero, 0x7F */
    /* 800759AC beq $v1, $v0, .L800763C0 */
    { int c_ = (r_v1 == r_v0); r_v0 = r_t1 << 2; if (c_) goto L_800763C0; }
    r_v1 = r_sp + 0x10u; /* 800759B4 addiu $v1, $sp, 0x10 */
    r_t0 = r_v0 + r_v1; /* 800759B8 addu $t0, $v0, $v1 */
    r_v0 = bl_lw(r_t0); /* 800759BC lw $v0, 0x0($t0) */
    ; /* 800759C0 nop  */
    /* 800759C4 bgez $v0, .L800759D0 */
    { int c_ = ((s32)r_v0 >= 0); ; if (c_) goto L_800759D0; }
    bl_sw(r_t0, 0u); /* 800759CC sw $zero, 0x0($t0) */
L_800759D0:
    r_v0 = 0x800D0000u; /* 800759D0 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 800759D4 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 800759D8 nop  */
    r_v0 = r_v0 + r_t1; /* 800759DC addu $v0, $v0, $t1 */
    r_v0 = bl_lbu(r_v0 + 0x90u); /* 800759E0 lbu $v0, 0x90($v0) */
    ; /* 800759E4 nop  */
    /* 800759E8 bnez $v0, .L80075F1C */
    { int c_ = (r_v0 != 0); r_v1 = 0u | 0x64u; if (c_) goto L_80075F1C; }
    r_v1 = bl_lh(r_t4); /* 800759F0 lh $v1, 0x0($t4) */
    ; /* 800759F4 nop  */
    r_v0 = r_v1 << 1; /* 800759F8 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 800759FC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 80075A00 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_v1; /* 80075A04 addu $v0, $v0, $v1 */
    r_v1 = bl_lh(r_t7); /* 80075A08 lh $v1, 0x0($t7) */
    r_v0 = r_v0 << 2; /* 80075A0C sll $v0, $v0, 2 */
    BL_DIV(r_v0, r_v1); /* 80075A10 div $zero, $v0, $v1 */
    r_v0 = lo; /* 80075A14 mflo $v0 */
    r_v1 = 0u | 0x64u; /* 80075A18 ori $v1, $zero, 0x64 */
    r_v1 = r_v1 - r_v0; /* 80075A1C subu $v1, $v1, $v0 */
    r_v0 = r_v1 << 1; /* 80075A20 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075A24 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075A28 sll $v0, $v0, 2 */
    r_v0 = r_v0 - r_v1; /* 80075A2C subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 4; /* 80075A30 sll $v0, $v0, 4 */
    r_v0 = r_v0 - r_v1; /* 80075A34 subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 5; /* 80075A38 sll $v0, $v0, 5 */
    BL_MULT(r_v0, r_t8); /* 80075A3C mult $v0, $t8 */
    r_v0 = BL_SRA(r_v0, 31); /* 80075A40 sra $v0, $v0, 31 */
    r_v1 = hi; /* 80075A44 mfhi $v1 */
    r_v1 = BL_SRA(r_v1, 12); /* 80075A48 sra $v1, $v1, 12 */
    r_v1 = r_v1 - r_v0; /* 80075A4C subu $v1, $v1, $v0 */
    bl_sw(r_t0, r_v1); /* 80075A50 sw $v1, 0x0($t0) */
    r_v0 = 0x800D0000u; /* 80075A54 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075A58 lw $v0, %lo(D_800CCB34)($v0) */
    r_a2 = 0x800C0000u; /* 80075A5C lui $a2, %hi(D_800C3EA4) */
    r_a2 = bl_lw(r_a2 + 0x3EA4u); /* 80075A60 lw $a2, %lo(D_800C3EA4)($a2) */
    r_v0 = r_a3 + r_v0; /* 80075A64 addu $v0, $a3, $v0 */
    r_v1 = r_v0 << 1; /* 80075A68 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80075A6C addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075A70 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075A74 addu $v1, $v1, $v0 */
    r_v0 = 0x800D0000u; /* 80075A78 lui $v0, %hi(D_800D3280) */
    r_v0 = bl_lbu(r_v0 + 0x3280u); /* 80075A7C lbu $v0, %lo(D_800D3280)($v0) */
    r_v1 = r_v1 << 2; /* 80075A80 sll $v1, $v1, 2 */
    r_a1 = r_v0 << 1; /* 80075A84 sll $a1, $v0, 1 */
    r_a1 = r_a1 + r_v0; /* 80075A88 addu $a1, $a1, $v0 */
    r_a1 = r_a1 + r_t1; /* 80075A8C addu $a1, $a1, $t1 */
    r_v0 = 0x800C0000u; /* 80075A90 lui $v0, %hi(D_800C3254) */
    r_v0 = r_v0 + 0x3254u; /* 80075A94 addiu $v0, $v0, %lo(D_800C3254) */
    r_a1 = r_a1 << 1; /* 80075A98 sll $a1, $a1, 1 */
    r_a1 = r_a1 + r_v0; /* 80075A9C addu $a1, $a1, $v0 */
    r_v0 = bl_lhu(r_a1); /* 80075AA0 lhu $v0, 0x0($a1) */
    r_v1 = r_v1 + r_a2; /* 80075AA4 addu $v1, $v1, $a2 */
    r_v0 = r_v0 + r_t3; /* 80075AA8 addu $v0, $v0, $t3 */
    r_v0 = r_v0 + 0x2Du; /* 80075AAC addiu $v0, $v0, 0x2D */
    bl_sh(r_v1 + 0x5A8u, r_v0); /* 80075AB0 sh $v0, 0x5A8($v1) */
    r_v1 = 0x800D0000u; /* 80075AB4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075AB8 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075ABC nop  */
    r_v1 = r_a3 + r_v1; /* 80075AC0 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075AC4 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075AC8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075ACC sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075AD0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075AD4 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a2; /* 80075AD8 addu $v0, $v0, $a2 */
    bl_sh(r_v0 + 0x5AAu, r_s0); /* 80075ADC sh $s0, 0x5AA($v0) */
    r_v0 = 0x800D0000u; /* 80075AE0 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075AE4 lw $v0, %lo(D_800CCB34)($v0) */
    r_a0 = bl_lhu(r_t0); /* 80075AE8 lhu $a0, 0x0($t0) */
    r_v0 = r_a3 + r_v0; /* 80075AEC addu $v0, $a3, $v0 */
    r_v1 = r_v0 << 1; /* 80075AF0 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80075AF4 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075AF8 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075AFC addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075B00 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_a2; /* 80075B04 addu $v1, $v1, $a2 */
    r_v0 = bl_lhu(r_a1); /* 80075B08 lhu $v0, 0x0($a1) */
    r_a0 = r_a0 + 0x1u; /* 80075B0C addiu $a0, $a0, 0x1 */
    r_v0 = r_v0 + 0x2Cu; /* 80075B10 addiu $v0, $v0, 0x2C */
    r_v0 = r_t3 + r_v0; /* 80075B14 addu $v0, $t3, $v0 */
    r_v0 = r_v0 + r_a0; /* 80075B18 addu $v0, $v0, $a0 */
    bl_sh(r_v1 + 0x5B4u, r_v0); /* 80075B1C sh $v0, 0x5B4($v1) */
    r_v1 = 0x800D0000u; /* 80075B20 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075B24 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075B28 nop  */
    r_v1 = r_a3 + r_v1; /* 80075B2C addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075B30 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075B34 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075B38 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075B3C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075B40 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a2; /* 80075B44 addu $v0, $v0, $a2 */
    bl_sh(r_v0 + 0x5B6u, r_s0); /* 80075B48 sh $s0, 0x5B6($v0) */
    r_v1 = 0x800D0000u; /* 80075B4C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075B50 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075B54 nop  */
    r_v1 = r_a3 + r_v1; /* 80075B58 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075B5C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075B60 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075B64 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075B68 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075B6C sll $v0, $v0, 2 */
    r_v1 = bl_lhu(r_a1); /* 80075B70 lhu $v1, 0x0($a1) */
    r_v0 = r_v0 + r_a2; /* 80075B74 addu $v0, $v0, $a2 */
    r_v1 = r_v1 + r_t3; /* 80075B78 addu $v1, $v1, $t3 */
    r_v1 = r_v1 + 0x2Du; /* 80075B7C addiu $v1, $v1, 0x2D */
    bl_sh(r_v0 + 0x5C0u, r_v1); /* 80075B80 sh $v1, 0x5C0($v0) */
    r_v1 = 0x800D0000u; /* 80075B84 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075B88 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075B8C nop  */
    r_v1 = r_a3 + r_v1; /* 80075B90 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075B94 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075B98 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075B9C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075BA0 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075BA4 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a2; /* 80075BA8 addu $v0, $v0, $a2 */
    bl_sh(r_v0 + 0x5C2u, r_s1); /* 80075BAC sh $s1, 0x5C2($v0) */
    r_v0 = 0x800D0000u; /* 80075BB0 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075BB4 lw $v0, %lo(D_800CCB34)($v0) */
    r_a0 = bl_lhu(r_t0); /* 80075BB8 lhu $a0, 0x0($t0) */
    r_v0 = r_a3 + r_v0; /* 80075BBC addu $v0, $a3, $v0 */
    r_v1 = r_v0 << 1; /* 80075BC0 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80075BC4 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075BC8 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075BCC addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075BD0 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_a2; /* 80075BD4 addu $v1, $v1, $a2 */
    r_v0 = bl_lhu(r_a1); /* 80075BD8 lhu $v0, 0x0($a1) */
    r_a0 = r_a0 + 0x1u; /* 80075BDC addiu $a0, $a0, 0x1 */
    r_v0 = r_v0 + 0x2Cu; /* 80075BE0 addiu $v0, $v0, 0x2C */
    r_v0 = r_t3 + r_v0; /* 80075BE4 addu $v0, $t3, $v0 */
    r_v0 = r_v0 + r_a0; /* 80075BE8 addu $v0, $v0, $a0 */
    bl_sh(r_v1 + 0x5CCu, r_v0); /* 80075BEC sh $v0, 0x5CC($v1) */
    r_v1 = 0x800D0000u; /* 80075BF0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075BF4 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075BF8 nop  */
    r_v1 = r_a3 + r_v1; /* 80075BFC addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075C00 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075C04 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075C08 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075C0C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075C10 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a2; /* 80075C14 addu $v0, $v0, $a2 */
    bl_sh(r_v0 + 0x5CEu, r_s1); /* 80075C18 sh $s1, 0x5CE($v0) */
    r_v1 = 0x800D0000u; /* 80075C1C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075C20 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075C24 nop  */
    r_v1 = r_a3 + r_v1; /* 80075C28 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075C2C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075C30 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075C34 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075C38 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075C3C sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80075C40 lui $at, (0x10000 >> 16) */
    r_at = r_a2 + r_at; /* 80075C44 addu $at, $a2, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA244u); /* 80075C48 lbu $v1, -0x5DBC($at) */
    r_v0 = r_v0 + r_a2; /* 80075C4C addu $v0, $v0, $a2 */
    r_v1 = r_v1 & 0x3Fu; /* 80075C50 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 80075C54 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x5ACu, r_v1); /* 80075C58 sb $v1, 0x5AC($v0) */
    r_v1 = 0x800D0000u; /* 80075C5C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075C60 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075C64 nop  */
    r_v1 = r_a3 + r_v1; /* 80075C68 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075C6C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075C70 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075C74 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075C78 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075C7C lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075C80 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075C84 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80075C88 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 80075C8C addu $at, $v1, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA248u); /* 80075C90 lbu $a0, -0x5DB8($at) */
    r_v0 = r_v0 + r_v1; /* 80075C94 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x5ADu, r_a0); /* 80075C98 sb $a0, 0x5AD($v0) */
    r_v0 = 0x800D0000u; /* 80075C9C lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075CA0 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80075CA4 nop  */
    r_v0 = r_a3 + r_v0; /* 80075CA8 addu $v0, $a3, $v0 */
    r_v1 = r_v0 << 1; /* 80075CAC sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80075CB0 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075CB4 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075CB8 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 80075CBC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80075CC0 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 2; /* 80075CC4 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075CC8 addu $v1, $v1, $v0 */
    r_at = 0x10000u; /* 80075CCC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80075CD0 addu $at, $v0, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA244u); /* 80075CD4 lbu $a0, -0x5DBC($at) */
    r_v0 = bl_lw(r_t0); /* 80075CD8 lw $v0, 0x0($t0) */
    r_a0 = r_a0 & 0x3Fu; /* 80075CDC andi $a0, $a0, 0x3F */
    r_v0 = r_v0 + r_a0; /* 80075CE0 addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 1; /* 80075CE4 sll $v0, $v0, 1 */
    bl_sb(r_v1 + 0x5B8u, r_v0); /* 80075CE8 sb $v0, 0x5B8($v1) */
    r_v1 = 0x800D0000u; /* 80075CEC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075CF0 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80075CF4 nop  */
    r_v1 = r_a3 + r_v1; /* 80075CF8 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075CFC sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075D00 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075D04 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075D08 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80075D0C lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075D10 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80075D14 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80075D18 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 80075D1C addu $at, $v1, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA248u); /* 80075D20 lbu $a0, -0x5DB8($at) */
    r_v0 = r_v0 + r_v1; /* 80075D24 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x5B9u, r_a0); /* 80075D28 sb $a0, 0x5B9($v0) */
    r_v1 = 0x800D0000u; /* 80075D2C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075D30 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 80075D34 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 80075D38 lw $a0, %lo(D_800C3EA4)($a0) */
    r_v1 = r_a3 + r_v1; /* 80075D3C addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075D40 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075D44 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075D48 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075D4C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075D50 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80075D54 lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 80075D58 addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA244u); /* 80075D5C lbu $v1, -0x5DBC($at) */
    r_v0 = r_v0 + r_a0; /* 80075D60 addu $v0, $v0, $a0 */
    r_v1 = r_v1 & 0x3Fu; /* 80075D64 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 80075D68 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x5C4u, r_v1); /* 80075D6C sb $v1, 0x5C4($v0) */
    r_v1 = 0x800D0000u; /* 80075D70 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075D74 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 80075D78 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 80075D7C lw $a0, %lo(D_800C3EA4)($a0) */
    r_v1 = r_a3 + r_v1; /* 80075D80 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075D84 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075D88 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075D8C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075D90 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075D94 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80075D98 lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 80075D9C addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA248u); /* 80075DA0 lbu $v1, -0x5DB8($at) */
    r_v0 = r_v0 + r_a0; /* 80075DA4 addu $v0, $v0, $a0 */
    r_v1 = r_v1 + 0x4u; /* 80075DA8 addiu $v1, $v1, 0x4 */
    bl_sb(r_v0 + 0x5C5u, r_v1); /* 80075DAC sb $v1, 0x5C5($v0) */
    r_v0 = 0x800D0000u; /* 80075DB0 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80075DB4 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80075DB8 nop  */
    r_v0 = r_a3 + r_v0; /* 80075DBC addu $v0, $a3, $v0 */
    r_v1 = r_v0 << 1; /* 80075DC0 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80075DC4 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80075DC8 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075DCC addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 80075DD0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80075DD4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 2; /* 80075DD8 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80075DDC addu $v1, $v1, $v0 */
    r_at = 0x10000u; /* 80075DE0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80075DE4 addu $at, $v0, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA244u); /* 80075DE8 lbu $a0, -0x5DBC($at) */
    r_v0 = bl_lw(r_t0); /* 80075DEC lw $v0, 0x0($t0) */
    r_a0 = r_a0 & 0x3Fu; /* 80075DF0 andi $a0, $a0, 0x3F */
    r_v0 = r_v0 + r_a0; /* 80075DF4 addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 1; /* 80075DF8 sll $v0, $v0, 1 */
    bl_sb(r_v1 + 0x5D0u, r_v0); /* 80075DFC sb $v0, 0x5D0($v1) */
    r_v1 = 0x800D0000u; /* 80075E00 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075E04 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 80075E08 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 80075E0C lw $a0, %lo(D_800C3EA4)($a0) */
    r_v1 = r_a3 + r_v1; /* 80075E10 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075E14 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075E18 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075E1C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075E20 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075E24 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80075E28 lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 80075E2C addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA248u); /* 80075E30 lbu $v1, -0x5DB8($at) */
    r_v0 = r_v0 + r_a0; /* 80075E34 addu $v0, $v0, $a0 */
    r_v1 = r_v1 + 0x4u; /* 80075E38 addiu $v1, $v1, 0x4 */
    bl_sb(r_v0 + 0x5D1u, r_v1); /* 80075E3C sb $v1, 0x5D1($v0) */
    r_at = 0x800D0000u; /* 80075E40 lui $at, %hi(D_800CCD6C) */
    r_at = r_at + r_t2; /* 80075E44 addu $at, $at, $t2 */
    r_v0 = bl_lhu(r_at + 0xFFFFCD6Cu); /* 80075E48 lhu $v0, %lo(D_800CCD6C)($at) */
    r_at = 0x800D0000u; /* 80075E4C lui $at, %hi(D_800CCD6E) */
    r_at = r_at + r_t2; /* 80075E50 addu $at, $at, $t2 */
    r_v1 = bl_lhu(r_at + 0xFFFFCD6Eu); /* 80075E54 lhu $v1, %lo(D_800CCD6E)($at) */
    ; /* 80075E58 nop  */
    r_v0 = r_v0 | r_v1; /* 80075E5C or $v0, $v0, $v1 */
    r_v0 = r_v0 & 0x8000u; /* 80075E60 andi $v0, $v0, 0x8000 */
    /* 80075E64 beqz $v0, .L80075E88 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80075E88; }
    r_v0 = 0x800C0000u; /* 80075E6C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80075E70 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80075E74 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80075E78 addu $at, $v0, $at */
    r_a1 = bl_lhu(r_at + 0xFFFFA2B2u); /* 80075E7C lhu $a1, -0x5D4E($at) */
    /* 80075E80 j .L80075ED4 */
    ; goto L_80075ED4;
L_80075E88:
    r_at = 0x800D0000u; /* 80075E88 lui $at, %hi(D_800CCD64) */
    r_at = r_at + r_t2; /* 80075E8C addu $at, $at, $t2 */
    r_v0 = bl_lhu(r_at + 0xFFFFCD64u); /* 80075E90 lhu $v0, %lo(D_800CCD64)($at) */
    ; /* 80075E94 nop  */
    r_v0 = r_v0 & 0x1000u; /* 80075E98 andi $v0, $v0, 0x1000 */
    /* 80075E9C beqz $v0, .L80075EC0 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80075EC0; }
    r_v0 = 0x800C0000u; /* 80075EA4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80075EA8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80075EAC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80075EB0 addu $at, $v0, $at */
    r_a1 = bl_lhu(r_at + 0xFFFFA2B0u); /* 80075EB4 lhu $a1, -0x5D50($at) */
    /* 80075EB8 j .L80075ED4 */
    ; goto L_80075ED4;
L_80075EC0:
    r_v0 = 0x800C0000u; /* 80075EC0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80075EC4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80075EC8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80075ECC addu $at, $v0, $at */
    r_a1 = bl_lhu(r_at + 0xFFFFA2ACu); /* 80075ED0 lhu $a1, -0x5D54($at) */
L_80075ED4:
    r_v1 = 0x800D0000u; /* 80075ED4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80075ED8 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800D0000u; /* 80075EDC lui $a0, %hi(D_800D2D28) */
    r_a0 = bl_lw(r_a0 + 0x2D28u); /* 80075EE0 lw $a0, %lo(D_800D2D28)($a0) */
    r_v1 = r_a3 + r_v1; /* 80075EE4 addu $v1, $a3, $v1 */
    r_v0 = r_v1 << 1; /* 80075EE8 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075EEC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075EF0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80075EF4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075EF8 sll $v0, $v0, 2 */
    r_v1 = 0x800C0000u; /* 80075EFC lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80075F00 lw $v1, %lo(D_800C3EA4)($v1) */
    r_a0 = r_a0 + r_t1; /* 80075F04 addu $a0, $a0, $t1 */
    r_v1 = r_v1 + r_v0; /* 80075F08 addu $v1, $v1, $v0 */
    r_v0 = 0u | 0x1u; /* 80075F0C ori $v0, $zero, 0x1 */
    bl_sh(r_v1 + 0x5AEu, r_a1); /* 80075F10 sh $a1, 0x5AE($v1) */
    /* 80075F14 j .L800763D4 */
    bl_sb(r_a0 + 0x7Fu, r_v0); goto L_800763D4;
L_80075F1C:
    r_v0 = bl_lh(r_t4); /* 80075F1C lh $v0, 0x0($t4) */
    ; /* 80075F20 nop  */
    r_v1 = r_v1 - r_v0; /* 80075F24 subu $v1, $v1, $v0 */
    r_v0 = r_v1 << 1; /* 80075F28 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075F2C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075F30 sll $v0, $v0, 2 */
    r_v0 = r_v0 - r_v1; /* 80075F34 subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 4; /* 80075F38 sll $v0, $v0, 4 */
    r_v0 = r_v0 - r_v1; /* 80075F3C subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 5; /* 80075F40 sll $v0, $v0, 5 */
    BL_MULT(r_v0, r_t8); /* 80075F44 mult $v0, $t8 */
    r_v0 = BL_SRA(r_v0, 31); /* 80075F48 sra $v0, $v0, 31 */
    r_v1 = hi; /* 80075F4C mfhi $v1 */
    r_v1 = BL_SRA(r_v1, 12); /* 80075F50 sra $v1, $v1, 12 */
    r_v1 = r_v1 - r_v0; /* 80075F54 subu $v1, $v1, $v0 */
    bl_sw(r_t0, r_v1); /* 80075F58 sw $v1, 0x0($t0) */
    r_at = 0x800D0000u; /* 80075F5C lui $at, %hi(D_800D32A1) */
    r_at = r_at + r_t5; /* 80075F60 addu $at, $at, $t5 */
    r_v0 = bl_lbu(r_at + 0x32A1u); /* 80075F64 lbu $v0, %lo(D_800D32A1)($at) */
    ; /* 80075F68 nop  */
    /* 80075F6C beqz $v0, .L80075FFC */
    { int c_ = (r_v0 == 0); r_a2 = r_t1 << 2; if (c_) goto L_80075FFC; }
    r_at = 0x800D0000u; /* 80075F74 lui $at, %hi(D_800D2D24) */
    r_at = r_at + r_t1; /* 80075F78 addu $at, $at, $t1 */
    r_v1 = bl_lbu(r_at + 0x2D24u); /* 80075F7C lbu $v1, %lo(D_800D2D24)($at) */
    r_v0 = 0u | 0x7u; /* 80075F80 ori $v0, $zero, 0x7 */
    /* 80075F84 beq $v1, $v0, .L80075FFC */
    { int c_ = (r_v1 == r_v0); ; if (c_) goto L_80075FFC; }
    r_at = 0x800D0000u; /* 80075F8C lui $at, %hi(D_800CCDC4) */
    r_at = r_at + r_t2; /* 80075F90 addu $at, $at, $t2 */
    r_v0 = bl_lhu(r_at + 0xFFFFCDC4u); /* 80075F94 lhu $v0, %lo(D_800CCDC4)($at) */
    ; /* 80075F98 nop  */
    r_v1 = r_v0 << 1; /* 80075F9C sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80075FA0 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 3; /* 80075FA4 sll $v1, $v1, 3 */
    r_v1 = r_v1 + r_v0; /* 80075FA8 addu $v1, $v1, $v0 */
    r_at = 0x800D0000u; /* 80075FAC lui $at, %hi(D_800CCDC6) */
    r_at = r_at + r_t2; /* 80075FB0 addu $at, $at, $t2 */
    r_v0 = bl_lhu(r_at + 0xFFFFCDC6u); /* 80075FB4 lhu $v0, %lo(D_800CCDC6)($at) */
    r_v1 = r_v1 << 2; /* 80075FB8 sll $v1, $v1, 2 */
    BL_DIV(r_v1, r_v0); /* 80075FBC div $zero, $v1, $v0 */
    r_v1 = lo; /* 80075FC0 mflo $v1 */
    ; /* 80075FC4 nop  */
    r_v0 = r_v1 << 1; /* 80075FC8 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80075FCC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80075FD0 sll $v0, $v0, 2 */
    r_v0 = r_v0 - r_v1; /* 80075FD4 subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 4; /* 80075FD8 sll $v0, $v0, 4 */
    r_v0 = r_v0 - r_v1; /* 80075FDC subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 5; /* 80075FE0 sll $v0, $v0, 5 */
    BL_MULT(r_v0, r_t8); /* 80075FE4 mult $v0, $t8 */
    r_v0 = BL_SRA(r_v0, 31); /* 80075FE8 sra $v0, $v0, 31 */
    r_v1 = hi; /* 80075FEC mfhi $v1 */
    r_v1 = BL_SRA(r_v1, 12); /* 80075FF0 sra $v1, $v1, 12 */
    r_v1 = r_v1 - r_v0; /* 80075FF4 subu $v1, $v1, $v0 */
    bl_sw(r_t0, r_v1); /* 80075FF8 sw $v1, 0x0($t0) */
L_80075FFC:
    r_v1 = 0x800D0000u; /* 80075FFC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076000 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 80076004 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 80076008 lw $a0, %lo(D_800C3EA4)($a0) */
    r_v0 = r_v1 << 1; /* 8007600C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076010 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076014 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076018 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8007601C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a0; /* 80076020 addu $v0, $v0, $a0 */
    bl_sh(r_v0 + 0x6E0u, r_s2); /* 80076024 sh $s2, 0x6E0($v0) */
    r_v1 = 0x800D0000u; /* 80076028 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007602C lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80076030 nop  */
    r_v0 = r_v1 << 1; /* 80076034 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076038 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8007603C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076040 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076044 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a0; /* 80076048 addu $v0, $v0, $a0 */
    bl_sh(r_v0 + 0x6E2u, r_t9); /* 8007604C sh $t9, 0x6E2($v0) */
    r_v1 = 0x800D0000u; /* 80076050 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076054 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80076058 nop  */
    r_v0 = r_v1 << 1; /* 8007605C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076060 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076064 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076068 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8007606C sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a0; /* 80076070 addu $v0, $v0, $a0 */
    bl_sh(r_v0 + 0x6ECu, r_s2); /* 80076074 sh $s2, 0x6EC($v0) */
    r_v0 = 0x800D0000u; /* 80076078 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 8007607C lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80076080 nop  */
    r_v1 = r_v0 << 1; /* 80076084 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80076088 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 8007608C sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80076090 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 80076094 sll $v1, $v1, 2 */
    r_v0 = r_sp + 0x10u; /* 80076098 addiu $v0, $sp, 0x10 */
    r_a2 = r_a2 + r_v0; /* 8007609C addu $a2, $a2, $v0 */
    r_v0 = bl_lw(r_a2); /* 800760A0 lw $v0, 0x0($a2) */
    r_v1 = r_v1 + r_a0; /* 800760A4 addu $v1, $v1, $a0 */
    r_v0 = r_v0 << 1; /* 800760A8 sll $v0, $v0, 1 */
    r_v0 = r_t9 - r_v0; /* 800760AC subu $v0, $t9, $v0 */
    bl_sh(r_v1 + 0x6EEu, r_v0); /* 800760B0 sh $v0, 0x6EE($v1) */
    r_v1 = 0x800D0000u; /* 800760B4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800760B8 lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = 0u | 0x14u; /* 800760BC ori $a1, $zero, 0x14 */
    r_v0 = r_v1 << 1; /* 800760C0 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 800760C4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800760C8 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800760CC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800760D0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a0; /* 800760D4 addu $v0, $v0, $a0 */
    bl_sh(r_v0 + 0x6F8u, r_a1); /* 800760D8 sh $a1, 0x6F8($v0) */
    r_v1 = 0x800D0000u; /* 800760DC lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800760E0 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800760E4 nop  */
    r_v0 = r_v1 << 1; /* 800760E8 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 800760EC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800760F0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800760F4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800760F8 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a0; /* 800760FC addu $v0, $v0, $a0 */
    bl_sh(r_v0 + 0x6FAu, r_t9); /* 80076100 sh $t9, 0x6FA($v0) */
    r_v1 = 0x800D0000u; /* 80076104 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076108 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8007610C nop  */
    r_v0 = r_v1 << 1; /* 80076110 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076114 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076118 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 8007611C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076120 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_a0; /* 80076124 addu $v0, $v0, $a0 */
    bl_sh(r_v0 + 0x704u, r_a1); /* 80076128 sh $a1, 0x704($v0) */
    r_v1 = 0x800D0000u; /* 8007612C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076130 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80076134 nop  */
    r_v0 = r_v1 << 1; /* 80076138 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 8007613C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076140 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076144 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076148 sll $v0, $v0, 2 */
    r_v1 = bl_lw(r_a2); /* 8007614C lw $v1, 0x0($a2) */
    r_v0 = r_v0 + r_a0; /* 80076150 addu $v0, $v0, $a0 */
    r_v1 = r_v1 << 1; /* 80076154 sll $v1, $v1, 1 */
    r_v1 = r_t9 - r_v1; /* 80076158 subu $v1, $t9, $v1 */
    bl_sh(r_v0 + 0x706u, r_v1); /* 8007615C sh $v1, 0x706($v0) */
    r_v1 = 0x800D0000u; /* 80076160 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076164 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80076168 nop  */
    r_v0 = r_v1 << 1; /* 8007616C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076170 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076174 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076178 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 8007617C sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80076180 lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 80076184 addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA244u); /* 80076188 lbu $v1, -0x5DBC($at) */
    r_v0 = r_v0 + r_a0; /* 8007618C addu $v0, $v0, $a0 */
    r_v1 = r_v1 & 0x3Fu; /* 80076190 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 80076194 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x6E4u, r_v1); /* 80076198 sb $v1, 0x6E4($v0) */
    r_v1 = 0x800D0000u; /* 8007619C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800761A0 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 800761A4 nop  */
    r_v0 = r_v1 << 1; /* 800761A8 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 800761AC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800761B0 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800761B4 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 800761B8 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800761BC lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 800761C0 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 800761C4 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 800761C8 addu $at, $v1, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA248u); /* 800761CC lbu $a0, -0x5DB8($at) */
    r_v0 = r_v0 + r_v1; /* 800761D0 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x6E5u, r_a0); /* 800761D4 sb $a0, 0x6E5($v0) */
    r_v0 = 0x800D0000u; /* 800761D8 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 800761DC lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 800761E0 nop  */
    r_v1 = r_v0 << 1; /* 800761E4 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 800761E8 addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 800761EC sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 800761F0 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 800761F4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800761F8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 2; /* 800761FC sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80076200 addu $v1, $v1, $v0 */
    r_at = 0x10000u; /* 80076204 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80076208 addu $at, $v0, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA244u); /* 8007620C lbu $a0, -0x5DBC($at) */
    r_v0 = bl_lw(r_a2); /* 80076210 lw $v0, 0x0($a2) */
    r_a0 = r_a0 & 0x3Fu; /* 80076214 andi $a0, $a0, 0x3F */
    r_v0 = r_v0 + r_a0; /* 80076218 addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 1; /* 8007621C sll $v0, $v0, 1 */
    bl_sb(r_v1 + 0x6F0u, r_v0); /* 80076220 sb $v0, 0x6F0($v1) */
    r_v1 = 0x800D0000u; /* 80076224 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076228 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 8007622C nop  */
    r_v0 = r_v1 << 1; /* 80076230 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076234 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076238 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 8007623C addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80076240 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 80076244 lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80076248 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 8007624C lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 80076250 addu $at, $v1, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA248u); /* 80076254 lbu $a0, -0x5DB8($at) */
    r_v0 = r_v0 + r_v1; /* 80076258 addu $v0, $v0, $v1 */
    bl_sb(r_v0 + 0x6F1u, r_a0); /* 8007625C sb $a0, 0x6F1($v0) */
    r_v1 = 0x800D0000u; /* 80076260 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076264 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 80076268 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 8007626C lw $a0, %lo(D_800C3EA4)($a0) */
    r_v0 = r_v1 << 1; /* 80076270 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80076274 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076278 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 8007627C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076280 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80076284 lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 80076288 addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA244u); /* 8007628C lbu $v1, -0x5DBC($at) */
    r_v0 = r_v0 + r_a0; /* 80076290 addu $v0, $v0, $a0 */
    r_v1 = r_v1 & 0x3Fu; /* 80076294 andi $v1, $v1, 0x3F */
    r_v1 = r_v1 << 1; /* 80076298 sll $v1, $v1, 1 */
    bl_sb(r_v0 + 0x6FCu, r_v1); /* 8007629C sb $v1, 0x6FC($v0) */
    r_v1 = 0x800D0000u; /* 800762A0 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 800762A4 lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 800762A8 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 800762AC lw $a0, %lo(D_800C3EA4)($a0) */
    r_v0 = r_v1 << 1; /* 800762B0 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 800762B4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800762B8 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 800762BC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 800762C0 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 800762C4 lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 800762C8 addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA248u); /* 800762CC lbu $v1, -0x5DB8($at) */
    r_v0 = r_v0 + r_a0; /* 800762D0 addu $v0, $v0, $a0 */
    r_v1 = r_v1 + 0x4u; /* 800762D4 addiu $v1, $v1, 0x4 */
    bl_sb(r_v0 + 0x6FDu, r_v1); /* 800762D8 sb $v1, 0x6FD($v0) */
    r_v0 = 0x800D0000u; /* 800762DC lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 800762E0 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 800762E4 nop  */
    r_v1 = r_v0 << 1; /* 800762E8 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 800762EC addu $v1, $v1, $v0 */
    r_v1 = r_v1 << 2; /* 800762F0 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 800762F4 addu $v1, $v1, $v0 */
    r_v0 = 0x800C0000u; /* 800762F8 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800762FC lw $v0, %lo(D_800C3EA4)($v0) */
    r_v1 = r_v1 << 2; /* 80076300 sll $v1, $v1, 2 */
    r_v1 = r_v1 + r_v0; /* 80076304 addu $v1, $v1, $v0 */
    r_at = 0x10000u; /* 80076308 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8007630C addu $at, $v0, $at */
    r_a0 = bl_lbu(r_at + 0xFFFFA244u); /* 80076310 lbu $a0, -0x5DBC($at) */
    r_v0 = bl_lw(r_a2); /* 80076314 lw $v0, 0x0($a2) */
    r_a0 = r_a0 & 0x3Fu; /* 80076318 andi $a0, $a0, 0x3F */
    r_v0 = r_v0 + r_a0; /* 8007631C addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 1; /* 80076320 sll $v0, $v0, 1 */
    bl_sb(r_v1 + 0x708u, r_v0); /* 80076324 sb $v0, 0x708($v1) */
    r_v1 = 0x800D0000u; /* 80076328 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 8007632C lw $v1, %lo(D_800CCB34)($v1) */
    r_a0 = 0x800C0000u; /* 80076330 lui $a0, %hi(D_800C3EA4) */
    r_a0 = bl_lw(r_a0 + 0x3EA4u); /* 80076334 lw $a0, %lo(D_800C3EA4)($a0) */
    r_v0 = r_v1 << 1; /* 80076338 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 8007633C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076340 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076344 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076348 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 8007634C lui $at, (0x10000 >> 16) */
    r_at = r_a0 + r_at; /* 80076350 addu $at, $a0, $at */
    r_v1 = bl_lbu(r_at + 0xFFFFA248u); /* 80076354 lbu $v1, -0x5DB8($at) */
    r_v0 = r_v0 + r_a0; /* 80076358 addu $v0, $v0, $a0 */
    r_v1 = r_v1 + 0x4u; /* 8007635C addiu $v1, $v1, 0x4 */
    bl_sb(r_v0 + 0x709u, r_v1); /* 80076360 sb $v1, 0x709($v0) */
    r_v1 = 0x800D0000u; /* 80076364 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80076368 lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = 0x800D0000u; /* 8007636C lui $a1, %hi(D_800D2D28) */
    r_a1 = bl_lw(r_a1 + 0x2D28u); /* 80076370 lw $a1, %lo(D_800D2D28)($a1) */
    r_v1 = r_v1 + 0x6u; /* 80076374 addiu $v1, $v1, 0x6 */
    r_v0 = r_v1 << 1; /* 80076378 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 8007637C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80076380 sll $v0, $v0, 2 */
    r_v0 = r_v0 + r_v1; /* 80076384 addu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80076388 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8007638C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = r_v0 << 2; /* 80076390 sll $v0, $v0, 2 */
    r_at = 0x10000u; /* 80076394 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 80076398 addu $at, $v1, $at */
    r_a0 = bl_lhu(r_at + 0xFFFFA2AEu); /* 8007639C lhu $a0, -0x5D52($at) */
    r_v1 = r_v1 + r_v0; /* 800763A0 addu $v1, $v1, $v0 */
    bl_sh(r_v1 + 0x5AEu, r_a0); /* 800763A4 sh $a0, 0x5AE($v1) */
    r_v0 = bl_lbu(r_a1 + 0xCBu); /* 800763A8 lbu $v0, 0xCB($a1) */
    ; /* 800763AC nop  */
    /* 800763B0 beqz $v0, .L800763D4 */
    { int c_ = (r_v0 == 0); r_v0 = 0u | 0x1u; if (c_) goto L_800763D4; }
    /* 800763B8 j .L800763D4 */
    bl_sb(r_a1 + 0x82u, r_v0); goto L_800763D4;
L_800763C0:
    r_v0 = 0x800D0000u; /* 800763C0 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 800763C4 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 800763C8 nop  */
    r_v0 = r_v0 + r_t1; /* 800763CC addu $v0, $v0, $t1 */
    bl_sb(r_v0 + 0x7Fu, 0u); /* 800763D0 sb $zero, 0x7F($v0) */
L_800763D4:
    r_t2 = r_t2 + 0x170u; /* 800763D4 addiu $t2, $t2, 0x170 */
    r_t4 = r_t4 + 0x2u; /* 800763D8 addiu $t4, $t4, 0x2 */
    r_a3 = r_a3 + 0x2u; /* 800763DC addiu $a3, $a3, 0x2 */
    r_t3 = r_t3 + 0x60u; /* 800763E0 addiu $t3, $t3, 0x60 */
    r_t7 = r_t7 + 0x2u; /* 800763E4 addiu $t7, $t7, 0x2 */
    r_t6 = r_t6 + 0x1Cu; /* 800763E8 addiu $t6, $t6, 0x1C */
    r_t1 = r_t1 + 0x1u; /* 800763EC addiu $t1, $t1, 0x1 */
    r_v0 = (u32)((s32)r_t1 < 3); /* 800763F0 slti $v0, $t1, 0x3 */
    /* 800763F4 bnez $v0, .L8007599C */
    { int c_ = (r_v0 != 0); r_t5 = r_t5 + 0x8u; if (c_) goto L_8007599C; }
    r_ra = bl_lw(r_sp + 0x2Cu); /* 800763FC lw $ra, 0x2C($sp) */
    r_s2 = bl_lw(r_sp + 0x28u); /* 80076400 lw $s2, 0x28($sp) */
    r_s1 = bl_lw(r_sp + 0x24u); /* 80076404 lw $s1, 0x24($sp) */
    r_s0 = bl_lw(r_sp + 0x20u); /* 80076408 lw $s0, 0x20($sp) */
    r_sp = r_sp + 0x30u; /* 8007640C addiu $sp, $sp, 0x30 */
    /* 80076410 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800745EC.s (307 instructions; --args 0 --void), hot path batch 4:
 * the menu panel's extra prims (main2 retail).  It switches on the panel
 * state D_800D2D28[0xB7] (1..5) and adds the panel set's double-buffered
 * prims (*(D_800C3EA4 + 0xA230)) to OT entry 1.  Callees through the jal
 * service: AddPrim, func_800743A4/800744BC/80074554, and func_800728B8,
 * which in native-only the optional-strips host claims (target== for any
 * return address), as for the interpreter's jal. */
void func_800745EC(void) {
    static const char FN[] = "func_800745EC";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_v0 = 0x800D0000u; /* 800745EC lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 800745F0 lw $v0, %lo(D_800D2D28)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 800745F4 addiu $sp, $sp, -0x18 */
    bl_sw(r_sp + 0x14u, r_ra); /* 800745F8 sw $ra, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 800745FC sw $s0, 0x10($sp) */
    r_v0 = bl_lbu(r_v0 + 0xB7u); /* 80074600 lbu $v0, 0xB7($v0) */
    ; /* 80074604 nop  */
    r_v1 = r_v0 + 0xFFFFFFFFu; /* 80074608 addiu $v1, $v0, -0x1 */
    r_v0 = (u32)(r_v1 < 0x5u); /* 8007460C sltiu $v0, $v1, 0x5 */
    /* 80074610 beqz $v0, .L80074AA4 */
    { int c_ = (r_v0 == 0); r_v0 = r_v1 << 2; if (c_) goto L_80074AA4; }
    r_at = 0x80070000u; /* 80074618 lui $at, %hi(jtbl_8006FAF4) */
    r_at = r_at + r_v0; /* 8007461C addu $at, $at, $v0 */
    r_v0 = bl_lw(r_at + 0xFFFFFAF4u); /* 80074620 lw $v0, %lo(jtbl_8006FAF4)($at) */
    ; /* 80074624 nop  */
    /* 80074628 jr $v0 */
    { u32 t_ = r_v0; ; switch (t_) { case 0x80074630u: goto L_80074630; case 0x80074870u: goto L_80074870; case 0x80074944u: goto L_80074944; case 0x800749E0u: goto L_800749E0; case 0x80074A9Cu: goto L_80074A9C; default: bl_fail(FN, "jump-table target", t_); } }
L_80074630:
    /* 80074630 jal func_800743A4 */
    ;
    bl_jal(FN, 0x800743A4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800743A4 */
    r_v0 = 0x800C0000u; /* 80074638 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8007463C lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074640 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074644 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074648 lw $v1, -0x5DD0($at) */
    ; /* 8007464C nop  */
    r_v0 = bl_lbu(r_v1 + 0x669u); /* 80074650 lbu $v0, 0x669($v1) */
    ; /* 80074654 nop  */
    /* 80074658 beqz $v0, .L800747A4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800747A4; }
    r_s0 = 0x800D0000u; /* 80074660 lui $s0, %hi(D_800CCB04) */
    r_s0 = r_s0 + 0xFFFFCB04u; /* 80074664 addiu $s0, $s0, %lo(D_800CCB04) */
    r_a0 = bl_lw(r_s0); /* 80074668 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 8007466C lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074670 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074674 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074678 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 8007467C sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x410u; /* 80074680 addiu $a1, $a1, 0x410 */
    /* 80074684 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 8007468C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074690 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074694 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074698 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 8007469C lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 800746A0 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 800746A4 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 800746A8 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800746AC sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800746B0 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800746B4 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x460u; /* 800746B8 addiu $a1, $a1, 0x460 */
    /* 800746BC jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 800746C4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800746C8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 800746CC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800746D0 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800746D4 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 800746D8 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 800746DC lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 800746E0 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800746E4 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800746E8 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800746EC sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x4B0u; /* 800746F0 addiu $a1, $a1, 0x4B0 */
    /* 800746F4 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 800746FC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074700 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074704 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074708 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 8007470C lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 80074710 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 80074714 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074718 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 8007471C sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074720 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074724 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x500u; /* 80074728 addiu $a1, $a1, 0x500 */
    /* 8007472C jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 80074734 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074738 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 8007473C lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074740 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074744 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 80074748 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 8007474C lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074750 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074754 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074758 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 8007475C sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x550u; /* 80074760 addiu $a1, $a1, 0x550 */
    /* 80074764 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 8007476C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074770 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074774 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074778 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 8007477C lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 80074780 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 80074784 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074788 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 8007478C sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074790 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074794 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x5A0u; /* 80074798 addiu $a1, $a1, 0x5A0 */
    /* 8007479C jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
L_800747A4:
    r_v0 = 0x800C0000u; /* 800747A4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800747A8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 800747AC lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800747B0 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800747B4 lw $v1, -0x5DD0($at) */
    ; /* 800747B8 nop  */
    r_v0 = bl_lbu(r_v1 + 0x66Bu); /* 800747BC lbu $v0, 0x66B($v1) */
    ; /* 800747C0 nop  */
    /* 800747C4 beqz $v0, .L80074AA4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074AA4; }
    r_v0 = bl_lbu(r_v1 + 0x66Eu); /* 800747CC lbu $v0, 0x66E($v1) */
    ; /* 800747D0 nop  */
    /* 800747D4 beqz $v0, .L80074804 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074804; }
    r_a0 = 0x800D0000u; /* 800747DC lui $a0, %hi(D_800CCB04) */
    r_a0 = bl_lw(r_a0 + 0xFFFFCB04u); /* 800747E0 lw $a0, %lo(D_800CCB04)($a0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 800747E4 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 800747E8 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800747EC sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800747F0 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800747F4 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x3C0u; /* 800747F8 addiu $a1, $a1, 0x3C0 */
    /* 800747FC jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
L_80074804:
    r_v0 = 0x800C0000u; /* 80074804 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074808 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 8007480C lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074810 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074814 lw $v1, -0x5DD0($at) */
    r_a0 = 0x800D0000u; /* 80074818 lui $a0, %hi(D_800CCB04) */
    r_a0 = bl_lw(r_a0 + 0xFFFFCB04u); /* 8007481C lw $a0, %lo(D_800CCB04)($a0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 80074820 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074824 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074828 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8007482C addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074830 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x140u; /* 80074834 addiu $a1, $a1, 0x140 */
    /* 80074838 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 80074840 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074844 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074848 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8007484C addu $at, $v0, $at */
    r_a0 = bl_lw(r_at + 0xFFFFA230u); /* 80074850 lw $a0, -0x5DD0($at) */
    ; /* 80074854 nop  */
    r_a1 = bl_lbu(r_a0 + 0x66Eu); /* 80074858 lbu $a1, 0x66E($a0) */
    r_a2 = bl_lbu(r_a0 + 0x66Au); /* 8007485C lbu $a2, 0x66A($a0) */
    /* 80074860 jal func_800728B8 */
    r_a0 = r_a0 + 0x190u;
    bl_jal(FN, 0x800728B8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800728B8 */
    /* 80074868 j .L80074A84 */
    ; goto L_80074A84;
L_80074870:
    /* 80074870 jal func_800743A4 */
    ;
    bl_jal(FN, 0x800743A4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800743A4 */
    r_v0 = 0x800C0000u; /* 80074878 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8007487C lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074880 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074884 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074888 lw $v1, -0x5DD0($at) */
    ; /* 8007488C nop  */
    r_v0 = bl_lbu(r_v1 + 0x66Bu); /* 80074890 lbu $v0, 0x66B($v1) */
    ; /* 80074894 nop  */
    /* 80074898 beqz $v0, .L80074904 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074904; }
    r_s0 = 0x800D0000u; /* 800748A0 lui $s0, %hi(D_800CCB04) */
    r_s0 = r_s0 + 0xFFFFCB04u; /* 800748A4 addiu $s0, $s0, %lo(D_800CCB04) */
    r_a0 = bl_lw(r_s0); /* 800748A8 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 800748AC lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 800748B0 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800748B4 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800748B8 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800748BC sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x140u; /* 800748C0 addiu $a1, $a1, 0x140 */
    /* 800748C4 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 800748CC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800748D0 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 800748D4 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800748D8 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800748DC lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 800748E0 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 800748E4 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 800748E8 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800748EC sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800748F0 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800748F4 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x190u; /* 800748F8 addiu $a1, $a1, 0x190 */
    /* 800748FC jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
L_80074904:
    r_v0 = 0x800C0000u; /* 80074904 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074908 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 8007490C lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074910 addu $at, $v0, $at */
    r_v0 = bl_lw(r_at + 0xFFFFA230u); /* 80074914 lw $v0, -0x5DD0($at) */
    ; /* 80074918 nop  */
    r_v0 = bl_lbu(r_v0 + 0x66Du); /* 8007491C lbu $v0, 0x66D($v0) */
    ; /* 80074920 nop  */
    /* 80074924 beqz $v0, .L80074AA4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074AA4; }
    /* 8007492C jal func_800744BC */
    ;
    bl_jal(FN, 0x800744BCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800744BC */
    /* 80074934 jal func_80074554 */
    ;
    bl_jal(FN, 0x80074554u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80074554 */
    /* 8007493C j .L80074AA4 */
    ; goto L_80074AA4;
L_80074944:
    /* 80074944 jal func_800743A4 */
    ;
    bl_jal(FN, 0x800743A4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800743A4 */
    r_v0 = 0x800C0000u; /* 8007494C lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074950 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074954 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074958 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 8007495C lw $v1, -0x5DD0($at) */
    ; /* 80074960 nop  */
    r_v0 = bl_lbu(r_v1 + 0x66Fu); /* 80074964 lbu $v0, 0x66F($v1) */
    ; /* 80074968 nop  */
    /* 8007496C beqz $v0, .L80074AA4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074AA4; }
    r_s0 = 0x800D0000u; /* 80074974 lui $s0, %hi(D_800CCB04) */
    r_s0 = r_s0 + 0xFFFFCB04u; /* 80074978 addiu $s0, $s0, %lo(D_800CCB04) */
    r_a0 = bl_lw(r_s0); /* 8007497C lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 80074980 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074984 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 3; /* 80074988 sll $a1, $v0, 3 */
    r_a1 = r_a1 + r_v0; /* 8007498C addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 2; /* 80074990 sll $a1, $a1, 2 */
    r_a1 = r_a1 + 0x5F0u; /* 80074994 addiu $a1, $a1, 0x5F0 */
    /* 80074998 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 800749A0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800749A4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 800749A8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800749AC addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800749B0 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 800749B4 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 800749B8 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 800749BC addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 1; /* 800749C0 sll $a1, $v0, 1 */
    r_a1 = r_a1 + r_v0; /* 800749C4 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800749C8 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x638u; /* 800749CC addiu $a1, $a1, 0x638 */
    /* 800749D0 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    /* 800749D8 j .L80074AA4 */
    ; goto L_80074AA4;
L_800749E0:
    /* 800749E0 jal func_800743A4 */
    ;
    bl_jal(FN, 0x800743A4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800743A4 */
    r_v0 = 0x800C0000u; /* 800749E8 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800749EC lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 800749F0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800749F4 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800749F8 lw $v1, -0x5DD0($at) */
    ; /* 800749FC nop  */
    r_v0 = bl_lbu(r_v1 + 0x66Bu); /* 80074A00 lbu $v0, 0x66B($v1) */
    ; /* 80074A04 nop  */
    /* 80074A08 beqz $v0, .L80074AA4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074AA4; }
    r_v0 = bl_lbu(r_v1 + 0x66Eu); /* 80074A10 lbu $v0, 0x66E($v1) */
    ; /* 80074A14 nop  */
    /* 80074A18 beqz $v0, .L80074A48 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80074A48; }
    r_a0 = 0x800D0000u; /* 80074A20 lui $a0, %hi(D_800CCB04) */
    r_a0 = bl_lw(r_a0 + 0xFFFFCB04u); /* 80074A24 lw $a0, %lo(D_800CCB04)($a0) */
    r_v0 = bl_lbu(r_v1 + 0x66Cu); /* 80074A28 lbu $v0, 0x66C($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074A2C addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074A30 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074A34 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074A38 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x3C0u; /* 80074A3C addiu $a1, $a1, 0x3C0 */
    /* 80074A40 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
L_80074A48:
    r_v0 = 0x800C0000u; /* 80074A48 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074A4C lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074A50 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074A54 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074A58 lw $v1, -0x5DD0($at) */
    r_a0 = 0x800D0000u; /* 80074A5C lui $a0, %hi(D_800CCB04) */
    r_a0 = bl_lw(r_a0 + 0xFFFFCB04u); /* 80074A60 lw $a0, %lo(D_800CCB04)($a0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 80074A64 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074A68 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074A6C sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074A70 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074A74 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x140u; /* 80074A78 addiu $a1, $a1, 0x140 */
    /* 80074A7C jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
L_80074A84:
    /* 80074A84 jal func_80074554 */
    ;
    bl_jal(FN, 0x80074554u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80074554 */
    /* 80074A8C jal func_800744BC */
    ;
    bl_jal(FN, 0x800744BCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800744BC */
    /* 80074A94 j .L80074AA4 */
    ; goto L_80074AA4;
L_80074A9C:
    /* 80074A9C jal func_800743A4 */
    ;
    bl_jal(FN, 0x800743A4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800743A4 */
L_80074AA4:
    r_ra = bl_lw(r_sp + 0x14u); /* 80074AA4 lw $ra, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 80074AA8 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 80074AAC addiu $sp, $sp, 0x18 */
    /* 80074AB0 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800743A4.s (70 instructions; --args 0 --void), hot path batch 4b: when the panel set *(D_800C3EA4 + 0xA230) is enabled (+0x669), add its four
 * double-buffered 0x28-byte prims (buffer +0x668, 0x50 apart) to OT entry 1
 * (main2 retail; called by 800745EC).  Callee through the jal service: AddPrim. */
void func_800743A4(void) {
    static const char FN[] = "func_800743A4";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_v0 = 0x800C0000u; /* 800743A4 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800743A8 lw $v0, %lo(D_800C3EA4)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 800743AC addiu $sp, $sp, -0x18 */
    bl_sw(r_sp + 0x14u, r_ra); /* 800743B0 sw $ra, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 800743B4 sw $s0, 0x10($sp) */
    r_at = 0x10000u; /* 800743B8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800743BC addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800743C0 lw $v1, -0x5DD0($at) */
    ; /* 800743C4 nop  */
    r_v0 = bl_lbu(r_v1 + 0x669u); /* 800743C8 lbu $v0, 0x669($v1) */
    ; /* 800743CC nop  */
    /* 800743D0 beqz $v0, .L800744A8 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800744A8; }
    r_s0 = 0x800D0000u; /* 800743D8 lui $s0, %hi(D_800CCB04) */
    r_s0 = r_s0 + 0xFFFFCB04u; /* 800743DC addiu $s0, $s0, %lo(D_800CCB04) */
    r_a0 = bl_lw(r_s0); /* 800743E0 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x668u); /* 800743E4 lbu $v0, 0x668($v1) */
    r_a0 = r_a0 + 0x4u; /* 800743E8 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800743EC sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800743F0 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800743F4 sll $a1, $a1, 3 */
    /* 800743F8 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 80074400 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074404 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074408 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8007440C addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074410 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 80074414 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x668u); /* 80074418 lbu $v0, 0x668($v1) */
    r_a0 = r_a0 + 0x4u; /* 8007441C addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074420 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074424 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074428 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x50u; /* 8007442C addiu $a1, $a1, 0x50 */
    /* 80074430 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 80074438 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8007443C lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074440 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074444 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074448 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 8007444C lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x668u); /* 80074450 lbu $v0, 0x668($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074454 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074458 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8007445C addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074460 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0xA0u; /* 80074464 addiu $a1, $a1, 0xA0 */
    /* 80074468 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 80074470 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074474 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074478 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8007447C addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074480 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 80074484 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x668u); /* 80074488 lbu $v0, 0x668($v1) */
    r_a0 = r_a0 + 0x4u; /* 8007448C addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074490 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 80074494 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074498 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0xF0u; /* 8007449C addiu $a1, $a1, 0xF0 */
    /* 800744A0 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
L_800744A8:
    r_ra = bl_lw(r_sp + 0x14u); /* 800744A8 lw $ra, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 800744AC lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 800744B0 addiu $sp, $sp, 0x18 */
    /* 800744B4 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800744BC.s (38 instructions; --args 0 --void), hot path batch 4b: add the panel set's two prims at +0x320/+0x370 (index +0x66A) to OT entry 1
 * (main2 retail; called by 800745EC).  Callee through the jal service: AddPrim. */
void func_800744BC(void) {
    static const char FN[] = "func_800744BC";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_v0 = 0x800C0000u; /* 800744BC lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800744C0 lw $v0, %lo(D_800C3EA4)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 800744C4 addiu $sp, $sp, -0x18 */
    bl_sw(r_sp + 0x10u, r_s0); /* 800744C8 sw $s0, 0x10($sp) */
    r_s0 = 0x800D0000u; /* 800744CC lui $s0, %hi(D_800CCB04) */
    r_s0 = r_s0 + 0xFFFFCB04u; /* 800744D0 addiu $s0, $s0, %lo(D_800CCB04) */
    bl_sw(r_sp + 0x14u, r_ra); /* 800744D4 sw $ra, 0x14($sp) */
    r_at = 0x10000u; /* 800744D8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800744DC addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800744E0 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 800744E4 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 800744E8 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 800744EC addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800744F0 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800744F4 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800744F8 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x320u; /* 800744FC addiu $a1, $a1, 0x320 */
    /* 80074500 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 80074508 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8007450C lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 80074510 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074514 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074518 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 8007451C lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 80074520 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074524 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074528 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8007452C addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074530 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x370u; /* 80074534 addiu $a1, $a1, 0x370 */
    /* 80074538 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_ra = bl_lw(r_sp + 0x14u); /* 80074540 lw $ra, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 80074544 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 80074548 addiu $sp, $sp, 0x18 */
    /* 8007454C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_80074554.s (38 instructions; --args 0 --void), hot path batch 4b: add the panel set's two prims at +0x280/+0x2D0 (index +0x66A) to OT entry 1
 * (main2 retail; called by 800745EC).  Callee through the jal service: AddPrim. */
void func_80074554(void) {
    static const char FN[] = "func_80074554";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_v0 = 0x800C0000u; /* 80074554 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 80074558 lw $v0, %lo(D_800C3EA4)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 8007455C addiu $sp, $sp, -0x18 */
    bl_sw(r_sp + 0x10u, r_s0); /* 80074560 sw $s0, 0x10($sp) */
    r_s0 = 0x800D0000u; /* 80074564 lui $s0, %hi(D_800CCB04) */
    r_s0 = r_s0 + 0xFFFFCB04u; /* 80074568 addiu $s0, $s0, %lo(D_800CCB04) */
    bl_sw(r_sp + 0x14u, r_ra); /* 8007456C sw $ra, 0x14($sp) */
    r_at = 0x10000u; /* 80074570 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80074574 addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 80074578 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 8007457C lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 80074580 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 80074584 addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 80074588 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 8007458C addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 80074590 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x280u; /* 80074594 addiu $a1, $a1, 0x280 */
    /* 80074598 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_v0 = 0x800C0000u; /* 800745A0 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800745A4 lw $v0, %lo(D_800C3EA4)($v0) */
    r_at = 0x10000u; /* 800745A8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800745AC addu $at, $v0, $at */
    r_v1 = bl_lw(r_at + 0xFFFFA230u); /* 800745B0 lw $v1, -0x5DD0($at) */
    r_a0 = bl_lw(r_s0); /* 800745B4 lw $a0, 0x0($s0) */
    r_v0 = bl_lbu(r_v1 + 0x66Au); /* 800745B8 lbu $v0, 0x66A($v1) */
    r_a0 = r_a0 + 0x4u; /* 800745BC addiu $a0, $a0, 0x4 */
    r_a1 = r_v0 << 2; /* 800745C0 sll $a1, $v0, 2 */
    r_a1 = r_a1 + r_v0; /* 800745C4 addu $a1, $a1, $v0 */
    r_a1 = r_a1 << 3; /* 800745C8 sll $a1, $a1, 3 */
    r_a1 = r_a1 + 0x2D0u; /* 800745CC addiu $a1, $a1, 0x2D0 */
    /* 800745D0 jal AddPrim */
    r_a1 = r_a1 + r_v1;
    bl_jal(FN, 0x80043B48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* AddPrim */
    r_ra = bl_lw(r_sp + 0x14u); /* 800745D8 lw $ra, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 800745DC lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 800745E0 addiu $sp, $sp, 0x18 */
    /* 800745E4 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#endif /* XENO_PC_PORT && !XENO_BATTLE_VIEW_MATRIX_STAGED: h-hotpath */

#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void func_80085350(void) {
    static const char FN[] = "func_80085350";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x0u, &bl_saved);
    r_v1 = 0u + 0u; /* 80085350 addu $v1, $zero, $zero */
    r_a1 = 0u | 0xFFu; /* 80085354 ori $a1, $zero, 0xFF */
    r_a0 = 0x800D0000u; /* 80085358 lui $a0, %hi(D_800D2D70) */
    r_a0 = r_a0 + 0x2D70u; /* 8008535C addiu $a0, $a0, %lo(D_800D2D70) */
L_80085360:
    r_at = 0x800D0000u; /* 80085360 lui $at, %hi(D_800D2D5C) */
    r_at = r_at + r_v1; /* 80085364 addu $at, $at, $v1 */
    bl_sb(r_at + 0x2D5Cu, r_a1); /* 80085368 sb $a1, %lo(D_800D2D5C)($at) */
    bl_sh(r_a0, 0u); /* 8008536C sh $zero, 0x0($a0) */
    r_v1 = r_v1 + 0x1u; /* 80085370 addiu $v1, $v1, 0x1 */
    r_v0 = (u32)((s32)r_v1 < 11); /* 80085374 slti $v0, $v1, 0xB */
    /* 80085378 bnez $v0, .L80085360 */
    { int c_ = (r_v0 != 0); r_a0 = r_a0 + 0x2u; if (c_) goto L_80085360; }
    /* 80085380 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop
#endif

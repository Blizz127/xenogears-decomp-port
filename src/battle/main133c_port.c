/* Host-only lift of the battle archive-loading entry. Calls continue through
 * the existing service bridge; unconverted module code must refuse. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void func_80070E2C(void) {
    static const char FN[] = "func_80070E2C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_v0 = 0x800C0000u; /* 80070E2C lui $v0, %hi(D_800C3D48) */
    r_v0 = bl_lbu(r_v0 + 0x3D48u); /* 80070E30 lbu $v0, %lo(D_800C3D48)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 80070E34 addiu $sp, $sp, -0x18 */
    /* 80070E38 beqz $v0, .L80070EA0 */
    { int c_ = (r_v0 == 0); bl_sw(r_sp + 0x10u, r_ra); if (c_) goto L_80070EA0; }
    /* 80070E40 jal func_8008AB4C */
    ;
    bl_jal(FN, 0x8008AB4Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008AB4C */
    r_a0 = 0u | 0x4u; /* 80070E48 ori $a0, $zero, 0x4 */
    /* 80070E4C jal func_8008ABB8 */
    r_a1 = 0u | 0x1u;
    bl_jal(FN, 0x8008ABB8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008ABB8 */
    r_a0 = 0x7FE10000u; /* 80070E54 lui $a0, (0x7FE1B000 >> 16) */
    r_a0 = r_a0 | 0xB000u; /* 80070E58 ori $a0, $a0, (0x7FE1B000 & 0xFFFF) */
    r_a0 = r_v0 + r_a0; /* 80070E5C addu $a0, $v0, $a0 */
    r_at = 0x800D0000u; /* 80070E60 lui $at, %hi(D_800D3284) */
    bl_sw(r_at + 0x3284u, r_v0); /* 80070E64 sw $v0, %lo(D_800D3284)($at) */
    /* 80070E68 jal func_8008ABB8 */
    r_a1 = 0u | 0x1u;
    bl_jal(FN, 0x8008ABB8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008ABB8 */
    r_a0 = 0u | 0x1u; /* 80070E70 ori $a0, $zero, 0x1 */
    r_a1 = 0x801E0000u; /* 80070E74 lui $a1, (0x801E5000 >> 16) */
    r_a1 = r_a1 | 0x5000u; /* 80070E78 ori $a1, $a1, (0x801E5000 & 0xFFFF) */
    r_a2 = 0u + 0u; /* 80070E7C addu $a2, $zero, $zero */
    r_at = 0x800D0000u; /* 80070E80 lui $at, %hi(D_800D328C) */
    bl_sw(r_at + 0x328Cu, r_v0); /* 80070E84 sw $v0, %lo(D_800D328C)($at) */
    /* 80070E88 jal ArchiveReadFileToBuffer */
    r_a3 = 0u | 0x80u;
    bl_jal(FN, 0x800295D8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveReadFileToBuffer */
    /* 80070E90 jal func_8008AC50 */
    ;
    bl_jal(FN, 0x8008AC50u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8008AC50 */
    /* 80070E98 jal func_801E5160 */
    ;
    bl_jal(FN, 0x801E5160u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_801E5160 */
L_80070EA0:
    r_ra = bl_lw(r_sp + 0x10u); /* 80070EA0 lw $ra, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 80070EA4 addiu $sp, $sp, 0x18 */
    /* 80070EA8 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

void func_80070EB0(u32 arg0) {
    static const char FN[] = "func_80070EB0";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_a0 = arg0;
    r_v0 = 0x800C0000u; /* 80070EB0 lui $v0, %hi(D_800C3D48) */
    r_v0 = bl_lbu(r_v0 + 0x3D48u); /* 80070EB4 lbu $v0, %lo(D_800C3D48)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 80070EB8 addiu $sp, $sp, -0x18 */
    /* 80070EBC beqz $v0, .L80070ECC */
    { int c_ = (r_v0 == 0); bl_sw(r_sp + 0x10u, r_ra); if (c_) goto L_80070ECC; }
    /* 80070EC4 jal func_801E879C */
    r_a0 = r_a0 & 0xFFu;
    bl_jal(FN, 0x801E879Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_801E879C */
L_80070ECC:
    r_ra = bl_lw(r_sp + 0x10u); /* 80070ECC lw $ra, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 80070ED0 addiu $sp, $sp, 0x18 */
    /* 80070ED4 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop
#endif

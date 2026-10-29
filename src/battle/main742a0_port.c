/* Host-only instruction lift for battle target processing. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void func_800742A0(void) {
    static const char FN[] = "func_800742A0";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFE0u; /* 800742A0 addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x10u, r_s0); /* 800742A4 sw $s0, 0x10($sp) */
    r_s0 = 0u + 0u; /* 800742A8 addu $s0, $zero, $zero */
    bl_sw(r_sp + 0x18u, r_s2); /* 800742AC sw $s2, 0x18($sp) */
    r_s2 = 0u + 0u; /* 800742B0 addu $s2, $zero, $zero */
    bl_sw(r_sp + 0x1Cu, r_ra); /* 800742B4 sw $ra, 0x1C($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 800742B8 sw $s1, 0x14($sp) */
L_800742BC:
    r_v1 = 0x800D0000u; /* 800742BC lui $v1, %hi(D_800D2D28) */
    r_v1 = bl_lw(r_v1 + 0x2D28u); /* 800742C0 lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 800742C4 nop  */
    r_v0 = r_v1 + r_s0; /* 800742C8 addu $v0, $v1, $s0 */
    r_v0 = bl_lbu(r_v0 + 0x7Cu); /* 800742CC lbu $v0, 0x7C($v0) */
    ; /* 800742D0 nop  */
    /* 800742D4 beqz $v0, .L80074378 */
    { int c_ = (r_v0 == 0); r_v0 = r_s0 << 2; if (c_) goto L_80074378; }
    r_v0 = r_v0 + r_v1; /* 800742DC addu $v0, $v0, $v1 */
    bl_sw(r_v0 + 0xE0u, 0u); /* 800742E0 sw $zero, 0xE0($v0) */
    bl_sw(r_v0 + 0xECu, 0u); /* 800742E4 sw $zero, 0xEC($v0) */
    r_at = 0x800C0000u; /* 800742E8 lui $at, %hi(D_800C3EB6) */
    r_at = r_at + r_s2; /* 800742EC addu $at, $at, $s2 */
    r_v1 = bl_lbu(r_at + 0x3EB6u); /* 800742F0 lbu $v1, %lo(D_800C3EB6)($at) */
    r_v0 = 0u | 0x7Fu; /* 800742F4 ori $v0, $zero, 0x7F */
    /* 800742F8 beq $v1, $v0, .L80074378 */
    { int c_ = (r_v1 == r_v0); r_s1 = r_s0 << 3; if (c_) goto L_80074378; }
    /* 80074300 jal func_80073380 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80073380u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80073380 */
    r_at = 0x800D0000u; /* 80074308 lui $at, %hi(D_800D32A1) */
    r_at = r_at + r_s1; /* 8007430C addu $at, $at, $s1 */
    r_a1 = bl_lbu(r_at + 0x32A1u); /* 80074310 lbu $a1, %lo(D_800D32A1)($at) */
    /* 80074314 jal func_80072F38 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80072F38u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80072F38 */
    r_at = 0x800D0000u; /* 8007431C lui $at, %hi(D_800D32A1) */
    r_at = r_at + r_s1; /* 80074320 addu $at, $at, $s1 */
    r_v1 = bl_lbu(r_at + 0x32A1u); /* 80074324 lbu $v1, %lo(D_800D32A1)($at) */
    ; /* 80074328 nop  */
    /* 8007432C beqz $v1, .L80074344 */
    { int c_ = (r_v1 == 0); r_a0 = r_s0 + 0u; if (c_) goto L_80074344; }
    /* 80074334 jal func_80072DA8 */
    r_a1 = r_v0 & 0xFFu;
    bl_jal(FN, 0x80072DA8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80072DA8 */
    /* 8007433C j .L8007434C */
    ; goto L_8007434C;
L_80074344:
    /* 80074344 jal func_80072A9C */
    r_a1 = r_v0 & 0xFFu;
    bl_jal(FN, 0x80072A9Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80072A9C */
L_8007434C:
    r_v0 = 0x800D0000u; /* 8007434C lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 80074350 lw $v0, %lo(D_800D2D28)($v0) */
    r_v1 = 0x800D0000u; /* 80074354 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lbu(r_v1 + 0xFFFFCB34u); /* 80074358 lbu $v1, %lo(D_800CCB34)($v1) */
    r_v0 = r_v0 + r_s0; /* 8007435C addu $v0, $v0, $s0 */
    bl_sb(r_v0 + 0x93u, r_v1); /* 80074360 sb $v1, 0x93($v0) */
    r_v0 = 0x800D0000u; /* 80074364 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 80074368 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 8007436C nop  */
    r_v0 = r_v0 + r_s0; /* 80074370 addu $v0, $v0, $s0 */
    bl_sb(r_v0 + 0x7Cu, 0u); /* 80074374 sb $zero, 0x7C($v0) */
L_80074378:
    r_s0 = r_s0 + 0x1u; /* 80074378 addiu $s0, $s0, 0x1 */
    r_v0 = (u32)((s32)r_s0 < 3); /* 8007437C slti $v0, $s0, 0x3 */
    /* 80074380 bnez $v0, .L800742BC */
    { int c_ = (r_v0 != 0); r_s2 = r_s2 + 0x1Cu; if (c_) goto L_800742BC; }
    r_ra = bl_lw(r_sp + 0x1Cu); /* 80074388 lw $ra, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 8007438C lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 80074390 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 80074394 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 80074398 addiu $sp, $sp, 0x20 */
    /* 8007439C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop
#endif

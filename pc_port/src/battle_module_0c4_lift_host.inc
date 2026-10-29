/* Archive 0x0C/file 4 only: these addresses overlap unrelated overlays.
 * Static lifts are reached only by battle_module_0c4_lift_try_host below.
 * Generated from the asm-only splat listing using the unchanged lifter;
 * scratch normalization replaces namespaced labels with their numeric
 * addresses and verifies every instruction byte against the extracted image.
 * The bodies use --void --jal-service, args 0 (6AC4, 6DC8, 6A4C) / 1 (6F00,
 * 6E48, 6D6C, 6D34, 6C80): 6E48, 6D6C, 6D34 and 6C80 are timer callbacks
 * of 6F00's task, in that order;
 * 6DC8 (func_8002DDE4 on a stack of its own, a heap block) and 6A4C (the
 * slot tasks, 800BB760) are their module callees.
 * Startup actor binding is not hosted here; its 800BA984 is an adopted leaf. */
#define BL_NO_GTE
#include "battle_lift.h"
#undef BL_NO_GTE

/* The unchanged lifter declares the complete register file, including words
 * these two bodies do not use. Keep warning suppression local to generated C. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

static void func_801E6AC4(void) {
    static const char FN[] = "func_801E6AC4";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFC8u; /* 801E6AC4 addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x18u, r_s2); /* 801E6AC8 sw $s2, 0x18($sp) */
    r_s2 = 0u + 0u; /* 801E6ACC addu $s2, $zero, $zero */
    bl_sw(r_sp + 0x28u, r_s6); /* 801E6AD0 sw $s6, 0x28($sp) */
    r_s6 = 0x801F0000u; /* 801E6AD4 lui $s6, %hi(D_801E9638) */
    r_v0 = 0x801F0000u; /* 801E6AD8 lui $v0, %hi(D_801E962C) */
    bl_sw(r_sp + 0x30u, r_fp); /* 801E6ADC sw $fp, 0x30($sp) */
    r_fp = r_v0 + 0xFFFF962Cu; /* 801E6AE0 addiu $fp, $v0, %lo(D_801E962C) */
    r_v0 = 0x801F0000u; /* 801E6AE4 lui $v0, %hi(D_801E95BC) */
    bl_sw(r_sp + 0x2Cu, r_s7); /* 801E6AE8 sw $s7, 0x2C($sp) */
    r_s7 = r_v0 + 0xFFFF95BCu; /* 801E6AEC addiu $s7, $v0, %lo(D_801E95BC) */
    r_v0 = 0x800C0000u; /* 801E6AF0 lui $v0, %hi(D_800C3EB0) */
    bl_sw(r_sp + 0x20u, r_s4); /* 801E6AF4 sw $s4, 0x20($sp) */
    r_s4 = r_v0 + 0x3EB0u; /* 801E6AF8 addiu $s4, $v0, %lo(D_800C3EB0) */
    bl_sw(r_sp + 0x24u, r_s5); /* 801E6AFC sw $s5, 0x24($sp) */
    r_s5 = r_s4 + 0u; /* 801E6B00 addu $s5, $s4, $zero */
    bl_sw(r_sp + 0x1Cu, r_s3); /* 801E6B04 sw $s3, 0x1C($sp) */
    r_s3 = r_s4 + 0u; /* 801E6B08 addu $s3, $s4, $zero */
    bl_sw(r_sp + 0x34u, r_ra); /* 801E6B0C sw $ra, 0x34($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 801E6B10 sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 801E6B14 sw $s0, 0x10($sp) */
L_801E6B18:
    r_s1 = bl_lbu(r_s3 + 0x6u); /* 801E6B18 lbu $s1, 0x6($s3) */
    ; /* 801E6B1C nop  */
    r_v0 = (u32)((s32)r_s1 < 17); /* 801E6B20 slti $v0, $s1, 0x11 */
    /* 801E6B24 beqz $v0, .L801E6BC4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_801E6BC4; }
    r_v0 = bl_lbu(r_s3 + 0x8u); /* 801E6B2C lbu $v0, 0x8($s3) */
    ; /* 801E6B30 nop  */
    /* 801E6B34 bnez $v0, .L801E6BC4 */
    { int c_ = (r_v0 != 0); r_a0 = r_s2 + 0u; if (c_) goto L_801E6BC4; }
    r_v1 = 0u | 0x8000u; /* 801E6B3C ori $v1, $zero, 0x8000 */
    r_v1 = r_s4 + r_v1; /* 801E6B40 addu $v1, $s4, $v1 */
    r_a2 = bl_lhu(r_s6 + 0xFFFF9638u); /* 801E6B44 lhu $a2, %lo(D_801E9638)($s6) */
    r_v0 = 0u + 0x1C0u; /* 801E6B48 addiu $v0, $zero, 0x1C0 */
    bl_sh(r_v1 + 0xD2Au, r_v0); /* 801E6B4C sh $v0, 0xD2A($v1) */
    r_v0 = r_a2 + 0x100u; /* 801E6B50 addiu $v0, $a2, 0x100 */
    bl_sh(r_v1 + 0xD28u, r_v0); /* 801E6B54 sh $v0, 0xD28($v1) */
    r_v0 = r_s1 + r_fp; /* 801E6B58 addu $v0, $s1, $fp */
    r_v0 = bl_lbu(r_v0); /* 801E6B5C lbu $v0, 0x0($v0) */
    r_a1 = r_s2 + 0u; /* 801E6B60 addu $a1, $s2, $zero */
    r_a2 = r_a2 + r_v0; /* 801E6B64 addu $a2, $a2, $v0 */
    bl_sh(r_s6 + 0xFFFF9638u, r_a2); /* 801E6B68 sh $a2, %lo(D_801E9638)($s6) */
    /* 801E6B6C jal func_801E67A4 */
    r_a2 = 0u + 0x1u;
    bl_jal(FN, 0x801E67A4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_801E67A4 */
    r_v0 = 0u | 0x8C8Cu; /* 801E6B74 ori $v0, $zero, 0x8C8C */
    r_v0 = r_s5 + r_v0; /* 801E6B78 addu $v0, $s5, $v0 */
    r_s0 = bl_lw(r_v0); /* 801E6B7C lw $s0, 0x0($v0) */
    r_v0 = r_s1 << 3; /* 801E6B80 sll $v0, $s1, 3 */
    r_v0 = r_v0 + r_s7; /* 801E6B84 addu $v0, $v0, $s7 */
    r_v0 = bl_lw(r_v0 + 0x4u); /* 801E6B88 lw $v0, 0x4($v0) */
    r_v1 = bl_lw(r_s0 + 0x7Cu); /* 801E6B8C lw $v1, 0x7C($s0) */
    ; /* 801E6B90 nop  */
    bl_sw(r_v1, r_v0); /* 801E6B94 sw $v0, 0x0($v1) */
    r_v0 = bl_lw(r_s0 + 0x20u); /* 801E6B98 lw $v0, 0x20($s0) */
    ; /* 801E6B9C nop  */
    r_a0 = bl_lw(r_v0 + 0x2Cu); /* 801E6BA0 lw $a0, 0x2C($v0) */
    /* 801E6BA4 jal HeapFree */
    ;
    bl_jal(FN, 0x800320E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapFree */
    r_a0 = 0u + 0x300u; /* 801E6BAC addiu $a0, $zero, 0x300 */
    /* 801E6BB0 jal HeapAlloc */
    r_a1 = 0u + 0u;
    bl_jal(FN, 0x80031BDCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapAlloc */
    r_v1 = bl_lw(r_s0 + 0x20u); /* 801E6BB8 lw $v1, 0x20($s0) */
    ; /* 801E6BBC nop  */
    bl_sw(r_v1 + 0x2Cu, r_v0); /* 801E6BC0 sw $v0, 0x2C($v1) */
L_801E6BC4:
    r_s5 = r_s5 + 0x4u; /* 801E6BC4 addiu $s5, $s5, 0x4 */
    r_s4 = r_s4 + 0xCu; /* 801E6BC8 addiu $s4, $s4, 0xC */
    r_s2 = r_s2 + 0x1u; /* 801E6BCC addiu $s2, $s2, 0x1 */
    r_v0 = 0u + 0x3u; /* 801E6BD0 addiu $v0, $zero, 0x3 */
    /* 801E6BD4 bne $s2, $v0, .L801E6B18 */
    { int c_ = (r_s2 != r_v0); r_s3 = r_s3 + 0x1Cu; if (c_) goto L_801E6B18; }
    r_v0 = 0x800D0000u; /* 801E6BDC lui $v0, %hi(D_800D36B8) */
    r_v0 = bl_lbu(r_v0 + 0x36B8u); /* 801E6BE0 lbu $v0, %lo(D_800D36B8)($v0) */
    ; /* 801E6BE4 nop  */
    /* 801E6BE8 bnez $v0, .L801E6C4C */
    { int c_ = (r_v0 != 0); r_s2 = 0u + 0u; if (c_) goto L_801E6C4C; }
    r_v0 = 0x800C0000u; /* 801E6BF0 lui $v0, %hi(D_800C3EB0) */
    r_s1 = r_v0 + 0x3EB0u; /* 801E6BF4 addiu $s1, $v0, %lo(D_800C3EB0) */
L_801E6BF8:
    r_v0 = 0u | 0x8C8Cu; /* 801E6BF8 ori $v0, $zero, 0x8C8C */
    r_v0 = r_s1 + r_v0; /* 801E6BFC addu $v0, $s1, $v0 */
    r_s0 = bl_lw(r_v0); /* 801E6C00 lw $s0, 0x0($v0) */
    ; /* 801E6C04 nop  */
    /* 801E6C08 beqz $s0, .L801E6C3C */
    { int c_ = (r_s0 == 0); ; if (c_) goto L_801E6C3C; }
    /* 801E6C10 jal func_800BA8F4 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x800BA8F4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BA8F4 */
    r_a0 = r_s0 + 0u; /* 801E6C18 addu $a0, $s0, $zero */
    r_v0 = bl_lh(r_a0 + 0x2u); /* 801E6C1C lh $v0, 0x2($a0) */
    r_v1 = bl_lh(r_a0 + 0x6u); /* 801E6C20 lh $v1, 0x6($a0) */
    r_a2 = bl_lh(r_a0 + 0xAu); /* 801E6C24 lh $a2, 0xA($a0) */
    r_a1 = 0u + 0x17u; /* 801E6C28 addiu $a1, $zero, 0x17 */
    bl_sh(r_a0 + 0xA0u, r_v0); /* 801E6C2C sh $v0, 0xA0($a0) */
    bl_sh(r_a0 + 0xA2u, r_v1); /* 801E6C30 sh $v1, 0xA2($a0) */
    /* 801E6C34 jal func_800245D8 */
    bl_sh(r_a0 + 0xA4u, r_a2);
    bl_jal(FN, 0x800245D8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800245D8 */
L_801E6C3C:
    r_s2 = r_s2 + 0x1u; /* 801E6C3C addiu $s2, $s2, 0x1 */
    r_v0 = 0u + 0x3u; /* 801E6C40 addiu $v0, $zero, 0x3 */
    /* 801E6C44 bne $s2, $v0, .L801E6BF8 */
    { int c_ = (r_s2 != r_v0); r_s1 = r_s1 + 0x4u; if (c_) goto L_801E6BF8; }
L_801E6C4C:
    r_ra = bl_lw(r_sp + 0x34u); /* 801E6C4C lw $ra, 0x34($sp) */
    r_fp = bl_lw(r_sp + 0x30u); /* 801E6C50 lw $fp, 0x30($sp) */
    r_s7 = bl_lw(r_sp + 0x2Cu); /* 801E6C54 lw $s7, 0x2C($sp) */
    r_s6 = bl_lw(r_sp + 0x28u); /* 801E6C58 lw $s6, 0x28($sp) */
    r_s5 = bl_lw(r_sp + 0x24u); /* 801E6C5C lw $s5, 0x24($sp) */
    r_s4 = bl_lw(r_sp + 0x20u); /* 801E6C60 lw $s4, 0x20($sp) */
    r_s3 = bl_lw(r_sp + 0x1Cu); /* 801E6C64 lw $s3, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 801E6C68 lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 801E6C6C lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 801E6C70 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x38u; /* 801E6C74 addiu $sp, $sp, 0x38 */
    /* 801E6C78 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

static void func_801E6F00(u32 arg0) {
    static const char FN[] = "func_801E6F00";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE0u; /* 801E6F00 addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x14u, r_s1); /* 801E6F04 sw $s1, 0x14($sp) */
    r_s1 = r_a0 + 0u; /* 801E6F08 addu $s1, $a0, $zero */
    bl_sw(r_sp + 0x18u, r_ra); /* 801E6F0C sw $ra, 0x18($sp) */
    /* 801E6F10 jal ArchiveDataSync */
    bl_sw(r_sp + 0x10u, r_s0);
    bl_jal(FN, 0x800286CCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveDataSync */
    r_a0 = 0u + 0x2Cu; /* 801E6F18 addiu $a0, $zero, 0x2C */
    r_a1 = 0u + 0u; /* 801E6F1C addu $a1, $zero, $zero */
    /* 801E6F20 jal ArchiveSetIndex */
    r_s0 = r_v0 + 0u;
    bl_jal(FN, 0x80028470u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveSetIndex */
    /* 801E6F28 bnez $s0, .L801E6FD4 */
    { int c_ = (r_s0 != 0); ; if (c_) goto L_801E6FD4; }
    /* 801E6F30 jal func_801E6AC4 */
    r_s0 = r_s1 + 0x50u;
    bl_jal(FN, 0x801E6AC4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_801E6AC4 */
    /* 801E6F38 jal ArchiveDecodeAlignedSize */
    r_a0 = 0u + 0x1u;
    bl_jal(FN, 0x800288ECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveDecodeAlignedSize */
    r_a0 = r_v0 + 0u; /* 801E6F40 addu $a0, $v0, $zero */
    /* 801E6F44 jal HeapAlloc */
    r_a1 = 0u + 0x1u;
    bl_jal(FN, 0x80031BDCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapAlloc */
    r_a0 = 0u + 0x2u; /* 801E6F4C addiu $a0, $zero, 0x2 */
    bl_sw(r_s0 + 0x4u, r_v0); /* 801E6F50 sw $v0, 0x4($s0) */
    bl_sw(r_s1 + 0x28u, r_v0); /* 801E6F54 sw $v0, 0x28($s1) */
    r_v0 = 0u + 0x1u; /* 801E6F58 addiu $v0, $zero, 0x1 */
    /* 801E6F5C jal ArchiveDecodeAlignedSize */
    bl_sh(r_s1 + 0x50u, r_v0);
    bl_jal(FN, 0x800288ECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveDecodeAlignedSize */
    r_a0 = r_v0 + 0u; /* 801E6F64 addu $a0, $v0, $zero */
    /* 801E6F68 jal HeapAlloc */
    r_a1 = 0u + 0u;
    bl_jal(FN, 0x80031BDCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapAlloc */
    r_a0 = 0u + 0x3u; /* 801E6F70 addiu $a0, $zero, 0x3 */
    r_v1 = 0x800D0000u; /* 801E6F74 lui $v1, %hi(D_800D2D54) */
    bl_sw(r_s1 + 0x5Cu, r_v0); /* 801E6F78 sw $v0, 0x5C($s1) */
    bl_sw(r_s1 + 0x24u, r_v0); /* 801E6F7C sw $v0, 0x24($s1) */
    bl_sw(r_v1 + 0x2D54u, r_v0); /* 801E6F80 sw $v0, %lo(D_800D2D54)($v1) */
    r_v0 = 0u + 0x2u; /* 801E6F84 addiu $v0, $zero, 0x2 */
    /* 801E6F88 jal ArchiveDecodeAlignedSize */
    bl_sh(r_s0 + 0x8u, r_v0);
    bl_jal(FN, 0x800288ECu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveDecodeAlignedSize */
    r_a0 = r_v0 + 0u; /* 801E6F90 addu $a0, $v0, $zero */
    /* 801E6F94 jal HeapAlloc */
    r_a1 = 0u + 0u;
    bl_jal(FN, 0x80031BDCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapAlloc */
    r_a0 = r_s0 + 0u; /* 801E6F9C addu $a0, $s0, $zero */
    r_a1 = 0u + 0u; /* 801E6FA0 addu $a1, $zero, $zero */
    bl_sw(r_s1 + 0x64u, r_v0); /* 801E6FA4 sw $v0, 0x64($s1) */
    bl_sw(r_s1 + 0x2Cu, r_v0); /* 801E6FA8 sw $v0, 0x2C($s1) */
    r_v0 = 0u + 0x3u; /* 801E6FAC addiu $v0, $zero, 0x3 */
    r_a2 = r_a1 + 0u; /* 801E6FB0 addu $a2, $a1, $zero */
    bl_sh(r_a0 + 0x10u, r_v0); /* 801E6FB4 sh $v0, 0x10($a0) */
    bl_sw(r_a0 + 0x1Cu, 0u); /* 801E6FB8 sw $zero, 0x1C($a0) */
    /* 801E6FBC jal func_80029AFC */
    bl_sh(r_a0 + 0x18u, 0u);
    bl_jal(FN, 0x80029AFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80029AFC */
    r_a0 = r_s1 + 0u; /* 801E6FC4 addu $a0, $s1, $zero */
    r_a1 = 0x801E0000u; /* 801E6FC8 lui $a1, %hi(func_801E6E48) */
    /* 801E6FCC jal TimerWorkListSetTaskCallback */
    r_a1 = r_a1 + 0x6E48u;
    bl_jal(FN, 0x8001CD6Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListSetTaskCallback */
L_801E6FD4:
    r_ra = bl_lw(r_sp + 0x18u); /* 801E6FD4 lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 801E6FD8 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 801E6FDC lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 801E6FE0 addiu $sp, $sp, 0x20 */
    /* 801E6FE4 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

static void func_801E6DC8(void) {
    static const char FN[] = "func_801E6DC8";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x28u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFD8u; /* 801E6DC8 addiu $sp, $sp, -0x28 */
    r_a0 = 0u + 0x2000u; /* 801E6DCC addiu $a0, $zero, 0x2000 */
    r_a1 = 0u + 0x1u; /* 801E6DD0 addiu $a1, $zero, 0x1 */
    bl_sw(r_sp + 0x24u, r_ra); /* 801E6DD4 sw $ra, 0x24($sp) */
    /* 801E6DD8 jal HeapAlloc */
    bl_sw(r_sp + 0x20u, r_s0);
    bl_jal(FN, 0x80031BDCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapAlloc */
    r_s0 = r_v0 + 0u; /* 801E6DE0 addu $s0, $v0, $zero */
    r_v0 = r_s0 + 0x1F00u; /* 801E6DE4 addiu $v0, $s0, 0x1F00 */
    r_t0 = r_v0 + 0u; /* 801E6DE8 addu $t0, $v0, $zero */
    bl_sw(r_t0, r_sp); /* 801E6DEC sw $sp, 0x0($t0) */
    r_t0 = r_t0 + 0xFFFFFFFCu; /* 801E6DF0 addiu $t0, $t0, -0x4 */
    r_sp = r_t0 + 0u; /* 801E6DF4 addu $sp, $t0, $zero */
    r_v0 = 0x801F0000u; /* 801E6DF8 lui $v0, %hi(D_801E96B4) */
    r_a1 = 0u + 0u; /* 801E6DFC addu $a1, $zero, $zero */
    r_a2 = r_a1 + 0u; /* 801E6E00 addu $a2, $a1, $zero */
    r_a0 = bl_lw(r_v0 + 0xFFFF96B4u); /* 801E6E04 lw $a0, %lo(D_801E96B4)($v0) */
    r_a3 = r_a1 + 0u; /* 801E6E08 addu $a3, $a1, $zero */
    bl_sw(r_sp + 0x10u, 0u); /* 801E6E0C sw $zero, 0x10($sp) */
    bl_sw(r_sp + 0x14u, 0u); /* 801E6E10 sw $zero, 0x14($sp) */
    /* 801E6E14 jal func_8002DDE4 */
    bl_sw(r_sp + 0x18u, 0u);
    bl_jal(FN, 0x8002DDE4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8002DDE4 */
    /* 801E6E1C jal DrawSync */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x800445D0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* DrawSync */
    r_sp = r_sp + 0x4u; /* 801E6E24 addiu $sp, $sp, 0x4 */
    r_sp = bl_lw(r_sp); /* 801E6E28 lw $sp, 0x0($sp) */
    /* 801E6E2C jal HeapFree */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x800320E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapFree */
    r_ra = bl_lw(r_sp + 0x24u); /* 801E6E34 lw $ra, 0x24($sp) */
    r_s0 = bl_lw(r_sp + 0x20u); /* 801E6E38 lw $s0, 0x20($sp) */
    r_sp = r_sp + 0x28u; /* 801E6E3C addiu $sp, $sp, 0x28 */
    /* 801E6E40 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

static void func_801E6E48(u32 arg0) {
    static const char FN[] = "func_801E6E48";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFC8u; /* 801E6E48 addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x30u, r_s0); /* 801E6E4C sw $s0, 0x30($sp) */
    bl_sw(r_sp + 0x34u, r_ra); /* 801E6E50 sw $ra, 0x34($sp) */
    /* 801E6E54 jal ArchiveDataSync */
    r_s0 = r_a0 + 0u;
    bl_jal(FN, 0x800286CCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveDataSync */
    /* 801E6E5C bnez $v0, .L801E6EEC */
    { int c_ = (r_v0 != 0); r_v0 = 0x801F0000u; if (c_) goto L_801E6EEC; }
    r_v1 = bl_lw(r_s0 + 0x28u); /* 801E6E64 lw $v1, 0x28($s0) */
    /* 801E6E68 jal func_801E6DC8 */
    bl_sw(r_v0 + 0xFFFF96B4u, r_v1);
    bl_jal(FN, 0x801E6DC8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_801E6DC8 */
    r_a0 = 0x80070000u; /* 801E6E70 lui $a0, %hi(D_8006BE10) */
    r_a0 = r_a0 + 0xFFFFBE10u; /* 801E6E74 addiu $a0, $a0, %lo(D_8006BE10) */
    r_v0 = 0u + 0x380u; /* 801E6E78 addiu $v0, $zero, 0x380 */
    bl_sh(r_sp + 0x18u, r_v0); /* 801E6E7C sh $v0, 0x18($sp) */
    r_v0 = 0u + 0x1D1u; /* 801E6E80 addiu $v0, $zero, 0x1D1 */
    bl_sh(r_sp + 0x1Au, 0u); /* 801E6E84 sh $zero, 0x1A($sp) */
    bl_sh(r_sp + 0x20u, 0u); /* 801E6E88 sh $zero, 0x20($sp) */
    bl_sh(r_sp + 0x22u, r_v0); /* 801E6E8C sh $v0, 0x22($sp) */
    bl_sw(r_sp + 0x10u, 0u); /* 801E6E90 sw $zero, 0x10($sp) */
    r_v0 = bl_lhu(r_sp + 0x18u); /* 801E6E94 lhu $v0, 0x18($sp) */
    r_v1 = bl_lhu(r_sp + 0x20u); /* 801E6E98 lhu $v1, 0x20($sp) */
    r_a1 = bl_lw(r_s0 + 0x24u); /* 801E6E9C lw $a1, 0x24($s0) */
    r_a2 = bl_lhu(r_sp + 0x1Au); /* 801E6EA0 lhu $a2, 0x1A($sp) */
    r_a3 = bl_lhu(r_sp + 0x22u); /* 801E6EA4 lhu $a3, 0x22($sp) */
    r_a2 = r_a2 << 16; /* 801E6EA8 sll $a2, $a2, 16 */
    r_a3 = r_a3 << 16; /* 801E6EAC sll $a3, $a3, 16 */
    r_a2 = r_v0 | r_a2; /* 801E6EB0 or $a2, $v0, $a2 */
    /* 801E6EB4 jal func_80022224 */
    r_a3 = r_v1 | r_a3;
    bl_jal(FN, 0x80022224u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80022224 */
    r_a0 = bl_lw(r_s0 + 0x2Cu); /* 801E6EBC lw $a0, 0x2C($s0) */
    /* 801E6EC0 jal SoundAddSedsEntry */
    ;
    bl_jal(FN, 0x80038428u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SoundAddSedsEntry */
    r_a0 = r_s0 + 0u; /* 801E6EC8 addu $a0, $s0, $zero */
    r_a1 = 0x801E0000u; /* 801E6ECC lui $a1, %hi(func_801E6D6C) */
    r_a1 = r_a1 + 0x6D6Cu; /* 801E6ED0 addiu $a1, $a1, %lo(func_801E6D6C) */
    r_v1 = bl_lw(r_a0 + 0x2Cu); /* 801E6ED4 lw $v1, 0x2C($a0) */
    r_v0 = 0x80060000u; /* 801E6ED8 lui $v0, %hi(D_8005919C) */
    /* 801E6EDC jal TimerWorkListSetTaskCallback */
    bl_sw(r_v0 + 0xFFFF919Cu, r_v1);
    bl_jal(FN, 0x8001CD6Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListSetTaskCallback */
    /* 801E6EE4 jal func_800B14B8 */
    ;
    bl_jal(FN, 0x800B14B8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800B14B8 */
L_801E6EEC:
    r_ra = bl_lw(r_sp + 0x34u); /* 801E6EEC lw $ra, 0x34($sp) */
    r_s0 = bl_lw(r_sp + 0x30u); /* 801E6EF0 lw $s0, 0x30($sp) */
    r_sp = r_sp + 0x38u; /* 801E6EF4 addiu $sp, $sp, 0x38 */
    /* 801E6EF8 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

static void func_801E6A4C(void) {
    static const char FN[] = "func_801E6A4C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_sp = r_sp + 0xFFFFFFE0u; /* 801E6A4C addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x14u, r_s1); /* 801E6A50 sw $s1, 0x14($sp) */
    r_s1 = 0u + 0u; /* 801E6A54 addu $s1, $zero, $zero */
    bl_sw(r_sp + 0x18u, r_s2); /* 801E6A58 sw $s2, 0x18($sp) */
    r_s2 = 0u + 0x3u; /* 801E6A5C addiu $s2, $zero, 0x3 */
    r_v0 = 0x800C0000u; /* 801E6A60 lui $v0, %hi(D_800C3EB0) */
    bl_sw(r_sp + 0x10u, r_s0); /* 801E6A64 sw $s0, 0x10($sp) */
    r_s0 = r_v0 + 0x3EB0u; /* 801E6A68 addiu $s0, $v0, %lo(D_800C3EB0) */
    bl_sw(r_sp + 0x1Cu, r_ra); /* 801E6A6C sw $ra, 0x1C($sp) */
L_801E6A70:
    r_v0 = bl_lbu(r_s0 + 0x6u); /* 801E6A70 lbu $v0, 0x6($s0) */
    ; /* 801E6A74 nop  */
    r_v0 = (u32)((s32)r_v0 < 17); /* 801E6A78 slti $v0, $v0, 0x11 */
    /* 801E6A7C beqz $v0, .L801E6A9C */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_801E6A9C; }
    r_v0 = bl_lbu(r_s0 + 0x8u); /* 801E6A84 lbu $v0, 0x8($s0) */
    ; /* 801E6A88 nop  */
    /* 801E6A8C beqz $v0, .L801E6A9C */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_801E6A9C; }
    /* 801E6A94 jal func_800BB760 */
    r_a0 = r_s1 + 0u;
    bl_jal(FN, 0x800BB760u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BB760 */
L_801E6A9C:
    r_s1 = r_s1 + 0x1u; /* 801E6A9C addiu $s1, $s1, 0x1 */
    /* 801E6AA0 bne $s1, $s2, .L801E6A70 */
    { int c_ = (r_s1 != r_s2); r_s0 = r_s0 + 0x1Cu; if (c_) goto L_801E6A70; }
    r_ra = bl_lw(r_sp + 0x1Cu); /* 801E6AA8 lw $ra, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 801E6AAC lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 801E6AB0 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 801E6AB4 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 801E6AB8 addiu $sp, $sp, 0x20 */
    /* 801E6ABC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

static void func_801E6D6C(u32 arg0) {
    static const char FN[] = "func_801E6D6C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE8u; /* 801E6D6C addiu $sp, $sp, -0x18 */
    bl_sw(r_sp + 0x10u, r_s0); /* 801E6D70 sw $s0, 0x10($sp) */
    r_s0 = r_a0 + 0u; /* 801E6D74 addu $s0, $a0, $zero */
    bl_sw(r_sp + 0x14u, r_ra); /* 801E6D78 sw $ra, 0x14($sp) */
    /* 801E6D7C jal func_8003BDFC */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x8003BDFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8003BDFC */
    r_v0 = r_v0 << 16; /* 801E6D84 sll $v0, $v0, 16 */
    /* 801E6D88 bnez $v0, .L801E6DB4 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_801E6DB4; }
    r_a0 = bl_lw(r_s0 + 0x28u); /* 801E6D90 lw $a0, 0x28($s0) */
    /* 801E6D94 jal HeapFree */
    ;
    bl_jal(FN, 0x800320E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapFree */
    /* 801E6D9C jal func_801E6A4C */
    ;
    bl_jal(FN, 0x801E6A4Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_801E6A4C */
    r_a0 = r_s0 + 0u; /* 801E6DA4 addu $a0, $s0, $zero */
    r_a1 = 0x801E0000u; /* 801E6DA8 lui $a1, %hi(func_801E6D34) */
    /* 801E6DAC jal TimerWorkListSetTaskCallback */
    r_a1 = r_a1 + 0x6D34u;
    bl_jal(FN, 0x8001CD6Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListSetTaskCallback */
L_801E6DB4:
    r_ra = bl_lw(r_sp + 0x14u); /* 801E6DB4 lw $ra, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 801E6DB8 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 801E6DBC addiu $sp, $sp, 0x18 */
    /* 801E6DC0 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

static void func_801E6D34(u32 arg0) {
    static const char FN[] = "func_801E6D34";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_a0 = arg0;
    r_v0 = 0x800C0000u; /* 801E6D34 lui $v0, %hi(D_800C35D8) */
    r_v0 = bl_lw(r_v0 + 0x35D8u); /* 801E6D38 lw $v0, %lo(D_800C35D8)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 801E6D3C addiu $sp, $sp, -0x18 */
    /* 801E6D40 bnez $v0, .L801E6D5C */
    { int c_ = (r_v0 != 0); bl_sw(r_sp + 0x10u, r_ra); if (c_) goto L_801E6D5C; }
    r_v0 = 0u + 0x10u; /* 801E6D48 addiu $v0, $zero, 0x10 */
    bl_sw(r_a0 + 0x90u, r_v0); /* 801E6D4C sw $v0, 0x90($a0) */
    r_a1 = 0x801E0000u; /* 801E6D50 lui $a1, %hi(func_801E6C80) */
    /* 801E6D54 jal TimerWorkListSetTaskCallback */
    r_a1 = r_a1 + 0x6C80u;
    bl_jal(FN, 0x8001CD6Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListSetTaskCallback */
L_801E6D5C:
    r_ra = bl_lw(r_sp + 0x10u); /* 801E6D5C lw $ra, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 801E6D60 addiu $sp, $sp, 0x18 */
    /* 801E6D64 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* Wait for the task counter and actor completion, then mark complete and
 * delete the timer task. This host remains module-identity-gated. */
static void func_801E6C80(u32 arg0) {
    static const char FN[] = "func_801E6C80";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE8u; /* 801E6C80 addiu $sp, $sp, -24 */
    bl_sw(r_sp + 0x10u, r_ra); /* 801E6C84 sw $ra, 16($sp) */
    r_v0 = bl_lw(r_a0 + 0x90u); /* 801E6C88 lw $v0, 144($a0) */
    ; /* 801E6C8C nop  */
    /* 801E6C90 bnez $v0, .L801E6D1C */
    { int c_ = (r_v0 != 0); r_t2 = r_a0; if (c_) goto L_801E6D1C; }
    r_a3 = 0u; /* 801E6C98 move $a3, $zero */
    r_t1 = 0u | 0x8C8Cu; /* 801E6C9C ori $t1, $zero, 0x8c8c */
    r_t0 = 0u + 0x3u; /* 801E6CA0 addiu $t0, $zero, 3 */
    r_v0 = 0x800C0000u; /* 801E6CA4 lui $v0, 0x800c */
    r_a1 = r_v0 + 0x3EB0u; /* 801E6CA8 addiu $a1, $v0, 16048 */
    r_a2 = r_a1; /* 801E6CAC move $a2, $a1 */
L_801E6CB0:
    r_v0 = bl_lbu(r_a1 + 0x8u); /* 801E6CB0 lbu $v0, 8($a1) */
    ; /* 801E6CB4 nop  */
    /* 801E6CB8 bnez $v0, .L801E6CE4 */
    { int c_ = (r_v0 != 0); r_v0 = r_a2 + r_t1; if (c_) goto L_801E6CE4; }
    r_v0 = bl_lw(r_v0); /* 801E6CC0 lw $v0, 0($v0) */
    ; /* 801E6CC4 nop  */
    /* 801E6CC8 beqz $v0, .L801E6CE4 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_801E6CE4; }
    r_v1 = bl_lh(r_v0 + 0x6u); /* 801E6CD0 lh $v1, 6($v0) */
    r_v0 = bl_lh(r_v0 + 0x84u); /* 801E6CD4 lh $v0, 132($v0) */
    ; /* 801E6CD8 nop  */
    /* 801E6CDC bne $v1, $v0, .L801E6D24 */
    { int c_ = (r_v1 != r_v0); ; if (c_) goto L_801E6D24; }
L_801E6CE4:
    r_a2 = r_a2 + 0x4u; /* 801E6CE4 addiu $a2, $a2, 4 */
    r_a3 = r_a3 + 0x1u; /* 801E6CE8 addiu $a3, $a3, 1 */
    /* 801E6CEC bne $a3, $t0, .L801E6CB0 */
    { int c_ = (r_a3 != r_t0); r_a1 = r_a1 + 0x1Cu; if (c_) goto L_801E6CB0; }
    r_v0 = 0x800C0000u; /* 801E6CF4 lui $v0, 0x800c */
    r_v0 = r_v0 + 0x3EB0u; /* 801E6CF8 addiu $v0, $v0, 16048 */
    r_v1 = 0u | 0x8000u; /* 801E6CFC ori $v1, $zero, 0x8000 */
    r_v0 = r_v0 + r_v1; /* 801E6D00 addu $v0, $v0, $v1 */
    r_v1 = 0u + 0x1u; /* 801E6D04 addiu $v1, $zero, 1 */
    bl_sb(r_v0 + 0xDA8u, r_v1); /* 801E6D08 sb $v1, 3496($v0) */
    /* 801E6D0C jal TimerWorkListDeleteTask */
    r_a0 = r_t2;
    bl_jal(FN, 0x8001CE44u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListDeleteTask */
    /* 801E6D14 j .L801E6D24 */
    ; goto L_801E6D24;
L_801E6D1C:
    r_v0 = r_v0 + 0xFFFFFFFFu; /* 801E6D1C addiu $v0, $v0, -1 */
    bl_sw(r_a0 + 0x90u, r_v0); /* 801E6D20 sw $v0, 144($a0) */
L_801E6D24:
    r_ra = bl_lw(r_sp + 0x10u); /* 801E6D24 lw $ra, 16($sp) */
    r_sp = r_sp + 0x18u; /* 801E6D28 addiu $sp, $sp, 24 */
    /* 801E6D2C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop

/* The hosted bodies, each identity-checked on its own SHA-pinned retail bytes
 * (with the module's header windows below). */
static const struct {
    uint32_t target;
    unsigned bytes;
    const char *sha256;
} kBattleModule0c4Lifts[] = {
    {0x801e6ac4u, 444u, "5cbd6b2eca3d8b3fbfb69df68cd3c6936141a10a652025a6e91fc757e8d4e8a4"},
    {0x801e6f00u, 236u, "68ceb6053a04456a6b4cbe64eb24c9299422f13173d23ce00bd10ce6095e95d2"},
    {0x801e6dc8u, 128u, "1dbf815550963af9af40127a9e6322cd7337d34f7e1cbdb31ad198d4c53428fb"},
    {0x801e6e48u, 184u, "5988b185900209d9912b1aa1f6aad855d0d78996d2cb9a735162a82f215574ae"},
    {0x801e6a4cu, 120u, "3823a255da1533e83a5bc96164b5f6f4c2458b011d538572d9a0cb2794794009"},
    {0x801e6d6cu, 92u, "383ceb506f061c10e50c098ece2c0768e0b990bc70d03c6fe765de719252eef1"},
    {0x801e6d34u, 56u, "4a27204573127dd3db98fd25e3a9f814ca852bcb834bc980a728e8e528e4f5be"},
    {0x801e6c80u, 180u, "90cf73391186b3873a119d81779c27f88a089bb684f042cd763473ae2a775389"},
};

/* Whether the host below would run `target`: the startup control is on and
 * the loaded module is the SHA-pinned 0x0C/4 one, for its header windows and
 * the body's own bytes.  (Also the differential prover's check before it
 * runs a callback with the interpreter disabled.) */
static int battle_module_0c4_lift_ready(uint32_t target)
{
    const char *control = getenv("XENO_BATTLE_NATIVE_STARTUP_ACTORS");
    unsigned i;

    if (control == NULL || strcmp(control, "1") != 0)
        return 0;
    for (i = 0; i < sizeof(kBattleModule0c4Lifts) / sizeof(kBattleModule0c4Lifts[0]); i++) {
        if (kBattleModule0c4Lifts[i].target == target)
            return battle_startup_actors_identity(0x801e4000u, 32u,
                       "6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140") &&
                   battle_startup_actors_identity(0x801e5840u, 228u,
                       "e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2") &&
                   battle_startup_actors_identity(target, kBattleModule0c4Lifts[i].bytes,
                       kBattleModule0c4Lifts[i].sha256);
    }
    return 0;
}

static int battle_module_0c4_lift_try_host(BattleMipsRuntime *runtime,
                                           PcPortMipsCpu *cpu, uint32_t target)
{
    if (runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0) ||
        !battle_module_0c4_lift_ready(target))
        return 0;
    switch (target) {
    case 0x801e6ac4u: func_801E6AC4(); break;
    case 0x801e6f00u: func_801E6F00(cpu->gpr[4]); break;
    case 0x801e6dc8u: func_801E6DC8(); break;
    case 0x801e6a4cu: func_801E6A4C(); break;
    case 0x801e6d6cu: func_801E6D6C(cpu->gpr[4]); break;
    case 0x801e6d34u: func_801E6D34(cpu->gpr[4]); break;
    case 0x801e6c80u: func_801E6C80(cpu->gpr[4]); break;
    default:          func_801E6E48(cpu->gpr[4]); break; /* 0x801e6e48 */
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

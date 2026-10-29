/* Host-only instruction lift for the battle state initializer. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void func_8009892C(void) {
    static const char FN[] = "func_8009892C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x0u, &bl_saved);
    r_a3 = 0u + 0u; /* 8009892C addu $a3, $zero, $zero */
    r_t1 = 0u | 0x7Fu; /* 80098930 ori $t1, $zero, 0x7F */
    r_t0 = 0x800C0000u; /* 80098934 lui $t0, %hi(D_800C3AA4) */
    r_t0 = r_t0 + 0x3AA4u; /* 80098938 addiu $t0, $t0, %lo(D_800C3AA4) */
    r_v1 = r_a3 & 0xFFu; /* 8009893C andi $v1, $a3, 0xFF */
L_80098940:
    r_at = 0x800D0000u; /* 80098940 lui $at, %hi(D_800D2D24) */
    r_at = r_at + r_v1; /* 80098944 addu $at, $at, $v1 */
    r_v0 = bl_lbu(r_at + 0x2D24u); /* 80098948 lbu $v0, %lo(D_800D2D24)($at) */
    ; /* 8009894C nop  */
    /* 80098950 beq $v0, $t1, .L80098A28 */
    { int c_ = (r_v0 == r_t1); r_a0 = r_v1 << 1; if (c_) goto L_80098A28; }
    r_a2 = 0u + 0u; /* 80098958 addu $a2, $zero, $zero */
    r_v0 = r_a0 + r_v1; /* 8009895C addu $v0, $a0, $v1 */
    r_v0 = r_v0 << 3; /* 80098960 sll $v0, $v0, 3 */
    r_v0 = r_v0 - r_v1; /* 80098964 subu $v0, $v0, $v1 */
    r_v1 = 0x800C0000u; /* 80098968 lui $v1, %hi(D_800C34B0) */
    r_v1 = bl_lw(r_v1 + 0x34B0u); /* 8009896C lw $v1, %lo(D_800C34B0)($v1) */
    r_v0 = r_v0 << 4; /* 80098970 sll $v0, $v0, 4 */
    r_v0 = r_v0 + r_v1; /* 80098974 addu $v0, $v0, $v1 */
    r_v1 = r_v0 + 0xA4u; /* 80098978 addiu $v1, $v0, 0xA4 */
    r_at = 0x800C0000u; /* 8009897C lui $at, %hi(D_800C3E00) */
    bl_sw(r_at + 0x3E00u, r_v0); /* 80098980 sw $v0, %lo(D_800C3E00)($at) */
    r_v0 = bl_lhu(r_v0 + 0x7Au); /* 80098984 lhu $v0, 0x7A($v0) */
    r_a0 = r_a0 + r_t0; /* 80098988 addu $a0, $a0, $t0 */
    r_at = 0x800D0000u; /* 8009898C lui $at, %hi(D_800D2D6C) */
    bl_sw(r_at + 0x2D6Cu, r_v1); /* 80098990 sw $v1, %lo(D_800D2D6C)($at) */
    bl_sh(r_a0, r_v0); /* 80098994 sh $v0, 0x0($a0) */
    r_a1 = r_a2 & 0xFFu; /* 80098998 andi $a1, $a2, 0xFF */
L_8009899C:
    r_at = 0x800D0000u; /* 8009899C lui $at, %hi(D_800D2D10) */
    r_at = r_at + r_a1; /* 800989A0 addu $at, $at, $a1 */
    r_a0 = bl_lbu(r_at + 0x2D10u); /* 800989A4 lbu $a0, %lo(D_800D2D10)($at) */
    ; /* 800989A8 nop  */
    /* 800989AC beqz $a0, .L800989F4 */
    { int c_ = (r_a0 == 0); r_a2 = r_a2 + 0x1u; if (c_) goto L_800989F4; }
    r_v1 = 0x800D0000u; /* 800989B4 lui $v1, %hi(D_800D2D6C) */
    r_v1 = bl_lw(r_v1 + 0x2D6Cu); /* 800989B8 lw $v1, %lo(D_800D2D6C)($v1) */
    ; /* 800989BC nop  */
    r_v0 = bl_lbu(r_v1 + 0x98u); /* 800989C0 lbu $v0, 0x98($v1) */
    ; /* 800989C4 nop  */
    r_v0 = r_v0 - r_a0; /* 800989C8 subu $v0, $v0, $a0 */
    bl_sb(r_v1 + 0x98u, r_v0); /* 800989CC sb $v0, 0x98($v1) */
    r_a0 = 0x800D0000u; /* 800989D0 lui $a0, %hi(D_800D2D6C) */
    r_a0 = bl_lw(r_a0 + 0x2D6Cu); /* 800989D4 lw $a0, %lo(D_800D2D6C)($a0) */
    ; /* 800989D8 nop  */
    r_v1 = r_a0 + r_a1; /* 800989DC addu $v1, $a0, $a1 */
    r_v0 = bl_lbu(r_a0 + 0x98u); /* 800989E0 lbu $v0, 0x98($a0) */
    r_v1 = bl_lbu(r_v1 + 0x50u); /* 800989E4 lbu $v1, 0x50($v1) */
    ; /* 800989E8 nop  */
    r_v0 = r_v0 + r_v1; /* 800989EC addu $v0, $v0, $v1 */
    bl_sb(r_a0 + 0x98u, r_v0); /* 800989F0 sb $v0, 0x98($a0) */
L_800989F4:
    r_v0 = r_a2 & 0xFFu; /* 800989F4 andi $v0, $a2, 0xFF */
    r_v0 = (u32)(r_v0 < 0x4u); /* 800989F8 sltiu $v0, $v0, 0x4 */
    /* 800989FC bnez $v0, .L8009899C */
    { int c_ = (r_v0 != 0); r_a1 = r_a2 & 0xFFu; if (c_) goto L_8009899C; }
    r_v1 = 0x800D0000u; /* 80098A04 lui $v1, %hi(D_800D2D6C) */
    r_v1 = bl_lw(r_v1 + 0x2D6Cu); /* 80098A08 lw $v1, %lo(D_800D2D6C)($v1) */
    ; /* 80098A0C nop  */
    r_v0 = bl_lbu(r_v1 + 0x98u); /* 80098A10 lbu $v0, 0x98($v1) */
    ; /* 80098A14 nop  */
    r_v0 = (u32)(r_v0 < 0x11u); /* 80098A18 sltiu $v0, $v0, 0x11 */
    /* 80098A1C bnez $v0, .L80098A28 */
    { int c_ = (r_v0 != 0); r_v0 = 0u | 0x10u; if (c_) goto L_80098A28; }
    bl_sb(r_v1 + 0x98u, r_v0); /* 80098A24 sb $v0, 0x98($v1) */
L_80098A28:
    r_a3 = r_a3 + 0x1u; /* 80098A28 addiu $a3, $a3, 0x1 */
    r_v0 = r_a3 & 0xFFu; /* 80098A2C andi $v0, $a3, 0xFF */
    r_v0 = (u32)(r_v0 < 0x3u); /* 80098A30 sltiu $v0, $v0, 0x3 */
    /* 80098A34 bnez $v0, .L80098940 */
    { int c_ = (r_v0 != 0); r_v1 = r_a3 & 0xFFu; if (c_) goto L_80098940; }
    r_v0 = 0x80070000u; /* 80098A3C lui $v0, %hi(g_GameState + 0x17DA) */
    r_v0 = bl_lhu(r_v0 + 0xFFFFEE0Eu); /* 80098A40 lhu $v0, %lo(g_GameState + 0x17DA)($v0) */
    r_v1 = 0u | 0x7u; /* 80098A44 ori $v1, $zero, 0x7 */
    r_at = 0x80070000u; /* 80098A48 lui $at, %hi(g_GameState + 0x7E6) */
    bl_sb(r_at + 0xFFFFDE1Au, r_v1); /* 80098A4C sb $v1, %lo(g_GameState + 0x7E6)($at) */
    r_v0 = r_v0 & 0x2000u; /* 80098A50 andi $v0, $v0, 0x2000 */
    /* 80098A54 beqz $v0, .L80098A74 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_80098A74; }
    r_v0 = 0x80070000u; /* 80098A5C lui $v0, %hi(g_GameState + 0x17C2) */
    r_v0 = bl_lhu(r_v0 + 0xFFFFEDF6u); /* 80098A60 lhu $v0, %lo(g_GameState + 0x17C2)($v0) */
    ; /* 80098A64 nop  */
    r_v0 = r_v0 | 0x800u; /* 80098A68 ori $v0, $v0, 0x800 */
    r_at = 0x80070000u; /* 80098A6C lui $at, %hi(g_GameState + 0x17C2) */
    bl_sh(r_at + 0xFFFFEDF6u, r_v0); /* 80098A70 sh $v0, %lo(g_GameState + 0x17C2)($at) */
L_80098A74:
    r_v0 = 0x80070000u; /* 80098A74 lui $v0, %hi(g_GameState + 0x1930) */
    r_v0 = bl_lhu(r_v0 + 0xFFFFEF64u); /* 80098A78 lhu $v0, %lo(g_GameState + 0x1930)($v0) */
    ; /* 80098A7C nop  */
    r_v0 = (u32)(r_v0 < 0xBBu); /* 80098A80 sltiu $v0, $v0, 0xBB */
    /* 80098A84 beqz $v0, .L80098AF0 */
    { int c_ = (r_v0 == 0); r_v0 = 0u | 0xAu; if (c_) goto L_80098AF0; }
    r_at = 0x80070000u; /* 80098A8C lui $at, %hi(g_GameState + 0x9EC) */
    bl_sb(r_at + 0xFFFFE020u, r_v0); /* 80098A90 sb $v0, %lo(g_GameState + 0x9EC)($at) */
    r_at = 0x80070000u; /* 80098A94 lui $at, %hi(g_GameState + 0xA90) */
    bl_sb(r_at + 0xFFFFE0C4u, r_v0); /* 80098A98 sb $v0, %lo(g_GameState + 0xA90)($at) */
    r_v0 = 0u | 0x9u; /* 80098A9C ori $v0, $zero, 0x9 */
    r_at = 0x80070000u; /* 80098AA0 lui $at, %hi(g_GameState + 0x10F8) */
    bl_sb(r_at + 0xFFFFE72Cu, r_v0); /* 80098AA4 sb $v0, %lo(g_GameState + 0x10F8)($at) */
    r_at = 0x80070000u; /* 80098AA8 lui $at, %hi(g_GameState + 0x119C) */
    bl_sb(r_at + 0xFFFFE7D0u, r_v0); /* 80098AAC sb $v0, %lo(g_GameState + 0x119C)($at) */
    r_v0 = 0u | 0x8u; /* 80098AB0 ori $v0, $zero, 0x8 */
    r_at = 0x80070000u; /* 80098AB4 lui $at, %hi(g_GameState + 0x1240) */
    bl_sb(r_at + 0xFFFFE874u, r_v0); /* 80098AB8 sb $v0, %lo(g_GameState + 0x1240)($at) */
    r_v0 = 0u | 0xCu; /* 80098ABC ori $v0, $zero, 0xC */
    r_at = 0x80070000u; /* 80098AC0 lui $at, %hi(g_GameState + 0x12E4) */
    bl_sb(r_at + 0xFFFFE918u, r_v0); /* 80098AC4 sb $v0, %lo(g_GameState + 0x12E4)($at) */
    r_at = 0x80070000u; /* 80098AC8 lui $at, %hi(g_GameState + 0x1388) */
    bl_sb(r_at + 0xFFFFE9BCu, r_v0); /* 80098ACC sb $v0, %lo(g_GameState + 0x1388)($at) */
    r_v0 = 0u | 0x58u; /* 80098AD0 ori $v0, $zero, 0x58 */
    r_at = 0x80070000u; /* 80098AD4 lui $at, %hi(g_GameState + 0x11CE) */
    bl_sb(r_at + 0xFFFFE802u, r_v0); /* 80098AD8 sb $v0, %lo(g_GameState + 0x11CE)($at) */
    r_v0 = 0u | 0x28u; /* 80098ADC ori $v0, $zero, 0x28 */
    r_at = 0x80070000u; /* 80098AE0 lui $at, %hi(g_GameState + 0xDF7) */
    bl_sb(r_at + 0xFFFFE42Bu, 0u); /* 80098AE4 sb $zero, %lo(g_GameState + 0xDF7)($at) */
    r_at = 0x80070000u; /* 80098AE8 lui $at, %hi(g_GameState + 0x131C) */
    bl_sb(r_at + 0xFFFFE950u, r_v0); /* 80098AEC sb $v0, %lo(g_GameState + 0x131C)($at) */
L_80098AF0:
    /* 80098AF0 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop
#endif

/* Host-only instruction lift for battle actor selection. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void func_800723E0(void) {
    static const char FN[] = "func_800723E0";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_v0 = 0x800D0000u; /* 800723E0 lui $v0, %hi(D_800D39E0) */
    r_v0 = bl_lhu(r_v0 + 0x39E0u); /* 800723E4 lhu $v0, %lo(D_800D39E0)($v0) */
    r_sp = r_sp + 0xFFFFFFE8u; /* 800723E8 addiu $sp, $sp, -0x18 */
    /* 800723EC bnez $v0, .L80072514 */
    { int c_ = (r_v0 != 0); bl_sw(r_sp + 0x10u, r_ra); if (c_) goto L_80072514; }
    r_v1 = 0x800D0000u; /* 800723F4 lui $v1, %hi(D_800D2DC0) */
    r_v1 = bl_lbu(r_v1 + 0x2DC0u); /* 800723F8 lbu $v1, %lo(D_800D2DC0)($v1) */
    ; /* 800723FC nop  */
    /* 80072400 beqz $v1, .L80072458 */
    { int c_ = (r_v1 == 0); r_t0 = 0u | 0x1u; if (c_) goto L_80072458; }
    r_v0 = 0x800C0000u; /* 80072408 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 8007240C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80072410 nop  */
    bl_sb(r_v0 + 0x2D3u, r_v1); /* 80072414 sb $v1, 0x2D3($v0) */
    r_v1 = 0x800D0000u; /* 80072418 lui $v1, %hi(D_800D2DC0) */
    r_v1 = bl_lbu(r_v1 + 0x2DC0u); /* 8007241C lbu $v1, %lo(D_800D2DC0)($v1) */
    r_v0 = 0u | 0x1u; /* 80072420 ori $v0, $zero, 0x1 */
    r_at = 0x800D0000u; /* 80072424 lui $at, %hi(D_800D2DE3) */
    r_at = r_at + r_v1; /* 80072428 addu $at, $at, $v1 */
    bl_sb(r_at + 0x2DE3u, r_v0); /* 8007242C sb $v0, %lo(D_800D2DE3)($at) */
    r_v0 = 0x800D0000u; /* 80072430 lui $v0, %hi(D_800D2DC0) */
    r_v0 = bl_lbu(r_v0 + 0x2DC0u); /* 80072434 lbu $v0, %lo(D_800D2DC0)($v0) */
    r_at = 0x800D0000u; /* 80072438 lui $at, %hi(D_800D2DC0) */
    bl_sb(r_at + 0x2DC0u, 0u); /* 8007243C sb $zero, %lo(D_800D2DC0)($at) */
    r_v0 = r_v0 << 1; /* 80072440 sll $v0, $v0, 1 */
    r_at = 0x800D0000u; /* 80072444 lui $at, %hi(D_800D2E04) */
    r_at = r_at + r_v0; /* 80072448 addu $at, $at, $v0 */
    bl_sh(r_at + 0x2E04u, 0u); /* 8007244C sh $zero, %lo(D_800D2E04)($at) */
    /* 80072450 j .L80072504 */
    ; goto L_80072504;
L_80072458:
    r_v0 = 0x800C0000u; /* 80072458 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 8007245C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80072460 nop  */
    bl_sb(r_v0 + 0x2D3u, 0u); /* 80072464 sb $zero, 0x2D3($v0) */
    r_v0 = 0x800D0000u; /* 80072468 lui $v0, %hi(D_800D2DD7) */
    r_v0 = r_v0 + 0x2DD7u; /* 8007246C addiu $v0, $v0, %lo(D_800D2DD7) */
    r_a1 = bl_lbu(r_v0); /* 80072470 lbu $a1, 0x0($v0) */
    r_at = 0x800D0000u; /* 80072474 lui $at, %hi(D_800D2DD8) */
    r_at = r_at + r_a1; /* 80072478 addu $at, $at, $a1 */
    r_v1 = bl_lbu(r_at + 0x2DD8u); /* 8007247C lbu $v1, %lo(D_800D2DD8)($at) */
    r_a2 = r_v0 + 0u; /* 80072480 addu $a2, $v0, $zero */
    r_at = 0x800D0000u; /* 80072484 lui $at, %hi(D_800D2DE4) */
    r_at = r_at + r_v1; /* 80072488 addu $at, $at, $v1 */
    r_v0 = bl_lbu(r_at + 0x2DE4u); /* 8007248C lbu $v0, %lo(D_800D2DE4)($at) */
    /* 80072490 j .L800724CC */
    r_a3 = 0u | 0xBu; goto L_800724CC;
L_80072498:
    /* 80072498 bne $a1, $a3, .L800724A4 */
    { int c_ = (r_a1 != r_a3); ; if (c_) goto L_800724A4; }
    r_a1 = 0u + 0u; /* 800724A0 addu $a1, $zero, $zero */
L_800724A4:
    r_v0 = bl_lbu(r_a2); /* 800724A4 lbu $v0, 0x0($a2) */
    ; /* 800724A8 nop  */
    /* 800724AC beq $a1, $v0, .L80072504 */
    { int c_ = (r_a1 == r_v0); ; if (c_) goto L_80072504; }
    r_at = 0x800D0000u; /* 800724B4 lui $at, %hi(D_800D2DD8) */
    r_at = r_at + r_a1; /* 800724B8 addu $at, $at, $a1 */
    r_v1 = bl_lbu(r_at + 0x2DD8u); /* 800724BC lbu $v1, %lo(D_800D2DD8)($at) */
    r_at = 0x800D0000u; /* 800724C0 lui $at, %hi(D_800D2DE4) */
    r_at = r_at + r_v1; /* 800724C4 addu $at, $at, $v1 */
    r_v0 = bl_lbu(r_at + 0x2DE4u); /* 800724C8 lbu $v0, %lo(D_800D2DE4)($at) */
L_800724CC:
    ; /* 800724CC nop  */
    /* 800724D0 bne $v0, $t0, .L80072498 */
    { int c_ = (r_v0 != r_t0); r_a1 = r_a1 + 0x1u; if (c_) goto L_80072498; }
    r_a1 = r_a1 + 0xFFFFFFFFu; /* 800724D8 addiu $a1, $a1, -0x1 */
    r_v0 = r_a1 + 0x1u; /* 800724DC addiu $v0, $a1, 0x1 */
    r_a0 = 0x800C0000u; /* 800724E0 lui $a0, %hi(D_800C3EAC) */
    r_a0 = bl_lw(r_a0 + 0x3EACu); /* 800724E4 lw $a0, %lo(D_800C3EAC)($a0) */
    r_v1 = r_v1 + 0x1u; /* 800724E8 addiu $v1, $v1, 0x1 */
    bl_sb(r_a0 + 0x2D3u, r_v1); /* 800724EC sb $v1, 0x2D3($a0) */
    bl_sb(r_a2, r_v0); /* 800724F0 sb $v0, 0x0($a2) */
    r_v0 = r_v0 & 0xFFu; /* 800724F4 andi $v0, $v0, 0xFF */
    /* 800724F8 bne $v0, $a3, .L80072498 */
    { int c_ = (r_v0 != r_a3); r_a1 = r_a1 + 0x1u; if (c_) goto L_80072498; }
    bl_sb(r_a2, 0u); /* 80072500 sb $zero, 0x0($a2) */
L_80072504:
    /* 80072504 jal func_80071B94 */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x80071B94u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071B94 */
    /* 8007250C j .L8007251C */
    ; goto L_8007251C;
L_80072514:
    /* 80072514 jal func_80072324 */
    ;
    bl_jal(FN, 0x80072324u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80072324 */
L_8007251C:
    r_ra = bl_lw(r_sp + 0x10u); /* 8007251C lw $ra, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 80072520 addiu $sp, $sp, 0x18 */
    /* 80072524 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop
#endif

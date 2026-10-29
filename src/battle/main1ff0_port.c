/* Host-only instruction lift for battle actor-action setup. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void func_80071B94(u32 arg0) {
    static const char FN[] = "func_80071B94";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_v0 = 0u | 0x1u; /* 80071B94 ori $v0, $zero, 0x1 */
    r_at = 0x800C0000u; /* 80071B98 lui $at, %hi(D_800C3E8C) */
    bl_sb(r_at + 0x3E8Cu, r_v0); /* 80071B9C sb $v0, %lo(D_800C3E8C)($at) */
    r_v0 = 0x800C0000u; /* 80071BA0 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071BA4 lw $v0, %lo(D_800C3EAC)($v0) */
    r_sp = r_sp + 0xFFFFFFC8u; /* 80071BA8 addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x28u, r_s0); /* 80071BAC sw $s0, 0x28($sp) */
    r_s0 = 0x800D0000u; /* 80071BB0 lui $s0, %hi(D_800D2CAF) */
    r_s0 = r_s0 + 0x2CAFu; /* 80071BB4 addiu $s0, $s0, %lo(D_800D2CAF) */
    bl_sw(r_sp + 0x34u, r_ra); /* 80071BB8 sw $ra, 0x34($sp) */
    bl_sw(r_sp + 0x30u, r_s2); /* 80071BBC sw $s2, 0x30($sp) */
    bl_sw(r_sp + 0x2Cu, r_s1); /* 80071BC0 sw $s1, 0x2C($sp) */
    bl_sb(r_s0, 0u); /* 80071BC4 sb $zero, 0x0($s0) */
    r_v0 = bl_lbu(r_v0 + 0x2D3u); /* 80071BC8 lbu $v0, 0x2D3($v0) */
    ; /* 80071BCC nop  */
    /* 80071BD0 beqz $v0, .L80072254 */
    { int c_ = (r_v0 == 0); r_s2 = r_a0 + 0u; if (c_) goto L_80072254; }
    /* 80071BD8 jal func_80085350 */
    r_s1 = 0u | 0x4u;
    bl_jal(FN, 0x80085350u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80085350 */
    /* 80071BE0 jal func_80071A08 */
    ;
    bl_jal(FN, 0x80071A08u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A08 */
    r_v1 = 0x800C0000u; /* 80071BE8 lui $v1, %hi(D_800C3EAC) */
    r_v1 = bl_lw(r_v1 + 0x3EACu); /* 80071BEC lw $v1, %lo(D_800C3EAC)($v1) */
    r_at = 0x800D0000u; /* 80071BF0 lui $at, %hi(D_800D3298) */
    bl_sb(r_at + 0x3298u, 0u); /* 80071BF4 sb $zero, %lo(D_800D3298)($at) */
    r_v0 = bl_lbu(r_v1 + 0x2D3u); /* 80071BF8 lbu $v0, 0x2D3($v1) */
    ; /* 80071BFC nop  */
    r_v0 = r_v0 + 0xFFFFFFFFu; /* 80071C00 addiu $v0, $v0, -0x1 */
    bl_sb(r_v1 + 0x2D3u, r_v0); /* 80071C04 sb $v0, 0x2D3($v1) */
    r_v1 = 0x800C0000u; /* 80071C08 lui $v1, %hi(D_800C3EAC) */
    r_v1 = bl_lw(r_v1 + 0x3EACu); /* 80071C0C lw $v1, %lo(D_800C3EAC)($v1) */
    ; /* 80071C10 nop  */
    r_v0 = bl_lbu(r_v1 + 0x2D3u); /* 80071C14 lbu $v0, 0x2D3($v1) */
    r_at = 0x800C0000u; /* 80071C18 lui $at, %hi(D_800C4922) */
    bl_sb(r_at + 0x4922u, r_v0); /* 80071C1C sb $v0, %lo(D_800C4922)($at) */
    bl_sb(r_v1 + 0x2DAu, 0u); /* 80071C20 sb $zero, 0x2DA($v1) */
    r_v0 = 0x800C0000u; /* 80071C24 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071C28 lw $v0, %lo(D_800C3EAC)($v0) */
    r_v1 = r_s0 + 0xFFFFFFF9u; /* 80071C2C addiu $v1, $s0, -0x7 */
    bl_sb(r_v0 + 0x2DBu, 0u); /* 80071C30 sb $zero, 0x2DB($v0) */
L_80071C34:
    bl_sb(r_v1, 0u); /* 80071C34 sb $zero, 0x0($v1) */
    r_s1 = r_s1 + 0xFFFFFFFFu; /* 80071C38 addiu $s1, $s1, -0x1 */
    /* 80071C3C bgez $s1, .L80071C34 */
    { int c_ = ((s32)r_s1 >= 0); r_v1 = r_v1 + 0xFFFFFFFFu; if (c_) goto L_80071C34; }
    r_v0 = 0x800C0000u; /* 80071C44 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071C48 lw $v0, %lo(D_800C3EAC)($v0) */
    r_at = 0x800D0000u; /* 80071C4C lui $at, %hi(D_800D2C94) */
    bl_sh(r_at + 0x2C94u, 0u); /* 80071C50 sh $zero, %lo(D_800D2C94)($at) */
    r_a0 = bl_lbu(r_v0 + 0x2D3u); /* 80071C54 lbu $a0, 0x2D3($v0) */
    /* 80071C58 jal func_80099890 */
    ;
    bl_jal(FN, 0x80099890u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80099890 */
    r_v0 = 0x800C0000u; /* 80071C60 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071C64 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80071C68 nop  */
    r_a0 = bl_lbu(r_v0 + 0x2D3u); /* 80071C6C lbu $a0, 0x2D3($v0) */
    ; /* 80071C70 nop  */
    r_v0 = (u32)(r_a0 < 0x3u); /* 80071C74 sltiu $v0, $a0, 0x3 */
    /* 80071C78 bnez $v0, .L80071E24 */
    { int c_ = (r_v0 != 0); r_v0 = 0u | 0x1Cu; if (c_) goto L_80071E24; }
    r_v0 = r_s2 & 0xFFu; /* 80071C80 andi $v0, $s2, 0xFF */
    /* 80071C84 bnez $v0, .L80071D8C */
    { int c_ = (r_v0 != 0); r_v0 = r_a0 << 1; if (c_) goto L_80071D8C; }
    r_v0 = r_v0 + r_a0; /* 80071C8C addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 3; /* 80071C90 sll $v0, $v0, 3 */
    r_v0 = r_v0 - r_a0; /* 80071C94 subu $v0, $v0, $a0 */
    r_v0 = r_v0 << 4; /* 80071C98 sll $v0, $v0, 4 */
    r_at = 0x800D0000u; /* 80071C9C lui $at, %hi(D_800CCD68) */
    r_at = r_at + r_v0; /* 80071CA0 addu $at, $at, $v0 */
    r_a1 = bl_lhu(r_at + 0xFFFFCD68u); /* 80071CA4 lhu $a1, %lo(D_800CCD68)($at) */
    /* 80071CA8 jal func_800799C8 */
    r_a1 = r_a1 & 0x2000u;
    bl_jal(FN, 0x800799C8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800799C8 */
    r_v0 = 0x800C0000u; /* 80071CB0 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071CB4 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80071CB8 nop  */
    r_v1 = bl_lbu(r_v0 + 0x2D3u); /* 80071CBC lbu $v1, 0x2D3($v0) */
    ; /* 80071CC0 nop  */
    r_v0 = r_v1 << 3; /* 80071CC4 sll $v0, $v1, 3 */
    r_v0 = r_v0 - r_v1; /* 80071CC8 subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 2; /* 80071CCC sll $v0, $v0, 2 */
    r_at = 0x800C0000u; /* 80071CD0 lui $at, %hi(D_800C3EB7) */
    r_at = r_at + r_v0; /* 80071CD4 addu $at, $at, $v0 */
    r_v0 = bl_lbu(r_at + 0x3EB7u); /* 80071CD8 lbu $v0, %lo(D_800C3EB7)($at) */
    ; /* 80071CDC nop  */
    r_v0 = r_v0 & 0x80u; /* 80071CE0 andi $v0, $v0, 0x80 */
    /* 80071CE4 bnez $v0, .L80071D8C */
    { int c_ = (r_v0 != 0); r_v0 = r_v1 << 1; if (c_) goto L_80071D8C; }
    r_v0 = r_v0 + r_v1; /* 80071CEC addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 80071CF0 sll $v0, $v0, 3 */
    r_v0 = r_v0 - r_v1; /* 80071CF4 subu $v0, $v0, $v1 */
    r_v0 = r_v0 << 4; /* 80071CF8 sll $v0, $v0, 4 */
    r_at = 0x800D0000u; /* 80071CFC lui $at, %hi(D_800CCD1C) */
    r_at = r_at + r_v0; /* 80071D00 addu $at, $at, $v0 */
    r_v0 = bl_lhu(r_at + 0xFFFFCD1Cu); /* 80071D04 lhu $v0, %lo(D_800CCD1C)($at) */
    ; /* 80071D08 nop  */
    r_v0 = r_v0 & 0x400u; /* 80071D0C andi $v0, $v0, 0x400 */
    /* 80071D10 bnez $v0, .L80071D8C */
    { int c_ = (r_v0 != 0); r_s1 = 0u | 0x1u; if (c_) goto L_80071D8C; }
    r_v0 = 0x800D0000u; /* 80071D18 lui $v0, %hi(D_800D2D28) */
    r_v0 = bl_lw(r_v0 + 0x2D28u); /* 80071D1C lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80071D20 nop  */
    bl_sb(r_v0 + 0xB5u, r_s1); /* 80071D24 sb $s1, 0xB5($v0) */
    r_v0 = 0x800C0000u; /* 80071D28 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071D2C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80071D30 nop  */
    r_v0 = bl_lbu(r_v0 + 0x2D3u); /* 80071D34 lbu $v0, 0x2D3($v0) */
    r_a0 = 0x800C0000u; /* 80071D38 lui $a0, %hi(D_800C3DDC) */
    r_a0 = bl_lw(r_a0 + 0x3DDCu); /* 80071D3C lw $a0, %lo(D_800C3DDC)($a0) */
    r_at = 0x800C0000u; /* 80071D40 lui $at, %hi(D_800C3E3D) */
    r_at = r_at + r_v0; /* 80071D44 addu $at, $at, $v0 */
    r_a1 = bl_lbu(r_at + 0x3E3Du); /* 80071D48 lbu $a1, %lo(D_800C3E3D)($at) */
    /* 80071D4C jal GetStringEntry */
    ;
    bl_jal(FN, 0x80033728u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* GetStringEntry */
    r_a0 = r_v0 + 0u; /* 80071D54 addu $a0, $v0, $zero */
    r_s0 = 0x800D0000u; /* 80071D58 lui $s0, %hi(D_800D3720) */
    r_s0 = r_s0 + 0x3720u; /* 80071D5C addiu $s0, $s0, %lo(D_800D3720) */
    r_a2 = 0u | 0x39u; /* 80071D60 ori $a2, $zero, 0x39 */
    r_a1 = bl_lw(r_s0); /* 80071D64 lw $a1, 0x0($s0) */
    /* 80071D68 jal SystemRenderStringEntry */
    r_a3 = 0u + 0u;
    bl_jal(FN, 0x80034EACu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SystemRenderStringEntry */
    r_a1 = bl_lw(r_s0); /* 80071D70 lw $a1, 0x0($s0) */
    r_at = 0x800D0000u; /* 80071D74 lui $at, %hi(D_800D3726) */
    bl_sb(r_at + 0x3726u, r_v0); /* 80071D78 sb $v0, %lo(D_800D3726)($at) */
    /* 80071D7C jal func_800769E8 */
    r_a0 = r_s0 + 0xFFFFFFF8u;
    bl_jal(FN, 0x800769E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800769E8 */
    r_at = 0x800D0000u; /* 80071D84 lui $at, %hi(D_800D3725) */
    bl_sb(r_at + 0x3725u, r_s1); /* 80071D88 sb $s1, %lo(D_800D3725)($at) */
L_80071D8C:
    r_v0 = 0x800C0000u; /* 80071D8C lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80071D90 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80071D94 nop  */
    r_a0 = bl_lbu(r_v0 + 0x2D3u); /* 80071D98 lbu $a0, 0x2D3($v0) */
    ; /* 80071D9C nop  */
    r_v0 = r_a0 << 1; /* 80071DA0 sll $v0, $a0, 1 */
    r_v0 = r_v0 + r_a0; /* 80071DA4 addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 3; /* 80071DA8 sll $v0, $v0, 3 */
    r_v0 = r_v0 - r_a0; /* 80071DAC subu $v0, $v0, $a0 */
    r_v1 = r_v0 << 4; /* 80071DB0 sll $v1, $v0, 4 */
    r_at = 0x800D0000u; /* 80071DB4 lui $at, %hi(D_800CCD64) */
    r_at = r_at + r_v1; /* 80071DB8 addu $at, $at, $v1 */
    r_v0 = bl_lhu(r_at + 0xFFFFCD64u); /* 80071DBC lhu $v0, %lo(D_800CCD64)($at) */
    ; /* 80071DC0 nop  */
    r_v0 = r_v0 & 0x2080u; /* 80071DC4 andi $v0, $v0, 0x2080 */
    /* 80071DC8 bnez $v0, .L80071E0C */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_80071E0C; }
    r_at = 0x800D0000u; /* 80071DD0 lui $at, %hi(D_800CCD68) */
    r_at = r_at + r_v1; /* 80071DD4 addu $at, $at, $v1 */
    r_v0 = bl_lhu(r_at + 0xFFFFCD68u); /* 80071DD8 lhu $v0, %lo(D_800CCD68)($at) */
    ; /* 80071DDC nop  */
    r_v0 = r_v0 & 0x1000u; /* 80071DE0 andi $v0, $v0, 0x1000 */
    /* 80071DE4 bnez $v0, .L80071E0C */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_80071E0C; }
    /* 80071DEC jal func_80079778 */
    ;
    bl_jal(FN, 0x80079778u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80079778 */
    /* 80071DF4 jal func_80071A8C */
    ;
    bl_jal(FN, 0x80071A8Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A8C */
    /* 80071DFC jal func_80071964 */
    ;
    bl_jal(FN, 0x80071964u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071964 */
    /* 80071E04 jal func_80071AE0 */
    ;
    bl_jal(FN, 0x80071AE0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071AE0 */
L_80071E0C:
    /* 80071E0C jal func_80071A8C */
    ;
    bl_jal(FN, 0x80071A8Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A8C */
    /* 80071E14 jal func_80070EB0 */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x80070EB0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80070EB0 */
    /* 80071E1C j .L80072158 */
    ; goto L_80072158;
L_80071E24:
    r_at = 0x800C0000u; /* 80071E24 lui $at, %hi(D_800C3D1A) */
    r_at = r_at + r_v0; /* 80071E28 addu $at, $at, $v0 */
    bl_sb(r_at + 0x3D1Au, 0u); /* 80071E2C sb $zero, %lo(D_800C3D1A)($at) */
    r_v0 = r_v0 + 0xFFFFFFFCu; /* 80071E30 addiu $v0, $v0, -0x4 */
    /* 80071E34 bgez $v0, .L80071E24 */
    { int c_ = ((s32)r_v0 >= 0); ; if (c_) goto L_80071E24; }
    r_v0 = 0x800D0000u; /* 80071E3C lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80071E40 lw $v0, %lo(D_800CCB34)($v0) */
    r_t0 = 0x800C0000u; /* 80071E44 lui $t0, %hi(D_800C3EAC) */
    r_t0 = bl_lw(r_t0 + 0x3EACu); /* 80071E48 lw $t0, %lo(D_800C3EAC)($t0) */
    r_a3 = 0x800C0000u; /* 80071E4C lui $a3, %hi(D_800C3EA4) */
    r_a3 = bl_lw(r_a3 + 0x3EA4u); /* 80071E50 lw $a3, %lo(D_800C3EA4)($a3) */
    r_a1 = 0x800D0000u; /* 80071E54 lui $a1, %hi(D_800D3280) */
    r_a1 = bl_lbu(r_a1 + 0x3280u); /* 80071E58 lbu $a1, %lo(D_800D3280)($a1) */
    r_a0 = r_v0 << 1; /* 80071E5C sll $a0, $v0, 1 */
    r_a0 = r_a0 + r_v0; /* 80071E60 addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 80071E64 sll $a0, $a0, 3 */
    r_a0 = r_a0 + r_a3; /* 80071E68 addu $a0, $a0, $a3 */
    r_a2 = r_a1 << 1; /* 80071E6C sll $a2, $a1, 1 */
    r_v0 = bl_lbu(r_t0 + 0x2D3u); /* 80071E70 lbu $v0, 0x2D3($t0) */
    r_a2 = r_a2 + r_a1; /* 80071E74 addu $a2, $a2, $a1 */
    r_v1 = r_v0 << 1; /* 80071E78 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80071E7C addu $v1, $v1, $v0 */
    r_v0 = r_a2 + r_v0; /* 80071E80 addu $v0, $a2, $v0 */
    r_v0 = r_v0 << 1; /* 80071E84 sll $v0, $v0, 1 */
    r_at = 0x800C0000u; /* 80071E88 lui $at, %hi(D_800C3254) */
    r_at = r_at + r_v0; /* 80071E8C addu $at, $at, $v0 */
    r_v0 = bl_lhu(r_at + 0x3254u); /* 80071E90 lhu $v0, %lo(D_800C3254)($at) */
    r_v1 = r_v1 << 5; /* 80071E94 sll $v1, $v1, 5 */
    r_v0 = r_v0 + 0x10u; /* 80071E98 addiu $v0, $v0, 0x10 */
    r_v1 = r_v1 + r_v0; /* 80071E9C addu $v1, $v1, $v0 */
    bl_sh(r_a0 + 0x63D0u, r_v1); /* 80071EA0 sh $v1, 0x63D0($a0) */
    r_v1 = 0x800D0000u; /* 80071EA4 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80071EA8 lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = 0u | 0x8u; /* 80071EAC ori $a1, $zero, 0x8 */
    r_v0 = r_v1 << 1; /* 80071EB0 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80071EB4 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 80071EB8 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_a3; /* 80071EBC addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x63D2u, r_a1); /* 80071EC0 sh $a1, 0x63D2($v0) */
    r_v0 = 0x800D0000u; /* 80071EC4 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80071EC8 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80071ECC nop  */
    r_a0 = r_v0 << 1; /* 80071ED0 sll $a0, $v0, 1 */
    r_a0 = r_a0 + r_v0; /* 80071ED4 addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 80071ED8 sll $a0, $a0, 3 */
    r_v0 = bl_lbu(r_t0 + 0x2D3u); /* 80071EDC lbu $v0, 0x2D3($t0) */
    r_a0 = r_a0 + r_a3; /* 80071EE0 addu $a0, $a0, $a3 */
    r_v1 = r_v0 << 1; /* 80071EE4 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80071EE8 addu $v1, $v1, $v0 */
    r_v0 = r_a2 + r_v0; /* 80071EEC addu $v0, $a2, $v0 */
    r_v0 = r_v0 << 1; /* 80071EF0 sll $v0, $v0, 1 */
    r_at = 0x800C0000u; /* 80071EF4 lui $at, %hi(D_800C3254) */
    r_at = r_at + r_v0; /* 80071EF8 addu $at, $at, $v0 */
    r_v0 = bl_lhu(r_at + 0x3254u); /* 80071EFC lhu $v0, %lo(D_800C3254)($at) */
    r_v1 = r_v1 << 5; /* 80071F00 sll $v1, $v1, 5 */
    r_v0 = r_v0 + r_v1; /* 80071F04 addu $v0, $v0, $v1 */
    r_v0 = r_v0 + 0x28u; /* 80071F08 addiu $v0, $v0, 0x28 */
    bl_sh(r_a0 + 0x63D4u, r_v0); /* 80071F0C sh $v0, 0x63D4($a0) */
    r_v1 = 0x800D0000u; /* 80071F10 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80071F14 lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80071F18 nop  */
    r_v0 = r_v1 << 1; /* 80071F1C sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80071F20 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 80071F24 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_a3; /* 80071F28 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x63D6u, r_a1); /* 80071F2C sh $a1, 0x63D6($v0) */
    r_v0 = 0x800D0000u; /* 80071F30 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80071F34 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80071F38 nop  */
    r_a0 = r_v0 << 1; /* 80071F3C sll $a0, $v0, 1 */
    r_a0 = r_a0 + r_v0; /* 80071F40 addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 80071F44 sll $a0, $a0, 3 */
    r_v0 = bl_lbu(r_t0 + 0x2D3u); /* 80071F48 lbu $v0, 0x2D3($t0) */
    r_a0 = r_a0 + r_a3; /* 80071F4C addu $a0, $a0, $a3 */
    r_v1 = r_v0 << 1; /* 80071F50 sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80071F54 addu $v1, $v1, $v0 */
    r_v0 = r_a2 + r_v0; /* 80071F58 addu $v0, $a2, $v0 */
    r_v0 = r_v0 << 1; /* 80071F5C sll $v0, $v0, 1 */
    r_at = 0x800C0000u; /* 80071F60 lui $at, %hi(D_800C3254) */
    r_at = r_at + r_v0; /* 80071F64 addu $at, $at, $v0 */
    r_v0 = bl_lhu(r_at + 0x3254u); /* 80071F68 lhu $v0, %lo(D_800C3254)($at) */
    r_v1 = r_v1 << 5; /* 80071F6C sll $v1, $v1, 5 */
    r_v0 = r_v0 + 0x10u; /* 80071F70 addiu $v0, $v0, 0x10 */
    r_v1 = r_v1 + r_v0; /* 80071F74 addu $v1, $v1, $v0 */
    bl_sh(r_a0 + 0x63D8u, r_v1); /* 80071F78 sh $v1, 0x63D8($a0) */
    r_v1 = 0x800D0000u; /* 80071F7C lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80071F80 lw $v1, %lo(D_800CCB34)($v1) */
    r_a1 = 0u | 0x20u; /* 80071F84 ori $a1, $zero, 0x20 */
    r_v0 = r_v1 << 1; /* 80071F88 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80071F8C addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 80071F90 sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_a3; /* 80071F94 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x63DAu, r_a1); /* 80071F98 sh $a1, 0x63DA($v0) */
    r_v0 = 0x800D0000u; /* 80071F9C lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lw(r_v0 + 0xFFFFCB34u); /* 80071FA0 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 80071FA4 nop  */
    r_a0 = r_v0 << 1; /* 80071FA8 sll $a0, $v0, 1 */
    r_a0 = r_a0 + r_v0; /* 80071FAC addu $a0, $a0, $v0 */
    r_a0 = r_a0 << 3; /* 80071FB0 sll $a0, $a0, 3 */
    r_v0 = bl_lbu(r_t0 + 0x2D3u); /* 80071FB4 lbu $v0, 0x2D3($t0) */
    r_a0 = r_a0 + r_a3; /* 80071FB8 addu $a0, $a0, $a3 */
    r_v1 = r_v0 << 1; /* 80071FBC sll $v1, $v0, 1 */
    r_v1 = r_v1 + r_v0; /* 80071FC0 addu $v1, $v1, $v0 */
    r_a2 = r_a2 + r_v0; /* 80071FC4 addu $a2, $a2, $v0 */
    r_a2 = r_a2 << 1; /* 80071FC8 sll $a2, $a2, 1 */
    r_at = 0x800C0000u; /* 80071FCC lui $at, %hi(D_800C3254) */
    r_at = r_at + r_a2; /* 80071FD0 addu $at, $at, $a2 */
    r_v0 = bl_lhu(r_at + 0x3254u); /* 80071FD4 lhu $v0, %lo(D_800C3254)($at) */
    r_v1 = r_v1 << 5; /* 80071FD8 sll $v1, $v1, 5 */
    r_v0 = r_v0 + r_v1; /* 80071FDC addu $v0, $v0, $v1 */
    r_v0 = r_v0 + 0x28u; /* 80071FE0 addiu $v0, $v0, 0x28 */
    bl_sh(r_a0 + 0x63DCu, r_v0); /* 80071FE4 sh $v0, 0x63DC($a0) */
    r_v1 = 0x800D0000u; /* 80071FE8 lui $v1, %hi(D_800CCB34) */
    r_v1 = bl_lw(r_v1 + 0xFFFFCB34u); /* 80071FEC lw $v1, %lo(D_800CCB34)($v1) */
    ; /* 80071FF0 nop  */
    r_v0 = r_v1 << 1; /* 80071FF4 sll $v0, $v1, 1 */
    r_v0 = r_v0 + r_v1; /* 80071FF8 addu $v0, $v0, $v1 */
    r_v0 = r_v0 << 3; /* 80071FFC sll $v0, $v0, 3 */
    r_v0 = r_v0 + r_a3; /* 80072000 addu $v0, $v0, $a3 */
    bl_sh(r_v0 + 0x63DEu, r_a1); /* 80072004 sh $a1, 0x63DE($v0) */
    r_v0 = 0x800D0000u; /* 80072008 lui $v0, %hi(D_800CCB34) */
    r_v0 = bl_lbu(r_v0 + 0xFFFFCB34u); /* 8007200C lbu $v0, %lo(D_800CCB34)($v0) */
    ; /* 80072010 nop  */
    bl_sb(r_a3 + 0x6414u, r_v0); /* 80072014 sb $v0, 0x6414($a3) */
    r_v1 = 0x800C0000u; /* 80072018 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 8007201C lw $v1, %lo(D_800C3EA4)($v1) */
    r_v0 = 0u | 0x1u; /* 80072020 ori $v0, $zero, 0x1 */
    bl_sb(r_v1 + 0x6415u, r_v0); /* 80072024 sb $v0, 0x6415($v1) */
    r_v0 = 0x800C0000u; /* 80072028 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 8007202C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80072030 nop  */
    r_a0 = bl_lbu(r_v0 + 0x2D3u); /* 80072034 lbu $a0, 0x2D3($v0) */
    ; /* 80072038 nop  */
    r_v0 = r_a0 << 1; /* 8007203C sll $v0, $a0, 1 */
    r_v0 = r_v0 + r_a0; /* 80072040 addu $v0, $v0, $a0 */
    r_v0 = r_v0 << 3; /* 80072044 sll $v0, $v0, 3 */
    r_v0 = r_v0 - r_a0; /* 80072048 subu $v0, $v0, $a0 */
    r_v1 = r_v0 << 4; /* 8007204C sll $v1, $v0, 4 */
    r_at = 0x800D0000u; /* 80072050 lui $at, %hi(D_800CCD64) */
    r_at = r_at + r_v1; /* 80072054 addu $at, $at, $v1 */
    r_v0 = bl_lhu(r_at + 0xFFFFCD64u); /* 80072058 lhu $v0, %lo(D_800CCD64)($at) */
    ; /* 8007205C nop  */
    r_v0 = r_v0 & 0x2080u; /* 80072060 andi $v0, $v0, 0x2080 */
    /* 80072064 bnez $v0, .L800720F8 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_800720F8; }
    r_at = 0x800D0000u; /* 8007206C lui $at, %hi(D_800CCD68) */
    r_at = r_at + r_v1; /* 80072070 addu $at, $at, $v1 */
    r_v1 = bl_lhu(r_at + 0xFFFFCD68u); /* 80072074 lhu $v1, %lo(D_800CCD68)($at) */
    ; /* 80072078 nop  */
    r_v0 = r_v1 & 0x1000u; /* 8007207C andi $v0, $v1, 0x1000 */
    /* 80072080 bnez $v0, .L800720F8 */
    { int c_ = (r_v0 != 0); r_v0 = r_v1 & 0x2000u; if (c_) goto L_800720F8; }
    /* 80072088 bnez $v0, .L800720A0 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_800720A0; }
    /* 80072090 jal func_80080160 */
    ;
    bl_jal(FN, 0x80080160u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80080160 */
    /* 80072098 j .L800720A8 */
    ; goto L_800720A8;
L_800720A0:
    /* 800720A0 jal func_80080C94 */
    ;
    bl_jal(FN, 0x80080C94u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80080C94 */
L_800720A8:
    r_v1 = 0x800C0000u; /* 800720A8 lui $v1, %hi(D_800C3EAC) */
    r_v1 = bl_lw(r_v1 + 0x3EACu); /* 800720AC lw $v1, %lo(D_800C3EAC)($v1) */
    ; /* 800720B0 nop  */
    r_v0 = bl_lbu(r_v1 + 0x2EAu); /* 800720B4 lbu $v0, 0x2EA($v1) */
    ; /* 800720B8 nop  */
    /* 800720BC beqz $v0, .L800720DC */
    { int c_ = (r_v0 == 0); r_a0 = 0u + 0u; if (c_) goto L_800720DC; }
    r_v0 = 0x80060000u; /* 800720C4 lui $v0, %hi(D_8005941C) */
    r_v0 = bl_lhu(r_v0 + 0xFFFF941Cu); /* 800720C8 lhu $v0, %lo(D_8005941C)($v0) */
    ; /* 800720CC nop  */
    r_v0 = r_v0 + 0x1u; /* 800720D0 addiu $v0, $v0, 0x1 */
    r_at = 0x80060000u; /* 800720D4 lui $at, %hi(D_8005941C) */
    bl_sh(r_at + 0xFFFF941Cu, r_v0); /* 800720D8 sh $v0, %lo(D_8005941C)($at) */
L_800720DC:
    r_v0 = bl_lbu(r_v1 + 0x2D3u); /* 800720DC lbu $v0, 0x2D3($v1) */
    r_v1 = 0x800D0000u; /* 800720E0 lui $v1, %hi(D_800D2D28) */
    r_v1 = bl_lw(r_v1 + 0x2D28u); /* 800720E4 lw $v1, %lo(D_800D2D28)($v1) */
    r_at = 0x800D0000u; /* 800720E8 lui $at, %hi(D_800D36C0) */
    bl_sb(r_at + 0x36C0u, r_v0); /* 800720EC sb $v0, %lo(D_800D36C0)($at) */
    /* 800720F0 jal func_800BCD98 */
    bl_sb(r_v1 + 0x97u, 0u);
    bl_jal(FN, 0x800BCD98u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BCD98 */
L_800720F8:
    /* 800720F8 jal func_80071964 */
    r_s1 = 0u + 0u;
    bl_jal(FN, 0x80071964u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071964 */
    /* 80072100 jal func_80071AE0 */
    r_s2 = 0u | 0x1u;
    bl_jal(FN, 0x80071AE0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071AE0 */
    /* 80072108 jal func_80071A8C */
    r_s0 = 0u + 0u;
    bl_jal(FN, 0x80071A8Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A8C */
    /* 80072110 jal func_80070EB0 */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x80070EB0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80070EB0 */
L_80072118:
    r_a1 = r_s1 + 0x3u; /* 80072118 addiu $a1, $s1, 0x3 */
    r_a0 = 0x800D0000u; /* 8007211C lui $a0, %hi(D_800D2C94) */
    r_a0 = bl_lhu(r_a0 + 0x2C94u); /* 80072120 lhu $a0, %lo(D_800D2C94)($a0) */
    /* 80072124 jal func_80089C9C */
    r_a1 = r_a1 & 0xFFu;
    bl_jal(FN, 0x80089C9Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80089C9C */
    r_v0 = r_v0 & 0xFFFFu; /* 8007212C andi $v0, $v0, 0xFFFF */
    /* 80072130 beqz $v0, .L80072144 */
    { int c_ = (r_v0 == 0); r_s1 = r_s1 + 0x1u; if (c_) goto L_80072144; }
    r_at = 0x800C0000u; /* 80072138 lui $at, %hi(D_800C3D1A) */
    r_at = r_at + r_s0; /* 8007213C addu $at, $at, $s0 */
    bl_sb(r_at + 0x3D1Au, r_s2); /* 80072140 sb $s2, %lo(D_800C3D1A)($at) */
L_80072144:
    r_v0 = (u32)((s32)r_s1 < 8); /* 80072144 slti $v0, $s1, 0x8 */
    /* 80072148 bnez $v0, .L80072118 */
    { int c_ = (r_v0 != 0); r_s0 = r_s0 + 0x4u; if (c_) goto L_80072118; }
    /* 80072150 jal func_80079C24 */
    ;
    bl_jal(FN, 0x80079C24u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80079C24 */
L_80072158:
    /* 80072158 jal func_80071A38 */
    r_s1 = 0u + 0u;
    bl_jal(FN, 0x80071A38u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A38 */
    /* 80072160 jal func_80072270 */
    r_s2 = 0u + 0u;
    bl_jal(FN, 0x80072270u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80072270 */
    r_v0 = 0x800C0000u; /* 80072168 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8007216C lw $v0, %lo(D_800C3EA4)($v0) */
    ; /* 80072170 nop  */
    bl_sb(r_v0 + 0x6415u, 0u); /* 80072174 sb $zero, 0x6415($v0) */
L_80072178:
    r_s0 = r_s1 & 0xFFu; /* 80072178 andi $s0, $s1, 0xFF */
    /* 8007217C jal func_800841E0 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x800841E0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800841E0 */
    r_v1 = 0x800C0000u; /* 80072184 lui $v1, %hi(D_800C3EAC) */
    r_v1 = bl_lw(r_v1 + 0x3EACu); /* 80072188 lw $v1, %lo(D_800C3EAC)($v1) */
    r_a1 = r_s1 << 6; /* 8007218C sll $a1, $s1, 6 */
    r_v1 = r_v1 + r_a1; /* 80072190 addu $v1, $v1, $a1 */
    bl_sb(r_v1 + 0x3Cu, r_v0); /* 80072194 sb $v0, 0x3C($v1) */
    r_v0 = 0x800C0000u; /* 80072198 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 8007219C lw $v0, %lo(D_800C3EAC)($v0) */
    r_a0 = r_s0 + 0u; /* 800721A0 addu $a0, $s0, $zero */
    r_v0 = r_v0 + r_a1; /* 800721A4 addu $v0, $v0, $a1 */
    r_a1 = bl_lbu(r_v0 + 0x3Cu); /* 800721A8 lbu $a1, 0x3C($v0) */
    /* 800721AC jal func_80085310 */
    r_s1 = r_s1 + 0x1u;
    bl_jal(FN, 0x80085310u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80085310 */
    r_at = 0x800C0000u; /* 800721B4 lui $at, %hi(D_800C3EBA) */
    r_at = r_at + r_s2; /* 800721B8 addu $at, $at, $s2 */
    bl_sb(r_at + 0x3EBAu, r_v0); /* 800721BC sb $v0, %lo(D_800C3EBA)($at) */
    r_v0 = (u32)((s32)r_s1 < 11); /* 800721C0 slti $v0, $s1, 0xB */
    /* 800721C4 bnez $v0, .L80072178 */
    { int c_ = (r_v0 != 0); r_s2 = r_s2 + 0x1Cu; if (c_) goto L_80072178; }
    r_v0 = 0x800C0000u; /* 800721CC lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 800721D0 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 800721D4 nop  */
    r_a0 = bl_lbu(r_v0 + 0x2D3u); /* 800721D8 lbu $a0, 0x2D3($v0) */
    /* 800721DC jal func_80080AE4 */
    ;
    bl_jal(FN, 0x80080AE4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80080AE4 */
    /* 800721E4 jal func_800BA4E0 */
    r_a0 = r_v0 + 0u;
    bl_jal(FN, 0x800BA4E0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BA4E0 */
    /* 800721EC jal func_80071A08 */
    ;
    bl_jal(FN, 0x80071A08u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A08 */
    /* 800721F4 jal func_8007252C */
    ;
    bl_jal(FN, 0x8007252Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8007252C */
    r_v0 = 0x800C0000u; /* 800721FC lui $v0, %hi(D_800C48EA) */
    r_v0 = bl_lbu(r_v0 + 0x48EAu); /* 80072200 lbu $v0, %lo(D_800C48EA)($v0) */
    ; /* 80072204 nop  */
    /* 80072208 bnez $v0, .L80072228 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_80072228; }
    r_v0 = 0x800C0000u; /* 80072210 lui $v0, %hi(D_800C3EAC) */
    r_v0 = bl_lw(r_v0 + 0x3EACu); /* 80072214 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80072218 nop  */
    r_a0 = bl_lbu(r_v0 + 0x2D3u); /* 8007221C lbu $a0, 0x2D3($v0) */
    /* 80072220 jal func_80085B58 */
    ;
    bl_jal(FN, 0x80085B58u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80085B58 */
L_80072228:
    /* 80072228 jal func_8007252C */
    ;
    bl_jal(FN, 0x8007252Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8007252C */
    /* 80072230 jal func_80071A38 */
    ;
    bl_jal(FN, 0x80071A38u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80071A38 */
    /* 80072238 jal func_800BFE48 */
    ;
    bl_jal(FN, 0x800BFE48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BFE48 */
    /* 80072240 jal func_800718BC */
    ;
    bl_jal(FN, 0x800718BCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800718BC */
    r_v0 = 0u | 0x1u; /* 80072248 ori $v0, $zero, 0x1 */
    r_at = 0x800D0000u; /* 8007224C lui $at, %hi(D_800D3298) */
    bl_sb(r_at + 0x3298u, r_v0); /* 80072250 sb $v0, %lo(D_800D3298)($at) */
L_80072254:
    r_ra = bl_lw(r_sp + 0x34u); /* 80072254 lw $ra, 0x34($sp) */
    r_s2 = bl_lw(r_sp + 0x30u); /* 80072258 lw $s2, 0x30($sp) */
    r_s1 = bl_lw(r_sp + 0x2Cu); /* 8007225C lw $s1, 0x2C($sp) */
    r_s0 = bl_lw(r_sp + 0x28u); /* 80072260 lw $s0, 0x28($sp) */
    r_sp = r_sp + 0x38u; /* 80072264 addiu $sp, $sp, 0x38 */
    /* 80072268 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop
#endif

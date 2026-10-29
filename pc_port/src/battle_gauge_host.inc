static void battle_gauge_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFD0u; /* 80075938 addiu $sp, $sp, -0x30 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x2Cu, 4, entry->cpu->gpr[31]); /* 8007593C sw $ra, 0x2C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x28u, 4, entry->cpu->gpr[18]); /* 80075940 sw $s2, 0x28($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x24u, 4, entry->cpu->gpr[17]); /* 80075944 sw $s1, 0x24($sp) */
    /* 80075948 jal func_80075168 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[16]);
    battle_phase_call(entry, 0x80075168u, 0x80075948u);
    entry->cpu->gpr[9] = 0u + 0u; /* 80075950 addu $t1, $zero, $zero */
    entry->cpu->gpr[24] = 0x68DB0000u; /* 80075954 lui $t8, (0x68DB8BAD >> 16) */
    entry->cpu->gpr[24] = entry->cpu->gpr[24] | 0x8BADu; /* 80075958 ori $t8, $t8, (0x68DB8BAD & 0xFFFF) */
    entry->cpu->gpr[16] = 0u | 0x1Au; /* 8007595C ori $s0, $zero, 0x1A */
    entry->cpu->gpr[17] = 0u | 0x1Eu; /* 80075960 ori $s1, $zero, 0x1E */
    entry->cpu->gpr[18] = 0u | 0xCu; /* 80075964 ori $s2, $zero, 0xC */
    entry->cpu->gpr[25] = 0u | 0xCEu; /* 80075968 ori $t9, $zero, 0xCE */
    entry->cpu->gpr[10] = 0u + 0u; /* 8007596C addu $t2, $zero, $zero */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075970 lui $v0, %entry->cpu->hi(D_800D2E06) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x2E06u; /* 80075974 addiu $v0, $v0, %entry->cpu->lo(D_800D2E06) */
    entry->cpu->gpr[12] = entry->cpu->gpr[2] + 0u; /* 80075978 addu $t4, $v0, $zero */
    entry->cpu->gpr[7] = 0u + 0u; /* 8007597C addu $a3, $zero, $zero */
    entry->cpu->gpr[11] = 0u + 0u; /* 80075980 addu $t3, $zero, $zero */
    entry->cpu->gpr[15] = entry->cpu->gpr[12] + 0xFFFFFFEAu; /* 80075984 addiu $t7, $t4, -0x16 */
    entry->cpu->gpr[14] = 0u + 0u; /* 80075988 addu $t6, $zero, $zero */
    entry->cpu->gpr[2] = 0x800D0000u; /* 8007598C lui $v0, %entry->cpu->hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80075990 lw $v0, %entry->cpu->lo(D_800D2D28)($v0) */
    entry->cpu->gpr[13] = 0u + 0u; /* 80075994 addu $t5, $zero, $zero */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x82u, 1, 0u); /* 80075998 sb $zero, 0x82($v0) */
L_8007599C:
    entry->cpu->gpr[1] = 0x800C0000u; /* 8007599C lui $at, %entry->cpu->hi(D_800C3EB6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[14]; /* 800759A0 addu $at, $at, $t6 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3EB6u, 1); /* 800759A4 lbu $v1, %entry->cpu->lo(D_800C3EB6)($at) */
    entry->cpu->gpr[2] = 0u | 0x7Fu; /* 800759A8 ori $v0, $zero, 0x7F */
    /* 800759AC beq $v1, $v0, .L800763C0 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[2] = entry->cpu->gpr[9] << 2; if (c_) goto L_800763C0; }
    entry->cpu->gpr[3] = entry->cpu->gpr[29] + 0x10u; /* 800759B4 addiu $v1, $sp, 0x10 */
    entry->cpu->gpr[8] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800759B8 addu $t0, $v0, $v1 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[8], 4); /* 800759BC lw $v0, 0x0($t0) */
    ; /* 800759C0 nop  */
    /* 800759C4 bgez $v0, .L800759D0 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] >= 0); ; if (c_) goto L_800759D0; }
    battle_entry_write(entry, entry->cpu->gpr[8], 4, 0u); /* 800759CC sw $zero, 0x0($t0) */
L_800759D0:
    entry->cpu->gpr[2] = 0x800D0000u; /* 800759D0 lui $v0, %entry->cpu->hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800759D4 lw $v0, %entry->cpu->lo(D_800D2D28)($v0) */
    ; /* 800759D8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[9]; /* 800759DC addu $v0, $v0, $t1 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x90u, 1); /* 800759E0 lbu $v0, 0x90($v0) */
    ; /* 800759E4 nop  */
    /* 800759E8 bnez $v0, .L80075F1C */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[3] = 0u | 0x64u; if (c_) goto L_80075F1C; }
    entry->cpu->gpr[3] = ((uint32_t)(int32_t)(int16_t)battle_entry_read(entry, entry->cpu->gpr[12], 2)); /* 800759F0 lh $v1, 0x0($t4) */
    ; /* 800759F4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800759F8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800759FC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80075A00 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075A04 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = ((uint32_t)(int32_t)(int16_t)battle_entry_read(entry, entry->cpu->gpr[15], 2)); /* 80075A08 lh $v1, 0x0($t7) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075A0C sll $v0, $v0, 2 */
    { int32_t lhs = (int32_t)entry->cpu->gpr[2], rhs = (int32_t)entry->cpu->gpr[3]; if (!rhs) { entry->cpu->lo = lhs >= 0 ? UINT32_MAX : 1u; entry->cpu->hi = (uint32_t)lhs; } else if (lhs == INT32_MIN && rhs == -1) { entry->cpu->lo = (uint32_t)INT32_MIN; entry->cpu->hi = 0; } else { entry->cpu->lo = (uint32_t)(lhs / rhs); entry->cpu->hi = (uint32_t)(lhs % rhs); } } /* 80075A10 div $zero, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->lo; /* 80075A14 mflo $v0 */
    entry->cpu->gpr[3] = 0u | 0x64u; /* 80075A18 ori $v1, $zero, 0x64 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80075A1C subu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075A20 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075A24 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075A28 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075A2C subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80075A30 sll $v0, $v0, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075A34 subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 5; /* 80075A38 sll $v0, $v0, 5 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[24]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80075A3C mult $v0, $t8 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 31)); /* 80075A40 sra $v0, $v0, 31 */
    entry->cpu->gpr[3] = entry->cpu->hi; /* 80075A44 mfhi $v1 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 12)); /* 80075A48 sra $v1, $v1, 12 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80075A4C subu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[8], 4, entry->cpu->gpr[3]); /* 80075A50 sw $v1, 0x0($t0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075A54 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075A58 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    entry->cpu->gpr[6] = 0x800C0000u; /* 80075A5C lui $a2, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x3EA4u, 4); /* 80075A60 lw $a2, %entry->cpu->lo(D_800C3EA4)($a2) */
    entry->cpu->gpr[2] = entry->cpu->gpr[7] + entry->cpu->gpr[2]; /* 80075A64 addu $v0, $a3, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80075A68 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075A6C addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075A70 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075A74 addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075A78 lui $v0, %entry->cpu->hi(D_800D3280) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3280u, 1); /* 80075A7C lbu $v0, %entry->cpu->lo(D_800D3280)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075A80 sll $v1, $v1, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 1; /* 80075A84 sll $a1, $v0, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80075A88 addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[9]; /* 80075A8C addu $a1, $a1, $t1 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80075A90 lui $v0, %entry->cpu->hi(D_800C3254) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x3254u; /* 80075A94 addiu $v0, $v0, %entry->cpu->lo(D_800C3254) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 1; /* 80075A98 sll $a1, $a1, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80075A9C addu $a1, $a1, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075AA0 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[6]; /* 80075AA4 addu $v1, $v1, $a2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[11]; /* 80075AA8 addu $v0, $v0, $t3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x2Du; /* 80075AAC addiu $v0, $v0, 0x2D */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5A8u, 2, entry->cpu->gpr[2]); /* 80075AB0 sh $v0, 0x5A8($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075AB4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075AB8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075ABC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075AC0 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075AC4 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075AC8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075ACC sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075AD0 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075AD4 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80075AD8 addu $v0, $v0, $a2 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5AAu, 2, entry->cpu->gpr[16]); /* 80075ADC sh $s0, 0x5AA($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075AE0 lui $v0, %entry->cpu->hi(D_800CCB34) */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075AE4 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[8], 2); /* 80075AE8 */
        entry->cpu->gpr[2] = pending_first;
        entry->cpu->gpr[4] = pending_second;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[7] + entry->cpu->gpr[2]; /* 80075AEC addu $v0, $a3, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80075AF0 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075AF4 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075AF8 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075AFC addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075B00 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[6]; /* 80075B04 addu $v1, $v1, $a2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075B08 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x1u; /* 80075B0C addiu $a0, $a0, 0x1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x2Cu; /* 80075B10 addiu $v0, $v0, 0x2C */
    entry->cpu->gpr[2] = entry->cpu->gpr[11] + entry->cpu->gpr[2]; /* 80075B14 addu $v0, $t3, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075B18 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5B4u, 2, entry->cpu->gpr[2]); /* 80075B1C sh $v0, 0x5B4($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075B20 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075B24 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075B28 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075B2C addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075B30 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075B34 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075B38 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075B3C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075B40 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80075B44 addu $v0, $v0, $a2 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5B6u, 2, entry->cpu->gpr[16]); /* 80075B48 sh $s0, 0x5B6($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075B4C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075B50 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075B54 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075B58 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075B5C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075B60 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075B64 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075B68 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075B6C sll $v0, $v0, 2 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075B70 lhu $v1, 0x0($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80075B74 addu $v0, $v0, $a2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[11]; /* 80075B78 addu $v1, $v1, $t3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x2Du; /* 80075B7C addiu $v1, $v1, 0x2D */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5C0u, 2, entry->cpu->gpr[3]); /* 80075B80 sh $v1, 0x5C0($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075B84 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075B88 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075B8C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075B90 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075B94 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075B98 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075B9C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075BA0 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075BA4 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80075BA8 addu $v0, $v0, $a2 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5C2u, 2, entry->cpu->gpr[17]); /* 80075BAC sh $s1, 0x5C2($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075BB0 lui $v0, %entry->cpu->hi(D_800CCB34) */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075BB4 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[8], 2); /* 80075BB8 */
        entry->cpu->gpr[2] = pending_first;
        entry->cpu->gpr[4] = pending_second;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[7] + entry->cpu->gpr[2]; /* 80075BBC addu $v0, $a3, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80075BC0 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075BC4 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075BC8 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075BCC addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075BD0 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[6]; /* 80075BD4 addu $v1, $v1, $a2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075BD8 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x1u; /* 80075BDC addiu $a0, $a0, 0x1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x2Cu; /* 80075BE0 addiu $v0, $v0, 0x2C */
    entry->cpu->gpr[2] = entry->cpu->gpr[11] + entry->cpu->gpr[2]; /* 80075BE4 addu $v0, $t3, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075BE8 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5CCu, 2, entry->cpu->gpr[2]); /* 80075BEC sh $v0, 0x5CC($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075BF0 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075BF4 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075BF8 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075BFC addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075C00 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C04 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075C08 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C0C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075C10 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80075C14 addu $v0, $v0, $a2 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5CEu, 2, entry->cpu->gpr[17]); /* 80075C18 sh $s1, 0x5CE($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075C1C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075C20 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075C24 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075C28 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075C2C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C30 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075C34 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C38 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075C3C sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075C40 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[6] + entry->cpu->gpr[1]; /* 80075C44 addu $at, $a2, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 80075C48 lbu $v1, -0x5DBC($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80075C4C addu $v0, $v0, $a2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & 0x3Fu; /* 80075C50 andi $v1, $v1, 0x3F */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 80075C54 sll $v1, $v1, 1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5ACu, 1, entry->cpu->gpr[3]); /* 80075C58 sb $v1, 0x5AC($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075C5C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075C60 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075C64 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075C68 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075C6C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C70 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075C74 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C78 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075C7C lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075C80 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075C84 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075C88 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80075C8C addu $at, $v1, $at */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 80075C90 lbu $a0, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075C94 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5ADu, 1, entry->cpu->gpr[4]); /* 80075C98 sb $a0, 0x5AD($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075C9C lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075CA0 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 80075CA4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[7] + entry->cpu->gpr[2]; /* 80075CA8 addu $v0, $a3, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80075CAC sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075CB0 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075CB4 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075CB8 addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80075CBC lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80075CC0 lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075CC4 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075CC8 addu $v1, $v1, $v0 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075CCC lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 80075CD0 addu $at, $v0, $at */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 80075CD4 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[8], 4); /* 80075CD8 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[2] = pending_second;
    }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] & 0x3Fu; /* 80075CDC andi $a0, $a0, 0x3F */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075CE0 addu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80075CE4 sll $v0, $v0, 1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5B8u, 1, entry->cpu->gpr[2]); /* 80075CE8 sb $v0, 0x5B8($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075CEC lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075CF0 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075CF4 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075CF8 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075CFC sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D00 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075D04 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D08 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075D0C lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075D10 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075D14 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075D18 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80075D1C addu $at, $v1, $at */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 80075D20 lbu $a0, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D24 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5B9u, 1, entry->cpu->gpr[4]); /* 80075D28 sb $a0, 0x5B9($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075D2C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075D30 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80075D34 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80075D38 lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075D3C addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075D40 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D44 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075D48 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D4C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075D50 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075D54 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 80075D58 addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 80075D5C lbu $v1, -0x5DBC($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075D60 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & 0x3Fu; /* 80075D64 andi $v1, $v1, 0x3F */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 80075D68 sll $v1, $v1, 1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5C4u, 1, entry->cpu->gpr[3]); /* 80075D6C sb $v1, 0x5C4($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075D70 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075D74 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80075D78 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80075D7C lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075D80 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075D84 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D88 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075D8C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075D90 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075D94 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075D98 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 80075D9C addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 80075DA0 lbu $v1, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075DA4 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x4u; /* 80075DA8 addiu $v1, $v1, 0x4 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5C5u, 1, entry->cpu->gpr[3]); /* 80075DAC sb $v1, 0x5C5($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075DB0 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075DB4 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 80075DB8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[7] + entry->cpu->gpr[2]; /* 80075DBC addu $v0, $a3, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80075DC0 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075DC4 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075DC8 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075DCC addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80075DD0 lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80075DD4 lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075DD8 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075DDC addu $v1, $v1, $v0 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075DE0 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 80075DE4 addu $at, $v0, $at */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 80075DE8 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[8], 4); /* 80075DEC */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[2] = pending_second;
    }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] & 0x3Fu; /* 80075DF0 andi $a0, $a0, 0x3F */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075DF4 addu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80075DF8 sll $v0, $v0, 1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5D0u, 1, entry->cpu->gpr[2]); /* 80075DFC sb $v0, 0x5D0($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075E00 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075E04 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80075E08 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80075E0C lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075E10 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075E14 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075E18 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075E1C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075E20 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075E24 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80075E28 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 80075E2C addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 80075E30 lbu $v1, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80075E34 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x4u; /* 80075E38 addiu $v1, $v1, 0x4 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5D1u, 1, entry->cpu->gpr[3]); /* 80075E3C sb $v1, 0x5D1($v0) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075E40 lui $at, %entry->cpu->hi(D_800CCD6C) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[10]; /* 80075E44 addu $at, $at, $t2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD6Cu, 2); /* 80075E48 lhu $v0, %entry->cpu->lo(D_800CCD6C)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075E4C lui $at, %entry->cpu->hi(D_800CCD6E) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[10]; /* 80075E50 addu $at, $at, $t2 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD6Eu, 2); /* 80075E54 lhu $v1, %entry->cpu->lo(D_800CCD6E)($at) */
    ; /* 80075E58 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | entry->cpu->gpr[3]; /* 80075E5C or $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x8000u; /* 80075E60 andi $v0, $v0, 0x8000 */
    /* 80075E64 beqz $v0, .L80075E88 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_80075E88; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 80075E6C lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80075E70 lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[1] = 0x10000u; /* 80075E74 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 80075E78 addu $at, $v0, $at */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA2B2u, 2); /* 80075E7C lhu $a1, -0x5D4E($at) */
    /* 80075E80 j .L80075ED4 */
    ; goto L_80075ED4;
L_80075E88:
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075E88 lui $at, %entry->cpu->hi(D_800CCD64) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[10]; /* 80075E8C addu $at, $at, $t2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD64u, 2); /* 80075E90 lhu $v0, %entry->cpu->lo(D_800CCD64)($at) */
    ; /* 80075E94 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x1000u; /* 80075E98 andi $v0, $v0, 0x1000 */
    /* 80075E9C beqz $v0, .L80075EC0 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_80075EC0; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 80075EA4 lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80075EA8 lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[1] = 0x10000u; /* 80075EAC lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 80075EB0 addu $at, $v0, $at */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA2B0u, 2); /* 80075EB4 lhu $a1, -0x5D50($at) */
    /* 80075EB8 j .L80075ED4 */
    ; goto L_80075ED4;
L_80075EC0:
    entry->cpu->gpr[2] = 0x800C0000u; /* 80075EC0 lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80075EC4 lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[1] = 0x10000u; /* 80075EC8 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 80075ECC addu $at, $v0, $at */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA2ACu, 2); /* 80075ED0 lhu $a1, -0x5D54($at) */
L_80075ED4:
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075ED4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075ED8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 80075EDC lui $a0, %entry->cpu->hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 80075EE0 lw $a0, %entry->cpu->lo(D_800D2D28)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[3]; /* 80075EE4 addu $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075EE8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075EEC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075EF0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075EF4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075EF8 sll $v0, $v0, 2 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075EFC lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075F00 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[9]; /* 80075F04 addu $a0, $a0, $t1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075F08 addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 80075F0C ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5AEu, 2, entry->cpu->gpr[5]); /* 80075F10 sh $a1, 0x5AE($v1) */
    /* 80075F14 j .L800763D4 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x7Fu, 1, entry->cpu->gpr[2]); goto L_800763D4;
L_80075F1C:
    entry->cpu->gpr[2] = ((uint32_t)(int32_t)(int16_t)battle_entry_read(entry, entry->cpu->gpr[12], 2)); /* 80075F1C lh $v0, 0x0($t4) */
    ; /* 80075F20 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80075F24 subu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075F28 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075F2C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075F30 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075F34 subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80075F38 sll $v0, $v0, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075F3C subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 5; /* 80075F40 sll $v0, $v0, 5 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[24]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80075F44 mult $v0, $t8 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 31)); /* 80075F48 sra $v0, $v0, 31 */
    entry->cpu->gpr[3] = entry->cpu->hi; /* 80075F4C mfhi $v1 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 12)); /* 80075F50 sra $v1, $v1, 12 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80075F54 subu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[8], 4, entry->cpu->gpr[3]); /* 80075F58 sw $v1, 0x0($t0) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075F5C lui $at, %entry->cpu->hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[13]; /* 80075F60 addu $at, $at, $t5 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 80075F64 lbu $v0, %entry->cpu->lo(D_800D32A1)($at) */
    ; /* 80075F68 nop  */
    /* 80075F6C beqz $v0, .L80075FFC */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[6] = entry->cpu->gpr[9] << 2; if (c_) goto L_80075FFC; }
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075F74 lui $at, %entry->cpu->hi(D_800D2D24) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[9]; /* 80075F78 addu $at, $at, $t1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x2D24u, 1); /* 80075F7C lbu $v1, %entry->cpu->lo(D_800D2D24)($at) */
    entry->cpu->gpr[2] = 0u | 0x7u; /* 80075F80 ori $v0, $zero, 0x7 */
    /* 80075F84 beq $v1, $v0, .L80075FFC */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); ; if (c_) goto L_80075FFC; }
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075F8C lui $at, %entry->cpu->hi(D_800CCDC4) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[10]; /* 80075F90 addu $at, $at, $t2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDC4u, 2); /* 80075F94 lhu $v0, %entry->cpu->lo(D_800CCDC4)($at) */
    ; /* 80075F98 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80075F9C sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075FA0 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 3; /* 80075FA4 sll $v1, $v1, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075FA8 addu $v1, $v1, $v0 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075FAC lui $at, %entry->cpu->hi(D_800CCDC6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[10]; /* 80075FB0 addu $at, $at, $t2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDC6u, 2); /* 80075FB4 lhu $v0, %entry->cpu->lo(D_800CCDC6)($at) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075FB8 sll $v1, $v1, 2 */
    { int32_t lhs = (int32_t)entry->cpu->gpr[3], rhs = (int32_t)entry->cpu->gpr[2]; if (!rhs) { entry->cpu->lo = lhs >= 0 ? UINT32_MAX : 1u; entry->cpu->hi = (uint32_t)lhs; } else if (lhs == INT32_MIN && rhs == -1) { entry->cpu->lo = (uint32_t)INT32_MIN; entry->cpu->hi = 0; } else { entry->cpu->lo = (uint32_t)(lhs / rhs); entry->cpu->hi = (uint32_t)(lhs % rhs); } } /* 80075FBC div $zero, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->lo; /* 80075FC0 mflo $v1 */
    ; /* 80075FC4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075FC8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075FCC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075FD0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075FD4 subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80075FD8 sll $v0, $v0, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075FDC subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 5; /* 80075FE0 sll $v0, $v0, 5 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[24]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80075FE4 mult $v0, $t8 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 31)); /* 80075FE8 sra $v0, $v0, 31 */
    entry->cpu->gpr[3] = entry->cpu->hi; /* 80075FEC mfhi $v1 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 12)); /* 80075FF0 sra $v1, $v1, 12 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80075FF4 subu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[8], 4, entry->cpu->gpr[3]); /* 80075FF8 sw $v1, 0x0($t0) */
L_80075FFC:
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075FFC lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076000 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80076004 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80076008 lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 8007600C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076010 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076014 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076018 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007601C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076020 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6E0u, 2, entry->cpu->gpr[18]); /* 80076024 sh $s2, 0x6E0($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076028 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007602C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80076030 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076034 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076038 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007603C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076040 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076044 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076048 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6E2u, 2, entry->cpu->gpr[25]); /* 8007604C sh $t9, 0x6E2($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076050 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076054 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80076058 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 8007605C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076060 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076064 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076068 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007606C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076070 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6ECu, 2, entry->cpu->gpr[18]); /* 80076074 sh $s2, 0x6EC($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80076078 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 8007607C lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 80076080 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80076084 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80076088 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 8007608C sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80076090 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80076094 sll $v1, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[29] + 0x10u; /* 80076098 addiu $v0, $sp, 0x10 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 8007609C addu $a2, $a2, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6], 4); /* 800760A0 lw $v0, 0x0($a2) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[4]; /* 800760A4 addu $v1, $v1, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 800760A8 sll $v0, $v0, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[25] - entry->cpu->gpr[2]; /* 800760AC subu $v0, $t9, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x6EEu, 2, entry->cpu->gpr[2]); /* 800760B0 sh $v0, 0x6EE($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800760B4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800760B8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[5] = 0u | 0x14u; /* 800760BC ori $a1, $zero, 0x14 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800760C0 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800760C4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800760C8 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800760CC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800760D0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 800760D4 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6F8u, 2, entry->cpu->gpr[5]); /* 800760D8 sh $a1, 0x6F8($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800760DC lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800760E0 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800760E4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800760E8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800760EC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800760F0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800760F4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800760F8 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 800760FC addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6FAu, 2, entry->cpu->gpr[25]); /* 80076100 sh $t9, 0x6FA($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076104 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076108 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 8007610C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076110 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076114 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076118 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007611C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076120 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076124 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x704u, 2, entry->cpu->gpr[5]); /* 80076128 sh $a1, 0x704($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007612C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076130 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80076134 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076138 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007613C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076140 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076144 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076148 sll $v0, $v0, 2 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[6], 4); /* 8007614C lw $v1, 0x0($a2) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076150 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 80076154 sll $v1, $v1, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[25] - entry->cpu->gpr[3]; /* 80076158 subu $v1, $t9, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x706u, 2, entry->cpu->gpr[3]); /* 8007615C sh $v1, 0x706($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076160 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076164 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80076168 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 8007616C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076170 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076174 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076178 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007617C sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80076180 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 80076184 addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 80076188 lbu $v1, -0x5DBC($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 8007618C addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & 0x3Fu; /* 80076190 andi $v1, $v1, 0x3F */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 80076194 sll $v1, $v1, 1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6E4u, 1, entry->cpu->gpr[3]); /* 80076198 sb $v1, 0x6E4($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007619C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800761A0 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800761A4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800761A8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800761AC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800761B0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800761B4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800761B8 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800761BC lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800761C0 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 800761C4 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 800761C8 addu $at, $v1, $at */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 800761CC lbu $a0, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800761D0 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6E5u, 1, entry->cpu->gpr[4]); /* 800761D4 sb $a0, 0x6E5($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800761D8 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 800761DC lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 800761E0 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 800761E4 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800761E8 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 800761EC sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800761F0 addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800761F4 lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 800761F8 lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 800761FC sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80076200 addu $v1, $v1, $v0 */
    entry->cpu->gpr[1] = 0x10000u; /* 80076204 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 80076208 addu $at, $v0, $at */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 8007620C */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[6], 4); /* 80076210 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[2] = pending_second;
    }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] & 0x3Fu; /* 80076214 andi $a0, $a0, 0x3F */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076218 addu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 8007621C sll $v0, $v0, 1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x6F0u, 1, entry->cpu->gpr[2]); /* 80076220 sb $v0, 0x6F0($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076224 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076228 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 8007622C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076230 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076234 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076238 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007623C addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80076240 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80076244 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076248 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 8007624C lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80076250 addu $at, $v1, $at */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 80076254 lbu $a0, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076258 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6F1u, 1, entry->cpu->gpr[4]); /* 8007625C sb $a0, 0x6F1($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076260 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076264 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80076268 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 8007626C lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076270 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076274 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076278 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007627C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076280 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80076284 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 80076288 addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 8007628C lbu $v1, -0x5DBC($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076290 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & 0x3Fu; /* 80076294 andi $v1, $v1, 0x3F */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 80076298 sll $v1, $v1, 1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6FCu, 1, entry->cpu->gpr[3]); /* 8007629C sb $v1, 0x6FC($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800762A0 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800762A4 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 800762A8 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 800762AC lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800762B0 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800762B4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800762B8 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800762BC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800762C0 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 800762C4 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 800762C8 addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 800762CC lbu $v1, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 800762D0 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x4u; /* 800762D4 addiu $v1, $v1, 0x4 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6FDu, 1, entry->cpu->gpr[3]); /* 800762D8 sb $v1, 0x6FD($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800762DC lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 800762E0 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 800762E4 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 800762E8 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800762EC addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 800762F0 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800762F4 addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800762F8 lui $v0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 800762FC lw $v0, %entry->cpu->lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80076300 sll $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80076304 addu $v1, $v1, $v0 */
    entry->cpu->gpr[1] = 0x10000u; /* 80076308 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 8007630C addu $at, $v0, $at */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA244u, 1); /* 80076310 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[6], 4); /* 80076314 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[2] = pending_second;
    }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] & 0x3Fu; /* 80076318 andi $a0, $a0, 0x3F */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 8007631C addu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80076320 sll $v0, $v0, 1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x708u, 1, entry->cpu->gpr[2]); /* 80076324 sb $v0, 0x708($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076328 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007632C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80076330 lui $a0, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80076334 lw $a0, %entry->cpu->lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076338 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007633C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076340 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076344 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076348 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 8007634C lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[4] + entry->cpu->gpr[1]; /* 80076350 addu $at, $a0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA248u, 1); /* 80076354 lbu $v1, -0x5DB8($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80076358 addu $v0, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x4u; /* 8007635C addiu $v1, $v1, 0x4 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x709u, 1, entry->cpu->gpr[3]); /* 80076360 sb $v1, 0x709($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076364 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80076368 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[5] = 0x800D0000u; /* 8007636C lui $a1, %entry->cpu->hi(D_800D2D28) */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x2D28u, 4); /* 80076370 lw $a1, %entry->cpu->lo(D_800D2D28)($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x6u; /* 80076374 addiu $v1, $v1, 0x6 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80076378 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007637C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076380 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076384 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80076388 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 8007638C lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80076390 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x10000u; /* 80076394 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80076398 addu $at, $v1, $at */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFA2AEu, 2); /* 8007639C lhu $a0, -0x5D52($at) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800763A0 addu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x5AEu, 2, entry->cpu->gpr[4]); /* 800763A4 sh $a0, 0x5AE($v1) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0xCBu, 1); /* 800763A8 lbu $v0, 0xCB($a1) */
    ; /* 800763AC nop  */
    /* 800763B0 beqz $v0, .L800763D4 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = 0u | 0x1u; if (c_) goto L_800763D4; }
    /* 800763B8 j .L800763D4 */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x82u, 1, entry->cpu->gpr[2]); goto L_800763D4;
L_800763C0:
    entry->cpu->gpr[2] = 0x800D0000u; /* 800763C0 lui $v0, %entry->cpu->hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800763C4 lw $v0, %entry->cpu->lo(D_800D2D28)($v0) */
    ; /* 800763C8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[9]; /* 800763CC addu $v0, $v0, $t1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x7Fu, 1, 0u); /* 800763D0 sb $zero, 0x7F($v0) */
L_800763D4:
    entry->cpu->gpr[10] = entry->cpu->gpr[10] + 0x170u; /* 800763D4 addiu $t2, $t2, 0x170 */
    entry->cpu->gpr[12] = entry->cpu->gpr[12] + 0x2u; /* 800763D8 addiu $t4, $t4, 0x2 */
    entry->cpu->gpr[7] = entry->cpu->gpr[7] + 0x2u; /* 800763DC addiu $a3, $a3, 0x2 */
    entry->cpu->gpr[11] = entry->cpu->gpr[11] + 0x60u; /* 800763E0 addiu $t3, $t3, 0x60 */
    entry->cpu->gpr[15] = entry->cpu->gpr[15] + 0x2u; /* 800763E4 addiu $t7, $t7, 0x2 */
    entry->cpu->gpr[14] = entry->cpu->gpr[14] + 0x1Cu; /* 800763E8 addiu $t6, $t6, 0x1C */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] + 0x1u; /* 800763EC addiu $t1, $t1, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[9] < 3); /* 800763F0 slti $v0, $t1, 0x3 */
    /* 800763F4 bnez $v0, .L8007599C */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[13] = entry->cpu->gpr[13] + 0x8u; if (c_) goto L_8007599C; }
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x2cu, 4);
    uint32_t saved_s2 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 4);
    entry->cpu->gpr[31] = saved_ra;
    uint32_t saved_s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x24u, 4);
    entry->cpu->gpr[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x20u, 4);
    entry->cpu->gpr[17] = saved_s1;
    entry->cpu->gpr[29] += 0x30u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80076410 jr $ra */
    ; goto bl_exit;
bl_exit:
}
/* Experimental gauge phase; its refresh service remains separately guarded. */
static int battle_gauge_try_host(BattleMipsRuntime *runtime,
                                 PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80075938u || cpu->gpr[31] != 0x80076444u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_GAUGE_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "f6ff5d87184bda619f277bddbddd45bfd4e12f6ac303e4bea9d694407d1b298b";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0xae0u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_gauge_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

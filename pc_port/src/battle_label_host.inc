static void battle_label_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFC8u; /* 80072A9C addiu $sp, $sp, -0x38 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[19]); /* 80072AA0 sw $s3, 0x1C($sp) */
    entry->cpu->gpr[19] = entry->cpu->gpr[4] + 0u; /* 80072AA4 addu $s3, $a0, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x2Cu, 4, entry->cpu->gpr[23]); /* 80072AA8 sw $s7, 0x2C($sp) */
    entry->cpu->gpr[23] = entry->cpu->gpr[5] + 0u; /* 80072AAC addu $s7, $a1, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[18]); /* 80072AB0 sw $s2, 0x18($sp) */
    entry->cpu->gpr[18] = 0u + 0u; /* 80072AB4 addu $s2, $zero, $zero */
    entry->cpu->gpr[2] = entry->cpu->gpr[19] << 1; /* 80072AB8 sll $v0, $s3, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[19]; /* 80072ABC addu $v0, $v0, $s3 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[17]); /* 80072AC0 sw $s1, 0x14($sp) */
    entry->cpu->gpr[17] = entry->cpu->gpr[2] << 3; /* 80072AC4 sll $s1, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + entry->cpu->gpr[19]; /* 80072AC8 addu $v0, $s1, $s3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 7; /* 80072ACC sll $v0, $v0, 7 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[20]); /* 80072AD0 sw $s4, 0x20($sp) */
    entry->cpu->gpr[20] = entry->cpu->gpr[2] + 0x3A88u; /* 80072AD4 addiu $s4, $v0, 0x3A88 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80072AD8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80072ADC lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[19] << 2; /* 80072AE0 sll $v1, $s3, 2 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[16]); /* 80072AE4 sw $s0, 0x10($sp) */
    entry->cpu->gpr[16] = entry->cpu->gpr[3] + 0u; /* 80072AE8 addu $s0, $v1, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x30u, 4, entry->cpu->gpr[31]); /* 80072AEC sw $ra, 0x30($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x28u, 4, entry->cpu->gpr[22]); /* 80072AF0 sw $s6, 0x28($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x24u, 4, entry->cpu->gpr[21]); /* 80072AF4 sw $s5, 0x24($sp) */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] + entry->cpu->gpr[2]; /* 80072AF8 addu $v0, $s0, $v0 */
    entry->cpu->gpr[22] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xE0u, 4); /* 80072AFC lw $s6, 0xE0($v0) */
L_80072B00:
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072B00 lui $at, %hi(D_800C3E08) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[18]; /* 80072B04 addu $at, $at, $s2 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3E08u, 1); /* 80072B08 lbu $a0, %lo(D_800C3E08)($at) */
    entry->cpu->gpr[2] = 0u | 0xFFu; /* 80072B0C ori $v0, $zero, 0xFF */
    /* 80072B10 beq $a0, $v0, .L80072BB0 */
    { int c_ = (entry->cpu->gpr[4] == entry->cpu->gpr[2]); entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x67u; if (c_) goto L_80072BB0; }
    entry->cpu->gpr[7] = 0u | 0x10u; /* 80072B18 ori $a3, $zero, 0x10 */
    entry->cpu->gpr[6] = entry->cpu->gpr[17] + entry->cpu->gpr[18]; /* 80072B1C addu $a2, $s1, $s2 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 1; /* 80072B20 sll $a2, $a2, 1 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80072B24 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80072B28 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[8] = 0x800C0000u; /* 80072B2C lui $t0, %hi(D_800C3EA4) */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[8] + 0x3EA4u, 4); /* 80072B30 lw $t0, %lo(D_800C3EA4)($t0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80072B34 lui $v1, %hi(D_800D3280) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3280u, 1); /* 80072B38 lbu $v1, %lo(D_800D3280)($v1) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072B3C lui $at, %hi(D_800C3068) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[6]; /* 80072B40 addu $at, $at, $a2 */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3068u, 2); /* 80072B44 lhu $a2, %lo(D_800C3068)($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] + entry->cpu->gpr[2]; /* 80072B48 addu $v0, $s0, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xE0u, 4); /* 80072B4C lw $v0, 0xE0($v0) */
    entry->cpu->gpr[8] = entry->cpu->gpr[20] + entry->cpu->gpr[8]; /* 80072B50 addu $t0, $s4, $t0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 2; /* 80072B54 sll $a1, $v0, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80072B58 addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 4; /* 80072B5C sll $a1, $a1, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80072B60 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80072B64 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[19]; /* 80072B68 addu $v0, $v0, $s3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80072B6C sll $v0, $v0, 1 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072B70 lui $at, %hi(D_800C3254) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072B74 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3254u, 2); /* 80072B78 lhu $v0, %lo(D_800C3254)($at) */
    entry->cpu->gpr[5] = entry->cpu->gpr[8] + entry->cpu->gpr[5]; /* 80072B7C addu $a1, $t0, $a1 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80072B80 addu $a2, $a2, $v0 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 16; /* 80072B84 sll $a2, $a2, 16 */
    /* 80072B88 jal func_80076A10 */
    entry->cpu->gpr[6] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 16));
    battle_phase_call(entry, 0x80076A10u, 0x80072b88u);
    entry->cpu->gpr[4] = 0x800D0000u; /* 80072B90 lui $a0, %hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 80072B94 lw $a0, %lo(D_800D2D28)($a0) */
    ; /* 80072B98 nop  */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + entry->cpu->gpr[4]; /* 80072B9C addu $a0, $s0, $a0 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0xE0u, 4); /* 80072BA0 lw $v1, 0xE0($a0) */
    ; /* 80072BA4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80072BA8 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0xE0u, 4, entry->cpu->gpr[2]); /* 80072BAC sw $v0, 0xE0($a0) */
L_80072BB0:
    entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x1u; /* 80072BB0 addiu $s2, $s2, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[18] < 3); /* 80072BB4 slti $v0, $s2, 0x3 */
    /* 80072BB8 bnez $v0, .L80072B00 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[4] = 0u | 0x71u; if (c_) goto L_80072B00; }
    entry->cpu->gpr[7] = 0u | 0x10u; /* 80072BC0 ori $a3, $zero, 0x10 */
    entry->cpu->gpr[6] = entry->cpu->gpr[19] << 1; /* 80072BC4 sll $a2, $s3, 1 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[19]; /* 80072BC8 addu $a2, $a2, $s3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] << 3; /* 80072BCC sll $v0, $a2, 3 */
    entry->cpu->gpr[21] = entry->cpu->gpr[2] + 0u; /* 80072BD0 addu $s5, $v0, $zero */
    entry->cpu->gpr[16] = entry->cpu->gpr[21] + entry->cpu->gpr[19]; /* 80072BD4 addu $s0, $s5, $s3 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] << 7; /* 80072BD8 sll $s0, $s0, 7 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x3A88u; /* 80072BDC addiu $s0, $s0, 0x3A88 */
    entry->cpu->gpr[17] = entry->cpu->gpr[19] << 2; /* 80072BE0 sll $s1, $s3, 2 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 4; /* 80072BE4 sll $a2, $a2, 4 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80072BE8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80072BEC lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[8] = 0x800C0000u; /* 80072BF0 lui $t0, %hi(D_800C3EA4) */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[8] + 0x3EA4u, 4); /* 80072BF4 lw $t0, %lo(D_800C3EA4)($t0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80072BF8 lui $v1, %hi(D_800D3280) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3280u, 1); /* 80072BFC lbu $v1, %lo(D_800D3280)($v1) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072C00 lui $at, %hi(D_800C306E) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[6]; /* 80072C04 addu $at, $at, $a2 */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x306Eu, 2); /* 80072C08 lhu $a2, %lo(D_800C306E)($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + entry->cpu->gpr[2]; /* 80072C0C addu $v0, $s1, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xE0u, 4); /* 80072C10 lw $v0, 0xE0($v0) */
    entry->cpu->gpr[8] = entry->cpu->gpr[16] + entry->cpu->gpr[8]; /* 80072C14 addu $t0, $s0, $t0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 2; /* 80072C18 sll $a1, $v0, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80072C1C addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 4; /* 80072C20 sll $a1, $a1, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80072C24 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80072C28 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[19]; /* 80072C2C addu $v0, $v0, $s3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80072C30 sll $v0, $v0, 1 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072C34 lui $at, %hi(D_800C3254) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072C38 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3254u, 2); /* 80072C3C lhu $v0, %lo(D_800C3254)($at) */
    entry->cpu->gpr[5] = entry->cpu->gpr[8] + entry->cpu->gpr[5]; /* 80072C40 addu $a1, $t0, $a1 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80072C44 addu $a2, $a2, $v0 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 16; /* 80072C48 sll $a2, $a2, 16 */
    /* 80072C4C jal func_80076A10 */
    entry->cpu->gpr[6] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 16));
    battle_phase_call(entry, 0x80076A10u, 0x80072c4cu);
    entry->cpu->gpr[3] = 0x800D0000u; /* 80072C54 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80072C58 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[18] = 0u | 0x4u; /* 80072C5C ori $s2, $zero, 0x4 */
    entry->cpu->gpr[3] = entry->cpu->gpr[17] + entry->cpu->gpr[3]; /* 80072C60 addu $v1, $s1, $v1 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xE0u, 4); /* 80072C64 lw $a0, 0xE0($v1) */
    entry->cpu->gpr[20] = 0u | 0x4u; /* 80072C68 ori $s4, $zero, 0x4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80072C6C addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0xE0u, 4, entry->cpu->gpr[2]); /* 80072C70 sw $v0, 0xE0($v1) */
L_80072C74:
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072C74 lui $at, %hi(D_800D2D54) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[18]; /* 80072C78 addu $at, $at, $s2 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x2D54u, 1); /* 80072C7C lbu $a0, %lo(D_800D2D54)($at) */
    entry->cpu->gpr[2] = 0u | 0xFFu; /* 80072C80 ori $v0, $zero, 0xFF */
    /* 80072C84 beq $a0, $v0, .L80072D28 */
    { int c_ = (entry->cpu->gpr[4] == entry->cpu->gpr[2]); entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x1u; if (c_) goto L_80072D28; }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x67u; /* 80072C8C addiu $a0, $a0, 0x67 */
    entry->cpu->gpr[7] = 0u | 0x10u; /* 80072C90 ori $a3, $zero, 0x10 */
    entry->cpu->gpr[6] = entry->cpu->gpr[21] + entry->cpu->gpr[20]; /* 80072C94 addu $a2, $s5, $s4 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 1; /* 80072C98 sll $a2, $a2, 1 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80072C9C lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80072CA0 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[8] = 0x800C0000u; /* 80072CA4 lui $t0, %hi(D_800C3EA4) */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[8] + 0x3EA4u, 4); /* 80072CA8 lw $t0, %lo(D_800C3EA4)($t0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80072CAC lui $v1, %hi(D_800D3280) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3280u, 1); /* 80072CB0 lbu $v1, %lo(D_800D3280)($v1) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072CB4 lui $at, %hi(D_800C3068) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[6]; /* 80072CB8 addu $at, $at, $a2 */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3068u, 2); /* 80072CBC lhu $a2, %lo(D_800C3068)($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + entry->cpu->gpr[2]; /* 80072CC0 addu $v0, $s1, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xE0u, 4); /* 80072CC4 lw $v0, 0xE0($v0) */
    entry->cpu->gpr[8] = entry->cpu->gpr[16] + entry->cpu->gpr[8]; /* 80072CC8 addu $t0, $s0, $t0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 2; /* 80072CCC sll $a1, $v0, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80072CD0 addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 4; /* 80072CD4 sll $a1, $a1, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80072CD8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80072CDC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[19]; /* 80072CE0 addu $v0, $v0, $s3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80072CE4 sll $v0, $v0, 1 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80072CE8 lui $at, %hi(D_800C3254) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072CEC addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3254u, 2); /* 80072CF0 lhu $v0, %lo(D_800C3254)($at) */
    entry->cpu->gpr[5] = entry->cpu->gpr[8] + entry->cpu->gpr[5]; /* 80072CF4 addu $a1, $t0, $a1 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80072CF8 addu $a2, $a2, $v0 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 16; /* 80072CFC sll $a2, $a2, 16 */
    /* 80072D00 jal func_80076A10 */
    entry->cpu->gpr[6] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 16));
    battle_phase_call(entry, 0x80076A10u, 0x80072d00u);
    entry->cpu->gpr[3] = 0x800D0000u; /* 80072D08 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80072D0C lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 80072D10 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[17] + entry->cpu->gpr[3]; /* 80072D14 addu $v1, $s1, $v1 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xE0u, 4); /* 80072D18 lw $a0, 0xE0($v1) */
    entry->cpu->gpr[20] = entry->cpu->gpr[20] + 0x1u; /* 80072D1C addiu $s4, $s4, 0x1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80072D20 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0xE0u, 4, entry->cpu->gpr[2]); /* 80072D24 sw $v0, 0xE0($v1) */
L_80072D28:
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[18] < 7); /* 80072D28 slti $v0, $s2, 0x7 */
    /* 80072D2C bnez $v0, .L80072C74 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[7] = entry->cpu->gpr[23] & 0xFFu; if (c_) goto L_80072C74; }
    /* 80072D34 beqz $a3, .L80072D78 */
    { int c_ = (entry->cpu->gpr[7] == 0); entry->cpu->gpr[5] = entry->cpu->gpr[22] + 0u; if (c_) goto L_80072D78; }
    entry->cpu->gpr[2] = entry->cpu->gpr[19] << 2; /* 80072D3C sll $v0, $s3, 2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[19] << 1; /* 80072D40 sll $a0, $s3, 1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[19]; /* 80072D44 addu $a0, $a0, $s3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 3; /* 80072D48 sll $a0, $a0, 3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[19]; /* 80072D4C addu $a0, $a0, $s3 */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80072D50 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80072D54 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 7; /* 80072D58 sll $a0, $a0, 7 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80072D5C addu $v0, $v0, $v1 */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xE0u, 4); /* 80072D60 lw $a2, 0xE0($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80072D64 lui $v0, %hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80072D68 lw $v0, %lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x3A88u; /* 80072D6C addiu $a0, $a0, 0x3A88 */
    /* 80072D70 jal func_80072938 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[2];
    battle_phase_call(entry, 0x80072938u, 0x80072d70u);
L_80072D78:
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x30u, 4);
    uint32_t saved_s7 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x2cu, 4);
    entry->cpu->gpr[31] = saved_ra;
    uint32_t saved_s6 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 4);
    entry->cpu->gpr[23] = saved_s7;
    uint32_t saved_s5 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x24u, 4);
    entry->cpu->gpr[22] = saved_s6;
    uint32_t saved_s4 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x20u, 4);
    entry->cpu->gpr[21] = saved_s5;
    uint32_t saved_s3 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x1cu, 4);
    entry->cpu->gpr[20] = saved_s4;
    uint32_t saved_s2 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x18u, 4);
    entry->cpu->gpr[19] = saved_s3;
    uint32_t saved_s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x14u, 4);
    entry->cpu->gpr[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x10u, 4);
    entry->cpu->gpr[17] = saved_s1;
    entry->cpu->gpr[29] += 0x38u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80072DA0 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental label entry. Its outgoing services remain guarded separately. */
static int battle_label_try_host(BattleMipsRuntime *runtime,
                                 PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80072a9cu || cpu->gpr[31] != 0x8007434cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_LABEL_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "4bc431d665f54ddf8a41826d6161b3501a72cbe048544b1f10285f8c8e6b51d4";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x30cu, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_label_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

static int battle_label_unconverted(uint32_t target, uint32_t ra)
{
    return (target == 0x80076a10u &&
            (ra == 0x80072b90u || ra == 0x80072c54u || ra == 0x80072d08u)) ||
           (target == 0x80072938u && ra == 0x80072d78u);
}

static void battle_hud_submit_run(BattleEntryCall *entry) {
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073B64 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80073B68 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFD0u; /* 80073B6C addiu $sp, $sp, -0x30 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x2Cu, 4, entry->cpu->gpr[31]); /* 80073B70 sw $ra, 0x2C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x28u, 4, entry->cpu->gpr[20]); /* 80073B74 sw $s4, 0x28($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x24u, 4, entry->cpu->gpr[19]); /* 80073B78 sw $s3, 0x24($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[18]); /* 80073B7C sw $s2, 0x20($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[17]); /* 80073B80 sw $s1, 0x1C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[16]); /* 80073B84 sw $s0, 0x18($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x97u, 1); /* 80073B88 lbu $v0, 0x97($v1) */
    ; /* 80073B8C nop  */
    /* 80073B90 blez $v0, .L80073BE8 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] <= 0); entry->cpu->gpr[16] = 0u + 0u; if (c_) goto L_80073BE8; }
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 1; /* 80073B98 sll $v0, $s0, 1 */
L_80073B9C:
    entry->cpu->gpr[4] = 0x800D0000u; /* 80073B9C lui $a0, %hi(D_800CCB04) */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0xFFFFCB04u, 4); /* 80073BA0 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[3] + 0x98u, 1); /* 80073BA4 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[5] = pending_second;
    }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x4u; /* 80073BA8 addiu $a0, $a0, 0x4 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80073BAC addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 4; /* 80073BB0 sll $a1, $a1, 4 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80073BB4 lui $v0, %hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80073BB8 lw $v0, %lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x908u; /* 80073BBC addiu $a1, $a1, 0x908 */
    /* 80073BC0 jal AddPrim */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2];
    battle_phase_call(entry, 0x80043B48u, 0x80073bc0u);
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073BC8 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80073BCC lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 80073BD0 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x97u, 1); /* 80073BD4 lbu $v0, 0x97($v1) */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u; /* 80073BD8 addiu $s0, $s0, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < (int32_t)entry->cpu->gpr[2]); /* 80073BDC slt $v0, $s0, $v0 */
    /* 80073BE0 bnez $v0, .L80073B9C */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[16] << 1; if (c_) goto L_80073B9C; }
L_80073BE8:
    entry->cpu->gpr[16] = 0u + 0u; /* 80073BE8 addu $s0, $zero, $zero */
    entry->cpu->gpr[17] = 0u | 0x2E08u; /* 80073BEC ori $s1, $zero, 0x2E08 */
L_80073BF0:
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073BF0 lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073BF4 lw $a0, %lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80073BF8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80073BFC lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + entry->cpu->gpr[4]; /* 80073C00 addu $a0, $s1, $a0 */
    entry->cpu->gpr[17] = entry->cpu->gpr[17] + 0x320u; /* 80073C04 addiu $s1, $s1, 0x320 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073C08 addu $v0, $v0, $s0 */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0x74u, 1); /* 80073C0C */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[2] + 0x84u, 1); /* 80073C10 */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073C14 jal func_800728B8 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u;
    battle_phase_call(entry, 0x800728B8u, 0x80073c14u);
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < 4); /* 80073C1C slti $v0, $s0, 0x4 */
    /* 80073C20 bnez $v0, .L80073BF0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[19] = 0u | 0x835Cu; if (c_) goto L_80073BF0; }
    entry->cpu->gpr[16] = 0u + 0u; /* 80073C28 addu $s0, $zero, $zero */
    entry->cpu->gpr[18] = 0u | 0x6008u; /* 80073C2C ori $s2, $zero, 0x6008 */
    entry->cpu->gpr[17] = 0u + 0u; /* 80073C30 addu $s1, $zero, $zero */
L_80073C34:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80073C34 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80073C38 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80073C3C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073C40 addu $v0, $v0, $s0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xCCu, 1); /* 80073C44 lbu $v0, 0xCC($v0) */
    ; /* 80073C48 nop  */
    /* 80073C4C beqz $v0, .L80073D08 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = 0u | 0x1u; if (c_) goto L_80073D08; }
    entry->cpu->gpr[7] = 0x800C0000u; /* 80073C54 lui $a3, %hi(D_800C3EA4) */
    entry->cpu->gpr[7] = battle_entry_read(entry, entry->cpu->gpr[7] + 0x3EA4u, 4); /* 80073C58 lw $a3, %lo(D_800C3EA4)($a3) */
    ; /* 80073C5C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[7] + entry->cpu->gpr[17]; /* 80073C60 addu $v1, $a3, $s1 */
    entry->cpu->gpr[1] = 0x10000u; /* 80073C64 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80073C68 addu $at, $v1, $at */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Du, 1); /* 80073C6C lbu $a0, -0x7AC3($at) */
    ; /* 80073C70 nop  */
    /* 80073C74 beq $a0, $v0, .L80073C8C */
    { int c_ = (entry->cpu->gpr[4] == entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0u | 0x2u; if (c_) goto L_80073C8C; }
    /* 80073C7C beq $a0, $v0, .L80073CB0 */
    { int c_ = (entry->cpu->gpr[4] == entry->cpu->gpr[2]); ; if (c_) goto L_80073CB0; }
    /* 80073C84 j .L80073CE0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[16] << 2; goto L_80073CE0;
L_80073C8C:
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + entry->cpu->gpr[19]; /* 80073C8C addu $a0, $s1, $s3 */
    entry->cpu->gpr[1] = 0x10000u; /* 80073C90 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80073C94 addu $at, $v1, $at */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Eu, 1); /* 80073C98 lbu $a1, -0x7AC2($at) */
    entry->cpu->gpr[1] = 0x10000u; /* 80073C9C lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80073CA0 addu $at, $v1, $at */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Cu, 1); /* 80073CA4 lbu $a2, -0x7AC4($at) */
    /* 80073CA8 j .L80073CD4 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[7]; goto L_80073CD4;
L_80073CB0:
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + entry->cpu->gpr[19]; /* 80073CB0 addu $a0, $s1, $s3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[7]; /* 80073CB4 addu $a0, $a0, $a3 */
    entry->cpu->gpr[1] = 0x10000u; /* 80073CB8 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80073CBC addu $at, $v1, $at */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Fu, 1); /* 80073CC0 lbu $a1, -0x7AC1($at) */
    entry->cpu->gpr[1] = 0x10000u; /* 80073CC4 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 80073CC8 addu $at, $v1, $at */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Cu, 1); /* 80073CCC lbu $a2, -0x7AC4($at) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0xA0u; /* 80073CD0 addiu $a0, $a0, 0xA0 */
L_80073CD4:
    /* 80073CD4 jal func_800728B8 */
    ;
    battle_phase_call(entry, 0x800728B8u, 0x80073cd4u);
    entry->cpu->gpr[3] = entry->cpu->gpr[16] << 2; /* 80073CDC sll $v1, $s0, 2 */
L_80073CE0:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80073CE0 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80073CE4 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073CE8 lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073CEC lw $a0, %lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80073CF0 addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073CF4 addu $v0, $v0, $s0 */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[3] + 0xECu, 4); /* 80073CF8 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[2] + 0x99u, 1); /* 80073CFC */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073D00 jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[18] + entry->cpu->gpr[4];
    battle_phase_call(entry, 0x800728B8u, 0x80073d00u);
L_80073D08:
    entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x140u; /* 80073D08 addiu $s2, $s2, 0x140 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u; /* 80073D0C addiu $s0, $s0, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < 3); /* 80073D10 slti $v0, $s0, 0x3 */
    /* 80073D14 bnez $v0, .L80073C34 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[17] = entry->cpu->gpr[17] + 0x1E4u; if (c_) goto L_80073C34; }
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073D1C lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80073D20 lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 80073D24 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xAFu, 1); /* 80073D28 lbu $v0, 0xAF($v1) */
    ; /* 80073D2C nop  */
    /* 80073D30 beqz $v0, .L80073D50 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[16] = 0u + 0u; if (c_) goto L_80073D50; }
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x7Bu, 1); /* 80073D38 lbu $a1, 0x7B($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073D3C lui $a0, %hi(D_800C3EA4) */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073D40 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[3] + 0xA4u, 1); /* 80073D44 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073D48 jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x9C8u;
    battle_phase_call(entry, 0x800728B8u, 0x80073d48u);
L_80073D50:
    entry->cpu->gpr[17] = 0u | 0x3A88u; /* 80073D50 ori $s1, $zero, 0x3A88 */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 2; /* 80073D54 sll $v0, $s0, 2 */
L_80073D58:
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073D58 lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073D5C lw $a0, %lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073D60 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80073D64 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + entry->cpu->gpr[4]; /* 80073D68 addu $a0, $s1, $a0 */
    entry->cpu->gpr[17] = entry->cpu->gpr[17] + 0xC80u; /* 80073D6C addiu $s1, $s1, 0xC80 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073D70 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[16]; /* 80073D74 addu $v1, $v1, $s0 */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0xE0u, 4); /* 80073D78 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[3] + 0x93u, 1); /* 80073D7C */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073D80 jal func_800728B8 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u;
    battle_phase_call(entry, 0x800728B8u, 0x80073d80u);
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < 3); /* 80073D88 slti $v0, $s0, 0x3 */
    /* 80073D8C bnez $v0, .L80073D58 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[16] << 2; if (c_) goto L_80073D58; }
    entry->cpu->gpr[16] = 0u + 0u; /* 80073D94 addu $s0, $zero, $zero */
    entry->cpu->gpr[20] = 0x800D0000u; /* 80073D98 lui $s4, %hi(D_800CCB04) */
    entry->cpu->gpr[20] = entry->cpu->gpr[20] + 0xFFFFCB04u; /* 80073D9C addiu $s4, $s4, %lo(D_800CCB04) */
    entry->cpu->gpr[19] = 0u + 0u; /* 80073DA0 addu $s3, $zero, $zero */
    entry->cpu->gpr[18] = 0u | 0x818u; /* 80073DA4 ori $s2, $zero, 0x818 */
    entry->cpu->gpr[17] = 0u + 0u; /* 80073DA8 addu $s1, $zero, $zero */
L_80073DAC:
    entry->cpu->gpr[1] = 0x800C0000u; /* 80073DAC lui $at, %hi(D_800C3EB6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[17]; /* 80073DB0 addu $at, $at, $s1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3EB6u, 1); /* 80073DB4 lbu $v1, %lo(D_800C3EB6)($at) */
    entry->cpu->gpr[2] = 0u | 0x7Fu; /* 80073DB8 ori $v0, $zero, 0x7F */
    /* 80073DBC beq $v1, $v0, .L80073E50 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[17] = entry->cpu->gpr[17] + 0x1Cu; if (c_) goto L_80073E50; }
    entry->cpu->gpr[5] = 0u | 0x1u; /* 80073DC4 ori $a1, $zero, 0x1 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80073DC8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80073DCC lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073DD0 lui $a0, %hi(D_800C3EA4) */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073DD4 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[2] + 0x83u, 1); /* 80073DD8 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073DDC jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[18] + entry->cpu->gpr[4];
    battle_phase_call(entry, 0x800728B8u, 0x80073ddcu);
    entry->cpu->gpr[6] = 0x800D0000u; /* 80073DE4 lui $a2, %hi(D_800D2D28) */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x2D28u, 4); /* 80073DE8 lw $a2, %lo(D_800D2D28)($a2) */
    ; /* 80073DEC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[16]; /* 80073DF0 addu $v1, $a2, $s0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xCCu, 1); /* 80073DF4 lbu $v0, 0xCC($v1) */
    ; /* 80073DF8 nop  */
    /* 80073DFC beqz $v0, .L80073E50 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_80073E50; }
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x78u, 1); /* 80073E04 lbu $a1, 0x78($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073E08 lui $a0, %hi(D_800C3EA4) */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073E0C */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[6] + 0xA2u, 1); /* 80073E10 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073E14 jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[19] + entry->cpu->gpr[4];
    battle_phase_call(entry, 0x800728B8u, 0x80073e14u);
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 1; /* 80073E1C sll $v0, $s0, 1 */
    { /* Preserve the first delayed load if the second read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[20], 4); /* 80073E20 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[20] + 0x30u, 4); /* 80073E24 */
        entry->cpu->gpr[4] = pending_first;
        entry->cpu->gpr[3] = pending_second;
    }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x4u; /* 80073E28 addiu $a0, $a0, 0x4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073E2C addu $v0, $v0, $v1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 3; /* 80073E30 sll $a1, $v0, 3 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80073E34 addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 2; /* 80073E38 sll $a1, $a1, 2 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80073E3C lui $v0, %hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80073E40 lw $v0, %lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x740u; /* 80073E44 addiu $a1, $a1, 0x740 */
    /* 80073E48 jal AddPrim */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2];
    battle_phase_call(entry, 0x80043B48u, 0x80073e48u);
L_80073E50:
    entry->cpu->gpr[19] = entry->cpu->gpr[19] + 0x1E0u; /* 80073E50 addiu $s3, $s3, 0x1E0 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u; /* 80073E54 addiu $s0, $s0, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < 3); /* 80073E58 slti $v0, $s0, 0x3 */
    /* 80073E5C bnez $v0, .L80073DAC */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x50u; if (c_) goto L_80073DAC; }
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x2cu, 4);
    uint32_t saved_s4 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 4);
    entry->cpu->gpr[31] = saved_ra;
    uint32_t saved_s3 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x24u, 4);
    entry->cpu->gpr[20] = saved_s4;
    uint32_t saved_s2 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x20u, 4);
    entry->cpu->gpr[19] = saved_s3;
    uint32_t saved_s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x1cu, 4);
    entry->cpu->gpr[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x18u, 4);
    entry->cpu->gpr[17] = saved_s1;
    entry->cpu->gpr[29] += 0x30u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80073E80 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental HUD submission; child services remain separately guarded. */
static int battle_hud_submit_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80073b64u || cpu->gpr[31] != 0x8007645cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_HUD_SUBMIT_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "54b51a9eb4b5e59b24f8c747872c07b3a337cad5a285af54bcee2041b7cf5611";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x324u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_hud_submit_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

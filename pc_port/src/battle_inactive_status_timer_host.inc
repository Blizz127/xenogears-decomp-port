/* Experimental status-timer body; the entry gate accepts inactive records only. */
static void battle_status_timer_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFE8u; /* 80099890 addiu $sp, $sp, -0x18 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] & 0xFFu; /* 80099894 andi $a0, $a0, 0xFF */
    entry->cpu->gpr[2] = entry->cpu->gpr[4] << 1; /* 80099898 sll $v0, $a0, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 8009989C addu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 800998A0 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[4]; /* 800998A4 subu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 800998A8 sll $v0, $v0, 4 */
    entry->cpu->gpr[4] = 0x800D0000u; /* 800998AC lui $a0, %hi(D_800CCCE8) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0xFFFFCCE8u; /* 800998B0 addiu $a0, $a0, %lo(D_800CCCE8) */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 800998B4 addu $a1, $v0, $a0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[4] + 0xA4u; /* 800998B8 addiu $v1, $a0, 0xA4 */
    entry->cpu->gpr[7] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800998BC addu $a3, $v0, $v1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x15Cu; /* 800998C0 addiu $a0, $a0, 0x15C */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[31]); /* 800998C4 sw $ra, 0x10($sp) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800998C8 lui $at, %hi(D_800CCE42) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800998CC addu $at, $at, $v0 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCE42u, 1); /* 800998D0 lbu $v1, %lo(D_800CCE42)($at) */
    ; /* 800998D4 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & 0x80u; /* 800998D8 andi $v1, $v1, 0x80 */
    /* 800998DC beqz $v1, .L800998F4 */
    { int c_ = (entry->cpu->gpr[3] == 0); entry->cpu->gpr[6] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; if (c_) goto L_800998F4; }
    /* 800998E4 jal func_80099CF0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[7] + 0u;
    battle_entry_fail(entry, "special status timer unsupported", 0x80099cf0u); /* func_80099CF0 */
    /* 800998EC j .L80099CE0 */
    entry->cpu->gpr[2] = 0u + 0u; goto L_80099CE0;
L_800998F4:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x7Cu, 2); /* 800998F4 lhu $v0, 0x7C($a1) */
    ; /* 800998F8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x1000u; /* 800998FC andi $v0, $v0, 0x1000 */
    /* 80099900 beqz $v0, .L80099938 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[4] = 0u + 0u; if (c_) goto L_80099938; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x1u, 1); /* 80099908 lbu $v0, 0x1($a2) */
    ; /* 8009990C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099910 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x1u, 1, entry->cpu->gpr[2]); /* 80099914 sb $v0, 0x1($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x1u, 1); /* 80099918 lbu $v0, 0x1($a2) */
    ; /* 8009991C nop  */
    /* 80099920 bnez $v0, .L80099938 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80099938; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x7Cu, 2); /* 80099928 lhu $v0, 0x7C($a1) */
    entry->cpu->gpr[4] = 0u | 0x4000u; /* 8009992C ori $a0, $zero, 0x4000 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xEFFFu; /* 80099930 andi $v0, $v0, 0xEFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x7Cu, 2, entry->cpu->gpr[2]); /* 80099934 sh $v0, 0x7C($a1) */
L_80099938:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x80u, 2); /* 80099938 lhu $v0, 0x80($a1) */
    ; /* 8009993C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x1000u; /* 80099940 andi $v0, $v0, 0x1000 */
    /* 80099944 beqz $v0, .L8009997C */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_8009997C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x2u, 1); /* 8009994C lbu $v0, 0x2($a2) */
    ; /* 80099950 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099954 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x2u, 1, entry->cpu->gpr[2]); /* 80099958 sb $v0, 0x2($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x2u, 1); /* 8009995C lbu $v0, 0x2($a2) */
    ; /* 80099960 nop  */
    /* 80099964 bnez $v0, .L8009997C */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_8009997C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x80u, 2); /* 8009996C lhu $v0, 0x80($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x2000u; /* 80099970 ori $a0, $a0, 0x2000 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xEFFFu; /* 80099974 andi $v0, $v0, 0xEFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x80u, 2, entry->cpu->gpr[2]); /* 80099978 sh $v0, 0x80($a1) */
L_8009997C:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x80u, 2); /* 8009997C lhu $v0, 0x80($a1) */
    ; /* 80099980 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x800u; /* 80099984 andi $v0, $v0, 0x800 */
    /* 80099988 beqz $v0, .L800999D0 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = 0x80000000u; if (c_) goto L_800999D0; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x3u, 1); /* 80099990 lbu $v0, 0x3($a2) */
    ; /* 80099994 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099998 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x3u, 1, entry->cpu->gpr[2]); /* 8009999C sb $v0, 0x3($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x3u, 1); /* 800999A0 lbu $v0, 0x3($a2) */
    ; /* 800999A4 nop  */
    /* 800999A8 bnez $v0, .L800999D0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x80000000u; if (c_) goto L_800999D0; }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x1000u; /* 800999B0 ori $a0, $a0, 0x1000 */
    uint32_t delayed_v0 = battle_entry_read(entry, entry->cpu->gpr[5] + 0x80u, 2);
    uint32_t delayed_v1 = battle_entry_read(entry, entry->cpu->gpr[5] + 0x7au, 2);
    entry->cpu->gpr[2] = delayed_v0;
    entry->cpu->gpr[3] = delayed_v1;
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF7FFu; /* 800999BC andi $v0, $v0, 0xF7FF */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & 0xFFDFu; /* 800999C0 andi $v1, $v1, 0xFFDF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x80u, 2, entry->cpu->gpr[2]); /* 800999C4 sh $v0, 0x80($a1) */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x7Au, 2, entry->cpu->gpr[3]); /* 800999C8 sh $v1, 0x7A($a1) */
    entry->cpu->gpr[2] = 0x80000000u; /* 800999CC lui $v0, (0x80008000 >> 16) */
L_800999D0:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 4); /* 800999D0 lw $v1, 0x84($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x8000u; /* 800999D4 ori $v0, $v0, (0x80008000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 800999D8 and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x8000u; /* 800999DC ori $v0, $zero, 0x8000 */
    /* 800999E0 bne $v1, $v0, .L80099A1C */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x40000000u; if (c_) goto L_80099A1C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x4u, 1); /* 800999E8 lbu $v0, 0x4($a2) */
    ; /* 800999EC nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 800999F0 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x4u, 1, entry->cpu->gpr[2]); /* 800999F4 sb $v0, 0x4($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x4u, 1); /* 800999F8 lbu $v0, 0x4($a2) */
    ; /* 800999FC nop  */
    /* 80099A00 bnez $v0, .L80099A1C */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x40000000u; if (c_) goto L_80099A1C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 2); /* 80099A08 lhu $v0, 0x84($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x800u; /* 80099A0C ori $a0, $a0, 0x800 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x7FFFu; /* 80099A10 andi $v0, $v0, 0x7FFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x84u, 2, entry->cpu->gpr[2]); /* 80099A14 sh $v0, 0x84($a1) */
    entry->cpu->gpr[2] = 0x40000000u; /* 80099A18 lui $v0, (0x40004000 >> 16) */
L_80099A1C:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 4); /* 80099A1C lw $v1, 0x84($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x4000u; /* 80099A20 ori $v0, $v0, (0x40004000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099A24 and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x4000u; /* 80099A28 ori $v0, $zero, 0x4000 */
    /* 80099A2C bne $v1, $v0, .L80099A68 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x20000000u; if (c_) goto L_80099A68; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x5u, 1); /* 80099A34 lbu $v0, 0x5($a2) */
    ; /* 80099A38 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099A3C addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x5u, 1, entry->cpu->gpr[2]); /* 80099A40 sb $v0, 0x5($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x5u, 1); /* 80099A44 lbu $v0, 0x5($a2) */
    ; /* 80099A48 nop  */
    /* 80099A4C bnez $v0, .L80099A68 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x20000000u; if (c_) goto L_80099A68; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 2); /* 80099A54 lhu $v0, 0x84($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x400u; /* 80099A58 ori $a0, $a0, 0x400 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xBFFFu; /* 80099A5C andi $v0, $v0, 0xBFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x84u, 2, entry->cpu->gpr[2]); /* 80099A60 sh $v0, 0x84($a1) */
    entry->cpu->gpr[2] = 0x20000000u; /* 80099A64 lui $v0, (0x20002000 >> 16) */
L_80099A68:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 4); /* 80099A68 lw $v1, 0x84($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x2000u; /* 80099A6C ori $v0, $v0, (0x20002000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099A70 and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x2000u; /* 80099A74 ori $v0, $zero, 0x2000 */
    /* 80099A78 bne $v1, $v0, .L80099AB4 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x10000000u; if (c_) goto L_80099AB4; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x6u, 1); /* 80099A80 lbu $v0, 0x6($a2) */
    ; /* 80099A84 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099A88 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x6u, 1, entry->cpu->gpr[2]); /* 80099A8C sb $v0, 0x6($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x6u, 1); /* 80099A90 lbu $v0, 0x6($a2) */
    ; /* 80099A94 nop  */
    /* 80099A98 bnez $v0, .L80099AB4 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x10000000u; if (c_) goto L_80099AB4; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 2); /* 80099AA0 lhu $v0, 0x84($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x200u; /* 80099AA4 ori $a0, $a0, 0x200 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xDFFFu; /* 80099AA8 andi $v0, $v0, 0xDFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x84u, 2, entry->cpu->gpr[2]); /* 80099AAC sh $v0, 0x84($a1) */
    entry->cpu->gpr[2] = 0x10000000u; /* 80099AB0 lui $v0, (0x10001000 >> 16) */
L_80099AB4:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 4); /* 80099AB4 lw $v1, 0x84($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x1000u; /* 80099AB8 ori $v0, $v0, (0x10001000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099ABC and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x1000u; /* 80099AC0 ori $v0, $zero, 0x1000 */
    /* 80099AC4 bne $v1, $v0, .L80099B00 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x8000000u; if (c_) goto L_80099B00; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x7u, 1); /* 80099ACC lbu $v0, 0x7($a2) */
    ; /* 80099AD0 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099AD4 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x7u, 1, entry->cpu->gpr[2]); /* 80099AD8 sb $v0, 0x7($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x7u, 1); /* 80099ADC lbu $v0, 0x7($a2) */
    ; /* 80099AE0 nop  */
    /* 80099AE4 bnez $v0, .L80099B00 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x8000000u; if (c_) goto L_80099B00; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 2); /* 80099AEC lhu $v0, 0x84($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x100u; /* 80099AF0 ori $a0, $a0, 0x100 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xEFFFu; /* 80099AF4 andi $v0, $v0, 0xEFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x84u, 2, entry->cpu->gpr[2]); /* 80099AF8 sh $v0, 0x84($a1) */
    entry->cpu->gpr[2] = 0x8000000u; /* 80099AFC lui $v0, (0x8000800 >> 16) */
L_80099B00:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 4); /* 80099B00 lw $v1, 0x84($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x800u; /* 80099B04 ori $v0, $v0, (0x8000800 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099B08 and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x800u; /* 80099B0C ori $v0, $zero, 0x800 */
    /* 80099B10 bne $v1, $v0, .L80099B4C */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x80000000u; if (c_) goto L_80099B4C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x8u, 1); /* 80099B18 lbu $v0, 0x8($a2) */
    ; /* 80099B1C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099B20 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x8u, 1, entry->cpu->gpr[2]); /* 80099B24 sb $v0, 0x8($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x8u, 1); /* 80099B28 lbu $v0, 0x8($a2) */
    ; /* 80099B2C nop  */
    /* 80099B30 bnez $v0, .L80099B4C */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x80000000u; if (c_) goto L_80099B4C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x84u, 2); /* 80099B38 lhu $v0, 0x84($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x80u; /* 80099B3C ori $a0, $a0, 0x80 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF7FFu; /* 80099B40 andi $v0, $v0, 0xF7FF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x84u, 2, entry->cpu->gpr[2]); /* 80099B44 sh $v0, 0x84($a1) */
    entry->cpu->gpr[2] = 0x80000000u; /* 80099B48 lui $v0, (0x80008000 >> 16) */
L_80099B4C:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x88u, 4); /* 80099B4C lw $v1, 0x88($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x8000u; /* 80099B50 ori $v0, $v0, (0x80008000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099B54 and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x8000u; /* 80099B58 ori $v0, $zero, 0x8000 */
    /* 80099B5C bne $v1, $v0, .L80099B98 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x40000000u; if (c_) goto L_80099B98; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x9u, 1); /* 80099B64 lbu $v0, 0x9($a2) */
    ; /* 80099B68 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099B6C addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x9u, 1, entry->cpu->gpr[2]); /* 80099B70 sb $v0, 0x9($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x9u, 1); /* 80099B74 lbu $v0, 0x9($a2) */
    ; /* 80099B78 nop  */
    /* 80099B7C bnez $v0, .L80099B98 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x40000000u; if (c_) goto L_80099B98; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x88u, 2); /* 80099B84 lhu $v0, 0x88($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x40u; /* 80099B88 ori $a0, $a0, 0x40 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x7FFFu; /* 80099B8C andi $v0, $v0, 0x7FFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x88u, 2, entry->cpu->gpr[2]); /* 80099B90 sh $v0, 0x88($a1) */
    entry->cpu->gpr[2] = 0x40000000u; /* 80099B94 lui $v0, (0x40004000 >> 16) */
L_80099B98:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x88u, 4); /* 80099B98 lw $v1, 0x88($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x4000u; /* 80099B9C ori $v0, $v0, (0x40004000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099BA0 and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x4000u; /* 80099BA4 ori $v0, $zero, 0x4000 */
    /* 80099BA8 bne $v1, $v0, .L80099BE4 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0x10000000u; if (c_) goto L_80099BE4; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xAu, 1); /* 80099BB0 lbu $v0, 0xA($a2) */
    ; /* 80099BB4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099BB8 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0xAu, 1, entry->cpu->gpr[2]); /* 80099BBC sb $v0, 0xA($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xAu, 1); /* 80099BC0 lbu $v0, 0xA($a2) */
    ; /* 80099BC4 nop  */
    /* 80099BC8 bnez $v0, .L80099BE4 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0x10000000u; if (c_) goto L_80099BE4; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x88u, 2); /* 80099BD0 lhu $v0, 0x88($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x20u; /* 80099BD4 ori $a0, $a0, 0x20 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xBFFFu; /* 80099BD8 andi $v0, $v0, 0xBFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x88u, 2, entry->cpu->gpr[2]); /* 80099BDC sh $v0, 0x88($a1) */
    entry->cpu->gpr[2] = 0x10000000u; /* 80099BE0 lui $v0, (0x10001000 >> 16) */
L_80099BE4:
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x88u, 4); /* 80099BE4 lw $v1, 0x88($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x1000u; /* 80099BE8 ori $v0, $v0, (0x10001000 & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80099BEC and $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0u | 0x1000u; /* 80099BF0 ori $v0, $zero, 0x1000 */
    /* 80099BF4 bne $v1, $v0, .L80099C2C */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); ; if (c_) goto L_80099C2C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xBu, 1); /* 80099BFC lbu $v0, 0xB($a2) */
    ; /* 80099C00 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099C04 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0xBu, 1, entry->cpu->gpr[2]); /* 80099C08 sb $v0, 0xB($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xBu, 1); /* 80099C0C lbu $v0, 0xB($a2) */
    ; /* 80099C10 nop  */
    /* 80099C14 bnez $v0, .L80099C2C */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80099C2C; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x88u, 2); /* 80099C1C lhu $v0, 0x88($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x10u; /* 80099C20 ori $a0, $a0, 0x10 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xEFFFu; /* 80099C24 andi $v0, $v0, 0xEFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x88u, 2, entry->cpu->gpr[2]); /* 80099C28 sh $v0, 0x88($a1) */
L_80099C2C:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x8Cu, 2); /* 80099C2C lhu $v0, 0x8C($a1) */
    ; /* 80099C30 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF000u; /* 80099C34 andi $v0, $v0, 0xF000 */
    /* 80099C38 beqz $v0, .L80099C84 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_80099C84; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x8Eu, 2); /* 80099C40 lhu $v0, 0x8E($a1) */
    ; /* 80099C44 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF000u; /* 80099C48 andi $v0, $v0, 0xF000 */
    /* 80099C4C bnez $v0, .L80099C84 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80099C84; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xCu, 1); /* 80099C54 lbu $v0, 0xC($a2) */
    ; /* 80099C58 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099C5C addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0xCu, 1, entry->cpu->gpr[2]); /* 80099C60 sb $v0, 0xC($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xCu, 1); /* 80099C64 lbu $v0, 0xC($a2) */
    ; /* 80099C68 nop  */
    /* 80099C6C bnez $v0, .L80099C84 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80099C84; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x8Cu, 2); /* 80099C74 lhu $v0, 0x8C($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x8u; /* 80099C78 ori $a0, $a0, 0x8 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xFFFu; /* 80099C7C andi $v0, $v0, 0xFFF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x8Cu, 2, entry->cpu->gpr[2]); /* 80099C80 sh $v0, 0x8C($a1) */
L_80099C84:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x8Cu, 2); /* 80099C84 lhu $v0, 0x8C($a1) */
    ; /* 80099C88 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF00u; /* 80099C8C andi $v0, $v0, 0xF00 */
    /* 80099C90 beqz $v0, .L80099CE0 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] & 0xFFFFu; if (c_) goto L_80099CE0; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x8Eu, 2); /* 80099C98 lhu $v0, 0x8E($a1) */
    ; /* 80099C9C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF00u; /* 80099CA0 andi $v0, $v0, 0xF00 */
    /* 80099CA4 bnez $v0, .L80099CE0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] & 0xFFFFu; if (c_) goto L_80099CE0; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xDu, 1); /* 80099CAC lbu $v0, 0xD($a2) */
    ; /* 80099CB0 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFu; /* 80099CB4 addiu $v0, $v0, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0xDu, 1, entry->cpu->gpr[2]); /* 80099CB8 sb $v0, 0xD($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xDu, 1); /* 80099CBC lbu $v0, 0xD($a2) */
    ; /* 80099CC0 nop  */
    /* 80099CC4 bnez $v0, .L80099CE0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] & 0xFFFFu; if (c_) goto L_80099CE0; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x8Cu, 2); /* 80099CCC lhu $v0, 0x8C($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x4u; /* 80099CD0 ori $a0, $a0, 0x4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0xF0FFu; /* 80099CD4 andi $v0, $v0, 0xF0FF */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x8Cu, 2, entry->cpu->gpr[2]); /* 80099CD8 sh $v0, 0x8C($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[4] & 0xFFFFu; /* 80099CDC andi $v0, $a0, 0xFFFF */
L_80099CE0:
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x10u, 4);
    entry->cpu->gpr[29] += 0x18u;
    entry->cpu->gpr[31] = saved_ra;
    /* 80099CE8 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* A lifted native JAL uses BATTLE_HALT_PC, not the original guest call site.
 * Limit this experiment to inactive actor records and refuse the special child. */
static int battle_inactive_status_timer_try_host(BattleMipsRuntime *runtime,
                                                PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80099890u || cpu->gpr[31] != BATTLE_HALT_PC ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_INACTIVE_STATUS_TIMER_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "2a39cc5316131c415d72dde5d8e3a652656ae136f6abfae5e53a41a66e16104e";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x460u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    uint32_t actor = cpu->gpr[4] & 0xffu;
    if (actor >= 11u) return 0;
    uint32_t base = 0x800ccce8u + actor * 0x170u, value;
    static const uint32_t offsets[] = {0x7cu, 0x80u, 0x84u, 0x88u, 0x8cu};
    if (runtime_read(runtime, base + 0x15au, 1, &value) != 0 || (value & 0x80u)) return 0;
    for (unsigned i = 0; i < sizeof(offsets)/sizeof(offsets[0]); ++i)
        if (runtime_read(runtime, base + offsets[i], 2, &value) != 0 || value != 0) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_status_timer_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

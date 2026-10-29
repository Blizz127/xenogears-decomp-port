/* Experimental frame-sprite caller; rendering service edges are guarded. */
static void battle_frame_sprite_run(BattleEntryCall *entry) {
    entry->cpu->gpr[6] = 0x800D0000u; /* 80076710 lui $a2, %hi(D_800D2D28) */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x2D28u, 4); /* 80076714 lw $a2, %lo(D_800D2D28)($a2) */
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFA8u; /* 80076718 addiu $sp, $sp, -0x58 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x4Cu, 4, entry->cpu->gpr[17]); /* 8007671C sw $s1, 0x4C($sp) */
    entry->cpu->gpr[17] = entry->cpu->gpr[4] + 0u; /* 80076720 addu $s1, $a0, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x50u, 4, entry->cpu->gpr[31]); /* 80076724 sw $ra, 0x50($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x48u, 4, entry->cpu->gpr[16]); /* 80076728 sw $s0, 0x48($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xA9u, 1); /* 8007672C lbu $v0, 0xA9($a2) */
    ; /* 80076730 nop  */
    /* 80076734 blez $v0, .L80076798 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] <= 0); entry->cpu->gpr[7] = 0u + 0u; if (c_) goto L_80076798; }
L_8007673C:
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[6] + 0x64u, 4); /* 8007673C */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[6] + 0x54u, 4); /* 80076740 */
        entry->cpu->gpr[2]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[6] + 0x6Cu, 4); /* 80076744 */
        entry->cpu->gpr[4]=delayed_1;
        uint32_t delayed_3=battle_entry_read(entry, entry->cpu->gpr[6] + 0x5Cu, 4); /* 80076748 */
        entry->cpu->gpr[3]=delayed_2;
        entry->cpu->gpr[5]=delayed_3;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[4]; /* 8007674C subu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x64u, 4, entry->cpu->gpr[2]); /* 80076750 sw $v0, 0x64($a2) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x64u, 4); /* 80076754 lw $v0, 0x64($a2) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[5]; /* 80076758 addu $v1, $v1, $a1 */
    battle_entry_write(entry, entry->cpu->gpr[6] + 0x6Cu, 4, entry->cpu->gpr[3]); /* 8007675C sw $v1, 0x6C($a2) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[6] + 0x34u, 2); /* 80076760 lhu $v1, 0x34($a2) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] >> 8; /* 80076764 srl $v0, $v0, 8 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80076768 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x28u, 2, entry->cpu->gpr[2]); /* 8007676C sh $v0, 0x28($sp) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[6] + 0x6Cu, 4); /* 80076770 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[6] + 0x44u, 2); /* 80076774 */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[3]=delayed_1;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] >> 8; /* 80076778 srl $v0, $v0, 8 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007677C addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x30u, 2, entry->cpu->gpr[2]); /* 80076780 sh $v0, 0x30($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[6] + 0xA9u, 1); /* 80076784 lbu $v0, 0xA9($a2) */
    entry->cpu->gpr[7] = entry->cpu->gpr[7] + 0x1u; /* 80076788 addiu $a3, $a3, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[7] < (int32_t)entry->cpu->gpr[2]); /* 8007678C slt $v0, $a3, $v0 */
    /* 80076790 bnez $v0, .L8007673C */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_8007673C; }
L_80076798:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80076798 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 8007679C lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 800767A0 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xA9u, 1, 0u); /* 800767A4 sb $zero, 0xA9($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800767A8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800767AC lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[16] = 0x800D0000u; /* 800767B0 lui $s0, %hi(D_800CCB34) */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0xFFFFCB34u; /* 800767B4 addiu $s0, $s0, %lo(D_800CCB34) */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x77u, 1, 0u); /* 800767B8 sb $zero, 0x77($v0) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 2); /* 800767BC */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[16], 4); /* 800767C0 */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[7]=delayed_1;
    }
    entry->cpu->gpr[3] = 0x800D0000u; /* 800767C4 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 800767C8 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 800767CC lui $a0, %hi(D_800D2F5C) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2F5Cu, 4); /* 800767D0 lw $a0, %lo(D_800D2F5C)($a0) */
    entry->cpu->gpr[6] = 0x800C0000u; /* 800767D4 lui $a2, %hi(D_800C3EA4) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[6] + 0x3EA4u, 4); /* 800767D8 */
        battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[2]); /* 800767DC sw $v0, 0x10($sp) */
        entry->cpu->gpr[6]=delayed_0;
    }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[29] + 0x30u, 2); /* 800767E0 lhu $v0, 0x30($sp) */
    ; /* 800767E4 nop  */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[2]); /* 800767E8 sw $v0, 0x14($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x104u, 2); /* 800767EC lhu $v0, 0x104($v1) */
    ; /* 800767F0 nop  */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[2]); /* 800767F4 sw $v0, 0x18($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x104u, 2); /* 800767F8 lhu $v0, 0x104($v1) */
    entry->cpu->gpr[5] = 0u | 0x52u; /* 800767FC ori $a1, $zero, 0x52 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[2]); /* 80076800 sw $v0, 0x1C($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x106u, 2); /* 80076804 lhu $v0, 0x106($v1) */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + 0x3768u; /* 80076808 addiu $a2, $a2, 0x3768 */
    /* 8007680C jal func_80025FA8 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[2]);
    battle_phase_call(entry, 0x80025fa8u, 0x8007680cu); /* func_80025FA8 */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076814 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80076818 lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 8007681C nop  */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x77u, 1, entry->cpu->gpr[2]); /* 80076820 sb $v0, 0x77($v1) */
    entry->cpu->gpr[7] = battle_entry_read(entry, entry->cpu->gpr[16], 4); /* 80076824 lw $a3, 0x0($s0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80076828 lui $v1, %hi(D_800D2D28) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 8007682C */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 2); /* 80076830 */
        entry->cpu->gpr[3]=delayed_0;
        entry->cpu->gpr[2]=delayed_1;
    }
    entry->cpu->gpr[4] = 0x800D0000u; /* 80076834 lui $a0, %hi(D_800D2F5C) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[4] + 0x2F5Cu, 4); /* 80076838 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[3] + 0x77u, 1); /* 8007683C */
        entry->cpu->gpr[4]=delayed_0;
        entry->cpu->gpr[16]=delayed_1;
    }
    entry->cpu->gpr[5] = 0u | 0x53u; /* 80076840 ori $a1, $zero, 0x53 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[2]); /* 80076844 sw $v0, 0x10($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[29] + 0x30u, 2); /* 80076848 lhu $v0, 0x30($sp) */
    entry->cpu->gpr[6] = entry->cpu->gpr[16] << 2; /* 8007684C sll $a2, $s0, 2 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[16]; /* 80076850 addu $a2, $a2, $s0 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 4; /* 80076854 sll $a2, $a2, 4 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[2]); /* 80076858 sw $v0, 0x14($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x104u, 2); /* 8007685C lhu $v0, 0x104($v1) */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + 0x3768u; /* 80076860 addiu $a2, $a2, 0x3768 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[2]); /* 80076864 sw $v0, 0x18($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x104u, 2); /* 80076868 lhu $v0, 0x104($v1) */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] << 1; /* 8007686C sll $s0, $s0, 1 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[2]); /* 80076870 sw $v0, 0x1C($sp) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80076874 lui $v0, %hi(D_800C3EA4) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 80076878 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[3] + 0x106u, 2); /* 8007687C */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[3]=delayed_1;
    }
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80076880 addu $a2, $a2, $v0 */
    /* 80076884 jal func_80025FA8 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[3]);
    battle_phase_call(entry, 0x80025fa8u, 0x80076884u); /* func_80025FA8 */
    entry->cpu->gpr[4] = 0x800D0000u; /* 8007688C lui $a0, %hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 80076890 lw $a0, %lo(D_800D2D28)($a0) */
    ; /* 80076894 nop  */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x77u, 1); /* 80076898 lbu $v1, 0x77($a0) */
    ; /* 8007689C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800768A0 addu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x77u, 1, entry->cpu->gpr[3]); /* 800768A4 sb $v1, 0x77($a0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800768A8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800768AC lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 800768B0 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x77u, 1); /* 800768B4 lbu $v0, 0x77($v0) */
    /* 800768B8 j .L80076908 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; goto L_80076908;
L_800768C0:
    entry->cpu->gpr[2] = 0x800D0000u; /* 800768C0 lui $v0, %hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 800768C4 lw $v0, %lo(D_800CCB34)($v0) */
    ; /* 800768C8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] + entry->cpu->gpr[2]; /* 800768CC addu $v0, $s0, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[2] << 2; /* 800768D0 sll $a0, $v0, 2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[2]; /* 800768D4 addu $a0, $a0, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 3; /* 800768D8 sll $a0, $a0, 3 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800768DC lui $v0, %hi(D_800C3EA4) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EA4u, 4); /* 800768E0 lw $v0, %lo(D_800C3EA4)($v0) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x3768u; /* 800768E4 addiu $a0, $a0, 0x3768 */
    /* 800768E8 jal SetSemiTrans */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[2];
    battle_phase_call(entry, 0x80043bfcu, 0x800768e8u); /* SetSemiTrans */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800768F0 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800768F4 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 800768F8 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x77u, 1); /* 800768FC lbu $v0, 0x77($v0) */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x2u; /* 80076900 addiu $s0, $s0, 0x2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80076904 sll $v0, $v0, 1 */
L_80076908:
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < (int32_t)entry->cpu->gpr[2]); /* 80076908 slt $v0, $s0, $v0 */
    /* 8007690C bnez $v0, .L800768C0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[5] = 0u | 0x1u; if (c_) goto L_800768C0; }
    entry->cpu->gpr[2] = 0x800D0000u; /* 80076914 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80076918 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007691C lui $v1, %hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 1); /* 80076920 lbu $v1, %lo(D_800CCB34)($v1) */
    ; /* 80076924 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x87u, 1, entry->cpu->gpr[3]); /* 80076928 sb $v1, 0x87($v0) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 8007692C lui $a0, %hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 80076930 lw $a0, %lo(D_800D2D28)($a0) */
    ; /* 80076934 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[4] + 0xABu, 1); /* 80076938 lbu $v0, 0xAB($a0) */
    ; /* 8007693C nop  */
    /* 80076940 blez $v0, .L80076970 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] <= 0); entry->cpu->gpr[7] = 0u + 0u; if (c_) goto L_80076970; }
L_80076948:
    entry->cpu->gpr[7] = entry->cpu->gpr[7] + 0x1u; /* 80076948 addiu $a3, $a3, 0x1 */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[4] + 0x104u, 2); /* 8007694C */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[4] + 0xABu, 1); /* 80076950 */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[3]=delayed_1;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x66u; /* 80076954 addiu $v0, $v0, 0x66 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x104u, 2, entry->cpu->gpr[2]); /* 80076958 sh $v0, 0x104($a0) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x106u, 2); /* 8007695C lhu $v0, 0x106($a0) */
    entry->cpu->gpr[3] = (uint32_t)((int32_t)entry->cpu->gpr[7] < (int32_t)entry->cpu->gpr[3]); /* 80076960 slt $v1, $a3, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x80u; /* 80076964 addiu $v0, $v0, 0x80 */
    /* 80076968 bnez $v1, .L80076948 */
    { int c_ = (entry->cpu->gpr[3] != 0); battle_entry_write(entry, entry->cpu->gpr[4] + 0x106u, 2, entry->cpu->gpr[2]); if (c_) goto L_80076948; }
L_80076970:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80076970 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80076974 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80076978 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xABu, 1, 0u); /* 8007697C sb $zero, 0xAB($v0) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 80076980 lui $a0, %hi(D_800D2D28) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 80076984 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 2); /* 80076988 */
        entry->cpu->gpr[4]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[4] + 0x3Cu, 4); /* 8007698C */
        entry->cpu->gpr[3]=delayed_1;
        entry->cpu->gpr[2]=delayed_2;
    }
    ; /* 80076990 nop  */
    entry->cpu->gpr[2] = (uint32_t)(entry->cpu->gpr[2] < entry->cpu->gpr[3]); /* 80076994 sltu $v0, $v0, $v1 */
    /* 80076998 bnez $v0, .L800769B0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[3] = entry->cpu->gpr[4] + entry->cpu->gpr[17]; if (c_) goto L_800769B0; }
    entry->cpu->gpr[2] = 0u | 0x1u; /* 800769A0 ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x90u, 1, entry->cpu->gpr[2]); /* 800769A4 sb $v0, 0x90($v1) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 800769A8 lui $a0, %hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 800769AC lw $a0, %lo(D_800D2D28)($a0) */
L_800769B0:
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[29] + 0x30u, 2); /* 800769B0 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[4] + 0x4Cu, 4); /* 800769B4 */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[3]=delayed_1;
    }
    ; /* 800769B8 nop  */
    entry->cpu->gpr[2] = (uint32_t)(entry->cpu->gpr[2] < entry->cpu->gpr[3]); /* 800769BC sltu $v0, $v0, $v1 */
    /* 800769C0 bnez $v0, .L800769D0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[3] = entry->cpu->gpr[4] + entry->cpu->gpr[17]; if (c_) goto L_800769D0; }
    entry->cpu->gpr[2] = 0u | 0x1u; /* 800769C8 ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x90u, 1, entry->cpu->gpr[2]); /* 800769CC sb $v0, 0x90($v1) */
L_800769D0:
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[29] + 0x50u, 4); /* 800769D0 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[29] + 0x4Cu, 4); /* 800769D4 */
        entry->cpu->gpr[31]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[29] + 0x48u, 4); /* 800769D8 */
        entry->cpu->gpr[17]=delayed_1;
        entry->cpu->gpr[16]=delayed_2;
    }
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0x58u; /* 800769DC addiu $sp, $sp, 0x58 */
    /* 800769E0 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Restricted to the observed HUD frame caller. */
static int battle_frame_sprite_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if (target!=0x80076710u || cpu->gpr[31]!=0x800739ccu || cpu->gpr[4]>=11u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_FRAME_SPRITE_EXPERIMENT");
    if (!control || strcmp(control,"1")!=0) return 0;
    static const char expected[]="726f4bc38d7c68fa8dc46cb2a2a0e5011e5adb3fa00f69a9cafdcdcf671974a7";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),0x2d8u,digest);
    for (unsigned i=0;i<sizeof(digest);++i)
        if (file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
            file1_hex_digit(digest[i]&15u)!=expected[2*i+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_frame_sprite_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

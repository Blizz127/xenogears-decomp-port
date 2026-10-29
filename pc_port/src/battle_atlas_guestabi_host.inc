/* Experimental atlas register/guest-stack body; SDK edges remain guarded. */
static uint32_t battle_atlas_read_s16(BattleEntryCall *entry,uint32_t addr){return (uint32_t)(int32_t)(int16_t)battle_entry_read(entry,addr,2);}
static void battle_atlas_render_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFF60u; /* 80025FA8 addiu $sp, $sp, -0xA0 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x8Cu, 4, entry->cpu->gpr[21]); /* 80025FAC sw $s5, 0x8C($sp) */
    entry->cpu->gpr[21] = entry->cpu->gpr[4] + 0u; /* 80025FB0 addu $s5, $a0, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x84u, 4, entry->cpu->gpr[19]); /* 80025FB4 sw $s3, 0x84($sp) */
    entry->cpu->gpr[19] = entry->cpu->gpr[5] + 0u; /* 80025FB8 addu $s3, $a1, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x98u, 4, entry->cpu->gpr[30]); /* 80025FBC sw $fp, 0x98($sp) */
    entry->cpu->gpr[30] = entry->cpu->gpr[7] + 0u; /* 80025FC0 addu $fp, $a3, $zero */
    entry->cpu->gpr[2] = 0x80020000u; /* 80025FC4 lui $v0, %hi(D_800188CC) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x9Cu, 4, entry->cpu->gpr[31]); /* 80025FC8 sw $ra, 0x9C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x94u, 4, entry->cpu->gpr[23]); /* 80025FCC sw $s7, 0x94($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x90u, 4, entry->cpu->gpr[22]); /* 80025FD0 sw $s6, 0x90($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x88u, 4, entry->cpu->gpr[20]); /* 80025FD4 sw $s4, 0x88($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x80u, 4, entry->cpu->gpr[18]); /* 80025FD8 sw $s2, 0x80($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x7Cu, 4, entry->cpu->gpr[17]); /* 80025FDC sw $s1, 0x7C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x78u, 4, entry->cpu->gpr[16]); /* 80025FE0 sw $s0, 0x78($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0xA8u, 4, entry->cpu->gpr[6]); /* 80025FE4 sw $a2, 0xA8($sp) */
    entry->cpu->gpr[6] = entry->cpu->gpr[2] + 0xFFFF88CCu; /* 80025FE8 addiu $a2, $v0, %lo(D_800188CC) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[6], 4); /* 80025FEC */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[6] + 0x4u, 4); /* 80025FF0 */
        entry->cpu->gpr[3]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[6] + 0x8u, 4); /* 80025FF4 */
        entry->cpu->gpr[4]=delayed_1;
        battle_entry_write(entry, entry->cpu->gpr[29] + 0x28u, 4, entry->cpu->gpr[3]); /* 80025FF8 sw $v1, 0x28($sp) */
        entry->cpu->gpr[5]=delayed_2;
    }
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x2Cu, 4, entry->cpu->gpr[4]); /* 80025FFC sw $a0, 0x2C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x30u, 4, entry->cpu->gpr[5]); /* 80026000 sw $a1, 0x30($sp) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[6] + 0xCu, 4); /* 80026004 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[6] + 0x10u, 4); /* 80026008 */
        entry->cpu->gpr[3]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[6] + 0x14u, 4); /* 8002600C */
        entry->cpu->gpr[4]=delayed_1;
        battle_entry_write(entry, entry->cpu->gpr[29] + 0x34u, 4, entry->cpu->gpr[3]); /* 80026010 sw $v1, 0x34($sp) */
        entry->cpu->gpr[5]=delayed_2;
    }
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x38u, 4, entry->cpu->gpr[4]); /* 80026014 sw $a0, 0x38($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x3Cu, 4, entry->cpu->gpr[5]); /* 80026018 sw $a1, 0x3C($sp) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[6] + 0x18u, 4); /* 8002601C */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[6] + 0x1Cu, 4); /* 80026020 */
        entry->cpu->gpr[3]=delayed_0;
        battle_entry_write(entry, entry->cpu->gpr[29] + 0x40u, 4, entry->cpu->gpr[3]); /* 80026024 sw $v1, 0x40($sp) */
        entry->cpu->gpr[4]=delayed_1;
    }
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x44u, 4, entry->cpu->gpr[4]); /* 80026028 sw $a0, 0x44($sp) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[29] + 0xB0u, 4); /* 8002602C */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[29] + 0xB4u, 4); /* 80026030 */
        entry->cpu->gpr[17]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[29] + 0xB8u, 4); /* 80026034 */
        entry->cpu->gpr[18]=delayed_1;
        uint32_t delayed_3=battle_entry_read(entry, entry->cpu->gpr[29] + 0xBCu, 4); /* 80026038 */
        entry->cpu->gpr[3]=delayed_2;
        uint32_t delayed_4=battle_entry_read(entry, entry->cpu->gpr[29] + 0xC0u, 4); /* 8002603C */
        entry->cpu->gpr[4]=delayed_3;
        entry->cpu->gpr[20]=delayed_4;
    }
    entry->cpu->gpr[2] = 0u + 0x1000u; /* 80026040 addiu $v0, $zero, 0x1000 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x50u, 4, entry->cpu->gpr[2]); /* 80026044 sw $v0, 0x50($sp) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 16; /* 80026048 sll $v1, $v1, 16 */
    entry->cpu->gpr[3] = (uint32_t)((int32_t)entry->cpu->gpr[3] >> 16); /* 8002604C sra $v1, $v1, 16 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 16; /* 80026050 sll $a0, $a0, 16 */
    entry->cpu->gpr[4] = (uint32_t)((int32_t)entry->cpu->gpr[4] >> 16); /* 80026054 sra $a0, $a0, 16 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x48u, 4, entry->cpu->gpr[3]); /* 80026058 sw $v1, 0x48($sp) */
    /* 8002605C jal PushMatrix */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x4Cu, 4, entry->cpu->gpr[4]);
    battle_phase_call(entry, 0x8004960cu, 0x8002605cu); /* PushMatrix */
    entry->cpu->gpr[16] = entry->cpu->gpr[29] + 0x28u; /* 80026064 addiu $s0, $sp, 0x28 */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u; /* 80026068 addu $a0, $s0, $zero */
    /* 8002606C jal ScaleMatrixL */
    entry->cpu->gpr[5] = entry->cpu->gpr[29] + 0x48u;
    battle_phase_call(entry, 0x8004974cu, 0x8002606cu); /* ScaleMatrixL */
    entry->cpu->gpr[4] = entry->cpu->gpr[20] << 16; /* 80026074 sll $a0, $s4, 16 */
    entry->cpu->gpr[4] = (uint32_t)((int32_t)entry->cpu->gpr[4] >> 16); /* 80026078 sra $a0, $a0, 16 */
    /* 8002607C jal RotMatrixZ */
    entry->cpu->gpr[5] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x8004b18cu, 0x8002607cu); /* RotMatrixZ */
    entry->cpu->gpr[4] = entry->cpu->gpr[29] + 0x58u; /* 80026084 addiu $a0, $sp, 0x58 */
    /* 80026088 jal ReadGeomOffset */
    entry->cpu->gpr[5] = entry->cpu->gpr[29] + 0x5Cu;
    battle_phase_call(entry, 0x8004a0bcu, 0x80026088u); /* ReadGeomOffset */
    /* 80026090 jal ReadGeomScreen */
    entry->cpu->gpr[17] = entry->cpu->gpr[17] << 16;
    battle_phase_call(entry, 0x8004a0dcu, 0x80026090u); /* ReadGeomScreen */
    entry->cpu->gpr[4] = (uint32_t)((int32_t)entry->cpu->gpr[17] >> 16); /* 80026098 sra $a0, $s1, 16 */
    entry->cpu->gpr[18] = entry->cpu->gpr[18] << 16; /* 8002609C sll $s2, $s2, 16 */
    entry->cpu->gpr[5] = (uint32_t)((int32_t)entry->cpu->gpr[18] >> 16); /* 800260A0 sra $a1, $s2, 16 */
    /* 800260A4 jal SetGeomOffset */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x70u, 4, entry->cpu->gpr[2]);
    battle_phase_call(entry, 0x8004a12cu, 0x800260a4u); /* SetGeomOffset */
    /* 800260AC jal SetGeomScreen */
    entry->cpu->gpr[4] = 0u + 0x1000u;
    battle_phase_call(entry, 0x8004a14cu, 0x800260acu); /* SetGeomScreen */
    /* 800260B4 jal SetRotMatrix */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x80049efcu, 0x800260b4u); /* SetRotMatrix */
    /* 800260BC jal SetTransMatrix */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x80049f8cu, 0x800260bcu); /* SetTransMatrix */
    entry->cpu->gpr[19] = entry->cpu->gpr[19] << 1; /* 800260C4 sll $s3, $s3, 1 */
    entry->cpu->gpr[19] = entry->cpu->gpr[19] + entry->cpu->gpr[21]; /* 800260C8 addu $s3, $s3, $s5 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[19] + 0x4u, 2); /* 800260CC lhu $v0, 0x4($s3) */
    ; /* 800260D0 nop  */
    entry->cpu->gpr[23] = entry->cpu->gpr[2] + entry->cpu->gpr[21]; /* 800260D4 addu $s7, $v0, $s5 */
    entry->cpu->gpr[2] = battle_atlas_read_s16(entry, entry->cpu->gpr[23]); /* 800260D8 lh $v0, 0x0($s7) */
    entry->cpu->gpr[22] = 0u + 0u; /* 800260DC addu $s6, $zero, $zero */
    /* 800260E0 beqz $v0, .L800262DC */
    { int c_ = (entry->cpu->gpr[2] == 0); battle_entry_write(entry, entry->cpu->gpr[29] + 0x68u, 2, entry->cpu->gpr[20]); if (c_) goto L_800262DC; }
    entry->cpu->gpr[20] = 0x80050000u; /* 800260E8 lui $s4, %hi(D_8004FDC0) */
    entry->cpu->gpr[18] = entry->cpu->gpr[20] + 0xFFFFFDC0u; /* 800260EC addiu $s2, $s4, %lo(D_8004FDC0) */
    entry->cpu->gpr[21] = 0u + 0x4u; /* 800260F0 addiu $s5, $zero, 0x4 */
    entry->cpu->gpr[19] = battle_entry_read(entry, entry->cpu->gpr[29] + 0xA8u, 4); /* 800260F4 lw $s3, 0xA8($sp) */
L_800260F8:
    entry->cpu->gpr[2] = entry->cpu->gpr[30] << 2; /* 800260F8 sll $v0, $fp, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[30]; /* 800260FC addu $v0, $v0, $fp */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80026100 sll $v0, $v0, 3 */
    entry->cpu->gpr[17] = entry->cpu->gpr[19] + entry->cpu->gpr[2]; /* 80026104 addu $s1, $s3, $v0 */
    /* 80026108 jal SetPolyFT4 */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + 0u;
    battle_phase_call(entry, 0x80043cb0u, 0x80026108u); /* SetPolyFT4 */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + 0u; /* 80026110 addu $a0, $s1, $zero */
    /* 80026114 jal SetSemiTrans */
    entry->cpu->gpr[5] = 0u + 0u;
    battle_phase_call(entry, 0x80043bfcu, 0x80026114u); /* SetSemiTrans */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + 0u; /* 8002611C addu $a0, $s1, $zero */
    /* 80026120 jal SetShadeTex */
    entry->cpu->gpr[5] = 0u + 0x1u;
    battle_phase_call(entry, 0x80043c24u, 0x80026120u); /* SetShadeTex */
    entry->cpu->gpr[16] = entry->cpu->gpr[23] + entry->cpu->gpr[21]; /* 80026128 addu $s0, $s7, $s5 */
    {
        uint32_t delayed_0=battle_atlas_read_s16(entry, entry->cpu->gpr[16] + 0x10u); /* 8002612C */
        uint32_t delayed_1=battle_atlas_read_s16(entry, entry->cpu->gpr[16] + 0x16u); /* 80026130 */
        entry->cpu->gpr[4]=delayed_0;
        uint32_t delayed_2=battle_atlas_read_s16(entry, entry->cpu->gpr[16] + 0x18u); /* 80026134 */
        entry->cpu->gpr[6]=delayed_1;
        entry->cpu->gpr[7]=delayed_2;
    }
    /* 80026138 jal GetTPage */
    entry->cpu->gpr[5] = 0u + 0u;
    battle_phase_call(entry, 0x80043a1cu, 0x80026138u); /* GetTPage */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x16u, 2, entry->cpu->gpr[2]); /* 80026140 sh $v0, 0x16($s1) */
    {
        uint32_t delayed_0=battle_atlas_read_s16(entry, entry->cpu->gpr[16] + 0x12u); /* 80026144 */
        uint32_t delayed_1=battle_atlas_read_s16(entry, entry->cpu->gpr[16] + 0x14u); /* 80026148 */
        entry->cpu->gpr[4]=delayed_0;
        entry->cpu->gpr[5]=delayed_1;
    }
    /* 8002614C jal GetClut */
    ;
    battle_phase_call(entry, 0x80043a58u, 0x8002614cu); /* GetClut */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0xEu, 2, entry->cpu->gpr[2]); /* 80026154 sh $v0, 0xE($s1) */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[16] + 0x4u, 2); /* 80026158 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[16] + 0x6u, 2); /* 8002615C */
        entry->cpu->gpr[6]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[16] + 0x8u, 2); /* 80026160 */
        entry->cpu->gpr[7]=delayed_1;
        uint32_t delayed_3=battle_entry_read(entry, entry->cpu->gpr[16] + 0x1Au, 1); /* 80026164 */
        entry->cpu->gpr[3]=delayed_2;
        uint32_t delayed_4=battle_entry_read(entry, entry->cpu->gpr[16] + 0xAu, 2); /* 80026168 */
        entry->cpu->gpr[2]=delayed_3;
        entry->cpu->gpr[4]=delayed_4;
    }
    /* 8002616C bnez $v0, .L80026188 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[3] + entry->cpu->gpr[6]; if (c_) goto L_80026188; }
    battle_entry_write(entry, entry->cpu->gpr[20] + 0xFFFFFDC0u, 2, entry->cpu->gpr[3]); /* 80026174 sh $v1, %lo(D_8004FDC0)($s4) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x8u, 2, entry->cpu->gpr[2]); /* 80026178 sh $v0, 0x8($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x10u, 2, entry->cpu->gpr[2]); /* 8002617C sh $v0, 0x10($s2) */
    /* 80026180 j .L80026198 */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x18u, 2, entry->cpu->gpr[3]); goto L_80026198;
L_80026188:
    battle_entry_write(entry, entry->cpu->gpr[20] + 0xFFFFFDC0u, 2, entry->cpu->gpr[2]); /* 80026188 sh $v0, %lo(D_8004FDC0)($s4) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x8u, 2, entry->cpu->gpr[3]); /* 8002618C sh $v1, 0x8($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x10u, 2, entry->cpu->gpr[3]); /* 80026190 sh $v1, 0x10($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x18u, 2, entry->cpu->gpr[2]); /* 80026194 sh $v0, 0x18($s2) */
L_80026198:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[16] + 0x1Bu, 1); /* 80026198 lbu $v0, 0x1B($s0) */
    ; /* 8002619C nop  */
    /* 800261A0 bnez $v0, .L800261BC */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] + entry->cpu->gpr[7]; if (c_) goto L_800261BC; }
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x12u, 2, entry->cpu->gpr[2]); /* 800261A8 sh $v0, 0x12($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x1Au, 2, entry->cpu->gpr[2]); /* 800261AC sh $v0, 0x1A($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x2u, 2, entry->cpu->gpr[4]); /* 800261B0 sh $a0, 0x2($s2) */
    /* 800261B4 j .L800261CC */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0xAu, 2, entry->cpu->gpr[4]); goto L_800261CC;
L_800261BC:
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x12u, 2, entry->cpu->gpr[4]); /* 800261BC sh $a0, 0x12($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x1Au, 2, entry->cpu->gpr[4]); /* 800261C0 sh $a0, 0x1A($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0x2u, 2, entry->cpu->gpr[2]); /* 800261C4 sh $v0, 0x2($s2) */
    battle_entry_write(entry, entry->cpu->gpr[18] + 0xAu, 2, entry->cpu->gpr[2]); /* 800261C8 sh $v0, 0xA($s2) */
L_800261CC:
    entry->cpu->gpr[4] = entry->cpu->gpr[18] + 0u; /* 800261CC addu $a0, $s2, $zero */
    entry->cpu->gpr[5] = entry->cpu->gpr[18] + 0x8u; /* 800261D0 addiu $a1, $s2, 0x8 */
    entry->cpu->gpr[6] = entry->cpu->gpr[18] + 0x10u; /* 800261D4 addiu $a2, $s2, 0x10 */
    entry->cpu->gpr[7] = entry->cpu->gpr[18] + 0x18u; /* 800261D8 addiu $a3, $s2, 0x18 */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + 0x8u; /* 800261DC addiu $v0, $s1, 0x8 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[2]); /* 800261E0 sw $v0, 0x10($sp) */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + 0x10u; /* 800261E4 addiu $v0, $s1, 0x10 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[2]); /* 800261E8 sw $v0, 0x14($sp) */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + 0x20u; /* 800261EC addiu $v0, $s1, 0x20 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[2]); /* 800261F0 sw $v0, 0x18($sp) */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] + 0x18u; /* 800261F4 addiu $v0, $s1, 0x18 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[2]); /* 800261F8 sw $v0, 0x1C($sp) */
    entry->cpu->gpr[2] = entry->cpu->gpr[29] + 0x60u; /* 800261FC addiu $v0, $sp, 0x60 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[2]); /* 80026200 sw $v0, 0x20($sp) */
    entry->cpu->gpr[2] = entry->cpu->gpr[29] + 0x64u; /* 80026204 addiu $v0, $sp, 0x64 */
    /* 80026208 jal RotTransPers4 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x24u, 4, entry->cpu->gpr[2]);
    battle_phase_call(entry, 0x8004a73cu, 0x80026208u); /* RotTransPers4 */
    entry->cpu->gpr[2] = 0u + 0xC00u; /* 80026210 addiu $v0, $zero, 0xC00 */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[16], 2); /* 80026214 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[16] + 0x2u, 2); /* 80026218 */
        entry->cpu->gpr[4]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[16] + 0x4u, 2); /* 8002621C */
        entry->cpu->gpr[5]=delayed_1;
        uint32_t delayed_3=battle_entry_read(entry, entry->cpu->gpr[29] + 0x68u, 2); /* 80026220 */
        entry->cpu->gpr[6]=delayed_2;
        uint32_t delayed_4=battle_entry_read(entry, entry->cpu->gpr[16] + 0x6u, 2); /* 80026224 */
        entry->cpu->gpr[8]=delayed_3;
        entry->cpu->gpr[7]=delayed_4;
    }
    entry->cpu->gpr[3] = entry->cpu->gpr[8] & 0xFFFu; /* 80026228 andi $v1, $t0, 0xFFF */
    /* 8002622C bne $v1, $v0, .L80026238 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); ; if (c_) goto L_80026238; }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0xFFFFFFFFu; /* 80026234 addiu $a0, $a0, -0x1 */
L_80026238:
    /* 80026238 bnez $v1, .L800262A0 */
    { int c_ = (entry->cpu->gpr[3] != 0); ; if (c_) goto L_800262A0; }
    {
        uint32_t delayed_0=battle_atlas_read_s16(entry, entry->cpu->gpr[17] + 0x20u); /* 80026240 */
        uint32_t delayed_1=battle_atlas_read_s16(entry, entry->cpu->gpr[17] + 0x8u); /* 80026244 */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[3]=delayed_1;
    }
    ; /* 80026248 nop  */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[2] < (int32_t)entry->cpu->gpr[3]); /* 8002624C slt $v0, $v0, $v1 */
    /* 80026250 beqz $v0, .L80026270 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0xFFFFFFFFu; if (c_) goto L_80026270; }
    entry->cpu->gpr[4] = entry->cpu->gpr[2] + 0u; /* 80026258 addu $a0, $v0, $zero */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 16; /* 8002625C sll $v0, $v0, 16 */
    /* 80026260 bgez $v0, .L80026270 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] >= 0); ; if (c_) goto L_80026270; }
    entry->cpu->gpr[4] = 0u + 0u; /* 80026268 addu $a0, $zero, $zero */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + 0xFFFFFFFFu; /* 8002626C addiu $a2, $a2, -0x1 */
L_80026270:
    {
        uint32_t delayed_0=battle_atlas_read_s16(entry, entry->cpu->gpr[17] + 0x22u); /* 80026270 */
        uint32_t delayed_1=battle_atlas_read_s16(entry, entry->cpu->gpr[17] + 0xAu); /* 80026274 */
        entry->cpu->gpr[2]=delayed_0;
        entry->cpu->gpr[3]=delayed_1;
    }
    ; /* 80026278 nop  */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[2] < (int32_t)entry->cpu->gpr[3]); /* 8002627C slt $v0, $v0, $v1 */
    /* 80026280 beqz $v0, .L800262A0 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[5] + 0xFFFFFFFFu; if (c_) goto L_800262A0; }
    entry->cpu->gpr[5] = entry->cpu->gpr[2] + 0u; /* 80026288 addu $a1, $v0, $zero */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 16; /* 8002628C sll $v0, $v0, 16 */
    /* 80026290 bgez $v0, .L800262A0 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] >= 0); ; if (c_) goto L_800262A0; }
    entry->cpu->gpr[5] = 0u + 0u; /* 80026298 addu $a1, $zero, $zero */
    entry->cpu->gpr[7] = entry->cpu->gpr[7] + 0xFFFFFFFFu; /* 8002629C addiu $a3, $a3, -0x1 */
L_800262A0:
    entry->cpu->gpr[21] = entry->cpu->gpr[21] + 0x1Cu; /* 800262A0 addiu $s5, $s5, 0x1C */
    entry->cpu->gpr[2] = entry->cpu->gpr[4] + entry->cpu->gpr[6]; /* 800262A4 addu $v0, $a0, $a2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[5] + entry->cpu->gpr[7]; /* 800262A8 addu $v1, $a1, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0xCu, 1, entry->cpu->gpr[4]); /* 800262AC sb $a0, 0xC($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0xDu, 1, entry->cpu->gpr[5]); /* 800262B0 sb $a1, 0xD($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x14u, 1, entry->cpu->gpr[2]); /* 800262B4 sb $v0, 0x14($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x15u, 1, entry->cpu->gpr[5]); /* 800262B8 sb $a1, 0x15($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x1Cu, 1, entry->cpu->gpr[4]); /* 800262BC sb $a0, 0x1C($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x1Du, 1, entry->cpu->gpr[3]); /* 800262C0 sb $v1, 0x1D($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x24u, 1, entry->cpu->gpr[2]); /* 800262C4 sb $v0, 0x24($s1) */
    battle_entry_write(entry, entry->cpu->gpr[17] + 0x25u, 1, entry->cpu->gpr[3]); /* 800262C8 sb $v1, 0x25($s1) */
    entry->cpu->gpr[2] = battle_atlas_read_s16(entry, entry->cpu->gpr[23]); /* 800262CC lh $v0, 0x0($s7) */
    entry->cpu->gpr[22] = entry->cpu->gpr[22] + 0x1u; /* 800262D0 addiu $s6, $s6, 0x1 */
    /* 800262D4 bne $s6, $v0, .L800260F8 */
    { int c_ = (entry->cpu->gpr[22] != entry->cpu->gpr[2]); entry->cpu->gpr[19] = entry->cpu->gpr[19] + 0x50u; if (c_) goto L_800260F8; }
L_800262DC:
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[29] + 0x58u, 4); /* 800262DC */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[29] + 0x5Cu, 4); /* 800262E0 */
        entry->cpu->gpr[4]=delayed_0;
        entry->cpu->gpr[5]=delayed_1;
    }
    /* 800262E4 jal SetGeomOffset */
    ;
    battle_phase_call(entry, 0x8004a12cu, 0x800262e4u); /* SetGeomOffset */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[29] + 0x70u, 4); /* 800262EC lw $a0, 0x70($sp) */
    /* 800262F0 jal SetGeomScreen */
    ;
    battle_phase_call(entry, 0x8004a14cu, 0x800262f0u); /* SetGeomScreen */
    /* 800262F8 jal PopMatrix */
    ;
    battle_phase_call(entry, 0x800496acu, 0x800262f8u); /* PopMatrix */
    {
        uint32_t delayed_0=battle_atlas_read_s16(entry, entry->cpu->gpr[23]); /* 80026300 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[29] + 0x9Cu, 4); /* 80026304 */
        entry->cpu->gpr[2]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[29] + 0x98u, 4); /* 80026308 */
        entry->cpu->gpr[31]=delayed_1;
        uint32_t delayed_3=battle_entry_read(entry, entry->cpu->gpr[29] + 0x94u, 4); /* 8002630C */
        entry->cpu->gpr[30]=delayed_2;
        uint32_t delayed_4=battle_entry_read(entry, entry->cpu->gpr[29] + 0x90u, 4); /* 80026310 */
        entry->cpu->gpr[23]=delayed_3;
        uint32_t delayed_5=battle_entry_read(entry, entry->cpu->gpr[29] + 0x8Cu, 4); /* 80026314 */
        entry->cpu->gpr[22]=delayed_4;
        uint32_t delayed_6=battle_entry_read(entry, entry->cpu->gpr[29] + 0x88u, 4); /* 80026318 */
        entry->cpu->gpr[21]=delayed_5;
        uint32_t delayed_7=battle_entry_read(entry, entry->cpu->gpr[29] + 0x84u, 4); /* 8002631C */
        entry->cpu->gpr[20]=delayed_6;
        uint32_t delayed_8=battle_entry_read(entry, entry->cpu->gpr[29] + 0x80u, 4); /* 80026320 */
        entry->cpu->gpr[19]=delayed_7;
        uint32_t delayed_9=battle_entry_read(entry, entry->cpu->gpr[29] + 0x7Cu, 4); /* 80026324 */
        entry->cpu->gpr[18]=delayed_8;
        uint32_t delayed_10=battle_entry_read(entry, entry->cpu->gpr[29] + 0x78u, 4); /* 80026328 */
        entry->cpu->gpr[17]=delayed_9;
        entry->cpu->gpr[16]=delayed_10;
    }
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xA0u; /* 8002632C addiu $sp, $sp, 0xA0 */
    /* 80026330 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Restricted to the two frame-sprite atlas calls and the verified SLUS image. */
static int battle_atlas_render_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if (target!=0x80025fa8u ||
        (cpu->gpr[31]!=0x80076814u && cpu->gpr[31]!=0x8007688cu) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ATLAS_GUESTABI_EXPERIMENT");
    if (!control || strcmp(control,"1")!=0) return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if (!image || !image->bytes || image->size<offset+0x390u) return 0;
    static const char expected[]="a0f6f0de866dd0c2b8d4f7ae0733a3ba4b5586be3a4bb5ca2f49dd96b0fb5b78";
    uint8_t digest[32];file1_digest(image->bytes+offset,0x390u,digest);
    for (unsigned i=0;i<sizeof(digest);++i)
        if (file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
            file1_hex_digit(digest[i]&15u)!=expected[2*i+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_atlas_render_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

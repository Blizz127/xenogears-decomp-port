static void battle_hud_values_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFF0u; /* 80072F38 addiu $sp, $sp, -0x10 */
    entry->cpu->gpr[2] = entry->cpu->gpr[4] << 1; /* 80072F3C sll $v0, $a0, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80072F40 addu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80072F44 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[4]; /* 80072F48 subu $v0, $v0, $a0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80072F4C sll $v0, $v0, 4 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x8u, 4, entry->cpu->gpr[18]); /* 80072F50 sw $s2, 0x8($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x4u, 4, entry->cpu->gpr[17]); /* 80072F54 sw $s1, 0x4($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29], 4, entry->cpu->gpr[16]); /* 80072F58 sw $s0, 0x0($sp) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072F5C lui $at, %entry->cpu->hi(D_800CCD36) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072F60 addu $at, $at, $v0 */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD36u, 2); /* 80072F64 lhu $a2, %entry->cpu->lo(D_800CCD36)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072F68 lui $at, %entry->cpu->hi(D_800CCD34) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072F6C addu $at, $at, $v0 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD34u, 2); /* 80072F70 lhu $v1, %entry->cpu->lo(D_800CCD34)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072F74 lui $at, %entry->cpu->hi(D_800CCDEC) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072F78 addu $at, $at, $v0 */
    entry->cpu->gpr[7] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDECu, 4); /* 80072F7C lw $a3, %entry->cpu->lo(D_800CCDEC)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072F80 lui $at, %entry->cpu->hi(D_800CCDF0) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80072F84 addu $at, $at, $v0 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDF0u, 4); /* 80072F88 lw $a0, %entry->cpu->lo(D_800CCDF0)($at) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] & 0xFFu; /* 80072F8C andi $a1, $a1, 0xFF */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072F90 lui $at, %entry->cpu->hi(D_800D3330) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3330u, 2, entry->cpu->gpr[6]); /* 80072F94 sh $a2, %entry->cpu->lo(D_800D3330)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072F98 lui $at, %entry->cpu->hi(D_800D2FE0) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2FE0u, 2, entry->cpu->gpr[6]); /* 80072F9C sh $a2, %entry->cpu->lo(D_800D2FE0)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072FA0 lui $at, %entry->cpu->hi(D_800D2E58) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2E58u, 2, entry->cpu->gpr[3]); /* 80072FA4 sh $v1, %entry->cpu->lo(D_800D2E58)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072FA8 lui $at, %entry->cpu->hi(D_800D2D38) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D38u, 2, entry->cpu->gpr[3]); /* 80072FAC sh $v1, %entry->cpu->lo(D_800D2D38)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072FB0 lui $at, %entry->cpu->hi(D_800D333C) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x333Cu, 4, entry->cpu->gpr[7]); /* 80072FB4 sw $a3, %entry->cpu->lo(D_800D333C)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072FB8 lui $at, %entry->cpu->hi(D_800D3018) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3018u, 4, entry->cpu->gpr[7]); /* 80072FBC sw $a3, %entry->cpu->lo(D_800D3018)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80072FC0 lui $at, %entry->cpu->hi(D_800D3668) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3668u, 4, entry->cpu->gpr[4]); /* 80072FC4 sw $a0, %entry->cpu->lo(D_800D3668)($at) */
    /* 80072FC8 beqz $a1, .L80073004 */
    { int c_ = (entry->cpu->gpr[5] == 0); entry->cpu->gpr[25] = 0u + 0u; if (c_) goto L_80073004; }
    /* 80072FD0 bgez $a0, .L80072FDC */
    { int c_ = ((int32_t)entry->cpu->gpr[4] >= 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0u; if (c_) goto L_80072FDC; }
    entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0x7u; /* 80072FD8 addiu $v0, $a0, 0x7 */
L_80072FDC:
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 3)); /* 80072FDC sra $v0, $v0, 3 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[2] < (int32_t)entry->cpu->gpr[7]); /* 80072FE0 slt $v0, $v0, $a3 */
    /* 80072FE4 beqz $v0, .L80073030 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0u; if (c_) goto L_80073030; }
    /* 80072FEC bgez $v0, .L80072FF8 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] >= 0); ; if (c_) goto L_80072FF8; }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x3u; /* 80072FF4 addiu $v0, $v0, 0x3 */
L_80072FF8:
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 2)); /* 80072FF8 sra $v0, $v0, 2 */
    /* 80072FFC j .L8007304C */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[2] < (int32_t)entry->cpu->gpr[7]); goto L_8007304C;
L_80073004:
    entry->cpu->gpr[2] = entry->cpu->gpr[6] << 16; /* 80073004 sll $v0, $a2, 16 */
    entry->cpu->gpr[4] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 16)); /* 80073008 sra $a0, $v0, 16 */
    /* 8007300C bgez $a0, .L80073018 */
    { int c_ = ((int32_t)entry->cpu->gpr[4] >= 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0u; if (c_) goto L_80073018; }
    entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0x7u; /* 80073014 addiu $v0, $a0, 0x7 */
L_80073018:
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 3)); /* 80073018 sra $v0, $v0, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 16; /* 8007301C sll $v1, $v1, 16 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 80073020 sra $v1, $v1, 16 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[2] < (int32_t)entry->cpu->gpr[3]); /* 80073024 slt $v0, $v0, $v1 */
    /* 80073028 bnez $v0, .L80073038 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0u; if (c_) goto L_80073038; }
L_80073030:
    /* 80073030 j .L80073058 */
    entry->cpu->gpr[25] = 0u | 0x2u; goto L_80073058;
L_80073038:
    /* 80073038 bgez $v0, .L80073044 */
    { int c_ = ((int32_t)entry->cpu->gpr[2] >= 0); ; if (c_) goto L_80073044; }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x3u; /* 80073040 addiu $v0, $v0, 0x3 */
L_80073044:
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[2] >> 2)); /* 80073044 sra $v0, $v0, 2 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[2] < (int32_t)entry->cpu->gpr[3]); /* 80073048 slt $v0, $v0, $v1 */
L_8007304C:
    /* 8007304C bnez $v0, .L8007305C */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[16] = 0x51EB0000u; if (c_) goto L_8007305C; }
    entry->cpu->gpr[25] = 0u | 0x1u; /* 80073054 ori $t9, $zero, 0x1 */
L_80073058:
    entry->cpu->gpr[16] = 0x51EB0000u; /* 80073058 lui $s0, (0x51EB851F >> 16) */
L_8007305C:
    entry->cpu->gpr[4] = 0x800D0000u; /* 8007305C lui $a0, %entry->cpu->hi(D_800D2D38) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D38u, 2); /* 80073060 lhu $a0, %entry->cpu->lo(D_800D2D38)($a0) */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] | 0x851Fu; /* 80073064 ori $s0, $s0, (0x51EB851F & 0xFFFF) */
    entry->cpu->gpr[3] = entry->cpu->gpr[4] << 16; /* 80073068 sll $v1, $a0, 16 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 8007306C sra $v0, $v1, 16 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[16]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80073070 mult $v0, $s0 */
    entry->cpu->gpr[17] = 0x66660000u; /* 80073074 lui $s1, (0x66666667 >> 16) */
    entry->cpu->gpr[17] = entry->cpu->gpr[17] | 0x6667u; /* 80073078 ori $s1, $s1, (0x66666667 & 0xFFFF) */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 31)); /* 8007307C sra $v1, $v1, 31 */
    entry->cpu->gpr[15] = entry->cpu->hi; /* 80073080 mfhi $t7 */
    entry->cpu->gpr[15] = ((uint32_t)((int32_t)entry->cpu->gpr[15] >> 5)); /* 80073084 sra $t7, $t7, 5 */
    entry->cpu->gpr[15] = entry->cpu->gpr[15] - entry->cpu->gpr[3]; /* 80073088 subu $t7, $t7, $v1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[15] << 16; /* 8007308C sll $v1, $t7, 16 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 80073090 sra $v1, $v1, 16 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80073094 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073098 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 8007309C sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800730A0 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800730A4 sll $v0, $v0, 2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] - entry->cpu->gpr[2]; /* 800730A8 subu $a0, $a0, $v0 */
    entry->cpu->gpr[6] = entry->cpu->gpr[4] << 16; /* 800730AC sll $a2, $a0, 16 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 16)); /* 800730B0 sra $v0, $a2, 16 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[17]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 800730B4 mult $v0, $s1 */
    entry->cpu->gpr[10] = 0x800D0000u; /* 800730B8 lui $t2, %entry->cpu->hi(D_800D2FE0) */
    entry->cpu->gpr[10] = battle_entry_read(entry, entry->cpu->gpr[10] + 0x2FE0u, 2); /* 800730BC lhu $t2, %entry->cpu->lo(D_800D2FE0)($t2) */
    entry->cpu->gpr[14] = entry->cpu->hi; /* 800730C0 mfhi $t6 */
    entry->cpu->gpr[3] = entry->cpu->gpr[10] << 16; /* 800730C4 sll $v1, $t2, 16 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 800730C8 sra $v0, $v1, 16 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[16]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 800730CC mult $v0, $s0 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 31)); /* 800730D0 sra $v1, $v1, 31 */
    entry->cpu->gpr[13] = entry->cpu->hi; /* 800730D4 mfhi $t5 */
    entry->cpu->gpr[13] = ((uint32_t)((int32_t)entry->cpu->gpr[13] >> 5)); /* 800730D8 sra $t5, $t5, 5 */
    entry->cpu->gpr[13] = entry->cpu->gpr[13] - entry->cpu->gpr[3]; /* 800730DC subu $t5, $t5, $v1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[13] << 16; /* 800730E0 sll $v1, $t5, 16 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 800730E4 sra $v1, $v1, 16 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800730E8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800730EC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 800730F0 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800730F4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800730F8 sll $v0, $v0, 2 */
    entry->cpu->gpr[10] = entry->cpu->gpr[10] - entry->cpu->gpr[2]; /* 800730FC subu $t2, $t2, $v0 */
    entry->cpu->gpr[12] = entry->cpu->gpr[10] << 16; /* 80073100 sll $t4, $t2, 16 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[12] >> 16)); /* 80073104 sra $v0, $t4, 16 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[2] * (int64_t)(int32_t)entry->cpu->gpr[17]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80073108 mult $v0, $s1 */
    entry->cpu->gpr[7] = 0x800D0000u; /* 8007310C lui $a3, %entry->cpu->hi(D_800D3018) */
    entry->cpu->gpr[7] = battle_entry_read(entry, entry->cpu->gpr[7] + 0x3018u, 4); /* 80073110 lw $a3, %entry->cpu->lo(D_800D3018)($a3) */
    entry->cpu->gpr[11] = entry->cpu->hi; /* 80073114 mfhi $t3 */
    entry->cpu->gpr[2] = 0x68DB0000u; /* 80073118 lui $v0, (0x68DB8BAD >> 16) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] | 0x8BADu; /* 8007311C ori $v0, $v0, (0x68DB8BAD & 0xFFFF) */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[7] * (int64_t)(int32_t)entry->cpu->gpr[2]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80073120 mult $a3, $v0 */
    entry->cpu->gpr[3] = 0x10620000u; /* 80073124 lui $v1, (0x10624DD3 >> 16) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] | 0x4DD3u; /* 80073128 ori $v1, $v1, (0x10624DD3 & 0xFFFF) */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[7] >> 31)); /* 8007312C sra $v0, $a3, 31 */
    entry->cpu->gpr[9] = entry->cpu->hi; /* 80073130 mfhi $t1 */
    entry->cpu->gpr[9] = ((uint32_t)((int32_t)entry->cpu->gpr[9] >> 12)); /* 80073134 sra $t1, $t1, 12 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] - entry->cpu->gpr[2]; /* 80073138 subu $t1, $t1, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[9] << 2; /* 8007313C sll $v0, $t1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[9]; /* 80073140 addu $v0, $v0, $t1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80073144 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[9]; /* 80073148 subu $v0, $v0, $t1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 8007314C sll $v0, $v0, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[9]; /* 80073150 addu $v0, $v0, $t1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80073154 sll $v0, $v0, 4 */
    entry->cpu->gpr[7] = entry->cpu->gpr[7] - entry->cpu->gpr[2]; /* 80073158 subu $a3, $a3, $v0 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[7] * (int64_t)(int32_t)entry->cpu->gpr[3]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 8007315C mult $a3, $v1 */
    entry->cpu->gpr[6] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 31)); /* 80073160 sra $a2, $a2, 31 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80073164 lui $at, %entry->cpu->hi(D_800D2D38) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D38u, 2, entry->cpu->gpr[4]); /* 80073168 sh $a0, %entry->cpu->lo(D_800D2D38)($at) */
    entry->cpu->gpr[14] = ((uint32_t)((int32_t)entry->cpu->gpr[14] >> 2)); /* 8007316C sra $t6, $t6, 2 */
    entry->cpu->gpr[14] = entry->cpu->gpr[14] - entry->cpu->gpr[6]; /* 80073170 subu $t6, $t6, $a2 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80073174 lui $v0, %entry->cpu->hi(D_800C3E08) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x3E08u; /* 80073178 addiu $v0, $v0, %entry->cpu->lo(D_800C3E08) */
    entry->cpu->gpr[24] = entry->cpu->gpr[2] + 0u; /* 8007317C addu $t8, $v0, $zero */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[7] >> 31)); /* 80073180 sra $v0, $a3, 31 */
    entry->cpu->gpr[8] = entry->cpu->hi; /* 80073184 mfhi $t0 */
    entry->cpu->gpr[8] = ((uint32_t)((int32_t)entry->cpu->gpr[8] >> 6)); /* 80073188 sra $t0, $t0, 6 */
    entry->cpu->gpr[8] = entry->cpu->gpr[8] - entry->cpu->gpr[2]; /* 8007318C subu $t0, $t0, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[8] << 5; /* 80073190 sll $a1, $t0, 5 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] - entry->cpu->gpr[8]; /* 80073194 subu $a1, $a1, $t0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 2; /* 80073198 sll $a1, $a1, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[8]; /* 8007319C addu $a1, $a1, $t0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 3; /* 800731A0 sll $a1, $a1, 3 */
    entry->cpu->gpr[5] = entry->cpu->gpr[7] - entry->cpu->gpr[5]; /* 800731A4 subu $a1, $a3, $a1 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[5] * (int64_t)(int32_t)entry->cpu->gpr[16]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 800731A8 mult $a1, $s0 */
    entry->cpu->gpr[12] = ((uint32_t)((int32_t)entry->cpu->gpr[12] >> 31)); /* 800731AC sra $t4, $t4, 31 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800731B0 lui $at, %entry->cpu->hi(D_800D2FE0) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2FE0u, 2, entry->cpu->gpr[10]); /* 800731B4 sh $t2, %entry->cpu->lo(D_800D2FE0)($at) */
    entry->cpu->gpr[11] = ((uint32_t)((int32_t)entry->cpu->gpr[11] >> 2)); /* 800731B8 sra $t3, $t3, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[14] << 16; /* 800731BC sll $v1, $t6, 16 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 800731C0 sra $v1, $v1, 16 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; /* 800731C4 sll $v0, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800731C8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 800731CC sll $v0, $v0, 1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] - entry->cpu->gpr[2]; /* 800731D0 subu $a0, $a0, $v0 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[5] >> 31)); /* 800731D4 sra $v0, $a1, 31 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800731D8 lui $at, %entry->cpu->hi(D_800D2D38) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D38u, 2, entry->cpu->gpr[4]); /* 800731DC sh $a0, %entry->cpu->lo(D_800D2D38)($at) */
    entry->cpu->gpr[6] = entry->cpu->hi; /* 800731E0 mfhi $a2 */
    entry->cpu->gpr[6] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 5)); /* 800731E4 sra $a2, $a2, 5 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] - entry->cpu->gpr[2]; /* 800731E8 subu $a2, $a2, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[6] << 1; /* 800731EC sll $a0, $a2, 1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[6]; /* 800731F0 addu $a0, $a0, $a2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 3; /* 800731F4 sll $a0, $a0, 3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[6]; /* 800731F8 addu $a0, $a0, $a2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 2; /* 800731FC sll $a0, $a0, 2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[5] - entry->cpu->gpr[4]; /* 80073200 subu $a0, $a1, $a0 */
    { uint64_t product = (uint64_t)((int64_t)(int32_t)entry->cpu->gpr[4] * (int64_t)(int32_t)entry->cpu->gpr[17]); entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80073204 mult $a0, $s1 */
    entry->cpu->gpr[11] = entry->cpu->gpr[11] - entry->cpu->gpr[12]; /* 80073208 subu $t3, $t3, $t4 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 8007320C lui $at, %entry->cpu->hi(D_800D3018) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3018u, 4, entry->cpu->gpr[7]); /* 80073210 sw $a3, %entry->cpu->lo(D_800D3018)($at) */
    entry->cpu->gpr[3] = entry->cpu->gpr[11] << 16; /* 80073214 sll $v1, $t3, 16 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 16)); /* 80073218 sra $v1, $v1, 16 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 8007321C lui $at, %entry->cpu->hi(D_800D3018) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3018u, 4, entry->cpu->gpr[5]); /* 80073220 sw $a1, %entry->cpu->lo(D_800D3018)($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; /* 80073224 sll $v0, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073228 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 8007322C sll $v0, $v0, 1 */
    entry->cpu->gpr[10] = entry->cpu->gpr[10] - entry->cpu->gpr[2]; /* 80073230 subu $t2, $t2, $v0 */
    entry->cpu->gpr[2] = ((uint32_t)((int32_t)entry->cpu->gpr[4] >> 31)); /* 80073234 sra $v0, $a0, 31 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80073238 lui $at, %entry->cpu->hi(D_800D3018) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3018u, 4, entry->cpu->gpr[4]); /* 8007323C sw $a0, %entry->cpu->lo(D_800D3018)($at) */
    entry->cpu->gpr[3] = entry->cpu->hi; /* 80073240 mfhi $v1 */
    entry->cpu->gpr[3] = ((uint32_t)((int32_t)entry->cpu->gpr[3] >> 2)); /* 80073244 sra $v1, $v1, 2 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80073248 subu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; /* 8007324C sll $v0, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073250 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80073254 sll $v0, $v0, 1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] - entry->cpu->gpr[2]; /* 80073258 subu $a0, $a0, $v0 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 8007325C lui $v0, %entry->cpu->hi(D_800D2D38) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D38u, 1); /* 80073260 lbu $v0, %entry->cpu->lo(D_800D2D38)($v0) */
    entry->cpu->gpr[18] = 0u | 0xFFu; /* 80073264 ori $s2, $zero, 0xFF */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80073268 lui $at, %entry->cpu->hi(D_800D2FE0) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2FE0u, 2, entry->cpu->gpr[10]); /* 8007326C sh $t2, %entry->cpu->lo(D_800D2FE0)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80073270 lui $at, %entry->cpu->hi(D_800D3018) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3018u, 4, entry->cpu->gpr[4]); /* 80073274 sw $a0, %entry->cpu->lo(D_800D3018)($at) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 80073278 lui $a0, %entry->cpu->hi(D_800D2FE0) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2FE0u, 1); /* 8007327C lbu $a0, %entry->cpu->lo(D_800D2FE0)($a0) */
    entry->cpu->gpr[5] = 0x800D0000u; /* 80073280 lui $a1, %entry->cpu->hi(D_800D3018) */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x3018u, 1); /* 80073284 lbu $a1, %entry->cpu->lo(D_800D3018)($a1) */
    entry->cpu->gpr[7] = entry->cpu->gpr[24] + 0x2u; /* 80073288 addiu $a3, $t8, 0x2 */
    battle_entry_write(entry, entry->cpu->gpr[24], 1, entry->cpu->gpr[15]); /* 8007328C sb $t7, 0x0($t8) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80073290 lui $at, %entry->cpu->hi(D_800C3E09) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E09u, 1, entry->cpu->gpr[14]); /* 80073294 sb $t6, %entry->cpu->lo(D_800C3E09)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80073298 lui $at, %entry->cpu->hi(D_800D2D58) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D58u, 1, entry->cpu->gpr[13]); /* 8007329C sb $t5, %entry->cpu->lo(D_800D2D58)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732A0 lui $at, %entry->cpu->hi(D_800D2D59) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D59u, 1, entry->cpu->gpr[11]); /* 800732A4 sb $t3, %entry->cpu->lo(D_800D2D59)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732A8 lui $at, %entry->cpu->hi(D_800D2D88) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D88u, 1, entry->cpu->gpr[9]); /* 800732AC sb $t1, %entry->cpu->lo(D_800D2D88)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732B0 lui $at, %entry->cpu->hi(D_800D2D89) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D89u, 1, entry->cpu->gpr[8]); /* 800732B4 sb $t0, %entry->cpu->lo(D_800D2D89)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732B8 lui $at, %entry->cpu->hi(D_800D2D8A) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D8Au, 1, entry->cpu->gpr[6]); /* 800732BC sb $a2, %entry->cpu->lo(D_800D2D8A)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732C0 lui $at, %entry->cpu->hi(D_800D2D8B) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D8Bu, 1, entry->cpu->gpr[3]); /* 800732C4 sb $v1, %entry->cpu->lo(D_800D2D8B)($at) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 800732C8 lui $at, %entry->cpu->hi(D_800C3E0A) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E0Au, 1, entry->cpu->gpr[2]); /* 800732CC sb $v0, %entry->cpu->lo(D_800C3E0A)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732D0 lui $at, %entry->cpu->hi(D_800D2D5A) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D5Au, 1, entry->cpu->gpr[4]); /* 800732D4 sb $a0, %entry->cpu->lo(D_800D2D5A)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800732D8 lui $at, %entry->cpu->hi(D_800D2D8C) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x2D8Cu, 1, entry->cpu->gpr[5]); /* 800732DC sb $a1, %entry->cpu->lo(D_800D2D8C)($at) */
L_800732E0:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[24], 1); /* 800732E0 lbu $v0, 0x0($t8) */
    ; /* 800732E4 nop  */
    /* 800732E8 bnez $v0, .L80073304 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[5] = 0u | 0xFFu; if (c_) goto L_80073304; }
    battle_entry_write(entry, entry->cpu->gpr[24], 1, entry->cpu->gpr[18]); /* 800732F0 sb $s2, 0x0($t8) */
    entry->cpu->gpr[24] = entry->cpu->gpr[24] + 0x1u; /* 800732F4 addiu $t8, $t8, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[24] < (int32_t)entry->cpu->gpr[7]); /* 800732F8 slt $v0, $t8, $a3 */
    /* 800732FC bnez $v0, .L800732E0 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_800732E0; }
L_80073304:
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073304 lui $v1, %entry->cpu->hi(D_800D2D58) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x2D58u; /* 80073308 addiu $v1, $v1, %entry->cpu->lo(D_800D2D58) */
    entry->cpu->gpr[4] = entry->cpu->gpr[3] + 0x2u; /* 8007330C addiu $a0, $v1, 0x2 */
L_80073310:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3], 1); /* 80073310 lbu $v0, 0x0($v1) */
    ; /* 80073314 nop  */
    /* 80073318 bnez $v0, .L80073334 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80073334; }
    battle_entry_write(entry, entry->cpu->gpr[3], 1, entry->cpu->gpr[5]); /* 80073320 sb $a1, 0x0($v1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x1u; /* 80073324 addiu $v1, $v1, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[3] < (int32_t)entry->cpu->gpr[4]); /* 80073328 slt $v0, $v1, $a0 */
    /* 8007332C bnez $v0, .L80073310 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80073310; }
L_80073334:
    entry->cpu->gpr[5] = 0u | 0xFFu; /* 80073334 ori $a1, $zero, 0xFF */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073338 lui $v1, %entry->cpu->hi(D_800D2D88) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x2D88u; /* 8007333C addiu $v1, $v1, %entry->cpu->lo(D_800D2D88) */
    entry->cpu->gpr[4] = entry->cpu->gpr[3] + 0x4u; /* 80073340 addiu $a0, $v1, 0x4 */
L_80073344:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3], 1); /* 80073344 lbu $v0, 0x0($v1) */
    ; /* 80073348 nop  */
    /* 8007334C bnez $v0, .L80073368 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[25] + 0u; if (c_) goto L_80073368; }
    battle_entry_write(entry, entry->cpu->gpr[3], 1, entry->cpu->gpr[5]); /* 80073354 sb $a1, 0x0($v1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x1u; /* 80073358 addiu $v1, $v1, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[3] < (int32_t)entry->cpu->gpr[4]); /* 8007335C slt $v0, $v1, $a0 */
    /* 80073360 bnez $v0, .L80073344 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[25] + 0u; if (c_) goto L_80073344; }
L_80073368:
    uint32_t saved_s2 = battle_entry_read(entry, entry->cpu->gpr[29] + 8u, 4);
    uint32_t saved_s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 4u, 4);
    entry->cpu->gpr[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29], 4);
    entry->cpu->gpr[17] = saved_s1;
    entry->cpu->gpr[29] += 0x10u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80073378 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental HUD-value entry at target processing's exact call site. */
static int battle_hud_values_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80072f38u || cpu->gpr[31] != 0x8007431cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_HUD_VALUES_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "dfe133676a80f2528dd6672f120aaeefb606eff8c56efb785f71bb85be51f3bd";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x448u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_hud_values_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

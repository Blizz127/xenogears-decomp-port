static void battle_gauge_refresh_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFF0u; /* 80075168 addiu $sp, $sp, -0x10 */
    entry->cpu->gpr[12] = 0u + 0u; /* 8007516C addu $t4, $zero, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x8u, 4, entry->cpu->gpr[18]); /* 80075170 sw $s2, 0x8($sp) */
    entry->cpu->gpr[18] = 0x800C0000u; /* 80075174 lui $s2, %entry->cpu->hi(D_800C3254) */
    entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x3254u; /* 80075178 addiu $s2, $s2, %entry->cpu->lo(D_800C3254) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x4u, 4, entry->cpu->gpr[17]); /* 8007517C sw $s1, 0x4($sp) */
    entry->cpu->gpr[17] = 0u | 0x22u; /* 80075180 ori $s1, $zero, 0x22 */
    battle_entry_write(entry, entry->cpu->gpr[29], 4, entry->cpu->gpr[16]); /* 80075184 sw $s0, 0x0($sp) */
    entry->cpu->gpr[16] = 0u | 0x26u; /* 80075188 ori $s0, $zero, 0x26 */
    entry->cpu->gpr[11] = 0u | 0xFFu; /* 8007518C ori $t3, $zero, 0xFF */
    entry->cpu->gpr[25] = 0u | 0x7Fu; /* 80075190 ori $t9, $zero, 0x7F */
    entry->cpu->gpr[15] = 0u | 0x28u; /* 80075194 ori $t7, $zero, 0x28 */
    entry->cpu->gpr[10] = 0u + 0u; /* 80075198 addu $t2, $zero, $zero */
    entry->cpu->gpr[14] = 0u + 0u; /* 8007519C addu $t6, $zero, $zero */
    entry->cpu->gpr[6] = 0u + 0u; /* 800751A0 addu $a2, $zero, $zero */
    entry->cpu->gpr[13] = 0x800D0000u; /* 800751A4 lui $t5, %entry->cpu->hi(D_800D32A0) */
    entry->cpu->gpr[13] = entry->cpu->gpr[13] + 0x32A0u; /* 800751A8 addiu $t5, $t5, %entry->cpu->lo(D_800D32A0) */
    entry->cpu->gpr[24] = 0u + 0u; /* 800751AC addu $t8, $zero, $zero */
L_800751B0:
    entry->cpu->gpr[7] = 0x800C0000u; /* 800751B0 lui $a3, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[7] = battle_entry_read(entry, entry->cpu->gpr[7] + 0x3EA4u, 4); /* 800751B4 lw $a3, %entry->cpu->lo(D_800C3EA4)($a3) */
    ; /* 800751B8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[7] + entry->cpu->gpr[24]; /* 800751BC addu $v0, $a3, $t8 */
    entry->cpu->gpr[1] = 0x10000u; /* 800751C0 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[2] + entry->cpu->gpr[1]; /* 800751C4 addu $at, $v0, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Du, 1); /* 800751C8 lbu $v1, -0x7AC3($at) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 800751CC ori $v0, $zero, 0x1 */
    /* 800751D0 beq $v1, $v0, .L800751E8 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0u | 0x2u; if (c_) goto L_800751E8; }
    /* 800751D8 beq $v1, $v0, .L80075410 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[4] = 0xD1B70000u; if (c_) goto L_80075410; }
    /* 800751E0 j .L80075900 */
    entry->cpu->gpr[15] = entry->cpu->gpr[15] + 0x60u; goto L_80075900;
L_800751E8:
    entry->cpu->gpr[2] = 0x800D0000u; /* 800751E8 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 800751EC lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 800751F0 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 800751F4 addu $v0, $a2, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 3; /* 800751F8 sll $v1, $v0, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800751FC addu $v1, $v1, $v0 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075200 lui $v0, %entry->cpu->hi(D_800D3280) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3280u, 1); /* 80075204 lbu $v0, %entry->cpu->lo(D_800D3280)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075208 sll $v1, $v1, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 1; /* 8007520C sll $a1, $v0, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80075210 addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[12]; /* 80075214 addu $a1, $a1, $t4 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 1; /* 80075218 sll $a1, $a1, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[18]; /* 8007521C addu $a1, $a1, $s2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075220 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[7]; /* 80075224 addu $v1, $v1, $a3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x28u; /* 80075228 addiu $v0, $v0, 0x28 */
    entry->cpu->gpr[2] = entry->cpu->gpr[10] + entry->cpu->gpr[2]; /* 8007522C addu $v0, $t2, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x748u, 2, entry->cpu->gpr[2]); /* 80075230 sh $v0, 0x748($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075234 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075238 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 8007523C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075240 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075244 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075248 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007524C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075250 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Au, 2, entry->cpu->gpr[17]); /* 80075254 sh $s1, 0x74A($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075258 lui $v0, %entry->cpu->hi(D_800CCB34) */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 8007525C */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[13], 1); /* 80075260 */
        entry->cpu->gpr[2] = pending_first;
        entry->cpu->gpr[3] = pending_second;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80075264 addu $v0, $a2, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[2] << 3; /* 80075268 sll $a0, $v0, 3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[2]; /* 8007526C addu $a0, $a0, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 2; /* 80075270 sll $a0, $a0, 2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[7]; /* 80075274 addu $a0, $a0, $a3 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075278 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 8007527C sll $v1, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x28u; /* 80075280 addiu $v0, $v0, 0x28 */
    entry->cpu->gpr[2] = entry->cpu->gpr[10] + entry->cpu->gpr[2]; /* 80075284 addu $v0, $t2, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075288 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x750u, 2, entry->cpu->gpr[2]); /* 8007528C sh $v0, 0x750($a0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075290 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075294 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075298 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 8007529C addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800752A0 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800752A4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800752A8 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 800752AC addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x752u, 2, entry->cpu->gpr[17]); /* 800752B0 sh $s1, 0x752($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800752B4 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 800752B8 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 800752BC nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 800752C0 addu $v0, $a2, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 3; /* 800752C4 sll $v1, $v0, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800752C8 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 800752CC sll $v1, $v1, 2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 800752D0 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[7]; /* 800752D4 addu $v1, $v1, $a3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x28u; /* 800752D8 addiu $v0, $v0, 0x28 */
    entry->cpu->gpr[2] = entry->cpu->gpr[10] + entry->cpu->gpr[2]; /* 800752DC addu $v0, $t2, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x758u, 2, entry->cpu->gpr[2]); /* 800752E0 sh $v0, 0x758($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800752E4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800752E8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800752EC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800752F0 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800752F4 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800752F8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800752FC sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075300 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x75Au, 2, entry->cpu->gpr[16]); /* 80075304 sh $s0, 0x75A($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075308 lui $v0, %entry->cpu->hi(D_800CCB34) */
    { /* Keep the first load pending if the following read faults. */
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 8007530C */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[13], 1); /* 80075310 */
        entry->cpu->gpr[2] = pending_first;
        entry->cpu->gpr[3] = pending_second;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80075314 addu $v0, $a2, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[2] << 3; /* 80075318 sll $a0, $v0, 3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[2]; /* 8007531C addu $a0, $a0, $v0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 2; /* 80075320 sll $a0, $a0, 2 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[7]; /* 80075324 addu $a0, $a0, $a3 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 80075328 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 1; /* 8007532C sll $v1, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x28u; /* 80075330 addiu $v0, $v0, 0x28 */
    entry->cpu->gpr[2] = entry->cpu->gpr[10] + entry->cpu->gpr[2]; /* 80075334 addu $v0, $t2, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075338 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x760u, 2, entry->cpu->gpr[2]); /* 8007533C sh $v0, 0x760($a0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075340 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075344 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075348 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 8007534C addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075350 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075354 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075358 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 8007535C addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x762u, 2, entry->cpu->gpr[16]); /* 80075360 sh $s0, 0x762($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075364 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075368 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 8007536C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075370 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075374 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075378 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007537C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075380 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x744u, 1, 0u); /* 80075384 sb $zero, 0x744($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075388 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007538C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075390 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075394 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075398 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007539C addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800753A0 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800753A4 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800753A8 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800753AC addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x745u, 1, entry->cpu->gpr[11]); /* 800753B0 sb $t3, 0x745($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800753B4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800753B8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800753BC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800753C0 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800753C4 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800753C8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800753CC lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800753D0 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800753D4 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800753D8 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x746u, 1, 0u); /* 800753DC sb $zero, 0x746($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800753E0 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800753E4 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800753E8 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800753EC addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800753F0 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800753F4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800753F8 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800753FC lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075400 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075404 addu $v0, $v0, $v1 */
    /* 80075408 j .L800757A0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Cu, 1, 0u); goto L_800757A0;
L_80075410:
    entry->cpu->gpr[1] = 0x800D0000u; /* 80075410 lui $at, %entry->cpu->hi(D_800CCDC4) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[14]; /* 80075414 addu $at, $at, $t6 */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDC4u, 2); /* 80075418 lhu $t0, %entry->cpu->lo(D_800CCDC4)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 8007541C lui $at, %entry->cpu->hi(D_800CCDC6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[14]; /* 80075420 addu $at, $at, $t6 */
    entry->cpu->gpr[9] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDC6u, 2); /* 80075424 lhu $t1, %entry->cpu->lo(D_800CCDC6)($at) */
    entry->cpu->gpr[3] = entry->cpu->gpr[8] << 1; /* 80075428 sll $v1, $t0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[8]; /* 8007542C addu $v1, $v1, $t0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 3; /* 80075430 sll $v1, $v1, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[8]; /* 80075434 addu $v1, $v1, $t0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075438 sll $v1, $v1, 2 */
    { uint32_t lhs = entry->cpu->gpr[3], rhs = entry->cpu->gpr[9]; entry->cpu->lo = rhs ? lhs / rhs : UINT32_MAX; entry->cpu->hi = rhs ? lhs % rhs : lhs; } /* 8007543C divu $zero, $v1, $t1 */
    entry->cpu->gpr[3] = entry->cpu->lo; /* 80075440 mflo $v1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] | 0x1759u; /* 80075444 ori $a0, $a0, (0xD1B71759 & 0xFFFF) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80075448 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007544C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075450 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 80075454 subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80075458 sll $v0, $v0, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 8007545C subu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 5; /* 80075460 sll $v0, $v0, 5 */
    { uint64_t product = (uint64_t)entry->cpu->gpr[2] * entry->cpu->gpr[4]; entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 80075464 multu $v0, $a0 */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075468 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007546C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075470 lui $v0, %entry->cpu->hi(D_800D3280) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3280u, 1); /* 80075474 lbu $v0, %entry->cpu->lo(D_800D3280)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075478 addu $v1, $a2, $v1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[3] << 3; /* 8007547C sll $a0, $v1, 3 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[3]; /* 80075480 addu $a0, $a0, $v1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 2; /* 80075484 sll $a0, $a0, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 1; /* 80075488 sll $a1, $v0, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 8007548C addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[12]; /* 80075490 addu $a1, $a1, $t4 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 1; /* 80075494 sll $a1, $a1, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[18]; /* 80075498 addu $a1, $a1, $s2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 8007549C lhu $v0, 0x0($a1) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + entry->cpu->gpr[7]; /* 800754A0 addu $a0, $a0, $a3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x28u; /* 800754A4 addiu $v0, $v0, 0x28 */
    entry->cpu->gpr[2] = entry->cpu->gpr[10] + entry->cpu->gpr[2]; /* 800754A8 addu $v0, $t2, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x748u, 2, entry->cpu->gpr[2]); /* 800754AC sh $v0, 0x748($a0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800754B0 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800754B4 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800754B8 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800754BC addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800754C0 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800754C4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800754C8 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 800754CC addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Au, 2, entry->cpu->gpr[17]); /* 800754D0 sh $s1, 0x74A($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800754D4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800754D8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800754DC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800754E0 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800754E4 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800754E8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800754EC sll $v0, $v0, 2 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 800754F0 lhu $v1, 0x0($a1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 800754F4 addu $v0, $v0, $a3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[15] + entry->cpu->gpr[3]; /* 800754F8 addu $v1, $t7, $v1 */
    entry->cpu->gpr[4] = entry->cpu->hi; /* 800754FC mfhi $a0 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] >> 13; /* 80075500 srl $a0, $a0, 13 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[4]; /* 80075504 addu $v1, $v1, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x750u, 2, entry->cpu->gpr[3]); /* 80075508 sh $v1, 0x750($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007550C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075510 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075514 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075518 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 8007551C sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075520 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075524 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075528 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x752u, 2, entry->cpu->gpr[17]); /* 8007552C sh $s1, 0x752($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075530 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075534 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 80075538 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 8007553C addu $v0, $a2, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 3; /* 80075540 sll $v1, $v0, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075544 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 80075548 sll $v1, $v1, 2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 8007554C lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[7]; /* 80075550 addu $v1, $v1, $a3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x28u; /* 80075554 addiu $v0, $v0, 0x28 */
    entry->cpu->gpr[2] = entry->cpu->gpr[10] + entry->cpu->gpr[2]; /* 80075558 addu $v0, $t2, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x758u, 2, entry->cpu->gpr[2]); /* 8007555C sh $v0, 0x758($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075560 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075564 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075568 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 8007556C addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075570 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075574 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075578 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 8007557C addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x75Au, 2, entry->cpu->gpr[16]); /* 80075580 sh $s0, 0x75A($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80075584 lui $v0, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFCB34u, 4); /* 80075588 lw $v0, %entry->cpu->lo(D_800CCB34)($v0) */
    ; /* 8007558C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] + entry->cpu->gpr[2]; /* 80075590 addu $v0, $a2, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 3; /* 80075594 sll $v1, $v0, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 80075598 addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 2; /* 8007559C sll $v1, $v1, 2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 800755A0 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[7]; /* 800755A4 addu $v1, $v1, $a3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[15] + entry->cpu->gpr[2]; /* 800755A8 addu $v0, $t7, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 800755AC addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x760u, 2, entry->cpu->gpr[2]); /* 800755B0 sh $v0, 0x760($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800755B4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800755B8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800755BC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800755C0 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800755C4 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800755C8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800755CC sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 800755D0 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x762u, 2, entry->cpu->gpr[16]); /* 800755D4 sh $s0, 0x762($v0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[9] >> 2; /* 800755D8 srl $v0, $t1, 2 */
    entry->cpu->gpr[2] = (uint32_t)(entry->cpu->gpr[8] < entry->cpu->gpr[2]); /* 800755DC sltu $v0, $t0, $v0 */
    /* 800755E0 bnez $v0, .L800756EC */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[9] >> 3; if (c_) goto L_800756EC; }
    entry->cpu->gpr[3] = 0x800D0000u; /* 800755E8 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800755EC lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800755F0 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800755F4 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800755F8 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800755FC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075600 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075604 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x744u, 1, 0u); /* 80075608 sb $zero, 0x744($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007560C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075610 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075614 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075618 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 8007561C sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075620 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075624 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075628 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007562C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075630 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x745u, 1, 0u); /* 80075634 sb $zero, 0x745($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075638 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007563C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075640 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075644 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075648 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007564C addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075650 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075654 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075658 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007565C addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x746u, 1, entry->cpu->gpr[11]); /* 80075660 sb $t3, 0x746($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075664 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075668 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 8007566C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075670 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075674 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075678 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8007567C lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075680 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075684 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075688 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Cu, 1, 0u); /* 8007568C sb $zero, 0x74C($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075690 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075694 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075698 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 8007569C addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800756A0 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800756A4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800756A8 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800756AC lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800756B0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800756B4 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Du, 1, 0u); /* 800756B8 sb $zero, 0x74D($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800756BC lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800756C0 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800756C4 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800756C8 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800756CC sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800756D0 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800756D4 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800756D8 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800756DC sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800756E0 addu $v0, $v0, $v1 */
    /* 800756E4 j .L800758FC */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Eu, 1, entry->cpu->gpr[11]); goto L_800758FC;
L_800756EC:
    entry->cpu->gpr[2] = (uint32_t)(entry->cpu->gpr[8] < entry->cpu->gpr[2]); /* 800756EC sltu $v0, $t0, $v0 */
    /* 800756F0 bnez $v0, .L800757FC */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_800757FC; }
    entry->cpu->gpr[3] = 0x800D0000u; /* 800756F8 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800756FC lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075700 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075704 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075708 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007570C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075710 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075714 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x744u, 1, entry->cpu->gpr[11]); /* 80075718 sb $t3, 0x744($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007571C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075720 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075724 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075728 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 8007572C sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075730 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075734 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075738 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007573C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075740 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x745u, 1, entry->cpu->gpr[11]); /* 80075744 sb $t3, 0x745($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075748 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007574C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075750 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075754 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075758 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007575C addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075760 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075764 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075768 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007576C addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x746u, 1, 0u); /* 80075770 sb $zero, 0x746($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075774 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075778 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 8007577C nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075780 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075784 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075788 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8007578C lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075790 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075794 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075798 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Cu, 1, entry->cpu->gpr[11]); /* 8007579C sb $t3, 0x74C($v0) */
L_800757A0:
    entry->cpu->gpr[3] = 0x800D0000u; /* 800757A0 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800757A4 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800757A8 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800757AC addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800757B0 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800757B4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800757B8 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800757BC lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800757C0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800757C4 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Du, 1, entry->cpu->gpr[11]); /* 800757C8 sb $t3, 0x74D($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800757CC lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800757D0 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800757D4 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800757D8 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800757DC sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800757E0 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800757E4 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800757E8 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800757EC sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800757F0 addu $v0, $v0, $v1 */
    /* 800757F4 j .L800758FC */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Eu, 1, 0u); goto L_800758FC;
L_800757FC:
    entry->cpu->gpr[3] = 0x800D0000u; /* 800757FC lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075800 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075804 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075808 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 8007580C sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075810 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075814 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[7]; /* 80075818 addu $v0, $v0, $a3 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x744u, 1, entry->cpu->gpr[11]); /* 8007581C sb $t3, 0x744($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075820 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075824 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075828 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 8007582C addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075830 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075834 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075838 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 8007583C lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075840 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075844 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x745u, 1, entry->cpu->gpr[25]); /* 80075848 sb $t9, 0x745($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 8007584C lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 80075850 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075854 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075858 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 8007585C sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075860 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075864 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075868 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 8007586C sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80075870 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x746u, 1, entry->cpu->gpr[25]); /* 80075874 sb $t9, 0x746($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80075878 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 8007587C lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 80075880 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 80075884 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 80075888 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007588C addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80075890 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 80075894 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80075898 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8007589C addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Cu, 1, entry->cpu->gpr[11]); /* 800758A0 sb $t3, 0x74C($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800758A4 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800758A8 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800758AC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800758B0 addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800758B4 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800758B8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800758BC lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800758C0 lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800758C4 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800758C8 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Du, 1, entry->cpu->gpr[25]); /* 800758CC sb $t9, 0x74D($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800758D0 lui $v1, %entry->cpu->hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 4); /* 800758D4 lw $v1, %entry->cpu->lo(D_800CCB34)($v1) */
    ; /* 800758D8 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[6] + entry->cpu->gpr[3]; /* 800758DC addu $v1, $a2, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800758E0 sll $v0, $v1, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800758E4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800758E8 lui $v1, %entry->cpu->hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800758EC lw $v1, %entry->cpu->lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800758F0 sll $v0, $v0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800758F4 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x74Eu, 1, entry->cpu->gpr[25]); /* 800758F8 sb $t9, 0x74E($v0) */
L_800758FC:
    entry->cpu->gpr[15] = entry->cpu->gpr[15] + 0x60u; /* 800758FC addiu $t7, $t7, 0x60 */
L_80075900:
    entry->cpu->gpr[10] = entry->cpu->gpr[10] + 0x60u; /* 80075900 addiu $t2, $t2, 0x60 */
    entry->cpu->gpr[14] = entry->cpu->gpr[14] + 0x170u; /* 80075904 addiu $t6, $t6, 0x170 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + 0x2u; /* 80075908 addiu $a2, $a2, 0x2 */
    entry->cpu->gpr[13] = entry->cpu->gpr[13] + 0x8u; /* 8007590C addiu $t5, $t5, 0x8 */
    entry->cpu->gpr[12] = entry->cpu->gpr[12] + 0x1u; /* 80075910 addiu $t4, $t4, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[12] < 3); /* 80075914 slti $v0, $t4, 0x3 */
    /* 80075918 bnez $v0, .L800751B0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[24] = entry->cpu->gpr[24] + 0x1E4u; if (c_) goto L_800751B0; }
    uint32_t saved_s2 = battle_entry_read(entry, entry->cpu->gpr[29] + 8u, 4);
    uint32_t saved_s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 4u, 4);
    entry->cpu->gpr[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29], 4);
    entry->cpu->gpr[17] = saved_s1;
    entry->cpu->gpr[29] += 0x10u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80075930 jr $ra */
    ; goto bl_exit;
bl_exit:
}
/* Experimental refresh entry at the gauge phase's exact call site. */
static int battle_gauge_refresh_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80075168u || cpu->gpr[31] != 0x80075950u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_GAUGE_REFRESH_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "76f5811b6a7fb351d57ba9519d92bf4937deb446d365baa71df5290f8daeb98b";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x7d0u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_gauge_refresh_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

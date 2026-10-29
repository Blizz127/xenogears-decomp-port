/* Experimental partial command-menu driver: supported state targets are 1, 4, 19, 100 and the retail default path; child edges have separate refusal guards. */
static void battle_command_menu_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFD8u; /* 80080160 addiu $sp, $sp, -0x28 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80080164 lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080168 lw $v1, %lo(D_800C3EAC)($v1) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 8008016C ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[31]); /* 80080170 sw $ra, 0x20($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[19]); /* 80080174 sw $s3, 0x1C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[18]); /* 80080178 sw $s2, 0x18($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[17]); /* 8008017C sw $s1, 0x14($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[16]); /* 80080180 sw $s0, 0x10($sp) */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x2EAu, 1, entry->cpu->gpr[2]); /* 80080184 sb $v0, 0x2EA($v1) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080188 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 8008018C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80080190 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E9u, 1, 0u); /* 80080194 sb $zero, 0x2E9($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080198 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 8008019C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 800801A0 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2DEu, 1, 0u); /* 800801A4 sb $zero, 0x2DE($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800801A8 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800801AC lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 800801B0 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E0u, 1, 0u); /* 800801B4 sb $zero, 0x2E0($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800801B8 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800801BC lw $v0, %lo(D_800C3EAC)($v0) */
    entry->cpu->gpr[18] = entry->cpu->gpr[4] + 0u; /* 800801C0 addu $s2, $a0, $zero */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E1u, 1, 0u); /* 800801C4 sb $zero, 0x2E1($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800801C8 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800801CC lw $v0, %lo(D_800C3EAC)($v0) */
    entry->cpu->gpr[4] = 0u | 0xFFu; /* 800801D0 ori $a0, $zero, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E2u, 1, entry->cpu->gpr[4]); /* 800801D4 sb $a0, 0x2E2($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800801D8 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800801DC lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 800801E0 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E3u, 1, 0u); /* 800801E4 sb $zero, 0x2E3($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800801E8 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800801EC lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 800801F0 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E4u, 1, 0u); /* 800801F4 sb $zero, 0x2E4($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800801F8 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800801FC lw $v0, %lo(D_800C3EAC)($v0) */
    entry->cpu->gpr[5] = entry->cpu->gpr[18] & 0xFFu; /* 80080200 andi $a1, $s2, 0xFF */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2DCu, 1, 0u); /* 80080204 sb $zero, 0x2DC($v0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[5] << 3; /* 80080208 sll $v0, $a1, 3 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8008020C lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080210 lw $v1, %lo(D_800C3EAC)($v1) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080214 lui $at, %hi(D_800D32A4) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80080218 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A4u, 1); /* 8008021C lbu $v0, %lo(D_800D32A4)($at) */
    ; /* 80080220 nop  */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x2D5u, 1, entry->cpu->gpr[2]); /* 80080224 sb $v0, 0x2D5($v1) */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x2D4u, 1, entry->cpu->gpr[2]); /* 80080228 sb $v0, 0x2D4($v1) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 8008022C lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80080230 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[3] = 0u | 0x3u; /* 80080234 ori $v1, $zero, 0x3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[5]; /* 80080238 addu $v0, $v0, $a1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x90u, 1, entry->cpu->gpr[3]); /* 8008023C sb $v1, 0x90($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080240 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 80080244 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80080248 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2E7u, 1, 0u); /* 8008024C sb $zero, 0x2E7($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80080250 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80080254 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80080258 lui $at, %hi(D_800C3E29) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E29u, 1, entry->cpu->gpr[4]); /* 8008025C sb $a0, %lo(D_800C3E29)($at) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80080260 lui $at, %hi(D_800C4929) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x4929u, 1, 0u); /* 80080264 sb $zero, %lo(D_800C4929)($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[5]; /* 80080268 addu $v0, $v0, $a1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x90u, 1); /* 8008026C lbu $v1, 0x90($v0) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 80080270 ori $v0, $zero, 0x1 */
    /* 80080274 beq $v1, $v0, .L800802AC */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[2] = 0u | 0x1u; if (c_) battle_entry_fail(entry,"unexpected menu prefix branch",0x800802acu); }
    entry->cpu->gpr[16] = entry->cpu->gpr[5] + 0u; /* 8008027C addu $s0, $a1, $zero */
    entry->cpu->gpr[17] = 0u | 0x1u; /* 80080280 ori $s1, $zero, 0x1 */
    for (;;) {
        battle_phase_call(entry, 0x800716d8u, 0x80080284u);
        entry->cpu->gpr[2] = 0x800d0000u;
        entry->cpu->gpr[2] = battle_entry_read(entry, 0x800d2d28u, 4);
        entry->cpu->gpr[2] += entry->cpu->gpr[16];
        entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x90u, 1);
        int repeat = entry->cpu->gpr[2] != entry->cpu->gpr[17];
        entry->cpu->gpr[2] = 1;
        if (!repeat) break;
    }
    entry->cpu->gpr[1] = 0x800D0000u; /* 800802AC lui $at, %hi(D_800D366C) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x366Cu, 1, entry->cpu->gpr[2]); /* 800802B0 sb $v0, %lo(D_800D366C)($at) */
    /* 800802B4 jal func_8008AA74 */
    entry->cpu->gpr[4] = 0u | 0x5Au;
    battle_phase_call(entry, 0x8008aa74u, 0x800802b4u); /* func_8008AA74 */
    entry->cpu->gpr[6] = 0u + 0u; /* 800802BC addu $a2, $zero, $zero */
    entry->cpu->gpr[3] = entry->cpu->gpr[18] & 0xFFu; /* 800802C0 andi $v1, $s2, 0xFF */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800802C4 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800802C8 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 800802CC sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[3]; /* 800802D0 subu $v0, $v0, $v1 */
    entry->cpu->gpr[7] = entry->cpu->gpr[2] << 4; /* 800802D4 sll $a3, $v0, 4 */
    entry->cpu->gpr[5] = 0x800C0000u; /* 800802D8 lui $a1, %hi(D_800C3234) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x3234u; /* 800802DC addiu $a1, $a1, %lo(D_800C3234) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 800802E0 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 800802E4 lw $v0, %lo(D_800C3EAC)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 6; /* 800802E8 sll $v1, $v1, 6 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800802EC lui $at, %hi(D_800D366C) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x366Cu, 1, 0u); /* 800802F0 sb $zero, %lo(D_800D366C)($at) */
    entry->cpu->gpr[4] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800802F4 addu $a0, $v0, $v1 */
L_800802F8:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5], 2); /* 800802F8 lhu $v0, 0x0($a1) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x2u; /* 800802FC addiu $a1, $a1, 0x2 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080300 lui $at, %hi(D_800CCD62) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[7]; /* 80080304 addu $at, $at, $a3 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD62u, 2); /* 80080308 lhu $v1, %lo(D_800CCD62)($at) */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + 0x1u; /* 8008030C addiu $a2, $a2, 0x1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] & entry->cpu->gpr[2]; /* 80080310 and $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x1Cu, 2, entry->cpu->gpr[3]); /* 80080314 sh $v1, 0x1C($a0) */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[6] < 16); /* 80080318 slti $v0, $a2, 0x10 */
    /* 8008031C bnez $v0, .L800802F8 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x2u; if (c_) goto L_800802F8; }
    entry->cpu->gpr[4] = entry->cpu->gpr[18] & 0xFFu; /* 80080324 andi $a0, $s2, 0xFF */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080328 lui $at, %hi(D_800D2D24) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[4]; /* 8008032C addu $at, $at, $a0 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x2D24u, 1); /* 80080330 lbu $v1, %lo(D_800D2D24)($at) */
    entry->cpu->gpr[2] = 0u | 0x7u; /* 80080334 ori $v0, $zero, 0x7 */
    /* 80080338 bne $v1, $v0, .L80080384 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[5] = entry->cpu->gpr[18] & 0xFFu; if (c_) goto L_80080384; }
    entry->cpu->gpr[2] = entry->cpu->gpr[4] << 3; /* 80080340 sll $v0, $a0, 3 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080344 lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80080348 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 8008034C lbu $v0, %lo(D_800D32A1)($at) */
    ; /* 80080350 nop  */
    /* 80080354 beqz $v0, .L80080384 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[4] = entry->cpu->gpr[4] << 6; if (c_) goto L_80080384; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 8008035C lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 80080360 lw $v0, %lo(D_800C3EAC)($v0) */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80080364 lui $v1, %hi(D_800C324E) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x324Eu, 2); /* 80080368 lhu $v1, %lo(D_800C324E)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 8008036C addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x36u, 2, entry->cpu->gpr[3]); /* 80080370 sh $v1, 0x36($v0) */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80080374 lui $v1, %hi(D_800C323C) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x323Cu, 2); /* 80080378 lhu $v1, %lo(D_800C323C)($v1) */
    ; /* 8008037C nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x24u, 2, entry->cpu->gpr[3]); /* 80080380 sh $v1, 0x24($v0) */
L_80080384:
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080384 lui $at, %hi(D_800D2D24) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[5]; /* 80080388 addu $at, $at, $a1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x2D24u, 1); /* 8008038C lbu $v1, %lo(D_800D2D24)($at) */
    entry->cpu->gpr[2] = 0u | 0x4u; /* 80080390 ori $v0, $zero, 0x4 */
    /* 80080394 bne $v1, $v0, .L800804EC */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = entry->cpu->gpr[5] << 3; if (c_) goto L_800804EC; }
    entry->cpu->gpr[1] = 0x800D0000u; /* 8008039C lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800803A0 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 800803A4 lbu $v0, %lo(D_800D32A1)($at) */
    ; /* 800803A8 nop  */
    /* 800803AC bnez $v0, .L80080408 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080408; }
    entry->cpu->gpr[2] = 0x80070000u; /* 800803B4 lui $v0, %hi(g_GameState + 0x56B) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFDB9Fu, 1); /* 800803B8 lbu $v0, %lo(g_GameState + 0x56B)($v0) */
    entry->cpu->gpr[1] = 0x80070000u; /* 800803BC lui $at, %hi(g_GameState + 0x2286) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800803C0 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFF8BAu, 1); /* 800803C4 lbu $v0, %lo(g_GameState + 0x2286)($at) */
    ; /* 800803C8 nop  */
    /* 800803CC bnez $v0, .L800803EC */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[5] << 6; if (c_) goto L_800803EC; }
    entry->cpu->gpr[3] = 0x800C0000u; /* 800803D4 lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 800803D8 lw $v1, %lo(D_800C3EAC)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 800803DC lui $a0, %hi(D_800C3234) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3234u, 2); /* 800803E0 lhu $a0, %lo(D_800C3234)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800803E4 addu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x1Cu, 2, entry->cpu->gpr[4]); /* 800803E8 sh $a0, 0x1C($v1) */
L_800803EC:
    entry->cpu->gpr[2] = 0x80070000u; /* 800803EC lui $v0, %hi(g_GameState + 0x56E) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xFFFFDBA2u, 1); /* 800803F0 lbu $v0, %lo(g_GameState + 0x56E)($v0) */
    entry->cpu->gpr[1] = 0x80070000u; /* 800803F4 lui $at, %hi(g_GameState + 0x2286) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800803F8 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFF8BAu, 1); /* 800803FC lbu $v0, %lo(g_GameState + 0x2286)($at) */
    /* 80080400 j .L800804C4 */
    ; goto L_800804C4;
L_80080408:
    entry->cpu->gpr[3] = 0x80070000u; /* 80080408 lui $v1, %hi(g_GameState + 0x59C) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFDBD0u, 1); /* 8008040C lbu $v1, %lo(g_GameState + 0x59C)($v1) */
    ; /* 80080410 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; /* 80080414 sll $v0, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80080418 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 8008041C sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80080420 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80080424 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x80070000u; /* 80080428 lui $at, %hi(g_GameState + 0x97C) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 8008042C addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFDFB0u, 1); /* 80080430 lbu $v0, %lo(g_GameState + 0x97C)($at) */
    entry->cpu->gpr[1] = 0x80070000u; /* 80080434 lui $at, %hi(g_GameState + 0x22B6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80080438 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFF8EAu, 1); /* 8008043C lbu $v0, %lo(g_GameState + 0x22B6)($at) */
    ; /* 80080440 nop  */
    /* 80080444 bnez $v0, .L80080464 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[5] << 6; if (c_) goto L_80080464; }
    entry->cpu->gpr[3] = 0x800C0000u; /* 8008044C lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080450 lw $v1, %lo(D_800C3EAC)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80080454 lui $a0, %hi(D_800C3234) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3234u, 2); /* 80080458 lhu $a0, %lo(D_800C3234)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 8008045C addu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x1Cu, 2, entry->cpu->gpr[4]); /* 80080460 sh $a0, 0x1C($v1) */
L_80080464:
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080464 lui $at, %hi(D_800D2D24) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[5]; /* 80080468 addu $at, $at, $a1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x2D24u, 1); /* 8008046C lbu $v1, %lo(D_800D2D24)($at) */
    ; /* 80080470 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; /* 80080474 sll $v0, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80080478 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 8008047C sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80080480 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 80080484 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x80070000u; /* 80080488 lui $at, %hi(g_GameState + 0x30C) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 8008048C addu $at, $at, $v0 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFD940u, 1); /* 80080490 lbu $v1, %lo(g_GameState + 0x30C)($at) */
    ; /* 80080494 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; /* 80080498 sll $v0, $v1, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8008049C addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 800804A0 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800804A4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800804A8 sll $v0, $v0, 2 */
    entry->cpu->gpr[1] = 0x80070000u; /* 800804AC lui $at, %hi(g_GameState + 0x97F) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800804B0 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFDFB3u, 1); /* 800804B4 lbu $v0, %lo(g_GameState + 0x97F)($at) */
    entry->cpu->gpr[1] = 0x80070000u; /* 800804B8 lui $at, %hi(g_GameState + 0x22B6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800804BC addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFF8EAu, 1); /* 800804C0 lbu $v0, %lo(g_GameState + 0x22B6)($at) */
L_800804C4:
    ; /* 800804C4 nop  */
    /* 800804C8 bnez $v0, .L800804F0 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[3] = entry->cpu->gpr[18] & 0xFFu; if (c_) goto L_800804F0; }
    entry->cpu->gpr[2] = entry->cpu->gpr[5] << 6; /* 800804D0 sll $v0, $a1, 6 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800804D4 lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 800804D8 lw $v1, %lo(D_800C3EAC)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 800804DC lui $a0, %hi(D_800C3238) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3238u, 2); /* 800804E0 lhu $a0, %lo(D_800C3238)($a0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800804E4 addu $v1, $v1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x20u, 2, entry->cpu->gpr[4]); /* 800804E8 sh $a0, 0x20($v1) */
L_800804EC:
    entry->cpu->gpr[3] = entry->cpu->gpr[18] & 0xFFu; /* 800804EC andi $v1, $s2, 0xFF */
L_800804F0:
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 3; /* 800804F0 sll $v0, $v1, 3 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800804F4 lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800804F8 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 800804FC lbu $v0, %lo(D_800D32A1)($at) */
    ; /* 80080500 nop  */
    /* 80080504 bnez $v0, .L800805B4 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[3] << 6; if (c_) goto L_800805B4; }
    entry->cpu->gpr[4] = 0x800C0000u; /* 8008050C lui $a0, %hi(D_800C3EAC) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EACu, 4); /* 80080510 lw $a0, %lo(D_800C3EAC)($a0) */
    ; /* 80080514 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[4] + entry->cpu->gpr[2]; /* 80080518 addu $v0, $a0, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2Eu, 2); /* 8008051C lhu $v0, 0x2E($v0) */
    ; /* 80080520 nop  */
    /* 80080524 bnez $v0, .L80080544 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0u | 0x4u; if (c_) goto L_80080544; }
    entry->cpu->gpr[2] = 0u | 0x1u; /* 8008052C ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x2DDu, 1, entry->cpu->gpr[2]); /* 80080530 sb $v0, 0x2DD($a0) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80080534 lui $at, %hi(D_800C3E28) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E28u, 1, 0u); /* 80080538 sb $zero, %lo(D_800C3E28)($at) */
    /* 8008053C j .L80080558 */
    entry->cpu->gpr[2] = entry->cpu->gpr[18] & 0xFFu; goto L_80080558;
L_80080544:
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x2DDu, 1, entry->cpu->gpr[2]); /* 80080544 sb $v0, 0x2DD($a0) */
    entry->cpu->gpr[2] = 0u | 0x3u; /* 80080548 ori $v0, $zero, 0x3 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 8008054C lui $at, %hi(D_800C3E28) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E28u, 1, entry->cpu->gpr[2]); /* 80080550 sb $v0, %lo(D_800C3E28)($at) */
    entry->cpu->gpr[2] = entry->cpu->gpr[18] & 0xFFu; /* 80080554 andi $v0, $s2, 0xFF */
L_80080558:
    entry->cpu->gpr[3] = entry->cpu->gpr[2] << 1; /* 80080558 sll $v1, $v0, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 8008055C addu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 3; /* 80080560 sll $v1, $v1, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] - entry->cpu->gpr[2]; /* 80080564 subu $v1, $v1, $v0 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] << 4; /* 80080568 sll $v1, $v1, 4 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 8008056C lui $at, %hi(D_800CCD64) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[3]; /* 80080570 addu $at, $at, $v1 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCD64u, 2); /* 80080574 lhu $v0, %lo(D_800CCD64)($at) */
    ; /* 80080578 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] & 0x2u; /* 8008057C andi $v0, $v0, 0x2 */
    /* 80080580 beqz $v0, .L800805A4 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[3] = 0u | 0x2u; if (c_) goto L_800805A4; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080588 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 8008058C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80080590 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x2DDu, 1, entry->cpu->gpr[3]); /* 80080594 sb $v1, 0x2DD($v0) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 80080598 ori $v0, $zero, 0x1 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 8008059C lui $at, %hi(D_800C3E28) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E28u, 1, entry->cpu->gpr[2]); /* 800805A0 sb $v0, %lo(D_800C3E28)($at) */
L_800805A4:
    /* 800805A4 jal func_8007FEC4 */
    ;
    battle_phase_call(entry, 0x8007fec4u, 0x800805a4u); /* func_8007FEC4 */
    /* 800805AC j .L80080604 */
    ; goto L_80080604;
L_800805B4:
    entry->cpu->gpr[4] = 0x800C0000u; /* 800805B4 lui $a0, %hi(D_800C3EAC) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EACu, 4); /* 800805B8 lw $a0, %lo(D_800C3EAC)($a0) */
    ; /* 800805BC nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[4] + entry->cpu->gpr[2]; /* 800805C0 addu $v0, $a0, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2Eu, 2); /* 800805C4 lhu $v0, 0x2E($v0) */
    ; /* 800805C8 nop  */
    /* 800805CC bnez $v0, .L800805EC */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0u | 0x13u; if (c_) goto L_800805EC; }
    entry->cpu->gpr[2] = 0u | 0x10u; /* 800805D4 ori $v0, $zero, 0x10 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x2DDu, 1, entry->cpu->gpr[2]); /* 800805D8 sb $v0, 0x2DD($a0) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 800805DC lui $at, %hi(D_800C3E28) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E28u, 1, 0u); /* 800805E0 sb $zero, %lo(D_800C3E28)($at) */
    /* 800805E4 j .L800805FC */
    ; goto L_800805FC;
L_800805EC:
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x2DDu, 1, entry->cpu->gpr[2]); /* 800805EC sb $v0, 0x2DD($a0) */
    entry->cpu->gpr[2] = 0u | 0x3u; /* 800805F0 ori $v0, $zero, 0x3 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 800805F4 lui $at, %hi(D_800C3E28) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3E28u, 1, entry->cpu->gpr[2]); /* 800805F8 sb $v0, %lo(D_800C3E28)($at) */
L_800805FC:
    /* 800805FC jal func_8007FF14 */
    entry->cpu->gpr[4] = entry->cpu->gpr[18] & 0xFFu;
    battle_phase_call(entry, 0x8007ff14u, 0x800805fcu); /* func_8007FF14 */
L_80080604:
    /* 80080604 jal func_80085E78 */
    entry->cpu->gpr[19] = entry->cpu->gpr[18] & 0xFFu;
    battle_phase_call(entry, 0x80085e78u, 0x80080604u); /* func_80085E78 */
    /* 8008060C jal func_8009AB00 */
    entry->cpu->gpr[4] = entry->cpu->gpr[19] + 0u;
    battle_phase_call(entry, 0x8009ab00u, 0x8008060cu); /* func_8009AB00 */
    /* 80080614 jal func_80089C08 */
    entry->cpu->gpr[4] = entry->cpu->gpr[19] + 0u;
    battle_phase_call(entry, 0x80089c08u, 0x80080614u); /* func_80089C08 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8008061C lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080620 lw $v1, %lo(D_800C3EAC)($v1) */
    entry->cpu->gpr[17] = entry->cpu->gpr[19] << 6; /* 80080624 sll $s1, $s3, 6 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[17]; /* 80080628 addu $v1, $v1, $s1 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3Cu, 1); /* 8008062C lbu $a0, 0x3C($v1) */
    /* 80080630 jal func_80089C08 */
    entry->cpu->gpr[16] = entry->cpu->gpr[2] + 0u;
    battle_phase_call(entry, 0x80089c08u, 0x80080630u); /* func_80089C08 */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] | entry->cpu->gpr[2]; /* 80080638 or $s0, $s0, $v0 */
    /* 8008063C jal func_800BC404 */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] & 0xFFFFu;
    battle_phase_call(entry, 0x800bc404u, 0x8008063cu); /* func_800BC404 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080644 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 80080648 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 8008064C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[17]; /* 80080650 addu $v0, $v0, $s1 */
    uint32_t delayed_a0 = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3cu, 1);
    uint32_t delayed_v1 = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3cu, 1);
    entry->cpu->gpr[4] = delayed_a0;
    entry->cpu->gpr[3] = delayed_v1;
    entry->cpu->gpr[2] = 0u | 0xFFu; /* 8008065C ori $v0, $zero, 0xFF */
    /* 80080660 bne $v1, $v0, .L8008066C */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); ; if (c_) goto L_8008066C; }
    entry->cpu->gpr[4] = 0u + 0u; /* 80080668 addu $a0, $zero, $zero */
L_8008066C:
    /* 8008066C jal func_80089C08 */
    ;
    battle_phase_call(entry, 0x80089c08u, 0x8008066cu); /* func_80089C08 */
    /* 80080674 jal func_800BCD98 */
    entry->cpu->gpr[4] = entry->cpu->gpr[2] & 0xFFFFu;
    battle_phase_call(entry, 0x800bcd98u, 0x80080674u); /* func_800BCD98 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8008067C lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080680 lw $v1, %lo(D_800C3EAC)($v1) */
    ; /* 80080684 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DEu, 1); /* 80080688 lbu $v0, 0x2DE($v1) */
    ; /* 8008068C nop  */
    /* 80080690 bnez $v0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080970; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080698 lui $v0, %hi(D_800C48EA) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x48EAu, 1); /* 8008069C lbu $v0, %lo(D_800C48EA)($v0) */
    ; /* 800806A0 nop  */
    /* 800806A4 bnez $v0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080970; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DBu, 1); /* 800806AC lbu $v0, 0x2DB($v1) */
    ; /* 800806B0 nop  */
    /* 800806B4 bnez $v0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080970; }
    entry->cpu->gpr[17] = entry->cpu->gpr[19] + 0u; /* 800806BC addu $s1, $s3, $zero */
    entry->cpu->gpr[16] = 0u | 0xFFu; /* 800806C0 ori $s0, $zero, 0xFF */
    entry->cpu->gpr[2] = entry->cpu->gpr[17] << 3; /* 800806C4 sll $v0, $s1, 3 */
L_800806C8:
    entry->cpu->gpr[1] = 0x800D0000u; /* 800806C8 lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800806CC addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 800806D0 lbu $v0, %lo(D_800D32A1)($at) */
    ; /* 800806D4 nop  */
    /* 800806D8 bnez $v0, .L800806F0 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_800806F0; }
    { uint32_t first = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D5u, 1);
      uint32_t second = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D4u, 1);
      entry->cpu->gpr[4] = first; entry->cpu->gpr[5] = second; }
    /* 800806E8 jal func_8007FBE0 */
    ;
    battle_phase_call(entry, 0x8007fbe0u, 0x800806e8u); /* func_8007FBE0 */
L_800806F0:
    entry->cpu->gpr[4] = 0x800C0000u; /* 800806F0 lui $a0, %hi(D_800C3EAC) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EACu, 4); /* 800806F4 lw $a0, %lo(D_800C3EAC)($a0) */
    ; /* 800806F8 nop  */
    { uint32_t first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2E2u, 1);
      uint32_t second = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2DDu, 1);
      entry->cpu->gpr[3] = first; entry->cpu->gpr[2] = second; }
    ; /* 80080704 nop  */
    /* 80080708 beq $v1, $v0, .L80080778 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); ; if (c_) goto L_80080778; }
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x2F6u, 1, 0u); /* 80080710 sb $zero, 0x2F6($a0) */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80080714 lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080718 lw $v1, %lo(D_800C3EAC)($v1) */
    ; /* 8008071C nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DDu, 1); /* 80080720 lbu $v0, 0x2DD($v1) */
    ; /* 80080724 nop  */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x2E2u, 1, entry->cpu->gpr[2]); /* 80080728 sb $v0, 0x2E2($v1) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 8008072C lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80080730 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80080734 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xCBu, 1, 0u); /* 80080738 sb $zero, 0xCB($v0) */
    entry->cpu->gpr[2] = 0x800C0000u; /* 8008073C lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 80080740 lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80080744 nop  */
    { uint32_t first = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2DDu, 1);
      uint32_t second = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2E0u, 1);
      entry->cpu->gpr[5] = first; entry->cpu->gpr[6] = second; }
    /* 80080750 jal func_8008D598 */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + 0u;
    battle_phase_call(entry, 0x8008d598u, 0x80080750u); /* func_8008D598 */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80080758 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 8008075C lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 80080760 nop  */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0xA3u, 1, entry->cpu->gpr[2]); /* 80080764 sb $v0, 0xA3($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80080768 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 8008076C lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 80080770 ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0xCBu, 1, entry->cpu->gpr[2]); /* 80080774 sb $v0, 0xCB($v1) */
L_80080778:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80080778 lui $v0, %hi(D_800D3014) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3014u, 1); /* 8008077C lbu $v0, %lo(D_800D3014)($v0) */
    ; /* 80080780 nop  */
    /* 80080784 beq $v0, $s0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] == entry->cpu->gpr[16]); ; if (c_) goto L_80080970; }
    /* 8008078C jal func_800716D8 */
    ;
    battle_phase_call(entry, 0x800716d8u, 0x8008078cu); /* func_800716D8 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80080794 lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080798 lw $v1, %lo(D_800C3EAC)($v1) */
    ; /* 8008079C nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DBu, 1); /* 800807A0 lbu $v0, 0x2DB($v1) */
    ; /* 800807A4 nop  */
    /* 800807A8 bnez $v0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080970; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2E0u, 1); /* 800807B0 lbu $v0, 0x2E0($v1) */
    ; /* 800807B4 nop  */
    /* 800807B8 bnez $v0, .L800807C8 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0u | 0x1u; if (c_) goto L_800807C8; }
    entry->cpu->gpr[1] = 0x800D0000u; /* 800807C0 lui $at, %hi(D_800D366C) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x366Cu, 1, entry->cpu->gpr[2]); /* 800807C4 sb $v0, %lo(D_800D366C)($at) */
L_800807C8:
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DDu, 1); /* 800807C8 lbu $v0, 0x2DD($v1) */
    ; /* 800807CC nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[2] + 0xFFFFFFFFu; /* 800807D0 addiu $v1, $v0, -0x1 */
    entry->cpu->gpr[2] = (uint32_t)(entry->cpu->gpr[3] < 0x65u); /* 800807D4 sltiu $v0, $v1, 0x65 */
    /* 800807D8 beqz $v0, .L80080930 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[3] << 2; if (c_) goto L_80080930; }
    entry->cpu->gpr[1] = 0x80070000u; /* 800807E0 lui $at, %hi(jtbl_8006FE7C) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800807E4 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFFE7Cu, 4); /* 800807E8 lw $v0, %lo(jtbl_8006FE7C)($at) */
    ; /* 800807EC nop  */
    if (entry->cpu->gpr[3] == 0 && entry->cpu->gpr[2] == 0x800807f8u) {
        entry->cpu->gpr[4] = entry->cpu->gpr[18] & 0xffu;
        battle_phase_call(entry,0x8008115cu,0x800807f8u);
    } else if (entry->cpu->gpr[3] == 99 && entry->cpu->gpr[2] == 0x80080838u) {
        entry->cpu->gpr[2] = 0x800c0000u;
        entry->cpu->gpr[2] = battle_entry_read(entry,entry->cpu->gpr[2]+0x3eacu,4);
        battle_entry_write(entry,entry->cpu->gpr[2]+0x2e2u,1,entry->cpu->gpr[16]);
        entry->cpu->gpr[4] = entry->cpu->gpr[18] & 0xffu;
        battle_phase_call(entry,0x80081b58u,0x80080848u);
    } else if (entry->cpu->gpr[3] == 3 && entry->cpu->gpr[2] == 0x80080828u) {
        entry->cpu->gpr[4] = entry->cpu->gpr[18] & 0xffu;
        battle_phase_call(entry,0x800816f8u,0x80080828u);
    } else if (entry->cpu->gpr[3] == 18 && entry->cpu->gpr[2] == 0x800808c8u) {
        entry->cpu->gpr[4] = entry->cpu->gpr[18] & 0xffu;
        battle_phase_call(entry,0x80082f7cu,0x800808c8u);
    } else if (entry->cpu->gpr[2] != 0x80080930u) battle_entry_fail(entry,"unsupported menu jump",entry->cpu->gpr[2]);
L_80080930:
    entry->cpu->gpr[3] = 0x800C0000u; /* 80080930 lui $v1, %hi(D_800C3EAC) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EACu, 4); /* 80080934 lw $v1, %lo(D_800C3EAC)($v1) */
    ; /* 80080938 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DEu, 1); /* 8008093C lbu $v0, 0x2DE($v1) */
    ; /* 80080940 nop  */
    /* 80080944 bnez $v0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080970; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 8008094C lui $v0, %hi(D_800C48EA) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x48EAu, 1); /* 80080950 lbu $v0, %lo(D_800C48EA)($v0) */
    ; /* 80080954 nop  */
    /* 80080958 bnez $v0, .L80080970 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080970; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2DBu, 1); /* 80080960 lbu $v0, 0x2DB($v1) */
    ; /* 80080964 nop  */
    /* 80080968 beqz $v0, .L800806C8 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[17] << 3; if (c_) goto L_800806C8; }
L_80080970:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80080970 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80080974 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80080978 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xCBu, 1, 0u); /* 8008097C sb $zero, 0xCB($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80080980 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80080984 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80080988 nop  */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xAFu, 1, 0u); /* 8008098C sb $zero, 0xAF($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80080990 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80080994 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[3] = entry->cpu->gpr[18] & 0xFFu; /* 80080998 andi $v1, $s2, 0xFF */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 8008099C addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x90u, 1, 0u); /* 800809A0 sb $zero, 0x90($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 800809A4 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800809A8 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 800809AC nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800809B0 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x7Fu, 1, 0u); /* 800809B4 sb $zero, 0x7F($v0) */
    entry->cpu->gpr[5] = 0x800C0000u; /* 800809B8 lui $a1, %hi(D_800C3EAC) */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x3EACu, 4); /* 800809BC lw $a1, %lo(D_800C3EAC)($a1) */
    ; /* 800809C0 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[5] + 0x2E3u, 1); /* 800809C4 lbu $v0, 0x2E3($a1) */
    ; /* 800809C8 nop  */
    /* 800809CC beqz $v0, .L80080A24 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[16] = entry->cpu->gpr[18] & 0xFFu; if (c_) goto L_80080A24; }
    entry->cpu->gpr[4] = entry->cpu->gpr[3] << 3; /* 800809D4 sll $a0, $v1, 3 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800809D8 lui $at, %hi(D_800D32A0) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[4]; /* 800809DC addu $at, $at, $a0 */
    { uint32_t first = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A0u, 1); uint32_t second = battle_entry_read(entry, entry->cpu->gpr[5] + 0x2D4u, 1); entry->cpu->gpr[2] = first; entry->cpu->gpr[3] = second; }
    ; /* 800809E8 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800809EC addu $v0, $v0, $v1 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800809F0 lui $at, %hi(D_800D32A0) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[4]; /* 800809F4 addu $at, $at, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x32A0u, 1, entry->cpu->gpr[2]); /* 800809F8 sb $v0, %lo(D_800D32A0)($at) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800809FC lui $at, %hi(D_800D32A0) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[4]; /* 80080A00 addu $at, $at, $a0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A0u, 1); /* 80080A04 lbu $v0, %lo(D_800D32A0)($at) */
    ; /* 80080A08 nop  */
    entry->cpu->gpr[2] = (uint32_t)(entry->cpu->gpr[2] < 0x1Du); /* 80080A0C sltiu $v0, $v0, 0x1D */
    /* 80080A10 bnez $v0, .L80080A24 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = 0u | 0x1Cu; if (c_) goto L_80080A24; }
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080A18 lui $at, %hi(D_800D32A0) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[4]; /* 80080A1C addu $at, $at, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x32A0u, 1, entry->cpu->gpr[2]); /* 80080A20 sb $v0, %lo(D_800D32A0)($at) */
L_80080A24:
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u; /* 80080A24 addu $a0, $s0, $zero */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80080A28 lui $at, %hi(D_800D366C) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x366Cu, 1, 0u); /* 80080A2C sb $zero, %lo(D_800D366C)($at) */
    /* 80080A30 jal func_800BAF40 */
    entry->cpu->gpr[5] = 0u | 0x100u;
    battle_phase_call(entry, 0x800baf40u, 0x80080a30u); /* func_800BAF40 */
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080A38 lui $v0, %hi(D_800C3EAC) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3EACu, 4); /* 80080A3C lw $v0, %lo(D_800C3EAC)($v0) */
    ; /* 80080A40 nop  */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2DEu, 1); /* 80080A44 lbu $v0, 0x2DE($v0) */
    ; /* 80080A48 nop  */
    /* 80080A4C bnez $v0, .L80080A68 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_80080A68; }
    entry->cpu->gpr[2] = 0x800C0000u; /* 80080A54 lui $v0, %hi(D_800C48EA) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x48EAu, 1); /* 80080A58 lbu $v0, %lo(D_800C48EA)($v0) */
    ; /* 80080A5C nop  */
    /* 80080A60 beqz $v0, .L80080A8C */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[6] = entry->cpu->gpr[18] & 0xFFu; if (c_) goto L_80080A8C; }
L_80080A68:
    /* 80080A68 jal func_8007FCE8 */
    ;
    battle_phase_call(entry, 0x8007fce8u, 0x80080a68u); /* func_8007FCE8 */
    /* 80080A70 jal func_8007FDEC */
    ;
    battle_phase_call(entry, 0x8007fdecu, 0x80080a70u); /* func_8007FDEC */
    /* 80080A78 jal func_800800E8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x800800e8u, 0x80080a78u); /* func_800800E8 */
    /* 80080A80 jal func_8007FB70 */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x8007fb70u, 0x80080a80u); /* func_8007FB70 */
    entry->cpu->gpr[6] = entry->cpu->gpr[18] & 0xFFu; /* 80080A88 andi $a2, $s2, 0xFF */
L_80080A8C:
    entry->cpu->gpr[5] = 0x800D0000u; /* 80080A8C lui $a1, %hi(D_800D2E06) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x2E06u; /* 80080A90 addiu $a1, $a1, %lo(D_800D2E06) */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] << 1; /* 80080A94 sll $v0, $a2, 1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80080A98 addu $v1, $a1, $v0 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFFFEAu, 2); /* 80080A9C lhu $a0, -0x16($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80080AA0 lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80080AA4 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[5]; /* 80080AA8 addu $v0, $v0, $a1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[6]; /* 80080AAC addu $v1, $v1, $a2 */
    battle_entry_write(entry, entry->cpu->gpr[2], 2, entry->cpu->gpr[4]); /* 80080AB0 sh $a0, 0x0($v0) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 80080AB4 ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x7Cu, 1, entry->cpu->gpr[2]); /* 80080AB8 sb $v0, 0x7C($v1) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 80080ABC lui $at, %hi(D_800C4928) */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x4928u, 1, 0u); /* 80080AC0 sb $zero, %lo(D_800C4928)($at) */
    uint32_t saved_ra=battle_entry_read(entry,entry->cpu->gpr[29]+0x20u,4);
    uint32_t saved_s3=battle_entry_read(entry,entry->cpu->gpr[29]+0x1cu,4); entry->cpu->gpr[31]=saved_ra;
    uint32_t saved_s2=battle_entry_read(entry,entry->cpu->gpr[29]+0x18u,4); entry->cpu->gpr[19]=saved_s3;
    uint32_t saved_s1=battle_entry_read(entry,entry->cpu->gpr[29]+0x14u,4); entry->cpu->gpr[18]=saved_s2;
    uint32_t saved_s0=battle_entry_read(entry,entry->cpu->gpr[29]+0x10u,4); entry->cpu->gpr[17]=saved_s1;
    entry->cpu->gpr[29]+=0x28u; entry->cpu->gpr[16]=saved_s0;
}

/* Lifted calls use the synthetic halt return, not a retail call-site address. */
static int battle_command_menu_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80080160u || cpu->gpr[31] != BATTLE_HALT_PC ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0) || (cpu->gpr[4] & 0xffu) >= 11u) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    const uint8_t *table = resolve_memory(runtime, 0x8006fe7cu, 404u, 0);
    if (!table) return 0;
    const uint8_t *regions[] = {PSX_ADDR(target), table};
    const unsigned lengths[] = {0x984u, 404u};
    const char *expected[] = {
        "f76e821213b89479a46015ccd5e890d5320bd3f3f253820c1da5e7cbeb3b2bd1",
        "78991335f8bd36c403ca717188633d490d8318ecf9692d8faac6f13e14641563"
    };
    for (unsigned k = 0; k < 2; ++k) {
        uint8_t digest[32]; file1_digest(regions[k], lengths[k], digest);
        for (unsigned i = 0; i < sizeof(digest); ++i)
            if (file1_hex_digit(digest[i] >> 4) != expected[k][2*i] ||
                file1_hex_digit(digest[i] & 15u) != expected[k][2*i+1]) return 0;
    }
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_command_menu_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

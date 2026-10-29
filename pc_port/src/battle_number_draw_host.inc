static void battle_number_draw_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFD0u; /* 80073380 addiu $sp, $sp, -0x30 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[16]); /* 80073384 sw $s0, 0x10($sp) */
    entry->cpu->gpr[16] = entry->cpu->gpr[4] + 0u; /* 80073388 addu $s0, $a0, $zero */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 4; /* 8007338C sll $v0, $s0, 4 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[16]; /* 80073390 subu $v0, $v0, $s0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80073394 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073398 addu $v0, $v0, $s0 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8007339C lui $v1, %hi(D_800C3EA4) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3EA4u, 4); /* 800733A0 lw $v1, %lo(D_800C3EA4)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 2; /* 800733A4 sll $v0, $v0, 2 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x28u, 4, entry->cpu->gpr[31]); /* 800733A8 sw $ra, 0x28($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x24u, 4, entry->cpu->gpr[21]); /* 800733AC sw $s5, 0x24($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x20u, 4, entry->cpu->gpr[20]); /* 800733B0 sw $s4, 0x20($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[19]); /* 800733B4 sw $s3, 0x1C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[18]); /* 800733B8 sw $s2, 0x18($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[17]); /* 800733BC sw $s1, 0x14($sp) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[2]; /* 800733C0 addu $v1, $v1, $v0 */
    entry->cpu->gpr[1] = 0x10000u; /* 800733C4 lui $at, (0x10000 >> 16) */
    entry->cpu->gpr[1] = entry->cpu->gpr[3] + entry->cpu->gpr[1]; /* 800733C8 addu $at, $v1, $at */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFF853Du, 1); /* 800733CC lbu $v1, -0x7AC3($at) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 800733D0 ori $v0, $zero, 0x1 */
    /* 800733D4 bne $v1, $v0, .L800733F4 */
    { int c_ = (entry->cpu->gpr[3] != entry->cpu->gpr[2]); entry->cpu->gpr[2] = entry->cpu->gpr[16] << 1; if (c_) goto L_800733F4; }
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 3; /* 800733DC sll $v0, $s0, 3 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 800733E0 lui $at, %hi(D_800D32A0) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800733E4 addu $at, $at, $v0 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A0u, 1); /* 800733E8 lbu $a0, %lo(D_800D32A0)($at) */
    /* 800733EC j .L80073418 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 16; goto L_80073418;
L_800733F4:
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 800733F4 addu $v0, $v0, $s0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 800733F8 sll $v0, $v0, 3 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] - entry->cpu->gpr[16]; /* 800733FC subu $v0, $v0, $s0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 4; /* 80073400 sll $v0, $v0, 4 */
    entry->cpu->gpr[1] = 0x800D0000u; /* 80073404 lui $at, %hi(D_800CCDC4) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80073408 addu $at, $at, $v0 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0xFFFFCDC4u, 2); /* 8007340C lhu $a0, %lo(D_800CCDC4)($at) */
    ; /* 80073410 nop  */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] << 16; /* 80073414 sll $a0, $a0, 16 */
L_80073418:
    /* 80073418 jal func_8008AAA0 */
    entry->cpu->gpr[4] = ((uint32_t)((int32_t)entry->cpu->gpr[4] >> 16));
    battle_phase_call(entry, 0x8008AAA0u, 0x80073418u);
    entry->cpu->gpr[17] = 0u + 0u; /* 80073420 addu $s1, $zero, $zero */
    entry->cpu->gpr[19] = entry->cpu->gpr[16] << 2; /* 80073424 sll $s3, $s0, 2 */
    entry->cpu->gpr[2] = entry->cpu->gpr[19] + entry->cpu->gpr[16]; /* 80073428 addu $v0, $s3, $s0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 6; /* 8007342C sll $v0, $v0, 6 */
    entry->cpu->gpr[21] = entry->cpu->gpr[2] + 0x6008u; /* 80073430 addiu $s5, $v0, 0x6008 */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 1; /* 80073434 sll $v0, $s0, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073438 addu $v0, $v0, $s0 */
    entry->cpu->gpr[20] = entry->cpu->gpr[2] << 5; /* 8007343C sll $s4, $v0, 5 */
    entry->cpu->gpr[18] = 0u + 0u; /* 80073440 addu $s2, $zero, $zero */
L_80073444:
    entry->cpu->gpr[1] = 0x800C0000u; /* 80073444 lui $at, %hi(D_800C3CF9) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[17]; /* 80073448 addu $at, $at, $s1 */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3CF9u, 1); /* 8007344C lbu $a0, %lo(D_800C3CF9)($at) */
    entry->cpu->gpr[2] = 0u | 0xFFu; /* 80073450 ori $v0, $zero, 0xFF */
    /* 80073454 beq $a0, $v0, .L800734EC */
    { int c_ = (entry->cpu->gpr[4] == entry->cpu->gpr[2]); entry->cpu->gpr[17] = entry->cpu->gpr[17] + 0x1u; if (c_) goto L_800734EC; }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x83u; /* 8007345C addiu $a0, $a0, 0x83 */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80073460 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80073464 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[7] = 0x800C0000u; /* 80073468 lui $a3, %hi(D_800C3EA4) */
    entry->cpu->gpr[7] = battle_entry_read(entry, entry->cpu->gpr[7] + 0x3EA4u, 4); /* 8007346C lw $a3, %lo(D_800C3EA4)($a3) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073470 lui $v1, %hi(D_800D3280) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3280u, 1); /* 80073474 lbu $v1, %lo(D_800D3280)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[19] + entry->cpu->gpr[2]; /* 80073478 addu $v0, $s3, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xECu, 4); /* 8007347C lw $v0, 0xEC($v0) */
    entry->cpu->gpr[7] = entry->cpu->gpr[21] + entry->cpu->gpr[7]; /* 80073480 addu $a3, $s5, $a3 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 2; /* 80073484 sll $a1, $v0, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80073488 addu $a1, $a1, $v0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 8007348C sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073490 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073494 addu $v0, $v0, $s0 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 80073498 sll $v0, $v0, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 4; /* 8007349C sll $a1, $a1, 4 */
    entry->cpu->gpr[5] = entry->cpu->gpr[7] + entry->cpu->gpr[5]; /* 800734A0 addu $a1, $a3, $a1 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 800734A4 lui $at, %hi(D_800C3254) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 800734A8 addu $at, $at, $v0 */
    entry->cpu->gpr[6] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3254u, 2); /* 800734AC lhu $a2, %lo(D_800C3254)($at) */
    entry->cpu->gpr[7] = 0u | 0x25u; /* 800734B0 ori $a3, $zero, 0x25 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + 0x4Au; /* 800734B4 addiu $a2, $a2, 0x4A */
    entry->cpu->gpr[6] = entry->cpu->gpr[20] + entry->cpu->gpr[6]; /* 800734B8 addu $a2, $s4, $a2 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] + entry->cpu->gpr[18]; /* 800734BC addu $a2, $a2, $s2 */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] << 16; /* 800734C0 sll $a2, $a2, 16 */
    /* 800734C4 jal func_80076A10 */
    entry->cpu->gpr[6] = ((uint32_t)((int32_t)entry->cpu->gpr[6] >> 16));
    battle_phase_call(entry, 0x80076A10u, 0x800734c4u);
    entry->cpu->gpr[4] = 0x800D0000u; /* 800734CC lui $a0, %hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 800734D0 lw $a0, %lo(D_800D2D28)($a0) */
    ; /* 800734D4 nop  */
    entry->cpu->gpr[4] = entry->cpu->gpr[19] + entry->cpu->gpr[4]; /* 800734D8 addu $a0, $s3, $a0 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0xECu, 4); /* 800734DC lw $v1, 0xEC($a0) */
    ; /* 800734E0 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800734E4 addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0xECu, 4, entry->cpu->gpr[2]); /* 800734E8 sw $v0, 0xEC($a0) */
L_800734EC:
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[17] < 4); /* 800734EC slti $v0, $s1, 0x4 */
    /* 800734F0 bnez $v0, .L80073444 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x6u; if (c_) goto L_80073444; }
    entry->cpu->gpr[2] = 0x800D0000u; /* 800734F8 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 800734FC lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073500 lui $v1, %hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 1); /* 80073504 lbu $v1, %lo(D_800CCB34)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80073508 addu $v0, $v0, $s0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x99u, 1, entry->cpu->gpr[3]); /* 8007350C sb $v1, 0x99($v0) */
    /* Commit each delayed load after its successor succeeds. */
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x28u, 4);
    uint32_t saved_s5 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x24u, 4);
    entry->cpu->gpr[31] = saved_ra;
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
    entry->cpu->gpr[29] += 0x30u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80073530 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental child of target processing; both services remain guarded. */
static int battle_number_draw_try_host(BattleMipsRuntime *runtime,
                                       PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80073380u || cpu->gpr[31] != 0x80074308u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_NUMBER_DRAW_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "a0142ff326bff2990d7af1560c8d7207387c3a8697ba4faa050b0dee0661939a";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 440u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_number_draw_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

static int battle_number_draw_unconverted(uint32_t target, uint32_t ra)
{
    return (target == 0x8008aaa0u && ra == 0x80073420u) ||
           (target == 0x80076a10u && ra == 0x800734ccu);
}

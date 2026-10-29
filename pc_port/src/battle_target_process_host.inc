static void battle_target_process_run(BattleEntryCall *entry) {
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFE0u; /* 800742A0 addiu $sp, $sp, -0x20 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[16]); /* 800742A4 sw $s0, 0x10($sp) */
    entry->cpu->gpr[16] = 0u + 0u; /* 800742A8 addu $s0, $zero, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[18]); /* 800742AC sw $s2, 0x18($sp) */
    entry->cpu->gpr[18] = 0u + 0u; /* 800742B0 addu $s2, $zero, $zero */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x1Cu, 4, entry->cpu->gpr[31]); /* 800742B4 sw $ra, 0x1C($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[17]); /* 800742B8 sw $s1, 0x14($sp) */
L_800742BC:
    entry->cpu->gpr[3] = 0x800D0000u; /* 800742BC lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 800742C0 lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 800742C4 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] + entry->cpu->gpr[16]; /* 800742C8 addu $v0, $v1, $s0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x7Cu, 1); /* 800742CC lbu $v0, 0x7C($v0) */
    ; /* 800742D0 nop  */
    /* 800742D4 beqz $v0, .L80074378 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = entry->cpu->gpr[16] << 2; if (c_) goto L_80074378; }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800742DC addu $v0, $v0, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xE0u, 4, 0u); /* 800742E0 sw $zero, 0xE0($v0) */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xECu, 4, 0u); /* 800742E4 sw $zero, 0xEC($v0) */
    entry->cpu->gpr[1] = 0x800C0000u; /* 800742E8 lui $at, %hi(D_800C3EB6) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[18]; /* 800742EC addu $at, $at, $s2 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3EB6u, 1); /* 800742F0 lbu $v1, %lo(D_800C3EB6)($at) */
    entry->cpu->gpr[2] = 0u | 0x7Fu; /* 800742F4 ori $v0, $zero, 0x7F */
    /* 800742F8 beq $v1, $v0, .L80074378 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[17] = entry->cpu->gpr[16] << 3; if (c_) goto L_80074378; }
    /* 80074300 jal func_80073380 */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x80073380u, 0x80074300u);
    entry->cpu->gpr[1] = 0x800D0000u; /* 80074308 lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[17]; /* 8007430C addu $at, $at, $s1 */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 80074310 lbu $a1, %lo(D_800D32A1)($at) */
    /* 80074314 jal func_80072F38 */
    entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u;
    battle_phase_call(entry, 0x80072F38u, 0x80074314u);
    entry->cpu->gpr[1] = 0x800D0000u; /* 8007431C lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[17]; /* 80074320 addu $at, $at, $s1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 80074324 lbu $v1, %lo(D_800D32A1)($at) */
    ; /* 80074328 nop  */
    /* 8007432C beqz $v1, .L80074344 */
    { int c_ = (entry->cpu->gpr[3] == 0); entry->cpu->gpr[4] = entry->cpu->gpr[16] + 0u; if (c_) goto L_80074344; }
    /* 80074334 jal func_80072DA8 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] & 0xFFu;
    battle_phase_call(entry, 0x80072DA8u, 0x80074334u);
    /* 8007433C j .L8007434C */
    ; goto L_8007434C;
L_80074344:
    /* 80074344 jal func_80072A9C */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] & 0xFFu;
    battle_phase_call(entry, 0x80072A9Cu, 0x80074344u);
L_8007434C:
    entry->cpu->gpr[2] = 0x800D0000u; /* 8007434C lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80074350 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80074354 lui $v1, %hi(D_800CCB34) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0xFFFFCB34u, 1); /* 80074358 lbu $v1, %lo(D_800CCB34)($v1) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 8007435C addu $v0, $v0, $s0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x93u, 1, entry->cpu->gpr[3]); /* 80074360 sb $v1, 0x93($v0) */
    entry->cpu->gpr[2] = 0x800D0000u; /* 80074364 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80074368 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 8007436C nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[16]; /* 80074370 addu $v0, $v0, $s0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x7Cu, 1, 0u); /* 80074374 sb $zero, 0x7C($v0) */
L_80074378:
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u; /* 80074378 addiu $s0, $s0, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < 3); /* 8007437C slti $v0, $s0, 0x3 */
    /* 80074380 bnez $v0, .L800742BC */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[18] = entry->cpu->gpr[18] + 0x1Cu; if (c_) goto L_800742BC; }
    /* Delay each load's register commit until its following instruction succeeds. */
    uint32_t ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x1cu, 4);
    uint32_t s2 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x18u, 4);
    entry->cpu->gpr[31] = ra;
    uint32_t s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x14u, 4);
    entry->cpu->gpr[18] = s2;
    uint32_t s0 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x10u, 4);
    entry->cpu->gpr[17] = s1;
    entry->cpu->gpr[29] += 0x20u;
    entry->cpu->gpr[16] = s0;
    /* 8007439C jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Opt-in experiment at the recorded phase-chain edge only. Four outgoing
 * callees remain separate fail-closed boundaries until individually audited. */
static int battle_target_process_try_host(BattleMipsRuntime *runtime,
                                           PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x800742a0u || cpu->gpr[31] != 0x8007643cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_TARGET_PROCESS_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "0989526e7ee13c9431e788b682e798148e3c512d56df41dc4e7a84bfab6b84e6";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 260u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_target_process_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

static int battle_target_process_unconverted(uint32_t target, uint32_t ra)
{
    return (target == 0x80073380u && ra == 0x80074308u) ||
           (target == 0x80072f38u && ra == 0x8007431cu) ||
           (target == 0x80072da8u && ra == 0x8007433cu) ||
           (target == 0x80072a9cu && ra == 0x8007434cu);
}

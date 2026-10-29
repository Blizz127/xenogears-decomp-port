static void battle_optional_panel_run(BattleEntryCall *entry) {
    entry->cpu->gpr[2] = 0x800D0000u; /* 80073E88 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80073E8C lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFE0u; /* 80073E90 addiu $sp, $sp, -0x20 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x18u, 4, entry->cpu->gpr[31]); /* 80073E94 sw $ra, 0x18($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[17]); /* 80073E98 sw $s1, 0x14($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[16]); /* 80073E9C sw $s0, 0x10($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xCBu, 1); /* 80073EA0 lbu $v0, 0xCB($v0) */
    ; /* 80073EA4 nop  */
    /* 80073EA8 beqz $v0, .L80073EF0 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[16] = 0u + 0u; if (c_) goto L_80073EF0; }
    entry->cpu->gpr[17] = 0u | 0x641Cu; /* 80073EB0 ori $s1, $zero, 0x641C */
    entry->cpu->gpr[2] = entry->cpu->gpr[16] << 2; /* 80073EB4 sll $v0, $s0, 2 */
L_80073EB8:
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0x1u; /* 80073EB8 addiu $s0, $s0, 0x1 */
    entry->cpu->gpr[3] = 0x800D0000u; /* 80073EBC lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 80073EC0 lw $v1, %lo(D_800D2D28)($v1) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073EC4 lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073EC8 lw $a0, %lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073ECC addu $v0, $v0, $v1 */
    entry->cpu->gpr[4] = entry->cpu->gpr[17] + entry->cpu->gpr[4]; /* 80073ED0 addu $a0, $s1, $a0 */
    {
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[2] + 0xD0u, 4); /* 80073ED4 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[3] + 0xA3u, 1); /* 80073ED8 */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80073EDC jal func_800728B8 */
    entry->cpu->gpr[17] = entry->cpu->gpr[17] + 0xFA0u;
    battle_phase_call(entry, 0x800728B8u, 0x80073edcu);
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[16] < 2); /* 80073EE4 slti $v0, $s0, 0x2 */
    /* 80073EE8 bnez $v0, .L80073EB8 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[2] = entry->cpu->gpr[16] << 2; if (c_) goto L_80073EB8; }
L_80073EF0:
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x18u, 4);
    uint32_t saved_s1 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x14u, 4);
    entry->cpu->gpr[31] = saved_ra;
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x10u, 4);
    entry->cpu->gpr[17] = saved_s1;
    entry->cpu->gpr[29] += 0x20u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80073F00 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental optional panel; its strip child remains separately guarded. */
static int battle_optional_panel_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80073e88u || cpu->gpr[31] != 0x80076464u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_OPTIONAL_PANEL_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "0453e8b08866c7a70f0f14c69ad3a594eae8b943b01ec02ba419e45abb17b483";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x80u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_optional_panel_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

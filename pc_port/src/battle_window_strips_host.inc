static void battle_window_strips_run(BattleEntryCall *entry) {
    entry->cpu->gpr[2] = 0x800D0000u; /* 80074EEC lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80074EF0 lw $v0, %lo(D_800D2D28)($v0) */
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFE8u; /* 80074EF4 addiu $sp, $sp, -0x18 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[31]); /* 80074EF8 sw $ra, 0x10($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0xA8u, 1); /* 80074EFC lbu $v0, 0xA8($v0) */
    ; /* 80074F00 nop  */
    /* 80074F04 beqz $v0, .L80074F60 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_80074F60; }
    entry->cpu->gpr[4] = 0x800D0000u; /* 80074F0C lui $a0, %hi(D_800D2DB4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2DB4u, 4); /* 80074F10 lw $a0, %lo(D_800D2DB4)($a0) */
    ; /* 80074F14 nop  */
    {
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x5D80u, 1); /* 80074F18 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[4] + 0x5D8Fu, 1); /* 80074F1C */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80074F20 jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x5550u;
    battle_phase_call(entry, 0x800728B8u, 0x80074f20u);
    entry->cpu->gpr[4] = 0x800D0000u; /* 80074F28 lui $a0, %hi(D_800D2DB4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2DB4u, 4); /* 80074F2C lw $a0, %lo(D_800D2DB4)($a0) */
    ; /* 80074F30 nop  */
    {
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x5D81u, 1); /* 80074F34 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[4] + 0x5D90u, 1); /* 80074F38 */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80074F3C jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x5640u;
    battle_phase_call(entry, 0x800728B8u, 0x80074f3cu);
    entry->cpu->gpr[4] = 0x800D0000u; /* 80074F44 lui $a0, %hi(D_800D2DB4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2DB4u, 4); /* 80074F48 lw $a0, %lo(D_800D2DB4)($a0) */
    ; /* 80074F4C nop  */
    {
        uint32_t pending_first = battle_entry_read(entry, entry->cpu->gpr[4] + 0x5D82u, 1); /* 80074F50 */
        uint32_t pending_second = battle_entry_read(entry, entry->cpu->gpr[4] + 0x5D91u, 1); /* 80074F54 */
        entry->cpu->gpr[5] = pending_first;
        entry->cpu->gpr[6] = pending_second;
    }
    /* 80074F58 jal func_800728B8 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0x5C80u;
    battle_phase_call(entry, 0x800728B8u, 0x80074f58u);
L_80074F60:
    entry->cpu->gpr[31] = battle_entry_read(entry, entry->cpu->gpr[29] + 0x10u, 4); /* 80074F60 lw $ra, 0x10($sp) */
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0x18u; /* 80074F64 addiu $sp, $sp, 0x18 */
    /* 80074F68 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental window strips; child strip calls remain separately guarded. */
static int battle_window_strips_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80074eecu || cpu->gpr[31] != 0x8007647cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_WINDOW_STRIPS_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "4997951cc181b668228f3f050454424234fd46c8fd521ebd50bb74fbd80ae4b0";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x84u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_window_strips_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

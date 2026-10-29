static void battle_popup_run(BattleEntryCall *entry) {
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073A58 lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073A5C lw $a0, %lo(D_800C3EA4)($a0) */
    entry->cpu->gpr[29] = entry->cpu->gpr[29] + 0xFFFFFFE8u; /* 80073A60 addiu $sp, $sp, -0x18 */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x14u, 4, entry->cpu->gpr[31]); /* 80073A64 sw $ra, 0x14($sp) */
    battle_entry_write(entry, entry->cpu->gpr[29] + 0x10u, 4, entry->cpu->gpr[16]); /* 80073A68 sw $s0, 0x10($sp) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6415u, 1); /* 80073A6C lbu $v0, 0x6415($a0) */
    ; /* 80073A70 nop  */
    /* 80073A74 beqz $v0, .L80073B50 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_80073B50; }
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6414u, 1); /* 80073A7C lbu $v1, 0x6414($a0) */
    ; /* 80073A80 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80073A84 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073A88 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80073A8C sll $v0, $v0, 3 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6410u, 1); /* 80073A90 lbu $v1, 0x6410($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80073A94 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x63CCu, 1, entry->cpu->gpr[3]); /* 80073A98 sb $v1, 0x63CC($v0) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073A9C lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073AA0 lw $a0, %lo(D_800C3EA4)($a0) */
    ; /* 80073AA4 nop  */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6414u, 1); /* 80073AA8 lbu $v1, 0x6414($a0) */
    ; /* 80073AAC nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80073AB0 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073AB4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80073AB8 sll $v0, $v0, 3 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6410u, 1); /* 80073ABC lbu $v1, 0x6410($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80073AC0 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x63CDu, 1, entry->cpu->gpr[3]); /* 80073AC4 sb $v1, 0x63CD($v0) */
    entry->cpu->gpr[4] = 0x800C0000u; /* 80073AC8 lui $a0, %hi(D_800C3EA4) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x3EA4u, 4); /* 80073ACC lw $a0, %lo(D_800C3EA4)($a0) */
    ; /* 80073AD0 nop  */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6414u, 1); /* 80073AD4 lbu $v1, 0x6414($a0) */
    entry->cpu->gpr[16] = 0x800D0000u; /* 80073AD8 lui $s0, %hi(D_800CCB04) */
    entry->cpu->gpr[16] = entry->cpu->gpr[16] + 0xFFFFCB04u; /* 80073ADC addiu $s0, $s0, %lo(D_800CCB04) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 80073AE0 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 80073AE4 addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 3; /* 80073AE8 sll $v0, $v0, 3 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x6410u, 1); /* 80073AEC lbu $v1, 0x6410($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[4]; /* 80073AF0 addu $v0, $v0, $a0 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x63CEu, 1, entry->cpu->gpr[3]); /* 80073AF4 sb $v1, 0x63CE($v0) */
    entry->cpu->gpr[3] = 0x800C0000u; /* 80073AF8 lui $v1, %hi(D_800C3EA4) */
    { /* Preserve delayed loads if a following read fails. */
        uint32_t pending_v1 = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3ea4u, 4);
        uint32_t pending_a0 = battle_entry_read(entry, entry->cpu->gpr[16], 4);
        entry->cpu->gpr[3] = pending_v1;
        uint32_t pending_v0 = battle_entry_read(entry, entry->cpu->gpr[3] + 0x6414u, 1);
        entry->cpu->gpr[4] = pending_a0 + 4u;
        entry->cpu->gpr[2] = pending_v0;
    }
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 1; /* 80073B0C sll $a1, $v0, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80073B10 addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 3; /* 80073B14 sll $a1, $a1, 3 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x63C8u; /* 80073B18 addiu $a1, $a1, 0x63C8 */
    /* 80073B1C jal AddPrim */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[3];
    battle_phase_call(entry, 0x80043B48u, 0x80073b1cu);
    entry->cpu->gpr[3] = 0x800C0000u; /* 80073B24 lui $v1, %hi(D_800C3EA4) */
    { /* Preserve delayed loads if a following read fails. */
        uint32_t pending_v1 = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3ea4u, 4);
        uint32_t pending_a0 = battle_entry_read(entry, entry->cpu->gpr[16], 4);
        entry->cpu->gpr[3] = pending_v1;
        uint32_t pending_v0 = battle_entry_read(entry, entry->cpu->gpr[3] + 0x6414u, 1);
        entry->cpu->gpr[4] = pending_a0 + 4u;
        entry->cpu->gpr[2] = pending_v0;
    }
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 1; /* 80073B38 sll $a1, $v0, 1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80073B3C addu $a1, $a1, $v0 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 2; /* 80073B40 sll $a1, $a1, 2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x63F8u; /* 80073B44 addiu $a1, $a1, 0x63F8 */
    /* 80073B48 jal AddPrim */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + entry->cpu->gpr[3];
    battle_phase_call(entry, 0x80043B48u, 0x80073b48u);
L_80073B50:
    uint32_t saved_ra = battle_entry_read(entry, entry->cpu->gpr[29] + 0x14u, 4);
    uint32_t saved_s0 = battle_entry_read(entry, entry->cpu->gpr[29] + 0x10u, 4);
    entry->cpu->gpr[31] = saved_ra;
    entry->cpu->gpr[29] += 0x18u;
    entry->cpu->gpr[16] = saved_s0;
    /* 80073B5C jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental popup entry; both AddPrim calls remain separate boundaries. */
static int battle_popup_try_host(BattleMipsRuntime *runtime,
                                PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80073a58u || cpu->gpr[31] != 0x80076454u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_POPUP_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "31f471576cc5dd1449ecf5d305dc16f1b0cb8916a88b57723d9c21ffde30871b";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x10cu, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_popup_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

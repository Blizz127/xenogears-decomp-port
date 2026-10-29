static void battle_digits_run(BattleEntryCall *entry) {
    entry->cpu->gpr[6] = 0x5F50000u; /* 8008AAA0 lui $a2, (0x5F5E100 >> 16) */
    entry->cpu->gpr[6] = entry->cpu->gpr[6] | 0xE100u; /* 8008AAA4 ori $a2, $a2, (0x5F5E100 & 0xFFFF) */
    entry->cpu->gpr[5] = 0u + 0u; /* 8008AAA8 addu $a1, $zero, $zero */
    entry->cpu->gpr[7] = 0xCCCC0000u; /* 8008AAAC lui $a3, (0xCCCCCCCD >> 16) */
    entry->cpu->gpr[7] = entry->cpu->gpr[7] | 0xCCCDu; /* 8008AAB0 ori $a3, $a3, (0xCCCCCCCD & 0xFFFF) */
L_8008AAB4:
    { uint32_t lhs = entry->cpu->gpr[4], rhs = entry->cpu->gpr[6]; entry->cpu->lo = rhs ? lhs / rhs : UINT32_MAX; entry->cpu->hi = rhs ? lhs % rhs : lhs; } /* 8008AAB4 divu $zero, $a0, $a2 */
    entry->cpu->gpr[3] = entry->cpu->lo; /* 8008AAB8 mflo $v1 */
    entry->cpu->gpr[4] = entry->cpu->hi; /* 8008AABC mfhi $a0 */
    ; /* 8008AAC0 nop  */
    ; /* 8008AAC4 nop  */
    { uint64_t product = (uint64_t)entry->cpu->gpr[6] * entry->cpu->gpr[7]; entry->cpu->lo = (uint32_t)product; entry->cpu->hi = (uint32_t)(product >> 32); } /* 8008AAC8 multu $a2, $a3 */
    entry->cpu->gpr[2] = entry->cpu->hi; /* 8008AACC mfhi $v0 */
    entry->cpu->gpr[6] = entry->cpu->gpr[2] >> 3; /* 8008AAD0 srl $a2, $v0, 3 */
    entry->cpu->gpr[1] = 0x800C0000u; /* 8008AAD4 lui $at, %entry->cpu->hi(D_800C3CF4) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[5]; /* 8008AAD8 addu $at, $at, $a1 */
    battle_entry_write(entry, entry->cpu->gpr[1] + 0x3CF4u, 1, entry->cpu->gpr[3]); /* 8008AADC sb $v1, %entry->cpu->lo(D_800C3CF4)($at) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x1u; /* 8008AAE0 addiu $a1, $a1, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[5] < 9); /* 8008AAE4 slti $v0, $a1, 0x9 */
    /* 8008AAE8 bnez $v0, .L8008AAB4 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_8008AAB4; }
    entry->cpu->gpr[5] = 0u | 0x1u; /* 8008AAF0 ori $a1, $zero, 0x1 */
    entry->cpu->gpr[4] = 0u | 0xFFu; /* 8008AAF4 ori $a0, $zero, 0xFF */
    entry->cpu->gpr[3] = 0x800C0000u; /* 8008AAF8 lui $v1, %entry->cpu->hi(D_800C3CF4) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x3CF4u; /* 8008AAFC addiu $v1, $v1, %entry->cpu->lo(D_800C3CF4) */
L_8008AB00:
    entry->cpu->gpr[1] = 0x800C0000u; /* 8008AB00 lui $at, %entry->cpu->hi(D_800C3CF4) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[5]; /* 8008AB04 addu $at, $at, $a1 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x3CF4u, 1); /* 8008AB08 lbu $v0, %entry->cpu->lo(D_800C3CF4)($at) */
    ; /* 8008AB0C nop  */
    /* 8008AB10 beqz $v0, .L8008AB30 */
    { int c_ = (entry->cpu->gpr[2] == 0); ; if (c_) goto L_8008AB30; }
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3], 1); /* 8008AB18 lbu $v0, 0x0($v1) */
    ; /* 8008AB1C nop  */
    /* 8008AB20 bnez $v0, .L8008AB44 */
    { int c_ = (entry->cpu->gpr[2] != 0); ; if (c_) goto L_8008AB44; }
    /* 8008AB28 j .L8008AB44 */
    battle_entry_write(entry, entry->cpu->gpr[3], 1, entry->cpu->gpr[4]); goto L_8008AB44;
L_8008AB30:
    battle_entry_write(entry, entry->cpu->gpr[3], 1, entry->cpu->gpr[4]); /* 8008AB30 sb $a0, 0x0($v1) */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] + 0x1u; /* 8008AB34 addiu $a1, $a1, 0x1 */
    entry->cpu->gpr[2] = (uint32_t)((int32_t)entry->cpu->gpr[5] < 9); /* 8008AB38 slti $v0, $a1, 0x9 */
    /* 8008AB3C bnez $v0, .L8008AB00 */
    { int c_ = (entry->cpu->gpr[2] != 0); entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x1u; if (c_) goto L_8008AB00; }
L_8008AB44:
    /* 8008AB44 jr $ra */
    ; goto bl_exit;
bl_exit:
}

/* Experimental digit splitter at the 73380 call site. */
static int battle_digits_try_host(BattleMipsRuntime *runtime,
                                   PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x8008aaa0u || cpu->gpr[31] != 0x80073420u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_DIGITS_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "5c2a4c312491450a45a1f206af504bbbeb407d1421244a24e27dc0549af65214";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 172u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_digits_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

/* Scratch-only: disabled indicator path. Active path remains unsupported. */
static void battle_indicator_inactive_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[2] = 0x800d0000u;
    uint32_t owner = battle_entry_read(entry, r[2] + 0x2d28u, 4);
    r[29] -= 0x18u;
    r[2] = owner;
    battle_entry_write(entry, r[29] + 0x14u, 4, r[31]);
    battle_entry_write(entry, r[29] + 0x10u, 4, r[16]);
    r[2] = battle_entry_read(entry, r[2] + 0xc6u, 1);
    if (r[2]) battle_entry_fail(entry, "active indicator unsupported", 0x80074d4cu);
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x14u, 4);
    uint32_t saved_s0 = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[31] = saved_ra;
    r[29] += 0x18u;
    r[16] = saved_s0;
}

/* Only the disabled indicator is supported; active states decline untouched. */
static int battle_indicator_inactive_try_host(BattleMipsRuntime *runtime,
                                             PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x80074d4cu || cpu->gpr[31] != 0x8007648cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_INACTIVE_INDICATOR_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "35d0fd4edda5001d41d60d5c8ca525cb70491750a33ad19d5ad88c29ae44e407";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x1a0u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    uint32_t owner, state;
    if (runtime_read(runtime, 0x800d2d28u, 4, &owner) != 0 ||
        runtime_read(runtime, owner + 0xc6u, 1, &state) != 0 || state != 0) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_indicator_inactive_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

/* Inactive selector: states 1..5 are deliberately unsupported. */
static void battle_panel_inactive_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[2]=0x800d0000u;
    uint32_t owner=battle_entry_read(entry,r[2]+0x2d28u,4);
    r[29]-=0x18u;
    r[2]=owner;
    battle_entry_write(entry,r[29]+0x14u,4,r[31]);
    battle_entry_write(entry,r[29]+0x10u,4,r[16]);
    r[2]=battle_entry_read(entry,r[2]+0xb7u,1);
    r[3]=r[2]-1u;
    r[2]=r[3]<5u;
    int unsupported=r[2]!=0;
    r[2]=r[3]<<2;
    if(unsupported) battle_entry_fail(entry,"active panel unsupported",0x800745ecu);
    uint32_t saved_ra=battle_entry_read(entry,r[29]+0x14u,4);
    uint32_t saved_s0=battle_entry_read(entry,r[29]+0x10u,4);
    r[31]=saved_ra;
    r[29]+=0x18u;
    r[16]=saved_s0;
}

/* Only the no-panel states are supported. All active menu states decline. */
static int battle_panel_inactive_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x800745ecu || cpu->gpr[31] != 0x80076484u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_INACTIVE_PANEL_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "79d9a8afc79bcfcdfad5da8e0523ae78342937170833fc87d60dc2d9edb9c3fd";
    uint8_t digest[32]; file1_digest(PSX_ADDR(target), 0x4ccu, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    uint32_t owner, state;
    if (runtime_read(runtime, 0x800d2d28u, 4, &owner) != 0 ||
        runtime_read(runtime, owner + 0xb7u, 1, &state) != 0 ||
        state - 1u < 5u) return 0;
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    battle_panel_inactive_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

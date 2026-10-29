/* Native phase selector and three
 * render-chain callers. Original CPU bytes are used only for identity gates,
 * never fetched/decoded/executed here. External callees keep their own native
 * conversion boundaries. Guest frames, return sites, volatile registers and
 * ordered memory effects remain observable across those boundaries. */
static void battle_phase_call(BattleEntryCall *entry, uint32_t target,
                              uint32_t site)
{
    PcPortMipsCpu *cpu = entry->cpu;
    cpu->gpr[31] = site + 8u;
    PcPortMipsCpu callee = *cpu;
    cpu_meter_mark_entry(&callee, target);
    int rc = PcPortMipsRun(&callee, target, site + 8u, battle_step_limit());
    /* Preserve committed effects even if the child refuses after doing work.
     * This does not promise restart from an instruction-level fault pipeline. */
    memcpy(cpu->gpr, callee.gpr, sizeof(callee.gpr));
    memcpy(cpu->cp0, callee.cp0, sizeof(callee.cp0));
    cpu->hi = callee.hi;
    cpu->lo = callee.lo;
    cpu->steps = callee.steps;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "[xeno-port][battle-phase] callee 0x%08x from 0x%08x: %s\n",
                target, site, callee.error);
        battle_entry_fail(entry, "phase callee stopped", target);
    }
    if (callee.load_valid && callee.load_reg != 0)
        battle_entry_fail(entry, "pending phase callee return", target);
}

static void battle_phase_return(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu = entry->cpu;
    uint32_t saved_ra = battle_entry_read(entry, cpu->gpr[29] + 0x10u, 4);
    cpu->gpr[29] += 0x18u;
    cpu->gpr[31] = saved_ra;
}

static void battle_phase_select(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu = entry->cpu;
    cpu->gpr[29] -= 0x18u;
    cpu->gpr[3] = 0x800c0000u;
    uint32_t phase = battle_entry_read(entry, 0x800c3e4cu, 1);
    cpu->gpr[2] = 1;
    cpu->gpr[3] = phase;
    battle_entry_write(entry, cpu->gpr[29] + 0x10u, 4, cpu->gpr[31]);
    if (phase == 1) {
        battle_phase_call(entry, 0x80076418u, 0x8007659cu);
    } else if (phase == 0) {
        battle_phase_call(entry, 0x800764b4u, 0x8007658cu);
    } else {
        cpu->gpr[2] = 2;
        if (phase == 2)
            battle_phase_call(entry, 0x800764ecu, 0x800765acu);
    }
    battle_phase_return(entry);
}

static void battle_phase_main_chain(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu = entry->cpu;
    cpu->gpr[2] = 0x800c0000u;
    uint32_t disabled = battle_entry_read(entry, 0x800c492au, 1);
    cpu->gpr[29] -= 0x18u;
    cpu->gpr[2] = disabled;
    battle_entry_write(entry, cpu->gpr[29] + 0x10u, 4, cpu->gpr[31]);
    if (!disabled) {
        battle_phase_call(entry, 0x8008fad8u, 0x8007642cu);
        battle_phase_call(entry, 0x800742a0u, 0x80076434u);
        battle_phase_call(entry, 0x80075938u, 0x8007643cu);
        battle_phase_call(entry, 0x80073538u, 0x80076444u);
        battle_phase_call(entry, 0x80073a58u, 0x8007644cu);
        battle_phase_call(entry, 0x80073b64u, 0x80076454u);
        battle_phase_call(entry, 0x80073e88u, 0x8007645cu);
        battle_phase_call(entry, 0x80073f08u, 0x80076464u);
        battle_phase_call(entry, 0x8007500cu, 0x8007646cu);
        battle_phase_call(entry, 0x80074eecu, 0x80076474u);
        battle_phase_call(entry, 0x800745ecu, 0x8007647cu);
        battle_phase_call(entry, 0x80074d4cu, 0x80076484u);
        battle_phase_call(entry, 0x80073fb8u, 0x8007648cu);
        battle_phase_call(entry, 0x80088b80u, 0x80076494u);
        battle_phase_call(entry, 0x80074ab8u, 0x8007649cu);
    }
    battle_phase_return(entry);
}

static void battle_phase_initial_chain(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu = entry->cpu;
    cpu->gpr[29] -= 0x18u;
    battle_entry_write(entry, cpu->gpr[29] + 0x10u, 4, cpu->gpr[31]);
    cpu->gpr[1] = 0x80060000u;
    battle_entry_write(entry, 0x8005959cu, 1, 0);
    battle_phase_call(entry, 0x8008fad8u, 0x800764c4u);
    battle_phase_call(entry, 0x801de594u, 0x800764ccu);
    battle_phase_call(entry, 0x80073fb8u, 0x800764d4u);
    battle_phase_return(entry);
}

static void battle_phase_alternate_chain(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu = entry->cpu;
    cpu->gpr[29] -= 0x18u;
    battle_entry_write(entry, cpu->gpr[29] + 0x10u, 4, cpu->gpr[31]);
    battle_phase_call(entry, 0x8008fad8u, 0x800764f4u);
    battle_phase_call(entry, 0x80073538u, 0x800764fcu);
    battle_phase_call(entry, 0x80073f08u, 0x80076504u);
    battle_phase_call(entry, 0x8007500cu, 0x8007650cu);
    battle_phase_call(entry, 0x80074f70u, 0x80076514u);
    battle_phase_call(entry, 0x80073fb8u, 0x8007651cu);
    battle_phase_call(entry, 0x80088b80u, 0x80076524u);
    battle_phase_call(entry, 0x80074ab8u, 0x8007652cu);
    battle_phase_return(entry);
}

static const struct {
    uint32_t target;
    unsigned bytes;
    const char *sha;
    void (*run)(BattleEntryCall *);
} battle_phase_bodies[] = {
    {0x80076544u, 128, "280fd383eb97e6f42d62e46ba0dbf161a79b4ee1d3088d35fc6969ac4ccba53b", battle_phase_select},
    {0x80076418u, 156, "b3cc10b716a7ec385b1ed08b59e0074b8ccdad578862d9a0d22adb6dda57458d", battle_phase_main_chain},
    {0x800764b4u, 56, "fa3d1d87a20dce7123e93e8e8c604132464be9d9e199feab01594c8955b014d9", battle_phase_initial_chain},
    {0x800764ecu, 88, "56aee83121ac837401e82bf1e946f9533515c1101d915b2798ed342498d68742", battle_phase_alternate_chain},
};

/* Keep the identity-search locals outside the setjmp scope. The chosen
 * host function is immutable for this invocation; failures return directly. */
static int battle_phase_execute(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                void (*run)(BattleEntryCall *))
{
    BattleEntryCall entry = {.runtime = runtime, .cpu = cpu};
    if (setjmp(entry.failure) != 0) return -1;
    run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

static int battle_phase_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                  uint32_t target)
{
    unsigned index = 0;
    for (; index < sizeof(battle_phase_bodies) / sizeof(battle_phase_bodies[0]); ++index)
        if (battle_phase_bodies[index].target == target) break;
    if (index == sizeof(battle_phase_bodies) / sizeof(battle_phase_bodies[0]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_PHASE_CHAIN");
    if (!control || strcmp(control, "1") != 0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target), battle_phase_bodies[index].bytes, digest);
    const char *expected = battle_phase_bodies[index].sha;
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1])
            return 0;
    return battle_phase_execute(runtime, cpu, battle_phase_bodies[index].run);
}

/* Exact outgoing edges of these callers: decline legacy resolved/cache
 * dispatch until each child has its own audited native entry above this
 * boundary. No broad target/overlay range is exempted or silently stubbed. */
static int battle_phase_unconverted(uint32_t target, uint32_t return_pc)
{
    static const struct { uint32_t target, return_pc; } edges[] = {
        {0x8008fad8u, 0x80076434u},
        {0x800742a0u, 0x8007643cu},
        {0x80075938u, 0x80076444u},
        {0x80073538u, 0x8007644cu},
        {0x80073a58u, 0x80076454u},
        {0x80073b64u, 0x8007645cu},
        {0x80073e88u, 0x80076464u},
        {0x80073f08u, 0x8007646cu},
        {0x8007500cu, 0x80076474u},
        {0x80074eecu, 0x8007647cu},
        {0x800745ecu, 0x80076484u},
        {0x80074d4cu, 0x8007648cu},
        {0x80073fb8u, 0x80076494u},
        {0x80088b80u, 0x8007649cu},
        {0x80074ab8u, 0x800764a4u},
        {0x8008fad8u, 0x800764ccu},
        {0x801de594u, 0x800764d4u},
        {0x80073fb8u, 0x800764dcu},
        {0x8008fad8u, 0x800764fcu},
        {0x80073538u, 0x80076504u},
        {0x80073f08u, 0x8007650cu},
        {0x8007500cu, 0x80076514u},
        {0x80074f70u, 0x8007651cu},
        {0x80073fb8u, 0x80076524u},
        {0x80088b80u, 0x8007652cu},
        {0x80074ab8u, 0x80076534u},
    };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        if (target == edges[i].target && return_pc == edges[i].return_pc)
            return 1;
    return 0;
}

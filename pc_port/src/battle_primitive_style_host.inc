/* Complete primitive-style callers and their two scoped byte-flag services.
 * Fixed host C; private instruction bytes are used only for identity checks.
 * Checked accesses preserve aliases, call state and committed fault prefixes. */
static void battle_primitive_style_setup(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[29] -= 0x18u;
    battle_entry_write(entry, r[29] + 0x10u, 4, r[16]);
    r[16] = r[4];
    battle_entry_write(entry, r[29] + 0x14u, 4, r[31]);
    r[31] = 0x80076ae0u;
    r[5] = 1;
    battle_phase_call(entry, 0x80043bfcu, 0x80076ad8u);
    r[4] = r[16];
    r[31] = 0x80076aecu;
    r[5] = 0;
    battle_phase_call(entry, 0x80043c24u, 0x80076ae4u);
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x14u, 4);
    uint32_t saved_s0 = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[31] = saved_ra;
    r[29] += 0x18u;
    r[16] = saved_s0;
}

static void battle_primitive_style_tint(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[29] -= 0x18u;
    battle_entry_write(entry, r[29] + 0x10u, 4, r[16]);
    battle_entry_write(entry, r[29] + 0x14u, 4, r[31]);
    r[31] = 0x80076c48u;
    r[16] = r[4];
    battle_phase_call(entry, 0x80076ac8u, 0x80076c40u);
    uint32_t tpage = battle_entry_read(entry, r[16] + 0x16u, 2);
    r[2] = 0x40u;
    r[3] = tpage;
    battle_entry_write(entry, r[16] + 4u, 1, r[2]);
    battle_entry_write(entry, r[16] + 5u, 1, r[2]);
    battle_entry_write(entry, r[16] + 6u, 1, r[2]);
    r[3] |= 0x40u;
    battle_entry_write(entry, r[16] + 0x16u, 2, r[3]);
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x14u, 4);
    uint32_t saved_s0 = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[31] = saved_ra;
    r[29] += 0x18u;
    r[16] = saved_s0;
}

static void battle_primitive_style_semi(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[2] = battle_entry_read(entry, r[4] + 7u, 1);
    r[2] = r[5] ? r[2] | 2u : r[2] & 0xfdu;
    battle_entry_write(entry, r[4] + 7u, 1, r[2]);
}

static void battle_primitive_style_shade(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[2] = battle_entry_read(entry, r[4] + 7u, 1);
    r[2] = r[5] ? r[2] | 1u : r[2] & 0xfeu;
    battle_entry_write(entry, r[4] + 7u, 1, r[2]);
}

static int battle_primitive_style_target(uint32_t target, uint32_t return_pc)
{
    return target == 0x80076c34u || target == 0x80076ac8u ||
        (target == 0x80043bfcu && return_pc == 0x80076ae0u) ||
        (target == 0x80043c24u && return_pc == 0x80076aecu);
}

static const struct {
    uint32_t target;
    unsigned length;
    const char *sha;
    void (*run)(BattleEntryCall *);
} battle_primitive_style_bodies[] = {
    {0x80076c34u, 68, "021bfb06c19a94b3f42eababd6a81f6c3dc66db1e00e53818c97023424efe278", battle_primitive_style_tint},
    {0x80076ac8u, 56, "d4e25a9b08db06700779622103033e53dd639b39a6575c1d89a813470af99df6", battle_primitive_style_setup},
    {0x80043bfcu, 40, "f7bd3db94e23b108242338e61ab13d8b1f6877d7a227b90ed3f11c550612804b", battle_primitive_style_semi},
    {0x80043c24u, 40, "0c8699692a9c170bc55cb4e3171f8001f6b2f19d33b1e8c6cbb722317bf88377", battle_primitive_style_shade},
};

static int battle_primitive_style_try_host(BattleMipsRuntime *runtime,
                                           PcPortMipsCpu *cpu, uint32_t target)
{
    if (!battle_primitive_style_target(target, cpu->gpr[31]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_PRIMITIVE_STYLE");
    if (!control || strcmp(control, "1") != 0) return 0;
    unsigned index = 0;
    while (battle_primitive_style_bodies[index].target != target) ++index;
    unsigned length = battle_primitive_style_bodies[index].length;
    const uint8_t *bytes;
    if (index < 2) {
        bytes = PSX_ADDR(target);
    } else {
        const XenoRetailFileSlot *image = XenoRetailData_File(XENO_RD_SLUS);
        size_t offset = XENO_RD_SLUS_OFF(target);
        if (!image || !image->bytes || image->size < offset + length) return 0;
        bytes = image->bytes + offset;
    }
    uint8_t digest[32];
    file1_digest(bytes, length, digest);
    const char *expected = battle_primitive_style_bodies[index].sha;
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, battle_primitive_style_bodies[index].run);
}

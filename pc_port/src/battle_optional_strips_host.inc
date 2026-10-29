/* Complete optional-strip caller and primitive loop. The scoped AddPrim
 * service reuses the checked implementation already used by fade drawing.
 * No instruction payload, fetch, decoder or runtime translation is present. */
static void battle_fade_draw_sdk_run(BattleEntryCall *, uint32_t);

static void battle_optional_strips_arguments(BattleEntryCall *entry,
                                             uint32_t count_offset,
                                             uint32_t start_offset,
                                             uint32_t packet_offset,
                                             uint32_t site)
{
    uint32_t *r = entry->cpu->gpr;
    uint32_t count = battle_entry_read(entry, r[3] + count_offset, 4);
    r[4] = 0x800c0000u;
    r[5] = count;
    uint32_t ui = battle_entry_read(entry, r[4] + 0x3ea4u, 4);
    uint32_t start = battle_entry_read(entry, r[3] + start_offset, 1);
    r[4] = ui;
    r[31] = site + 8u;
    r[6] = start;
    r[4] += packet_offset;
    battle_phase_call(entry, 0x800728b8u, site);
}

static void battle_optional_strips_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[3] = 0x800d0000u;
    uint32_t state = battle_entry_read(entry, r[3] + 0x2d28u, 4);
    r[29] -= 0x18u;
    r[3] = state;
    battle_entry_write(entry, r[29] + 0x10u, 4, r[31]);
    r[2] = battle_entry_read(entry, r[3] + 0x9cu, 1);
    if (r[2])
        battle_optional_strips_arguments(entry, 0xf8u, 0xa5u, 0xba8u, 0x80073f38u);
    r[3] = 0x800d0000u;
    r[3] = battle_entry_read(entry, r[3] + 0x2d28u, 4);
    r[2] = battle_entry_read(entry, r[3] + 0x9eu, 1);
    if (r[2]) {
        battle_optional_strips_arguments(entry, 0x100u, 0xa7u, 0x27c8u, 0x80073f6cu);
        r[3] = 0x800d0000u;
        r[3] = battle_entry_read(entry, r[3] + 0x2d28u, 4);
    }
    r[2] = battle_entry_read(entry, r[3] + 0x9du, 1);
    if (r[2])
        battle_optional_strips_arguments(entry, 0xfcu, 0xa6u, 0x1e68u, 0x80073fa0u);
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[29] += 0x18u;
    r[31] = saved_ra;
}

static void battle_optional_strips_link(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[29] -= 0x30u;
    battle_entry_write(entry, r[29] + 0x24u, 4, r[19]);
    r[19] = r[4];
    battle_entry_write(entry, r[29] + 0x20u, 4, r[18]);
    r[18] = r[5];
    battle_entry_write(entry, r[29] + 0x1cu, 4, r[17]);
    r[17] = 0;
    battle_entry_write(entry, r[29] + 0x28u, 4, r[31]);
    int positive = (int32_t)r[18] > 0;
    battle_entry_write(entry, r[29] + 0x18u, 4, r[16]);
    if (positive) {
        r[16] = r[6];
        r[5] = r[16] << 2;
        for (;;) {
            r[5] += r[16];
            r[16] += 2u;
            r[17] += 1u;
            r[4] = 0x800d0000u;
            uint32_t order_table = battle_entry_read(entry, 0x800ccb04u, 4);
            r[5] <<= 3;
            r[4] = order_table;
            r[5] += r[19];
            r[31] = 0x8007290cu;
            r[4] += 4u;
            battle_phase_call(entry, 0x80043b48u, 0x80072904u);
            r[2] = (int32_t)r[17] < (int32_t)r[18];
            int again = r[2] != 0;
            r[5] = r[16] << 2;
            if (!again) break;
        }
    }
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x28u, 4);
    uint32_t saved_s3 = battle_entry_read(entry, r[29] + 0x24u, 4);
    r[31] = saved_ra;
    uint32_t saved_s2 = battle_entry_read(entry, r[29] + 0x20u, 4);
    r[19] = saved_s3;
    uint32_t saved_s1 = battle_entry_read(entry, r[29] + 0x1cu, 4);
    r[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, r[29] + 0x18u, 4);
    r[17] = saved_s1;
    r[29] += 0x30u;
    r[16] = saved_s0;
}

static void battle_optional_strips_add_prim(BattleEntryCall *entry)
{
    battle_fade_draw_sdk_run(entry, 0x80043b48u);
}

static int battle_optional_strips_target(uint32_t target, uint32_t return_pc)
{
    return target == 0x80073f08u || target == 0x800728b8u ||
        (target == 0x80043b48u && return_pc == 0x8007290cu);
}

static const struct {
    uint32_t target;
    unsigned length;
    const char *sha;
    void (*run)(BattleEntryCall *);
} battle_optional_strips_bodies[] = {
    {0x80073f08u, 176, "690f57be2d826434654050e60e954c155cee5eba522239354fa40c3d43951c3a", battle_optional_strips_run},
    {0x800728b8u, 128, "8b4d52dee2fc9939d9cf1532ca4c102b7755571e567a634465f6b1d0b9c6ac54", battle_optional_strips_link},
    {0x80043b48u, 60, "1c2e33c9e0ddfcd56385916112089fab3d7d343336e2514633a70260ef7d4064", battle_optional_strips_add_prim},
};

static int battle_optional_strips_try_host(BattleMipsRuntime *runtime,
                                           PcPortMipsCpu *cpu, uint32_t target)
{
    if (!battle_optional_strips_target(target, cpu->gpr[31]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_OPTIONAL_STRIPS");
    if (!control || strcmp(control, "1") != 0) return 0;
    unsigned index = 0;
    while (battle_optional_strips_bodies[index].target != target) ++index;
    unsigned length = battle_optional_strips_bodies[index].length;
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
    const char *expected = battle_optional_strips_bodies[index].sha;
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, battle_optional_strips_bodies[index].run);
}

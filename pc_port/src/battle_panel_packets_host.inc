/* Complete eight-record panel packet caller and quad writer. The scoped
 * AddPrim service uses the existing checked implementation. No opcode
 * fetch, decoder, instruction payload or runtime translation is present. */
static void battle_fade_draw_sdk_run(BattleEntryCall *, uint32_t);

static void battle_panel_packets_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[29] -= 0x30u;
    battle_entry_write(entry, r[29] + 0x1cu, 4, r[17]);
    r[17] = 0;
    battle_entry_write(entry, r[29] + 0x20u, 4, r[18]);
    r[18] = 0x800d36c8u;
    battle_entry_write(entry, r[29] + 0x24u, 4, r[19]);
    r[19] = 0x800ccb04u;
    battle_entry_write(entry, r[29] + 0x18u, 4, r[16]);
    r[16] = 0;
    battle_entry_write(entry, r[29] + 0x28u, 4, r[31]);
    for (;;) {
        r[1] = 0x800d0000u + r[16];
        r[2] = battle_entry_read(entry, r[1] + 0x3725u, 1);
        if (r[2]) {
            int later = r[17] != 0;
            r[2] = r[17] >> 31;
            if (!later) {
                r[6] = 0x2cu;
                r[7] = 0;
                r[2] = 0x800d0000u;
                uint32_t width = battle_entry_read(entry, r[2] + 0x3726u, 1);
                r[3] = 0x800d0000u;
                r[2] = width;
                uint32_t slot = battle_entry_read(entry, 0x800ccb34u, 4);
                r[5] = 0x40u;
                r[3] = slot;
                battle_entry_write(entry, r[29] + 0x10u, 4, 0);
                r[2] >>= 1;
                r[5] -= r[2];
                r[4] = r[3] << 2;
                r[4] += r[3];
                r[4] <<= 3;
                r[2] = 0x800d0000u;
                r[2] = battle_entry_read(entry, r[2] + 0x3726u, 1);
                r[4] += r[18];
            } else {
                r[2] += r[17];
                r[2] = (r[2] >> 1) | (r[2] & 0x80000000u);
                r[3] = r[2] << 1;
                r[3] += r[2];
                r[3] <<= 2;
                r[3] += r[2];
                r[6] = 0xcau;
                r[7] = 0;
                r[3] &= 0xffu;
                r[1] = 0x800d0000u + r[16];
                uint32_t width = battle_entry_read(entry, r[1] + 0x3726u, 1);
                r[5] = 0x9au;
                r[2] = width;
                battle_entry_write(entry, r[29] + 0x10u, 4, r[3]);
                r[3] = 0x800d0000u;
                uint32_t slot = battle_entry_read(entry, 0x800ccb34u, 4);
                r[2] >>= 1;
                r[3] = slot;
                r[5] -= r[2];
                r[4] = r[3] << 2;
                r[4] += r[3];
                r[4] <<= 3;
                r[4] += r[18];
                r[4] += r[16];
                r[1] = 0x800d0000u + r[16];
                width = battle_entry_read(entry, r[1] + 0x3726u, 1);
                r[5] &= 0xffffu;
                r[2] = width;
            }
            r[5] &= 0xffffu;
            r[31] = 0x80075114u;
            battle_entry_write(entry, r[29] + 0x14u, 4, r[2]);
            battle_phase_call(entry, 0x80076c78u, 0x8007510cu);
            uint32_t order_table = battle_entry_read(entry, r[19], 4);
            uint32_t slot = battle_entry_read(entry, r[19] + 0x30u, 4);
            r[4] = order_table;
            r[4] += 4u;
            r[2] = slot;
            r[5] = r[2] << 2;
            r[5] += r[2];
            r[5] <<= 3;
            r[5] += r[18];
            r[31] = 0x80075138u;
            r[5] += r[16];
            battle_phase_call(entry, 0x80043b48u, 0x80075130u);
        }
        r[17] += 1u;
        r[2] = (int32_t)r[17] < 8;
        int again = r[2] != 0;
        r[16] += 0x60u;
        if (!again) break;
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

static void battle_panel_packets_quad(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[3] = r[5];
    r[2] = r[6];
    r[6] += 0xdu;
    battle_entry_write(entry, r[4] + 8u, 2, r[3]);
    battle_entry_write(entry, r[4] + 0xau, 2, r[2]);
    battle_entry_write(entry, r[4] + 0x12u, 2, r[2]);
    battle_entry_write(entry, r[4] + 0x18u, 2, r[3]);
    battle_entry_write(entry, r[4] + 0x1au, 2, r[6]);
    battle_entry_write(entry, r[4] + 0x22u, 2, r[6]);
    uint32_t v = battle_entry_read(entry, r[29] + 0x10u, 4);
    uint32_t width = battle_entry_read(entry, r[29] + 0x14u, 4);
    r[3] = v;
    r[2] = r[7];
    r[6] = width;
    battle_entry_write(entry, r[4] + 0xcu, 1, r[2]);
    battle_entry_write(entry, r[4] + 0x1cu, 1, r[2]);
    r[2] = r[6] & 0xffu;
    r[5] += r[2];
    r[2] = r[3];
    r[7] += r[6];
    r[3] += 0xdu;
    battle_entry_write(entry, r[4] + 0x10u, 2, r[5]);
    battle_entry_write(entry, r[4] + 0x20u, 2, r[5]);
    battle_entry_write(entry, r[4] + 0xdu, 1, r[2]);
    battle_entry_write(entry, r[4] + 0x14u, 1, r[7]);
    battle_entry_write(entry, r[4] + 0x15u, 1, r[2]);
    battle_entry_write(entry, r[4] + 0x1du, 1, r[3]);
    battle_entry_write(entry, r[4] + 0x24u, 1, r[7]);
    battle_entry_write(entry, r[4] + 0x25u, 1, r[3]);
}

static void battle_panel_packets_add_prim(BattleEntryCall *entry)
{
    battle_fade_draw_sdk_run(entry, 0x80043b48u);
}

static int battle_panel_packets_target(uint32_t target, uint32_t return_pc)
{
    return target == 0x8007500cu ||
        (target == 0x80076c78u && return_pc == 0x80075114u) ||
        (target == 0x80043b48u && return_pc == 0x80075138u);
}

static const struct {
    uint32_t target;
    unsigned length;
    const char *sha;
    void (*run)(BattleEntryCall *);
} battle_panel_packets_bodies[] = {
    {0x8007500cu, 348, "3fd64ee0e351c937fcffe01d7d8fbbd81055f644ea241824f182b021b84eb989", battle_panel_packets_run},
    {0x80076c78u, 112, "7dda174e978eda4a24a5e277e8d919330eb6f175c05b063d2fd72563835d943e", battle_panel_packets_quad},
    {0x80043b48u, 60, "1c2e33c9e0ddfcd56385916112089fab3d7d343336e2514633a70260ef7d4064", battle_panel_packets_add_prim},
};

static int battle_panel_packets_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu, uint32_t target)
{
    if (!battle_panel_packets_target(target, cpu->gpr[31]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_PANEL_PACKETS");
    if (!control || strcmp(control, "1") != 0) return 0;
    unsigned index = 0;
    while (battle_panel_packets_bodies[index].target != target) ++index;
    unsigned length = battle_panel_packets_bodies[index].length;
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
    const char *expected = battle_panel_packets_bodies[index].sha;
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, battle_panel_packets_bodies[index].run);
}

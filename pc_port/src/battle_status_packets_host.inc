/* Complete744-byte status/record packet caller. The existing strip loop keeps
 * its own conversion gate. Only the listed AddPrim call sites are adopted here.
 * Fixed host logic; no CPU fetch, decoder or runtime instruction translation. */
static void battle_fade_draw_sdk_run(BattleEntryCall *, uint32_t);

static void battle_status_record_packet(BattleEntryCall *entry,
                                         unsigned shift, unsigned scale,
                                         uint32_t offset, uint32_t site)
{
    uint32_t *r = entry->cpu->gpr;
    uint32_t ot = battle_entry_read(entry, r[18], 4);
    uint32_t slot = battle_entry_read(entry, r[16] + 0x5a4u, 1);
    r[4] = ot;
    r[4] += 4u;
    r[2] = slot;
    r[5] = r[2] << shift;
    r[5] += r[2];
    r[5] <<= scale;
    r[5] += offset;
    r[31] = site + 8u;
    r[5] += r[16];
    battle_phase_call(entry, 0x80043b48u, site);
}

static void battle_status_packets_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[4] = 0x800d0000u;
    uint32_t ot = battle_entry_read(entry, 0x800ccb04u, 4);
    r[2] = 0x800d0000u;
    r[4] = ot;
    uint32_t slot = battle_entry_read(entry, 0x800ccb34u, 4);
    r[3] = 0x800c0000u;
    r[2] = slot;
    uint32_t ui = battle_entry_read(entry, 0x800c3ea4u, 4);
    r[29] -= 0x20u;
    r[3] = ui;
    battle_entry_write(entry, r[29] + 0x14u, 4, r[17]);
    r[17] = 0;
    battle_entry_write(entry, r[29] + 0x1cu, 4, r[31]);
    battle_entry_write(entry, r[29] + 0x18u, 4, r[18]);
    battle_entry_write(entry, r[29] + 0x10u, 4, r[16]);
    r[4] += 4u;
    r[5] = r[2] << 1;
    r[5] += r[2];
    r[5] <<= 2;
    r[2] = 0x8920u;
    r[5] += r[2];
    r[31] = 0x80074008u;
    r[5] += r[3];
    battle_phase_call(entry, 0x80043b48u, 0x80074000u);
    for (;;) {
        r[2] = 0x800d0000u;
        r[2] = battle_entry_read(entry, 0x800d2d28u, 4);
        r[5] = r[2] + r[17];
        r[2] = battle_entry_read(entry, r[5] + 0x7fu, 1);
        int present = r[2] != 0;
        r[2] = 1;
        if (present) {
            r[3] = 0x800c0000u;
            r[3] = battle_entry_read(entry, 0x800c3e4cu, 1);
            int phase_one = r[3] == r[2];
            r[2] = r[17] << 1;
            if (phase_one) {
                r[4] = 0x800d0000u;
                ot = battle_entry_read(entry, 0x800ccb04u, 4);
                uint32_t byte = battle_entry_read(entry, r[5] + 0x84u, 1);
                r[4] = ot;
                r[4] += 4u;
                r[3] = byte;
                r[2] += r[3];
                r[5] = r[2] << 1;
                r[5] += r[2];
                r[5] <<= 2;
                r[5] += r[2];
                r[5] <<= 2;
                r[2] = 0x800c0000u;
                ui = battle_entry_read(entry, 0x800c3ea4u, 4);
                r[5] += 0x5a0u;
                r[2] = ui;
                r[31] = 0x80074078u;
                r[5] += r[2];
                battle_phase_call(entry, 0x80043b48u, 0x80074070u);
            }
        }
        r[17] += 1u;
        r[2] = (int32_t)r[17] < 4;
        if (!r[2]) break;
    }
    r[17] = 0;
    r[2] = 0x800ccb04u;
    r[18] = r[2];
    ot = battle_entry_read(entry, r[2], 4);
    r[2] = 0x800d0000u;
    r[4] = ot;
    slot = battle_entry_read(entry, 0x800ccb34u, 4);
    r[3] = 0x800c0000u;
    r[2] = slot;
    ui = battle_entry_read(entry, 0x800c3ea4u, 4);
    r[4] += 4u;
    r[3] = ui;
    r[5] = r[2] << 1;
    r[5] += r[2];
    r[5] <<= 2;
    r[2] = 0x8908u;
    r[5] += r[2];
    r[31] = 0x800740ccu;
    r[5] += r[3];
    battle_phase_call(entry, 0x80043b48u, 0x800740c4u);
    for (;;) {
        r[2] = 0x800d0000u;
        r[2] = battle_entry_read(entry, 0x800d2d28u, 4);
        r[2] += r[17];
        r[2] = battle_entry_read(entry, r[2] + 0xb0u, 1);
        int present = r[2] != 0;
        r[2] = r[17] << 2;
        if (present) {
            r[1] = 0x800d0000u;
            r[1] += r[2];
            r[16] = battle_entry_read(entry, r[1] + 0x2e38u, 4);
            uint32_t count = battle_entry_read(entry, r[16] + 0x5a0u, 4);
            uint32_t start = battle_entry_read(entry, r[16] + 0x5a4u, 1);
            r[5] = count;
            r[31] = 0x8007410cu;
            r[6] = start;
            r[4] = r[16];
            battle_phase_call(entry, 0x800728b8u, 0x80074104u);
            battle_status_record_packet(entry, 2, 3, 0x280u, 0x80074128u);
            battle_status_record_packet(entry, 2, 3, 0x2d0u, 0x8007414cu);
            battle_status_record_packet(entry, 2, 3, 0x320u, 0x80074170u);
            battle_status_record_packet(entry, 2, 3, 0x370u, 0x80074194u);
            battle_status_record_packet(entry, 2, 3, 0x140u, 0x800741b8u);
            battle_status_record_packet(entry, 2, 3, 0x190u, 0x800741dcu);
            battle_status_record_packet(entry, 2, 3, 0x1e0u, 0x80074200u);
            battle_status_record_packet(entry, 2, 3, 0x230u, 0x80074224u);
            battle_status_record_packet(entry, 3, 2, 0x3c0u, 0x80074248u);
            battle_status_record_packet(entry, 1, 2, 0x408u, 0x8007426cu);
        }
        r[17] += 1u;
        r[2] = (int32_t)r[17] < 7;
        if (!r[2]) break;
    }
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x1cu, 4);
    uint32_t saved_s2 = battle_entry_read(entry, r[29] + 0x18u, 4);
    r[31] = saved_ra;
    uint32_t saved_s1 = battle_entry_read(entry, r[29] + 0x14u, 4);
    r[18] = saved_s2;
    uint32_t saved_s0 = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[17] = saved_s1;
    r[29] += 0x20u;
    r[16] = saved_s0;
}

static void battle_status_packets_add_prim(BattleEntryCall *entry)
{
    battle_fade_draw_sdk_run(entry, 0x80043b48u);
}

static int battle_status_packets_target(uint32_t target, uint32_t return_pc)
{
    if (target == 0x80073fb8u) return 1;
    if (target != 0x80043b48u) return 0;
    switch (return_pc) {
    case 0x80074008u: case 0x80074078u: case 0x800740ccu:
    case 0x80074130u: case 0x80074154u: case 0x80074178u:
    case 0x8007419cu: case 0x800741c0u: case 0x800741e4u:
    case 0x80074208u: case 0x8007422cu: case 0x80074250u:
    case 0x80074274u: return 1;
    default: return 0;
    }
}

static int battle_status_packets_try_host(BattleMipsRuntime *runtime,
                                          PcPortMipsCpu *cpu, uint32_t target)
{
    if (!battle_status_packets_target(target, cpu->gpr[31]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_STATUS_PACKETS");
    if (!control || strcmp(control, "1") != 0) return 0;
    const uint8_t *bytes;
    unsigned length;
    const char *expected;
    if (target == 0x80073fb8u) {
        bytes = PSX_ADDR(target); length = 744;
        expected = "0469dafe6e02b6c08ed840197470997d0263d7f60af94c82e4752cfcb84c6e6d";
    } else {
        const XenoRetailFileSlot *image = XenoRetailData_File(XENO_RD_SLUS);
        size_t offset = XENO_RD_SLUS_OFF(target);
        length = 60;
        if (!image || !image->bytes || image->size < offset + length) return 0;
        bytes = image->bytes + offset;
        expected = "1c2e33c9e0ddfcd56385916112089fab3d7d343336e2514633a70260ef7d4064";
    }
    uint8_t digest[32]; file1_digest(bytes, length, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, target == 0x80073fb8u ?
        battle_status_packets_run : battle_status_packets_add_prim);
}

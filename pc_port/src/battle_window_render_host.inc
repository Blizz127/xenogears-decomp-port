/* Complete 156-byte window caller, with ordered global rereads. No opcode
 * fetch/decoder is present. The two child boundaries keep their exact sites. */
static void battle_fade_draw_sdk_run(BattleEntryCall *, uint32_t);
static void battle_window_render_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[2] = 0x800d0000u;
    uint32_t state = battle_entry_read(entry, 0x800d2d28u, 4);
    r[29] -= 0x18u;
    r[2] = state;
    battle_entry_write(entry, r[29] + 0x10u, 4, r[31]);
    r[2] = battle_entry_read(entry, r[2] + 0xc8u, 1);
    if (r[2]) {
        r[3] = 0x800d0000u;
        uint32_t ui = battle_entry_read(entry, 0x800d3278u, 4);
        r[4] = 0x800d0000u;
        r[3] = ui;
        uint32_t ot = battle_entry_read(entry, 0x800ccb04u, 4);
        uint32_t slot = battle_entry_read(entry, r[3] + 0x7f4u, 1);
        r[4] = ot;
        r[4] += 4u;
        r[2] = slot;
        r[5] = r[2] << 2;
        r[5] += r[2];
        r[5] <<= 3;
        r[5] += 0x7a4u;
        r[31] = 0x80074fc0u;
        r[5] += r[3];
        battle_phase_call(entry, 0x80043b48u, 0x80074fb8u);
    }
    r[2] = 0x800d0000u;
    r[2] = battle_entry_read(entry, 0x800d2d28u, 4);
    r[2] = battle_entry_read(entry, r[2] + 0xc9u, 1);
    if (r[2]) {
        r[4] = 0x800d0000u;
        uint32_t window = battle_entry_read(entry, 0x800d2dacu, 4);
        r[5] = 0x800d0000u;
        r[4] = window;
        uint32_t ot = battle_entry_read(entry, 0x800ccb04u, 4);
        r[6] = 0x800d0000u;
        r[5] = ot;
        uint32_t slot = battle_entry_read(entry, 0x800ccb34u, 4);
        r[31] = 0x80074ffcu;
        r[6] = slot;
        r[5] += 4u;
        battle_phase_call(entry, 0x80034888u, 0x80074ff4u);
    }
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[29] += 0x18u;
    r[31] = saved_ra;
}

static int battle_window_render_target(uint32_t target, uint32_t return_pc)
{
    return target == 0x80074f70u ||
        (target == 0x80043b48u && return_pc == 0x80074fc0u);
}

static void battle_window_render_add_prim(BattleEntryCall *entry)
{
    battle_fade_draw_sdk_run(entry, 0x80043b48u);
}

static int battle_window_render_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu, uint32_t target)
{
    if (!battle_window_render_target(target, cpu->gpr[31]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_WINDOW_RENDER");
    if (!control || strcmp(control, "1") != 0) return 0;
    const uint8_t *bytes;
    unsigned length;
    const char *expected;
    if (target == 0x80074f70u) {
        bytes = PSX_ADDR(target); length = 156;
        expected = "edc2a3322cee8dd9ff729859c6740c3c03fb02b603fb6a94c06dddc78e4b050a";
    } else {
        const XenoRetailFileSlot *image = XenoRetailData_File(XENO_RD_SLUS);
        size_t offset = XENO_RD_SLUS_OFF(target);
        length = 60;
        if (!image || !image->bytes || image->size < offset + length) return 0;
        bytes = image->bytes + offset;
        expected = "1c2e33c9e0ddfcd56385916112089fab3d7d343336e2514633a70260ef7d4064";
    }
    uint8_t digest[32];
    file1_digest(bytes, length, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, target == 0x80074f70u ?
        battle_window_render_run : battle_window_render_add_prim);
}

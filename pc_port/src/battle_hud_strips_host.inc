/* Complete 80074AB8 HUD-strip caller. Each selected strip delegates to the
 * existing gated 800728B8 primitive linker through the ordinary guest call
 * boundary. Live pointer/count reads and committed fault prefixes are kept;
 * there is no original instruction payload or instruction execution here. */
static void battle_hud_strips_pair(BattleEntryCall *entry,
                                   uint32_t count_offset,
                                   uint32_t start_offset,
                                   uint32_t packet_offset,
                                   uint32_t site)
{
    uint32_t *r = entry->cpu->gpr;
    /* The first byte load is still pending during the second access. */
    uint32_t count = battle_entry_read(entry, r[4] + count_offset, 1);
    uint32_t start = battle_entry_read(entry, r[4] + start_offset, 1);
    r[5] = count;
    r[31] = site + 8u;
    r[6] = start;
    r[4] += packet_offset;
    battle_phase_call(entry, 0x800728b8u, site);
}

static void battle_hud_strips_strip(BattleEntryCall *entry,
                                    uint32_t count_offset,
                                    uint32_t start_offset,
                                    uint32_t packet_offset,
                                    uint32_t site)
{
    uint32_t *r = entry->cpu->gpr;
    r[4] = 0x800d0000u;
    r[4] = battle_entry_read(entry, r[4] + 0x2db4u, 4);
    battle_hud_strips_pair(entry, count_offset, start_offset, packet_offset, site);
}

static void battle_hud_strips_run(BattleEntryCall *entry)
{
    uint32_t *r = entry->cpu->gpr;
    r[2] = 0x800d0000u;
    uint32_t state = battle_entry_read(entry, r[2] + 0x2d28u, 4);
    r[29] -= 0x18u;
    r[2] = state;
    battle_entry_write(entry, r[29] + 0x10u, 4, r[31]);
    r[2] = battle_entry_read(entry, r[2] + 0xc7u, 1);
    if (r[2]) {
        battle_hud_strips_strip(entry, 0x5d77u, 0x5d86u, 0x3e80u, 0x80074aecu);
        battle_hud_strips_strip(entry, 0x5d7cu, 0x5d8bu, 0x5280u, 0x80074b08u);
        battle_hud_strips_strip(entry, 0x5d7du, 0x5d8cu, 0x53c0u, 0x80074b24u);
        battle_hud_strips_strip(entry, 0x5d7fu, 0x5d8eu, 0x43d0u, 0x80074b40u);
    }

    /* Earlier children may have replaced the state pointer or this flag. */
    r[2] = 0x800d0000u;
    r[2] = battle_entry_read(entry, r[2] + 0x2d28u, 4);
    r[2] = battle_entry_read(entry, r[2] + 0xadu, 1);
    int draw_hud = r[2] != 0;
    r[5] = 0x14u;
    if (draw_hud) {
        r[4] = 0x800d0000u;
        r[4] = battle_entry_read(entry, r[4] + 0x2db4u, 4);
        uint32_t start = battle_entry_read(entry, r[4] + 0x5d98u, 1);
        r[31] = 0x80074b7cu;
        r[6] = start;
        r[4] += 0x46a0u;
        battle_phase_call(entry, 0x800728b8u, 0x80074b74u);
        battle_hud_strips_strip(entry, 0x5d70u, 0x5d92u, 0,       0x80074b90u);
        battle_hud_strips_strip(entry, 0x5d74u, 0x5d83u, 0x1720u, 0x80074bacu);

        r[4] = 0x800d0000u;
        r[4] = battle_entry_read(entry, r[4] + 0x2db4u, 4);
        r[2] = battle_entry_read(entry, r[4] + 0x5da2u, 2);
        r[2] += 1u;
        /* Keep the full increment visible if the halfword store faults. */
        battle_entry_write(entry, r[4] + 0x5da2u, 2, r[2]);
        r[2] <<= 16;
        r[3] = (r[2] >> 16) | ((r[2] & 0x80000000u) ? 0xffff0000u : 0u);
        r[2] = (uint32_t)((int32_t)r[3] < 15);
        int draw_blink = r[2] != 0;
        r[2] = (uint32_t)((int32_t)r[3] < 21);
        if (draw_blink) {
            /* This pair uses the current HUD pointer; it has no reload. */
            battle_hud_strips_pair(entry, 0x5da1u, 0x5da0u, 0x4ce0u, 0x80074becu);
        } else if (!r[2]) {
            battle_entry_write(entry, r[4] + 0x5da2u, 2, 0);
        }

        battle_hud_strips_strip(entry, 0x5d75u, 0x5d84u, 0x3ac0u, 0x80074c1cu);
        battle_hud_strips_strip(entry, 0x5d76u, 0x5d85u, 0x3e30u, 0x80074c38u);
        battle_hud_strips_strip(entry, 0x5d78u, 0x5d87u, 0x4e70u, 0x80074c54u);
        battle_hud_strips_strip(entry, 0x5d79u, 0x5d88u, 0x4fb0u, 0x80074c70u);
        battle_hud_strips_strip(entry, 0x5d7au, 0x5d89u, 0x50a0u, 0x80074c8cu);
        battle_hud_strips_strip(entry, 0x5d7bu, 0x5d8au, 0x51e0u, 0x80074ca8u);
        battle_hud_strips_strip(entry, 0x5d7eu, 0x5d8du, 0x2530u, 0x80074cc4u);
        battle_hud_strips_strip(entry, 0x5d96u, 0x5d97u, 0x32a0u, 0x80074ce0u);
        battle_hud_strips_strip(entry, 0x5d71u, 0x5d93u, 0x03c0u, 0x80074cfcu);
        battle_hud_strips_strip(entry, 0x5d72u, 0x5d94u, 0x0d20u, 0x80074d18u);
        battle_hud_strips_strip(entry, 0x5d73u, 0x5d95u, 0x0fa0u, 0x80074d34u);
    }
    uint32_t saved_ra = battle_entry_read(entry, r[29] + 0x10u, 4);
    r[29] += 0x18u;
    r[31] = saved_ra;
}

static int battle_hud_strips_target(uint32_t target, uint32_t return_pc)
{
    (void)return_pc;
    return target == 0x80074ab8u;
}

static int battle_hud_strips_try_host(BattleMipsRuntime *runtime,
                                     PcPortMipsCpu *cpu, uint32_t target)
{
    if (!battle_hud_strips_target(target, cpu->gpr[31]) ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_HUD_STRIPS");
    if (!control || strcmp(control, "1") != 0) return 0;
    static const char expected[] =
        "7873091492f1ac30b2e79a96384ed0d9b80f76f58875e930996afd6eaf18a487";
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target), 660u, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != expected[2*i] ||
            file1_hex_digit(digest[i] & 15u) != expected[2*i+1]) return 0;
    return battle_phase_execute(runtime, cpu, battle_hud_strips_run);
}

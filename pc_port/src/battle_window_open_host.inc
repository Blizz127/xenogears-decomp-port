/* Window-opening caller. Original bytes serve only as a whole-body identity;
 * this code never fetches or decodes instructions. Ordered accesses and live
 * caller registers preserve aliases and committed fault prefixes. Rendering
 * remains a separate native conversion boundary. */
static void battle_window_open_run(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu = entry->cpu;
    cpu->gpr[29] -= 0x20u;
    battle_entry_write(entry, cpu->gpr[29] + 0x18u, 4, cpu->gpr[16]);
    cpu->gpr[16] = 0;
    battle_entry_write(entry, cpu->gpr[29] + 0x1cu, 4, cpu->gpr[31]);
    do {
        cpu->gpr[2] = 0x800d0000u;
        uint32_t state = battle_entry_read(entry, 0x800d2d28u, 4);
        cpu->gpr[3] = cpu->gpr[16] << 2;
        cpu->gpr[2] = state;
        cpu->gpr[5] = state + cpu->gpr[16];
        uint32_t shown = battle_entry_read(entry, cpu->gpr[5] + 0xb8u, 1);
        cpu->gpr[1] = 0x800d0000u;
        cpu->gpr[2] = shown;
        cpu->gpr[1] += cpu->gpr[3];
        cpu->gpr[4] = battle_entry_read(entry, cpu->gpr[1] + 0x2d90u, 4);
        if (shown) {
            cpu->gpr[2] = battle_entry_read(entry, cpu->gpr[5] + 0xbfu, 1);
            if (!cpu->gpr[2]) {
                /* Consecutive loads commit only after the following access
                 * succeeds, as observable on a fault at that access. */
                uint32_t current = battle_entry_read(entry, cpu->gpr[4] + 8u, 2);
                uint32_t full = battle_entry_read(entry, cpu->gpr[4] + 4u, 2);
                cpu->gpr[2] = current + 0x20u;
                cpu->gpr[3] = full;
                cpu->gpr[2] = cpu->gpr[2] < full;
                uint32_t grow = cpu->gpr[2];
                cpu->gpr[5] = 0;
                if (grow) {
                    cpu->gpr[2] = battle_entry_read(entry, cpu->gpr[4] + 8u, 2);
                    cpu->gpr[2] += 0x20u;
                } else {
                    cpu->gpr[2] = battle_entry_read(entry, cpu->gpr[4] + 4u, 2);
                    cpu->gpr[5] = 1;
                }
                battle_entry_write(entry, cpu->gpr[4] + 8u, 2, cpu->gpr[2]);
                current = battle_entry_read(entry, cpu->gpr[4] + 10u, 2);
                full = battle_entry_read(entry, cpu->gpr[4] + 6u, 2);
                cpu->gpr[2] = current + 0x20u;
                cpu->gpr[3] = full;
                cpu->gpr[2] = cpu->gpr[2] < full;
                if (cpu->gpr[2]) {
                    cpu->gpr[2] = battle_entry_read(entry, cpu->gpr[4] + 10u, 2);
                    cpu->gpr[2] += 0x20u;
                } else {
                    cpu->gpr[2] = battle_entry_read(entry, cpu->gpr[4] + 6u, 2);
                    cpu->gpr[5] += 1u;
                }
                battle_entry_write(entry, cpu->gpr[4] + 10u, 2, cpu->gpr[2]);
                cpu->gpr[3] = cpu->gpr[5] & 0xffu;
                cpu->gpr[2] = 2;
                uint32_t completed = cpu->gpr[3] == cpu->gpr[2];
                cpu->gpr[3] = 1;
                if (completed) {
                    cpu->gpr[2] = 0x800d0000u;
                    cpu->gpr[2] = battle_entry_read(entry, 0x800d2d28u, 4);
                    cpu->gpr[2] += cpu->gpr[16];
                    battle_entry_write(entry, cpu->gpr[2] + 0xbfu, 1, cpu->gpr[3]);
                }
                uint32_t width = battle_entry_read(entry, cpu->gpr[4] + 4u, 2);
                uint32_t x = battle_entry_read(entry, cpu->gpr[4], 2);
                cpu->gpr[2] = width;
                uint32_t draw_width = battle_entry_read(entry, cpu->gpr[4] + 8u, 2);
                cpu->gpr[5] = x;
                uint32_t y = battle_entry_read(entry, cpu->gpr[4] + 2u, 2);
                cpu->gpr[7] = draw_width;
                cpu->gpr[2] >>= 1;
                cpu->gpr[6] = y;
                cpu->gpr[5] += cpu->gpr[2];
                cpu->gpr[3] = cpu->gpr[7] >> 1;
                cpu->gpr[5] -= cpu->gpr[3];
                cpu->gpr[5] &= 0xffffu;
                uint32_t height = battle_entry_read(entry, cpu->gpr[4] + 6u, 2);
                uint32_t draw_height = battle_entry_read(entry, cpu->gpr[4] + 10u, 2);
                cpu->gpr[2] = height;
                uint32_t kind = battle_entry_read(entry, cpu->gpr[4] + 12u, 1);
                cpu->gpr[3] = draw_height;
                cpu->gpr[2] >>= 1;
                cpu->gpr[4] = kind;
                cpu->gpr[6] += cpu->gpr[2];
                cpu->gpr[2] = cpu->gpr[3] >> 1;
                cpu->gpr[6] -= cpu->gpr[2];
                cpu->gpr[6] &= 0xffffu;
                cpu->gpr[31] = 0x8008fbf8u;
                battle_entry_write(entry, cpu->gpr[29] + 0x10u, 4, cpu->gpr[3]);
                battle_phase_call(entry, 0x8008f6e4u, 0x8008fbf0u);
            }
        }
        cpu->gpr[16] += 1u;
        cpu->gpr[2] = (int32_t)cpu->gpr[16] < 7;
    } while (cpu->gpr[2]);
    uint32_t saved_ra = battle_entry_read(entry, cpu->gpr[29] + 0x1cu, 4);
    uint32_t saved_s0 = battle_entry_read(entry, cpu->gpr[29] + 0x18u, 4);
    cpu->gpr[31] = saved_ra;
    cpu->gpr[29] += 0x20u;
    cpu->gpr[16] = saved_s0;
}

static const char battle_window_open_sha[] = "57abf3d021db7c7be10f7f17ca679b032042dac507942290cc5d3e7d38ab2401";
static int battle_window_open_try_host(BattleMipsRuntime *runtime,
                                       PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x8008fad8u || runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_WINDOW_OPEN");
    if (!control || strcmp(control, "1") != 0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target), 324, digest);
    for (unsigned i = 0; i < sizeof(digest); ++i)
        if (file1_hex_digit(digest[i] >> 4) != battle_window_open_sha[2*i] ||
            file1_hex_digit(digest[i] & 15u) != battle_window_open_sha[2*i+1])
            return 0;
    return battle_phase_execute(runtime, cpu, battle_window_open_run);
}

static int battle_window_open_unconverted(uint32_t target, uint32_t return_pc)
{
    return target == 0x8008f6e4u && return_pc == 0x8008fbf8u;
}

/* Native battle display initialization (src/battle/main101.c, 800B8284).
 * The display/draw environments live in packed guest RAM. Let the existing
 * SDK bridge translate them; never overlay host-sized structs on that RAM.
 * Retain the real frame and fifth argument for all four environment calls.
 */
#define BATTLE_DISPLAY_ADDRESS 0x800b8284u
#define BATTLE_DISPLAY_BYTES 0xd0u
#define BATTLE_DISPLAY_SHA256 \
    "9a148103ca4144232c8f32b05b241da03729824b4f7552d1e0467c12f6009184"

#define BD_R32(a) battle_entry_read(entry, (a), 4)
#define BD_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define BD_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define BD_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define BD_S0 (entry->cpu->gpr[16])
#define BD_S1 (entry->cpu->gpr[17])

static void battle_display_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x28u;
    entry->cpu->gpr[29] = sp;
    BD_W32(sp + 0x20u, entry->cpu->gpr[31]);
    BD_W32(sp + 0x1cu, BD_S1);
    BD_W32(sp + 0x18u, BD_S0);
    BD_CALL(0x8004a12cu, 0x800b8298u, 160, 164); /* SetGeomOffset */
    BD_S0 = 0x800c4a7cu;
    BD_S1 = 224;
    BD_CALL(0x800439e0u, 0x800b82bcu, BD_S0, 0, 224, 320, BD_S1);
    BD_CALL(0x800439e0u, 0x800b82d4u, BD_S0 + 0x4070u, 0, 0, 320, BD_S1);
    BD_CALL(0x80043928u, 0x800b82ecu, BD_S0 - 0x5cu, 0, 0, 320, BD_S1);
    BD_CALL(0x80043928u, 0x800b8304u, BD_S0 + 0x4014u, 0, BD_S1, 320, BD_S1);
    BD_S0 -= 0xbccu;
    BD_W16(BD_S0 + 0x4c46u, 10);
    BD_W16(BD_S0 + 0x0bd6u, 10);
    BD_W16(BD_S0 + 0x4c48u, 256);
    BD_W16(BD_S0 + 0x0bd8u, 256);
    BD_W16(BD_S0 + 0x4c44u, 0);
    BD_W16(BD_S0 + 0x0bd4u, 0);
    BD_W16(BD_S0 + 0x4c4au, 216);
    BD_W16(BD_S0 + 0x0bdau, 216);
    entry->cpu->gpr[2] = 216;
    entry->cpu->gpr[31] = BD_R32(sp + 0x20u);
    BD_S1 = BD_R32(sp + 0x1cu);
    BD_S0 = BD_R32(sp + 0x18u);
    entry->cpu->gpr[29] = sp + 0x28u;
}

#undef BD_R32
#undef BD_W16
#undef BD_W32
#undef BD_CALL
#undef BD_S0
#undef BD_S1

static int battle_display_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                    uint32_t target)
{
    if (target != BATTLE_DISPLAY_ADDRESS || runtime->force_interpret ||
        !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_DISPLAY");
    if (control != NULL && strcmp(control, "0") == 0)
        return 0;
    uint8_t digest[32];
    const char *expected = BATTLE_DISPLAY_SHA256;
    file1_digest(PSX_ADDR(BATTLE_DISPLAY_ADDRESS), BATTLE_DISPLAY_BYTES, digest);
    for (unsigned i = 0; i < sizeof(digest); i++) {
        if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
            file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1])
            return 0;
    }
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0)
        return -1;
    battle_display_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

/* Native battle startup (src/battle/main101.c, func_800B8098).
 * Reuse the entry's checked memory/call helpers so each still-unconverted
 * display, actor or loaded-module callee fails closed in the native target.
 * Callee boundaries preserve the retail 0x20-byte frame and both stack args.
 */
#define BATTLE_STARTUP_ADDRESS 0x800b8098u
#define BATTLE_STARTUP_BYTES 0x124u
#define BATTLE_STARTUP_SHA256 \
    "fabcd91ad2aa55111fc92a54dfb9107f2e767ab8d1bf6337bc86baeed23cf186"
#define BATTLE_STARTUP_TABLE 0x80070a10u
#define BATTLE_STARTUP_TABLE_BYTES 24u
#define BATTLE_STARTUP_TABLE_SHA256 \
    "997503617c965694684c7402e456d24dac32a33fd1ff4c0c7e116181f16102a2"

#define BS_R32(a) battle_entry_read(entry, (a), 4)
#define BS_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define BS_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define BS_CALL0(target, site) battle_entry_call(entry, (target), (site), 0, NULL)
#define BS_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define BS_S0 (entry->cpu->gpr[16])

static void battle_startup_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x20u;
    entry->cpu->gpr[29] = sp;
    BS_W32(sp + 0x18u, BS_S0);
    BS_S0 = entry->cpu->gpr[4];
    BS_W32(sp + 0x1cu, entry->cpu->gpr[31]);
    BS_W8(0x800d36b8u, BS_S0);
    BS_CALL0(0x800b8284u, 0x800b80acu);
    BS_CALL0(0x8001bbacu, 0x800b80b4u);
    switch (BS_S0) {
    case 1:
        BS_CALL(0x80028a60u, 0x800b80e4u, 0);
        BS_CALL0(0x801e8588u, 0x800b80ecu);
        break;
    case 2:
        BS_CALL(0x80028a60u, 0x800b80fcu, 0);
        BS_CALL0(0x801e91e8u, 0x800b8104u);
        break;
    case 3:
        BS_CALL(0x80028a60u, 0x800b8114u, 0);
        BS_CALL0(0x801e9594u, 0x800b811cu);
        break;
    case 4:
        BS_CALL(0x80028a60u, 0x800b812cu, 0);
        BS_CALL0(0x801e893cu, 0x800b8134u);
        break;
    default:
        BS_CALL0(0x800b7870u, 0x800b8144u);
        break;
    }
    BS_CALL(0x80028a60u, 0x800b814cu, 0);
    BS_S0 = 0x800c0000u;
    BS_CALL0(0x800a8b0cu, 0x800b8154u);
    BS_S0 = 0x800c4a39u;
    uint32_t result = BS_CALL(0x801e7210u, 0x800b8188u, 0x8005949cu,
                             BS_R32(0x80059520u), BS_R32(0x80059470u),
                             0x800ccb94u, 0x800ccbb4u, BS_S0);
    BS_W8(0x800c4a38u, result);
    BS_CALL(0x800a5e9cu, 0x800b81a0u, BS_S0, 0x800c8aa9u);
    entry->cpu->gpr[31] = BS_R32(sp + 0x1cu);
    BS_S0 = BS_R32(sp + 0x18u);
    entry->cpu->gpr[29] = sp + 0x20u;
}

#undef BS_R32
#undef BS_W8
#undef BS_W32
#undef BS_CALL0
#undef BS_CALL
#undef BS_S0

static int battle_startup_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                    uint32_t target)
{
    if (target != BATTLE_STARTUP_ADDRESS || runtime->force_interpret ||
        !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_STARTUP");
    if (control != NULL && strcmp(control, "0") == 0)
        return 0;
    uint8_t digest[32];
    /* The C switch replaces a retail jump table outside the function body.
     * Both regions must match; a changed table must not select native code. */
    for (unsigned region = 0; region < 2; region++) {
        const char *expected = region ? BATTLE_STARTUP_TABLE_SHA256 : BATTLE_STARTUP_SHA256;
        file1_digest(PSX_ADDR(region ? BATTLE_STARTUP_TABLE : BATTLE_STARTUP_ADDRESS),
                     region ? BATTLE_STARTUP_TABLE_BYTES : BATTLE_STARTUP_BYTES, digest);
        for (unsigned i = 0; i < sizeof(digest); i++) {
            if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
                file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1])
                return 0;
        }
    }
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0)
        return -1;
    battle_startup_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

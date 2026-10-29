/* Archive 0x0c/file 4, loaded at 0x801e4000: transition loader stages and
 * initial party/resource setup. Both bodies identify this resident module;
 * addresses alone are insufficient because other modules overlay this range.
 * The remaining stages and callees keep their independent native/refusal path.
 */
#define BATTLE_MODULE_SETUP_ADDRESS 0x801e5384u
#define BATTLE_MODULE_SETUP_BYTES 0x4bcu
#define BATTLE_MODULE_SETUP_SHA256 \
    "e7a0cdba89318569f55dedcac5134df520eab4e352b4fe6495c7741dccb646a5"
#define BATTLE_MODULE_DISPATCH_ADDRESS 0x801e5840u
#define BATTLE_MODULE_DISPATCH_BYTES 0xe4u
#define BATTLE_MODULE_DISPATCH_SHA256 \
    "e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2"

#define BM_R8(a) battle_entry_read(entry, (a), 1)
#define BM_R16(a) battle_entry_read(entry, (a), 2)
#define BM_R32(a) battle_entry_read(entry, (a), 4)
#define BM_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define BM_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define BM_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define BM_CALL0(target, site) battle_entry_call(entry, (target), (site), 0, NULL)
#define BM_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define BM_S0 (entry->cpu->gpr[16])
#define BM_S1 (entry->cpu->gpr[17])
#define BM_S2 (entry->cpu->gpr[18])
#define BM_S3 (entry->cpu->gpr[19])
#define BM_S4 (entry->cpu->gpr[20])
#define BM_S5 (entry->cpu->gpr[21])
#define BM_S6 (entry->cpu->gpr[22])
#define BM_S7 (entry->cpu->gpr[23])
#define BM_FP (entry->cpu->gpr[30])

static void battle_module_setup_run(BattleEntryCall *entry)
{
    uint32_t special = BM_R8(0x8006f9ddu) & 0x10u;
    uint32_t sp = entry->cpu->gpr[29] - 0x48u;
    entry->cpu->gpr[29] = sp;
    BM_W32(sp + 0x44u, entry->cpu->gpr[31]);
    BM_W32(sp + 0x40u, BM_FP);
    for (unsigned r = 0; r < 8; r++)
        BM_W32(sp + 0x20u + r * 4u, entry->cpu->gpr[16 + r]);
    BM_W8(0x800d3294u, special != 0);
    BM_S3 = (BM_R16(0x8006f364u) & BM_R16(0x8006f366u)) & 0x7ffu;
    if (BM_R8(0x800d3294u) == 0) {
        BM_S0 = 0;
        BM_S1 = 0x8006f368u;
        BM_S2 = 0x8006f36bu;
        do {
            uint32_t selected = BM_CALL(0x80089c9cu, 0x801e541cu,
                                        BM_S3 & 0xffffu, BM_R8(BM_S1));
            if ((selected & 0xffffu) != 0) {
                BM_W8(0x800d2d24u + BM_S0, BM_R8(BM_S1) & 0x7fu);
                BM_S0++;
            }
            BM_S1++;
        } while ((int32_t)BM_S1 < (int32_t)BM_S2);
        while ((int32_t)BM_S0 < 3) {
            BM_W8(0x800d2d24u + BM_S0, 0x7f);
            BM_S0++;
        }
    } else {
        BM_W8(0x800d2d25u, 10);
        BM_W8(0x800d2d26u, 10);
        BM_W8(0x800d2d24u, BM_R8(0x8006f368u) & 0x7fu);
    }
    BM_S4 = 0;
    BM_FP = 0x800ccd8cu;
    BM_S7 = BM_FP + 0x2184u;
    BM_S6 = 0;
    BM_S3 = BM_FP + 0x5f98u;
    BM_S2 = 0;
    BM_CALL(0x8003342cu, 0x801e54d0u, BM_R32(0x800595a8u));
    BM_S5 = BM_R32(0x800595a8u);
    do {
        uint32_t actor = BM_R8(BM_S3);
        if (actor != 0x7fu) {
            BM_CALL(0x8003f99cu, 0x801e5518u, BM_S2 + 0x800ccce8u,
                    actor * 0xa4u + 0x8006d8a0u, 0xa4);
            if (BM_R8(0x800d3294u) != 0 && BM_S4 - 1u < 2u)
                BM_W8(BM_S2 + 0x800ccd88u, 0x11);
            uint32_t gear = BM_R8(BM_S2 + 0x800ccd88u);
            if (gear == 0xffu) gear = 0;
            BM_S0 = gear * 4u;
            BM_CALL(0x8003f99cu, 0x801e5590u, BM_S2 + BM_FP,
                    gear * 0xa4u + 0x8006dfacu, 0xa4);
            BM_S1 = BM_CALL(0x80032e88u, 0x801e55acu,
                           BM_R32(BM_S5 + BM_R8(BM_S3) * 4u + 0x14u), 1);
            BM_CALL(0x8003f99cu, 0x801e55c4u, BM_FP + 0xfb4u + BM_S6, BM_S1, 0x5f0);
            BM_CALL(0x800320e8u, 0x801e55ccu, BM_S1);
            BM_S0 += BM_S5;
            BM_S1 = BM_CALL(0x80032e88u, 0x801e55dcu, BM_R32(BM_S0 + 0x44u), 1);
            BM_CALL(0x8003f99cu, 0x801e55f0u, BM_S7, BM_S1, 0x690);
            BM_CALL(0x800320e8u, 0x801e55f8u, BM_S1);
        }
        BM_S7 += 0x690u;
        BM_S6 += 0x5f0u;
        BM_S3++;
        BM_S4++;
        BM_S2 += 0x170u;
    } while ((int32_t)BM_S4 < 3);

    BM_S1 = BM_CALL(0x80032e88u, 0x801e5620u, BM_R32(BM_S5 + 0x10u), 1);
    BM_S0 = 0x800d02c0u;
    BM_CALL(0x8003f99cu, 0x801e563cu, BM_S0, BM_S1, 0x1f40);
    BM_CALL(0x800320e8u, 0x801e5644u, BM_S1);
    BM_S1 = BM_CALL(0x80032e88u, 0x801e5650u, BM_R32(BM_S5 + 0xcu), 1);
    BM_CALL(0x8003f99cu, 0x801e5664u, BM_S0 + 0x1f40u, BM_S1, 0x300);
    BM_CALL(0x800320e8u, 0x801e566cu, BM_S1);
    BM_S1 = BM_CALL(0x80032e88u, 0x801e5678u, BM_R32(BM_S5 + 8u), 1);
    BM_CALL(0x8002dde4u, 0x801e569cu, BM_S1, 0, 0, 0, 0, 0, 0);
    BM_CALL(0x800320e8u, 0x801e56a4u, BM_S1);
    BM_W32(0x800d2f5cu, BM_CALL(0x80032e88u, 0x801e56b0u, BM_R32(BM_S5 + 4u), 0));
    BM_CALL(0x80033698u, 0x801e56c4u, 0, 0x1f0);
    uint32_t retained = BM_CALL(0x80032e88u, 0x801e56d0u, BM_R32(BM_S5 + 0x40u), 0);
    uint32_t source = BM_R32(BM_S5 + 0x90u);
    BM_W32(0x800d329cu, retained);
    BM_S1 = BM_CALL(0x80032e88u, 0x801e56e4u, source, 1);
    BM_CALL(0x80078310u, 0x801e56f4u, BM_S1, 0x61);
    BM_CALL(0x800320e8u, 0x801e56fcu, BM_S1);
    BM_S1 = BM_CALL(0x80032e88u, 0x801e5708u, BM_R32(BM_S5 + 0x94u), 1);
    BM_CALL(0x8003f99cu, 0x801e571cu, BM_S0 + 0x2240u, BM_S1 + 0x320u, 0x300);
    BM_CALL(0x800320e8u, 0x801e5724u, BM_S1);
    retained = BM_CALL(0x80032e88u, 0x801e5730u, BM_R32(BM_S5 + 0x98u), 0);
    source = BM_R32(0x800595a8u);
    BM_W32(0x800d39f0u, retained);
    BM_CALL(0x800320e8u, 0x801e5748u, source);
    BM_CALL(0x80028470u, 0x801e5754u, 0xc, 1);
    BM_S0 = 0x8006f9dcu;
    uint32_t size = BM_CALL(0x800288ecu, 0x801e5770u, BM_R8(BM_S0) * 2u + 2u);
    uint32_t buffer = BM_CALL(0x8008abb8u, 0x801e577cu, size, 0);
    uint32_t scenario = BM_R8(BM_S0);
    BM_S1 = 0x800d33e8u;
    BM_W32(0x800c3dd0u, buffer);
    BM_W32(0x800d33ecu, buffer);
    BM_W16(BM_S1, BM_R8(BM_S0) * 2u + 2u);
    size = BM_CALL(0x800288ecu, 0x801e57b4u, scenario * 2u + 3u);
    buffer = BM_CALL(0x8008abb8u, 0x801e57c0u, size, 1);
    scenario = BM_R8(BM_S0);
    BM_W32(0x800c3decu, buffer);
    BM_W32(0x800d33f4u, buffer);
    BM_W16(0x800d33f8u, 0);
    BM_W32(0x800d33fcu, 0);
    BM_W16(0x800d33f0u, scenario * 2u + 3u);
    BM_CALL(0x80029afcu, 0x801e5804u, BM_S1, 0, 0x80);
    entry->cpu->gpr[31] = BM_R32(sp + 0x44u);
    BM_FP = BM_R32(sp + 0x40u);
    for (unsigned r = 0; r < 8; r++)
        entry->cpu->gpr[16 + r] = BM_R32(sp + 0x20u + r * 4u);
    entry->cpu->gpr[29] = sp + 0x48u;
}

static void battle_module_dispatch_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x18u;
    entry->cpu->gpr[29] = sp;
    entry->cpu->gpr[4] &= 0xffu;
    uint32_t stage = entry->cpu->gpr[4];
    BM_W32(sp + 0x10u, entry->cpu->gpr[31]);
    entry->cpu->gpr[2] = stage < 2u ? 1u : 3u;
    switch (stage) {
    case 0: BM_CALL0(0x801e5384u, 0x801e588cu); break;
    case 1:
        BM_CALL0(0x801e4048u, 0x801e589cu);
        BM_CALL0(0x801e4160u, 0x801e58a4u);
        BM_CALL0(0x801e4870u, 0x801e58acu);
        BM_CALL0(0x801e4ac0u, 0x801e58b4u);
        break;
    case 2:
        BM_CALL0(0x801e4cd0u, 0x801e58c4u);
        BM_CALL0(0x801e4e7cu, 0x801e58ccu);
        BM_CALL0(0x801e5014u, 0x801e58d4u);
        BM_W32(0x800c3e24u, BM_CALL(0x8008abb8u, 0x801e58e0u, 0xec, 0));
        BM_CALL(0x8003f8e8u, 0x801e58f4u, entry->cpu->gpr[2], 0xec);
        break;
    case 3:
        BM_CALL0(0x801e6290u, 0x801e5904u);
        BM_CALL0(0x801e62b8u, 0x801e590cu);
        break;
    }
    entry->cpu->gpr[31] = BM_R32(sp + 0x10u);
    entry->cpu->gpr[29] = sp + 0x18u;
}

#undef BM_R8
#undef BM_R16
#undef BM_R32
#undef BM_W8
#undef BM_W16
#undef BM_W32
#undef BM_CALL0
#undef BM_CALL
#undef BM_S0
#undef BM_S1
#undef BM_S2
#undef BM_S3
#undef BM_S4
#undef BM_S5
#undef BM_S6
#undef BM_S7
#undef BM_FP

static int battle_module_setup_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                         uint32_t target)
{
    if ((target != BATTLE_MODULE_SETUP_ADDRESS && target != BATTLE_MODULE_DISPATCH_ADDRESS) ||
        runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_MODULE_SETUP");
    if (control != NULL && strcmp(control, "0") == 0) return 0;
    uint8_t digest[32];
    /* These two code regions identify only the adopted pair, not every byte
     * or callee in the module. Mutable game/loader data remains live. */
    for (unsigned region = 0; region < 2; region++) {
        uint32_t address = region ? BATTLE_MODULE_DISPATCH_ADDRESS : BATTLE_MODULE_SETUP_ADDRESS;
        unsigned bytes = region ? BATTLE_MODULE_DISPATCH_BYTES : BATTLE_MODULE_SETUP_BYTES;
        const char *expected = region ? BATTLE_MODULE_DISPATCH_SHA256 : BATTLE_MODULE_SETUP_SHA256;
        file1_digest(PSX_ADDR(address), bytes, digest);
        for (unsigned i = 0; i < sizeof(digest); i++)
            if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
                file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1]) return 0;
    }
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0) return -1;
    if (target == BATTLE_MODULE_SETUP_ADDRESS) battle_module_setup_run(&entry);
    else battle_module_dispatch_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

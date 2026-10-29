/* Native opening battle transition (src/battle/mainc99.c, 800B7870).
 * Preserve the framebuffer copy, alternating guest buffers, loader states,
 * scratchpad callback stack and Vsync(2) cadence. Callees still use native
 * dispatch and refuse when an implementation is missing in the strict build.
 */
#define BATTLE_TRANSITION_ADDRESS 0x800b7870u
#define BATTLE_TRANSITION_BYTES 0x3b8u
#define BATTLE_TRANSITION_SHA256 \
    "98b082a1b8b09298d58fcdf513a73da26b26cc4e46a44e7ca159e541693e1321"
#define BATTLE_TRANSITION_TABLE 0x800709fcu
#define BATTLE_TRANSITION_TABLE_BYTES 20u
#define BATTLE_TRANSITION_TABLE_SHA256 \
    "f8d2b85319a5ddcb5e31ce1ac671365b3b728957de9112f62e479a3a3f500f0d"

#define BT_R8(a) battle_entry_read(entry, (a), 1)
#define BT_R16(a) battle_entry_read(entry, (a), 2)
#define BT_R32(a) battle_entry_read(entry, (a), 4)
#define BT_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define BT_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define BT_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define BT_CALL0(target, site) battle_entry_call(entry, (target), (site), 0, NULL)
#define BT_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define BT_S0 (entry->cpu->gpr[16])
#define BT_S1 (entry->cpu->gpr[17])
#define BT_S2 (entry->cpu->gpr[18])
#define BT_S3 (entry->cpu->gpr[19])
#define BT_S4 (entry->cpu->gpr[20])
#define BT_S5 (entry->cpu->gpr[21])

static void battle_transition_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x38u;
    entry->cpu->gpr[29] = sp;
    BT_W32(sp + 0x24u, BT_S3);
    BT_S3 = 0x56;
    BT_W32(sp + 0x30u, entry->cpu->gpr[31]);
    BT_W32(sp + 0x2cu, BT_S5);
    BT_W32(sp + 0x28u, BT_S4);
    BT_W32(sp + 0x20u, BT_S2);
    BT_W32(sp + 0x1cu, BT_S1);
    BT_W32(sp + 0x18u, BT_S0);
    BT_CALL0(0x8001c944u, 0x800b7890u); /* WorkListsReset */
    BT_S2 = 1; /* loader state */
    BT_S5 = 0; /* loaded-module stage */
    BT_S1 = BT_CALL(0x80031bdcu, 0x800b78a4u, 0x30000, BT_S2);
    BT_S0 = BT_S1;
    BT_W16(sp + 0x14u, 320);
    BT_W16(sp + 0x10u, 0);
    BT_W16(sp + 0x12u, 0);
    BT_W16(sp + 0x16u, 224);
    BT_CALL(0x800448f8u, 0x800b78d0u, sp + 0x10u, BT_S1); /* StoreImage */
    BT_CALL(0x800445d0u, 0x800b78d8u, BT_S5); /* DrawSync */
    /* Retail touches 0x14000 halfwords, including padding beyond 320x224. */
    for (uint32_t i = 0; i < 0x14000u; i++, BT_S0 += 2u)
        BT_W16(BT_S0, BT_R16(BT_S0) | 0x8000u);
    BT_W16(sp + 0x10u, 0x2c0);
    BT_W16(sp + 0x12u, 0x100);
    BT_W16(sp + 0x14u, 320);
    BT_W16(sp + 0x16u, 224);
    BT_CALL(0x80044894u, 0x800b7928u, sp + 0x10u, BT_S1); /* LoadImage */
    BT_CALL(0x800445d0u, 0x800b7930u, 0);
    BT_CALL(0x800320e8u, 0x800b7938u, BT_S1);

    BT_S0 = 0x800c3eb0u;
    BT_S1 = BT_S0 + 0x8000u;
    BT_S4 = BT_S0 + 0xb70u;
    uint32_t next = BT_R32(BT_S1 + 0xc50u) == BT_S4 ? BT_S0 + 0x4be0u : BT_S4;
    BT_W32(BT_S1 + 0xc50u, next);
    BT_W32(BT_S1 + 0xc54u, next + 0x70u);
    BT_CALL(0x80044ad8u, 0x800b7974u, next + 0x70u, 0x1000); /* ClearOTagR */
    BT_W32(BT_S1 + 0xc84u, 0);
    BT_W32(BT_S1 + 0xc50u, BT_S4);
    BT_W8(BT_S0 + 0xb88u, 1);
    BT_W8(BT_S0 + 0x4bf8u, 1);
    for (uint32_t channel = 0; channel < 3; channel++) {
        BT_W8(BT_S0 + 0xb89u + channel, 0xff);
        BT_W8(BT_S0 + 0x4bf9u + channel, 0xff);
    }
    BT_S4 = BT_CALL0(0x800b73ecu, 0x800b79a8u);
    while (BT_S3 != 0 || BT_S2 != 5) {
        if ((int32_t)BT_S3 > 0)
            BT_S3--;
        BT_S0 = 0x800cbeb0u;
        next = BT_R32(BT_S0 + 0xc50u) == 0x800c4a20u ? 0x800c8a90u : 0x800c4a20u;
        BT_W32(BT_S0 + 0xc50u, next);
        BT_W32(BT_S0 + 0xc54u, next + 0x70u);
        BT_CALL(0x80044ad8u, 0x800b79f0u, next + 0x70u, 0x1000);
        BT_W32(BT_S0 + 0xc84u, 1u - BT_R32(BT_S0 + 0xc84u));
        BT_W32(0x800c3cb4u, BT_R32(BT_S0 + 0xc54u));
        if (BT_CALL0(0x800286ccu, 0x800b7a10u) == 0) { /* ArchiveDataSync */
            switch (BT_S2) {
            case 2:
                BT_S2++;
                BT_CALL0(0x8001bb0cu, 0x800b7a44u);
                break;
            case 1: case 3: case 4:
                BT_CALL(0x801e5840u, 0x800b7a54u, BT_S5 & 0xffu);
                BT_S5++;
                BT_S2++;
                break;
            default:
                break;
            }
        }
        BT_CALL0(0x80019ca0u, 0x800b7a64u); /* soft reset */
        BT_W32(0x1f8003fcu, entry->cpu->gpr[29]);
        entry->cpu->gpr[29] = 0x1f8003f8u;
        BT_CALL(0x800b6f0cu, 0x800b7a84u, BT_S4);
        BT_CALL(0x800b7160u, 0x800b7a8cu, BT_S4);
        entry->cpu->gpr[29] = BT_R32(entry->cpu->gpr[29] + 4u);
        BT_CALL(0x800445d0u, 0x800b7a9cu, 0);
        BT_CALL(0x8004b54cu, 0x800b7aa4u, 2); /* Vsync */
        BT_S1 = 0x800c3eb0u;
        BT_S0 = BT_S1 + 0x8000u;
        for (uint32_t channel = 0; channel < 3; channel++) {
            uint32_t offset = 0xb89u + channel;
            uint32_t color = BT_R8(BT_S1 + BT_R32(BT_S0 + 0xc84u) * 0x4070u + offset);
            color = BT_CALL(0x80021ad8u, 0x800b7ae0u + channel * 0x50u, color, 0xfffffff4u);
            /* The callee may update the index; retail reloads it for each store. */
            BT_W8(BT_S1 + BT_R32(BT_S0 + 0xc84u) * 0x4070u + offset, color);
        }
        BT_CALL(0x80044e9cu, 0x800b7bb0u, BT_R32(BT_S0 + 0xc50u) + 0x5cu);
        BT_CALL(0x80044c44u, 0x800b7bbcu, BT_R32(BT_S0 + 0xc50u));
        BT_CALL(0x80044bd0u, 0x800b7bc8u, BT_R32(BT_S0 + 0xc50u) + 0x406cu);
    }
    BT_CALL(0x800b7330u, 0x800b7be0u, BT_S4);
    BT_CALL(0x80044534u, 0x800b7be8u, 0);
    BT_CALL(0x80028a60u, 0x800b7bf0u, 0);
    BT_CALL(0x801e5840u, 0x800b7bf8u, 3);
    sp = entry->cpu->gpr[29];
    entry->cpu->gpr[31] = BT_R32(sp + 0x30u);
    BT_S5 = BT_R32(sp + 0x2cu);
    BT_S4 = BT_R32(sp + 0x28u);
    BT_S3 = BT_R32(sp + 0x24u);
    BT_S2 = BT_R32(sp + 0x20u);
    BT_S1 = BT_R32(sp + 0x1cu);
    BT_S0 = BT_R32(sp + 0x18u);
    entry->cpu->gpr[29] = sp + 0x38u;
}

#undef BT_R8
#undef BT_R16
#undef BT_R32
#undef BT_W8
#undef BT_W16
#undef BT_W32
#undef BT_CALL0
#undef BT_CALL
#undef BT_S0
#undef BT_S1
#undef BT_S2
#undef BT_S3
#undef BT_S4
#undef BT_S5

static int battle_transition_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                       uint32_t target)
{
    if (target != BATTLE_TRANSITION_ADDRESS || runtime->force_interpret ||
        !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_TRANSITION");
    if (control != NULL && strcmp(control, "0") == 0)
        return 0;
    uint8_t digest[32];
    for (unsigned region = 0; region < 2; region++) {
        const char *expected = region ? BATTLE_TRANSITION_TABLE_SHA256 : BATTLE_TRANSITION_SHA256;
        file1_digest(PSX_ADDR(region ? BATTLE_TRANSITION_TABLE : BATTLE_TRANSITION_ADDRESS),
                     region ? BATTLE_TRANSITION_TABLE_BYTES : BATTLE_TRANSITION_BYTES, digest);
        for (unsigned i = 0; i < sizeof(digest); i++) {
            if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
                file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1])
                return 0;
        }
    }
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0)
        return -1;
    battle_transition_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

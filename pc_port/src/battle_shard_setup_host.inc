/* Native shard allocation/initialization for the opening transition.
 * The 2x14x20 grid uses 0x7c-byte guest records and two 0x20-byte triangles
 * per shard. Keep guest frames and native SDK/RNG calls in retail order.
 * No original CPU instruction stream is executed by these bodies.
 */
#define BATTLE_SHARD_ALLOC_ADDRESS 0x800b73ecu
#define BATTLE_SHARD_ALLOC_BYTES 0x38u
#define BATTLE_SHARD_ALLOC_SHA256 \
    "4b4584aa569b7bf0707ee0a59028d0bf82443e6bdd54e2aa9916aa6d15ce3f95"
#define BATTLE_SHARD_INIT_ADDRESS 0x800b7424u
#define BATTLE_SHARD_INIT_BYTES 0x44cu
#define BATTLE_SHARD_INIT_SHA256 \
    "664c99b631d33992fa179242478bf1764771525d890c5359f7905378110ab729"

#define BH_R8(a) battle_entry_read(entry, (a), 1)
#define BH_R32(a) battle_entry_read(entry, (a), 4)
#define BH_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define BH_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define BH_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define BH_CALL0(target, site) battle_entry_call(entry, (target), (site), 0, NULL)
#define BH_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define BH_S0 (entry->cpu->gpr[16])
#define BH_S1 (entry->cpu->gpr[17])
#define BH_S2 (entry->cpu->gpr[18])
#define BH_S3 (entry->cpu->gpr[19])
#define BH_S4 (entry->cpu->gpr[20])
#define BH_S5 (entry->cpu->gpr[21])
#define BH_S6 (entry->cpu->gpr[22])
#define BH_S7 (entry->cpu->gpr[23])
#define BH_FP (entry->cpu->gpr[30])

/* The original remainder calculations leave their multiplication result in
 * HI/LO across SDK calls. Retain that boundary state while expressing the
 * actual calculation as native signed remainder. */
static uint32_t battle_shard_remainder(BattleEntryCall *entry, uint32_t value,
                                        uint32_t multiplier, int32_t divisor)
{
    int64_t product = (int64_t)(int32_t)value * (int32_t)multiplier;
    entry->cpu->hi = (uint32_t)((uint64_t)product >> 32);
    entry->cpu->lo = (uint32_t)product;
    return (uint32_t)((int32_t)value % divisor);
}

static uint32_t battle_shard_scale(BattleEntryCall *entry, uint32_t value,
                                    uint32_t radius)
{
    int32_t numerator = (int32_t)(value << 11);
    int32_t denominator = (int32_t)radius;
    if (denominator == 0) {
        entry->cpu->lo = numerator < 0 ? 1u : UINT32_MAX;
        entry->cpu->hi = (uint32_t)numerator;
    } else if (numerator == INT32_MIN && denominator == -1) {
        entry->cpu->lo = (uint32_t)INT32_MIN;
        entry->cpu->hi = 0;
    } else {
        entry->cpu->lo = (uint32_t)(numerator / denominator);
        entry->cpu->hi = (uint32_t)(numerator % denominator);
    }
    return entry->cpu->lo;
}

static void battle_shard_allocate_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x18u;
    entry->cpu->gpr[29] = sp;
    BH_W32(sp + 0x10u, entry->cpu->gpr[31]);
    uint32_t task = BH_CALL(0x80031bdcu, 0x800b73fcu, 0x10f7c, 1);
    BH_W32(task + 4u, task);
    BH_W32(task + 0x20u, task);
    BH_CALL(0x800b7424u, 0x800b740cu, task);
    /* Preserve the initializer's result; the recovered void prototype loses
     * the task pointer consumed by the transition. */
    entry->cpu->gpr[31] = BH_R32(sp + 0x10u);
    entry->cpu->gpr[29] = sp + 0x18u;
}

static void battle_shard_initialize_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0xa0u;
    entry->cpu->gpr[29] = sp;
    BH_W32(sp + 0x9cu, entry->cpu->gpr[31]);
    BH_W32(sp + 0x98u, BH_FP);
    for (unsigned r = 0; r < 8; r++)
        BH_W32(sp + 0x78u + r * 4u, entry->cpu->gpr[16 + r]);
    BH_W32(sp + 0xa0u, entry->cpu->gpr[4]);
    BH_W32(entry->cpu->gpr[4] + 0x38u, 0);
    uint32_t radius = BH_CALL(0x80048c4cu, 0x800b7458u, 0x9500);
    BH_W32(sp + 0x48u, radius << 10);
    BH_W32(sp + 0x50u, 0); /* grid half */
    BH_W32(sp + 0x70u, 0x3c); /* half's first record */
    do {
        BH_W32(sp + 0x4cu, 0); /* row */
        BH_W32(sp + 0x64u, 0); /* row offset */
        BH_W32(sp + 0x68u, 0xfffff360u);
        BH_W32(sp + 0x6cu, 0xfffff2a0u);
        do {
            BH_FP = 0;
            BH_W32(sp + 0x54u, BH_R32(sp + 0x4cu) << 4);
            BH_S7 = 0;
            BH_W32(sp + 0x58u, BH_R32(sp + 0x6cu));
            BH_W32(sp + 0x5cu, BH_R32(sp + 0x68u));
            do {
                BH_S2 = BH_R32(sp + 0xa0u) + BH_R32(sp + 0x70u) +
                        BH_R32(sp + 0x64u) + BH_FP * 0x7cu;
                BH_W16(BH_S2, 0);
                BH_W16(BH_S2 + 2u, 0);
                BH_W16(BH_S2 + 4u, 0);
                if (BH_R32(sp + 0x50u) == 0) {
                    BH_W32(BH_S2 + 0x10u, (BH_S7 - 155u) << 5);
                    BH_W32(BH_S2 + 0x18u, 0x4000);
                    BH_W32(BH_S2 + 0x14u, BH_R32(sp + 0x58u));
                } else {
                    BH_W32(BH_S2 + 0x10u, (BH_S7 - 149u) << 5);
                    BH_W32(BH_S2 + 0x14u, BH_R32(sp + 0x5cu));
                    BH_W32(BH_S2 + 0x18u, 0x4000);
                }
                BH_W32(0x800c35ccu, 0xfe0c0000u);
                uint32_t random = BH_CALL0(0x8003fa38u, 0x800b7534u);
                uint32_t remainder = battle_shard_remainder(entry, random, 0x10624dd3u, 1000);
                BH_W32(0x800c35ccu, BH_R32(0x800c35ccu) - (remainder << 16));
                BH_CALL(0x8004a414u, 0x800b758cu, BH_S2 + 0x10u, sp + 0x10u);
                BH_S1 = BH_CALL(0x80048c4cu, 0x800b759cu,
                               BH_R32(sp + 0x10u) + BH_R32(sp + 0x14u));
                /* Retail writes this distance-derived delay, then replaces
                 * it before ratan2. Keep both stores and signed truncation. */
                uint32_t delay = (uint32_t)((int32_t)BH_R32(sp + 0x48u) / 32) - BH_S1;
                BH_W32(BH_S2 + 0x64u, (uint32_t)((int32_t)delay / 2048));
                BH_W32(BH_S2 + 0x64u, (uint32_t)((int32_t)BH_S1 / 1024));
                BH_S0 = BH_CALL(0x8004b32cu, 0x800b75ecu,
                               BH_R32(BH_S2 + 0x14u), BH_R32(BH_S2 + 0x10u));
                random = BH_CALL0(0x8003fa38u, 0x800b75f4u);
                BH_S0 += 0x600u;
                BH_S1 = battle_shard_scale(entry, BH_S1, BH_R32(sp + 0x48u));
                BH_S3 = BH_S0 + (uint32_t)((int32_t)random % 1024);
                random = BH_CALL0(0x8003fa38u, 0x800b762cu);
                BH_S1 = BH_S1 - 32u + (uint32_t)((int32_t)random % 64);
                BH_S0 = sp + 0x28u;
                BH_W16(sp + 0x20u, 0);
                BH_W16(sp + 0x22u, BH_S1);
                BH_W16(sp + 0x24u, BH_S3);
                BH_CALL(0x8004abbcu, 0x800b7668u, sp + 0x20u, BH_S0);
                BH_CALL(0x8004947cu, 0x800b767cu, BH_S0, 0x800c35c4u, BH_S2 + 0x68u);
                BH_S6 = 0;
                random = BH_CALL0(0x8003fa38u, 0x800b7684u);
                remainder = battle_shard_remainder(entry, random, 0x51eb851fu, 1600);
                BH_S1 = BH_S7 & 0x3fu;
                BH_S5 = BH_S1 + 16u;
                BH_S3 = 0x24;
                BH_W32(sp + 0x60u, BH_S7);
                BH_S4 = BH_R32(sp + 0x54u) + 16u;
                BH_W32(BH_S2 + 0x78u, 0x70800u - (remainder << 8));
                random = BH_CALL0(0x8003fa38u, 0x800b76e8u);
                BH_W16(BH_S2 + 8u, (random & 0xffu) - 127u);
                random = BH_CALL0(0x8003fa38u, 0x800b76f8u);
                BH_W16(BH_S2 + 0xau, (random & 0xffu) - 127u);
                random = BH_CALL0(0x8003fa38u, 0x800b7708u);
                BH_W16(BH_S2 + 0xcu, (random & 0x1ffu) - 255u);
                do {
                    BH_S0 = BH_S2 + BH_S3;
                    BH_CALL(0x80043c60u, 0x800b7720u, BH_S0);
                    BH_CALL(0x80043c24u, 0x800b772cu, BH_S0, 0);
                    BH_W8(BH_S0 + 4u, 0xff);
                    BH_W8(BH_S0 + 6u, 0xff);
                    BH_W8(BH_S0 + 5u, 0xff);
                    BH_W8(BH_S0 + 7u, BH_R8(BH_S0 + 7u) & 0xfdu);
                    uint32_t page = BH_CALL(0x80043a1cu, 0x800b7764u,
                                           2, 1, BH_R32(sp + 0x60u) + 0x2c0u, 0x100);
                    BH_W16(BH_S0 + 0x16u, page);
                    if (BH_R32(sp + 0x50u) == 0) {
                        BH_W8(BH_S0 + 0xcu, BH_S1);
                        BH_W8(BH_S0 + 0x14u, BH_S5);
                        BH_W8(BH_S0 + 0xdu, BH_R8(sp + 0x54u));
                        BH_W8(BH_S0 + 0x1cu, BH_S1);
                        BH_W8(BH_S0 + 0x1du, BH_S4);
                        BH_W8(BH_S0 + 0x15u, BH_R8(sp + 0x54u));
                    } else {
                        BH_W8(BH_S0 + 0xcu, BH_S5);
                        BH_W8(BH_S0 + 0x14u, BH_S5);
                        BH_W8(BH_S0 + 0x15u, BH_S4);
                        BH_W8(BH_S0 + 0x1cu, BH_S1);
                        BH_W8(BH_S0 + 0x1du, BH_S4);
                        BH_W8(BH_S0 + 0xdu, BH_R8(sp + 0x54u));
                    }
                    BH_S6++;
                    BH_S3 += 0x20u;
                } while (BH_S6 != 2);
                BH_FP++;
                BH_S7 += 16u;
            } while (BH_FP != 20);
            BH_W32(sp + 0x64u, BH_R32(sp + 0x64u) + 0x9b0u);
            BH_W32(sp + 0x68u, BH_R32(sp + 0x68u) + 0x200u);
            BH_W32(sp + 0x6cu, BH_R32(sp + 0x6cu) + 0x200u);
            BH_W32(sp + 0x4cu, BH_R32(sp + 0x4cu) + 1u);
        } while (BH_R32(sp + 0x4cu) != 14);
        BH_W32(sp + 0x70u, BH_R32(sp + 0x70u) + 0x87a0u);
        BH_W32(sp + 0x50u, BH_R32(sp + 0x50u) + 1u);
    } while (BH_R32(sp + 0x50u) != BH_S6);
    entry->cpu->gpr[2] = BH_R32(sp + 0xa0u);
    entry->cpu->gpr[31] = BH_R32(sp + 0x9cu);
    BH_FP = BH_R32(sp + 0x98u);
    for (unsigned r = 0; r < 8; r++)
        entry->cpu->gpr[16 + r] = BH_R32(sp + 0x78u + r * 4u);
    entry->cpu->gpr[29] = sp + 0xa0u;
}

#undef BH_R8
#undef BH_R32
#undef BH_W8
#undef BH_W16
#undef BH_W32
#undef BH_CALL0
#undef BH_CALL
#undef BH_S0
#undef BH_S1
#undef BH_S2
#undef BH_S3
#undef BH_S4
#undef BH_S5
#undef BH_S6
#undef BH_S7
#undef BH_FP

static int battle_shard_setup_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                        uint32_t target)
{
    if ((target != BATTLE_SHARD_ALLOC_ADDRESS && target != BATTLE_SHARD_INIT_ADDRESS) ||
        runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_SHARD_SETUP");
    if (control != NULL && strcmp(control, "0") == 0) return 0;
    int allocating = target == BATTLE_SHARD_ALLOC_ADDRESS;
    const char *expected = allocating ? BATTLE_SHARD_ALLOC_SHA256 : BATTLE_SHARD_INIT_SHA256;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target), allocating ? BATTLE_SHARD_ALLOC_BYTES : BATTLE_SHARD_INIT_BYTES, digest);
    for (unsigned i = 0; i < sizeof(digest); i++) {
        if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
            file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1]) return 0;
    }
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0) return -1;
    if (allocating) battle_shard_allocate_run(&entry);
    else battle_shard_initialize_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

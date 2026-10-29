/* Native opening-transition shard callbacks. The 560 records, double-buffered
 * packets and real scratchpad frames remain in the game address space. */
#define BATTLE_SHARD_UPDATE_ADDRESS 0x800b6f0cu
#define BATTLE_SHARD_UPDATE_BYTES 0x228u
#define BATTLE_SHARD_UPDATE_SHA256 \
    "40857615e2893015fee79945c85827fc7192c80d3010461d6e1c3210f64d711e"
#define BATTLE_SHARD_RENDER_ADDRESS 0x800b7160u
#define BATTLE_SHARD_RENDER_BYTES 0x1d0u
#define BATTLE_SHARD_RENDER_SHA256 \
    "41e103d63547609e67d0796adaf1c9e9143e1c111132a62854c5feeed353aebf"
#define BATTLE_SHARD_CLEANUP_ADDRESS 0x800b7330u
#define BATTLE_SHARD_CLEANUP_BYTES 0x34u
#define BATTLE_SHARD_CLEANUP_SHA256 \
    "cbbfde0b27001af6425f36e435bd8cd5ba21eb4b27155157b70b11f39f6c90ed"

#define SC_R8(a) battle_entry_read(entry, (a), 1)
#define SC_R16(a) battle_entry_read(entry, (a), 2)
#define SC_R32(a) battle_entry_read(entry, (a), 4)
#define SC_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define SC_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define SC_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define SC_CALL0(target, site) battle_entry_call(entry, (target), (site), 0, NULL)
#define SC_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define SC_S0 (entry->cpu->gpr[16])
#define SC_S1 (entry->cpu->gpr[17])
#define SC_S2 (entry->cpu->gpr[18])
#define SC_S3 (entry->cpu->gpr[19])
#define SC_S4 (entry->cpu->gpr[20])
#define SC_S5 (entry->cpu->gpr[21])
#define SC_S6 (entry->cpu->gpr[22])
#define SC_S7 (entry->cpu->gpr[23])
#define SC_FP (entry->cpu->gpr[30])
#define SC_V0 (entry->cpu->gpr[2])
#define SC_V1 (entry->cpu->gpr[3])

static void battle_shard_update_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x48u;
    entry->cpu->gpr[29] = sp;
    for (unsigned r = 0; r < 8; r++)
        SC_W32(sp + 0x20u + r * 4u, entry->cpu->gpr[16 + r]);
    SC_W32(sp + 0x40u, SC_FP);
    SC_W32(sp + 0x44u, entry->cpu->gpr[31]);
    SC_FP = 0;
    uint32_t set = SC_R32(entry->cpu->gpr[4] + 4u);
    SC_W32(sp + 0x18u, set);
    SC_W32(set + 0x38u, SC_R32(set + 0x38u) + 1u);
    SC_S6 = 0x3c;
    do {
        SC_S5 = SC_S4 = 0;
        do {
            SC_S3 = SC_S2 = 0;
            SC_S7 = SC_R32(sp + 0x18u) + SC_S6;
            do {
                SC_S1 = SC_S7 + SC_S4 + SC_S2;
                uint32_t delay = SC_R32(SC_S1 + 0x64u);
                if (delay != 0) {
                    SC_W32(SC_S1 + 0x64u, delay - 1u);
                } else {
                    SC_S0 = SC_S1 + 0x24u + (SC_R32(0x800ccb34u) << 5);
                    SC_CALL(0x80021ad8u, 0x800b6fb8u, SC_R8(SC_S0 + 4u), (uint32_t)-6);
                    uint32_t green = SC_R8(SC_S0 + 5u);
                    SC_W8(SC_S0 + 4u, SC_V0);
                    SC_CALL(0x80021ad8u, 0x800b6fc8u, green, (uint32_t)-6);
                    uint32_t blue = SC_R8(SC_S0 + 6u);
                    SC_W8(SC_S0 + 5u, SC_V0);
                    SC_CALL(0x80021ad8u, 0x800b6fd8u, blue, (uint32_t)-6);
                    SC_W8(SC_S0 + 6u, SC_V0);
                    uint32_t vx = (uint32_t)(int32_t)(int16_t)SC_R16(SC_S1 + 0x6au);
                    SC_W16(sp + 0x10u, vx);
                    SC_W16(sp + 0x12u, SC_R16(SC_S1 + 0x6eu));
                    SC_W16(sp + 0x14u, SC_R16(SC_S1 + 0x72u));
                    uint32_t yspeed = SC_R32(SC_S1 + 0x6cu);
                    SC_W32(SC_S1 + 0x10u, SC_R32(SC_S1 + 0x10u) + vx);
                    SC_W32(SC_S1 + 0x14u, SC_R32(SC_S1 + 0x14u) +
                            (uint32_t)(int32_t)(int16_t)SC_R16(sp + 0x12u));
                    uint32_t vz = (uint32_t)(int32_t)(int16_t)SC_R16(sp + 0x14u);
                    for (unsigned axis = 0; axis < 3; axis++)
                        SC_W16(SC_S1 + axis * 2u, SC_R16(SC_S1 + axis * 2u) +
                                SC_R16(SC_S1 + 8u + axis * 2u));
                    SC_W32(SC_S1 + 0x18u, SC_R32(SC_S1 + 0x18u) + vz);
                    uint32_t xspeed = SC_R32(SC_S1 + 0x68u);
                    SC_W32(SC_S1 + 0x6cu, yspeed - (uint32_t)((int32_t)yspeed / 16));
                    uint32_t zspeed = SC_R32(SC_S1 + 0x70u);
                    SC_W32(SC_S1 + 0x68u, xspeed - (uint32_t)((int32_t)xspeed / 16));
                    SC_V1 = SC_R32(SC_S1 + 0x6cu) + SC_R32(SC_S1 + 0x78u);
                    SC_W32(SC_S1 + 0x70u, zspeed - (uint32_t)((int32_t)zspeed / 16));
                    SC_W32(SC_S1 + 0x6cu, SC_V1);
                }
                SC_S3++;
                SC_S2 += 0x7cu;
            } while (SC_S3 != 20);
            SC_S5++;
            SC_S4 += 0x9b0u;
        } while (SC_S5 != 14);
        SC_S6 += 0x87a0u;
        SC_FP++;
    } while (SC_FP != 2);
    SC_V0 = 2;
    entry->cpu->gpr[31] = SC_R32(sp + 0x44u);
    SC_FP = SC_R32(sp + 0x40u);
    for (unsigned r = 0; r < 8; r++)
        entry->cpu->gpr[16 + r] = SC_R32(sp + 0x20u + r * 4u);
    entry->cpu->gpr[29] = sp + 0x48u;
}

static void battle_shard_render_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x88u;
    entry->cpu->gpr[29] = sp;
    for (unsigned r = 0; r < 8; r++)
        SC_W32(sp + 0x60u + r * 4u, entry->cpu->gpr[16 + r]);
    SC_W32(sp + 0x80u, SC_FP);
    SC_W32(sp + 0x84u, entry->cpu->gpr[31]);
    SC_W32(sp + 0x50u, SC_R32(entry->cpu->gpr[4] + 4u));
    SC_CALL(0x8004a0bcu, 0x800b7198u, sp + 0x20u, sp + 0x24u);
    SC_S7 = 0;
    SC_W32(sp + 0x54u, SC_CALL0(0x8004a0dcu, 0x800b71a0u));
    SC_CALL(0x8004a12cu, 0x800b71b0u, 160, 112);
    SC_CALL(0x8004a14cu, 0x800b71b8u, 512);
    SC_S2 = sp + 0x28u;
    SC_FP = 0x3c;
    do {
        SC_S6 = SC_S5 = 0;
        do {
            SC_S4 = SC_S3 = 0;
            SC_W32(sp + 0x58u, SC_R32(sp + 0x50u) + SC_FP);
            do {
                SC_S0 = SC_R32(sp + 0x58u) + SC_S5 + SC_S3;
                SC_V1 = 0x24u + (SC_R32(0x800ccb34u) << 5);
                SC_S1 = SC_S0 + SC_V1;
                if ((int32_t)SC_R32(SC_S0 + 0x18u) >= 64) {
                    SC_CALL(0x8003f738u, 0x800b7220u, SC_S0, SC_S2);
                    SC_CALL(0x80049d9cu, 0x800b722cu, SC_S2, SC_S0 + 0x10u);
                    SC_CALL(0x80049efcu, 0x800b7234u, SC_S2);
                    SC_CALL(0x80049f8cu, 0x800b723cu, SC_S2);
                    uint32_t vertices = SC_S7 == 0 ? 0x800c3594u : 0x800c35acu;
                    uint32_t depth = SC_CALL(0x8004a67cu, 0x800b7284u,
                        vertices, vertices + 8u, vertices + 16u,
                        SC_S1 + 8u, SC_S1 + 0x10u, SC_S1 + 0x18u,
                        sp + 0x48u, sp + 0x4cu);
                    SC_V1 = 0x800c0000u;
                    SC_CALL(0x80043b48u, 0x800b72a0u,
                        SC_R32(0x800c3cb4u) + ((uint32_t)((int32_t)depth >> 6) << 2), SC_S1);
                }
                SC_S4++;
                SC_S3 += 0x7cu;
            } while (SC_S4 != 20);
            SC_S6++;
            SC_S5 += 0x9b0u;
        } while (SC_S6 != 14);
        SC_FP += 0x87a0u;
        SC_S7++;
    } while (SC_S7 != 2);
    SC_V0 = 2;
    SC_CALL(0x8004a12cu, 0x800b72e8u, SC_R32(sp + 0x20u), SC_R32(sp + 0x24u));
    SC_CALL(0x8004a14cu, 0x800b72f4u, SC_R32(sp + 0x54u));
    entry->cpu->gpr[31] = SC_R32(sp + 0x84u);
    SC_FP = SC_R32(sp + 0x80u);
    for (unsigned r = 0; r < 8; r++)
        entry->cpu->gpr[16 + r] = SC_R32(sp + 0x60u + r * 4u);
    entry->cpu->gpr[29] = sp + 0x88u;
}

static void battle_shard_cleanup_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x18u;
    entry->cpu->gpr[29] = sp;
    SC_W32(sp + 0x10u, SC_S0);
    SC_S0 = entry->cpu->gpr[4];
    SC_W32(sp + 0x14u, entry->cpu->gpr[31]);
    SC_CALL(0x800445d0u, 0x800b7340u, 0);
    SC_CALL(0x800320e8u, 0x800b7348u, SC_S0);
    entry->cpu->gpr[31] = SC_R32(sp + 0x14u);
    SC_S0 = SC_R32(sp + 0x10u);
    entry->cpu->gpr[29] = sp + 0x18u;
}

#undef SC_R8
#undef SC_R16
#undef SC_R32
#undef SC_W8
#undef SC_W16
#undef SC_W32
#undef SC_CALL0
#undef SC_CALL
#undef SC_S0
#undef SC_S1
#undef SC_S2
#undef SC_S3
#undef SC_S4
#undef SC_S5
#undef SC_S6
#undef SC_S7
#undef SC_FP
#undef SC_V0
#undef SC_V1

static int battle_shard_callbacks_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                           uint32_t target)
{
    unsigned bytes;
    const char *identity;
    void (*volatile run)(BattleEntryCall *);
    if (target == BATTLE_SHARD_UPDATE_ADDRESS) {
        bytes = BATTLE_SHARD_UPDATE_BYTES; identity = BATTLE_SHARD_UPDATE_SHA256;
        run = battle_shard_update_run;
    } else if (target == BATTLE_SHARD_RENDER_ADDRESS) {
        bytes = BATTLE_SHARD_RENDER_BYTES; identity = BATTLE_SHARD_RENDER_SHA256;
        run = battle_shard_render_run;
    } else if (target == BATTLE_SHARD_CLEANUP_ADDRESS) {
        bytes = BATTLE_SHARD_CLEANUP_BYTES; identity = BATTLE_SHARD_CLEANUP_SHA256;
        run = battle_shard_cleanup_run;
    } else return 0;
    if (runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_SHARD_CALLBACKS");
    if (control != NULL && strcmp(control, "0") == 0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target), bytes, digest);
    for (unsigned i = 0; i < sizeof(digest); i++)
        if (file1_hex_digit(digest[i] >> 4) != identity[i * 2] ||
            file1_hex_digit(digest[i] & 15u) != identity[i * 2 + 1]) return 0;
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0) return -1;
    run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

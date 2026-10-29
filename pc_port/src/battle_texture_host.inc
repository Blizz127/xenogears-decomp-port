/* Opening battle party textures (80078310). Keep the packed guest TIM_IMAGE
 * and six-word atlas slots in the real 0xc8-byte frame. Native SDK bridges
 * translate pointer fields; this body never uses a host-width TIM_IMAGE in
 * guest RAM or fetches an original CPU instruction. */
#define BATTLE_TEXTURE_ADDRESS 0x80078310u
#define BATTLE_TEXTURE_BYTES 0x1f8u
#define BATTLE_TEXTURE_SHA256 \
    "0d9c1fe3bd61c55baf49029bcaeb1df4fcc29a4b194a8aa78ef25aa5ecfe880c"

#define BT_R8(a) battle_entry_read(entry, (a), 1)
#define BT_R16(a) battle_entry_read(entry, (a), 2)
#define BT_R32(a) battle_entry_read(entry, (a), 4)
#define BT_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define BT_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define BT_W32(a, v) battle_entry_write(entry, (a), 4, (v))
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
#define BT_S6 (entry->cpu->gpr[22])
#define BT_S7 (entry->cpu->gpr[23])
#define BT_FP (entry->cpu->gpr[30])
#define BT_V1 (entry->cpu->gpr[3])

static void battle_texture_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0xc8u;
    entry->cpu->gpr[29] = sp;
    for (unsigned r = 0; r < 8; r++)
        BT_W32(sp + 0xa0u + r * 4u, entry->cpu->gpr[16 + r]);
    BT_W32(sp + 0xc0u, BT_FP);
    BT_W32(sp + 0xc4u, entry->cpu->gpr[31]);
    BT_S3 = BT_S0 = 0;
    BT_FP = 0x10;
    BT_S7 = 0xc;
    BT_S6 = 8;
    BT_S5 = 4;
    BT_S1 = BT_S4 = sp + 0x38u;
    BT_S2 = UINT32_MAX;
    BT_W32(sp + 0x80u, entry->cpu->gpr[4]);
    BT_W8(sp + 0x88u, entry->cpu->gpr[5]);
    BT_W32(sp + 0x90u, 0);
    BT_W32(sp + 0x98u, 0x14);
    do {
        BT_V1 = BT_R8(0x800d2d25u + BT_S2);
        if (BT_V1 != 0x7fu) {
            if (BT_R8(0x800d3294u) != 0 && BT_S2 < 2u) BT_V1 = 0xb;
            BT_CALL(0x800471b4u, 0x800783c8u,
                    BT_R32(sp + 0x80u) + (BT_V1 & 0xffu) * 0x460u);
            BT_CALL(0x800471c4u, 0x800783d0u, sp + 0x20u);
            BT_CALL(0x80026338u, 0x80078410u,
                    BT_R32(0x800d2f5cu), BT_R8(sp + 0x88u) + BT_S3,
                    BT_S4, BT_S1 + BT_S5, BT_S1 + BT_S6, BT_S1 + BT_S7,
                    BT_S1 + BT_FP, BT_S1 + BT_R32(sp + 0x98u));
            BT_V1 = BT_R32(sp + 0x24u);
            BT_W16(BT_V1, BT_R16(sp + BT_S0 + 0x40u));
            BT_V1 = BT_R32(sp + 0x24u);
            BT_W16(BT_V1 + 2u, BT_R16(sp + BT_S0 + 0x44u));
            uint32_t x = BT_R16(sp + BT_S0 + 0x48u) + BT_R32(sp + 0x90u);
            BT_V1 = BT_R32(sp + 0x2cu);
            BT_W16(BT_V1, x);
            BT_V1 = BT_R32(sp + 0x2cu);
            BT_W16(BT_V1 + 2u, BT_R16(sp + BT_S0 + 0x4cu));
            BT_CALL(0x80044894u, 0x80078474u, BT_R32(sp + 0x24u), BT_R32(sp + 0x28u));
            BT_CALL(0x80044894u, 0x80078484u, BT_R32(sp + 0x2cu), BT_R32(sp + 0x30u));
            BT_CALL(0x800445d0u, 0x8007848cu, 0);
        }
        BT_FP += 0x18u;
        BT_S7 += 0x18u;
        BT_S6 += 0x18u;
        BT_S5 += 0x18u;
        BT_S4 += 0x18u;
        BT_S0 += 0x18u;
        BT_S2++;
        BT_S3++;
        BT_W32(sp + 0x90u, BT_R32(sp + 0x90u) + 6u);
        BT_W32(sp + 0x98u, BT_R32(sp + 0x98u) + 0x18u);
    } while ((int32_t)BT_S3 < 3);
    entry->cpu->gpr[2] = 0;
    entry->cpu->gpr[31] = BT_R32(sp + 0xc4u);
    BT_FP = BT_R32(sp + 0xc0u);
    for (unsigned r = 0; r < 8; r++)
        entry->cpu->gpr[16 + r] = BT_R32(sp + 0xa0u + r * 4u);
    entry->cpu->gpr[29] = sp + 0xc8u;
}

#undef BT_R8
#undef BT_R16
#undef BT_R32
#undef BT_W8
#undef BT_W16
#undef BT_W32
#undef BT_CALL
#undef BT_S0
#undef BT_S1
#undef BT_S2
#undef BT_S3
#undef BT_S4
#undef BT_S5
#undef BT_S6
#undef BT_S7
#undef BT_FP
#undef BT_V1

static int battle_texture_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                    uint32_t target)
{
    if (target != BATTLE_TEXTURE_ADDRESS || runtime->force_interpret ||
        !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_TEXTURE");
    if (control != NULL && strcmp(control, "0") == 0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(BATTLE_TEXTURE_ADDRESS), BATTLE_TEXTURE_BYTES, digest);
    for (unsigned i = 0; i < sizeof(digest); i++)
        if (file1_hex_digit(digest[i] >> 4) != BATTLE_TEXTURE_SHA256[i * 2] ||
            file1_hex_digit(digest[i] & 15u) != BATTLE_TEXTURE_SHA256[i * 2 + 1]) return 0;
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0) return -1;
    battle_texture_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

/* Native battle entry (retail func_80070F40, src/battle/main.c).
 *
 * This is structured host C: allocation, archive loading, battle lifecycle,
 * outcome selection and cleanup. It never fetches or decodes CPU instructions.
 * The existing call bridge still owns callees; unconverted callees are refused
 * when XENO_DISABLE_CPU_INTERPRETER=1. This does not make battle fully native.
 *
 * Keep the retail 0x30-byte frame and call-site return addresses: callbacks
 * can re-enter while that frame is live. Pointer slots stay 32-bit, and all
 * data goes through the production bus, including main-executable globals.
 * Included after battle_results_gate.inc for the existing SHA-256 helper.
 */
#define BATTLE_ENTRY_BYTES 0x798u
#define BATTLE_ENTRY_SHA256 \
    "2df67f9cb21d6e025ed9164fcaae41dd971a2fd1c5f1bac0dcd0687aa4e54a02"

typedef struct BattleEntryCall {
    BattleMipsRuntime *runtime;
    PcPortMipsCpu *cpu;
    jmp_buf failure;
} BattleEntryCall;

static void battle_entry_fail(BattleEntryCall *entry, const char *what,
                               uint32_t address)
{
    fprintf(stderr, "[xeno-port][battle-entry] %s at 0x%08x\n", what, address);
    longjmp(entry->failure, 1);
}

static uint32_t battle_entry_read(BattleEntryCall *entry, uint32_t address,
                                   unsigned width)
{
    uint32_t value = 0;
    if ((address & (width - 1u)) != 0 ||
        runtime_read(entry->runtime, address, width, &value) != 0)
        battle_entry_fail(entry, "invalid read", address);
    return value;
}

static void battle_entry_write(BattleEntryCall *entry, uint32_t address,
                                 unsigned width, uint32_t value)
{
    if ((address & (width - 1u)) != 0 ||
        runtime_write(entry->runtime, address, width, value) != 0)
        battle_entry_fail(entry, "invalid write", address);
}

/* Only the declared arguments are changed. The fifth argument occupies the
 * actual caller frame, and a callee sees the same SP/RA as the retail JAL.
 * PcPortMipsRun first attempts a native bridge and enforces the disabled gate
 * before any CPU fallback. Keep its counters and callbacks on every call. */
static uint32_t battle_entry_call(BattleEntryCall *entry, uint32_t target,
                                   uint32_t site, unsigned argc,
                                   const uint32_t *args)
{
    PcPortMipsCpu callee = *entry->cpu;
    callee.gpr[31] = site + 8u;
    for (unsigned i = 0; i < argc; i++) {
        if (i < 4)
            callee.gpr[4 + i] = args[i];
        else
            battle_entry_write(entry, callee.gpr[29] + 0x10u + (i - 4u) * 4u,
                               4, args[i]);
    }
    cpu_meter_mark_entry(&callee, target);
    int rc = PcPortMipsRun(&callee, target, site + 8u, battle_step_limit());
    /* Include nested fallback in the outer instruction receipt as well as
     * the process-wide meter; a native caller must not hide its callees. */
    entry->cpu->steps = callee.steps;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "[xeno-port][battle-entry] callee 0x%08x from 0x%08x: %s\n",
                target, site, callee.error);
        battle_entry_fail(entry, "callee stopped", target);
    }
    /* A delayed return-slot load needs instruction-level scheduling, which
     * this entry bridge does not supply. Refuse instead of applying it at an
     * invented time. Completed writes remain visible on failure. */
    if (callee.load_valid && callee.load_reg != 0)
        battle_entry_fail(entry, "pending callee return load", target);
    memcpy(entry->cpu->gpr, callee.gpr, sizeof(callee.gpr));
    memcpy(entry->cpu->cp0, callee.cp0, sizeof(callee.cp0));
    entry->cpu->hi = callee.hi;
    entry->cpu->lo = callee.lo;
    return callee.gpr[2];
}

/* Addresses below name retail data and call sites, never executable bytes.
 * CALL0 is separate so an argument-free call has no zero-length C array. */
#define BE_R8(a) battle_entry_read(entry, (a), 1)
#define BE_R16(a) battle_entry_read(entry, (a), 2)
#define BE_R32(a) battle_entry_read(entry, (a), 4)
#define BE_W8(a, v) battle_entry_write(entry, (a), 1, (v))
#define BE_W16(a, v) battle_entry_write(entry, (a), 2, (v))
#define BE_W32(a, v) battle_entry_write(entry, (a), 4, (v))
#define BE_CALL0(target, site) battle_entry_call(entry, (target), (site), 0, NULL)
#define BE_CALL(target, site, ...) \
    battle_entry_call(entry, (target), (site), \
        sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), \
        (uint32_t[]){__VA_ARGS__})
#define BE_S0 (entry->cpu->gpr[16])
#define BE_S1 (entry->cpu->gpr[17])
#define BE_S2 (entry->cpu->gpr[18])
#define BE_S3 (entry->cpu->gpr[19])

static void battle_entry_run(BattleEntryCall *entry)
{
    uint32_t sp = entry->cpu->gpr[29] - 0x30u;
    entry->cpu->gpr[29] = sp;
    BE_W32(sp + 0x24u, BE_S3);
    BE_S3 = 1;
    BE_W32(sp + 0x28u, entry->cpu->gpr[31]);
    BE_W32(sp + 0x20u, BE_S2);
    BE_W32(sp + 0x1cu, BE_S1);
    BE_W32(sp + 0x18u, BE_S0);

    BE_W32(0x800c3ea4u, BE_CALL(0x8008abb8u, 0x80070f60u, 0xa2b4, 0));
    BE_W32(0x800d2d28u, BE_CALL(0x8008abb8u, 0x80070f74u, 0x10c, 0));
    BE_W32(0x800c3eacu, BE_CALL(0x8008abb8u, 0x80070f88u, 0x2f8, 0));
    BE_CALL(0x8003f8e8u, 0x80070fa0u, BE_R32(0x800c3ea4u), 0xa2b4);
    BE_CALL(0x8003f8e8u, 0x80070fb0u, BE_R32(0x800d2d28u), 0x10c);
    BE_CALL(0x8003f8e8u, 0x80070fc0u, BE_R32(0x800c3eacu), 0x2f8);
    BE_W8(0x8005959cu, 0);
    BE_W8(0x800c3e29u, 0xff);
    BE_W8(0x800c3e28u, 0xff);
    BE_W8(0x800d366cu, 0);
    BE_W32(0x800c3e54u, BE_R32(0x80062528u));
    if (BE_R8(0x8005947cu) != 0) {
        BE_W8(0x80059508u, BE_R8(0x8005947cu) - 1u);
        if (BE_R8(0x80059180u) != 0) {
            BE_W8(0x80059180u, 0);
            BE_CALL(0x8003a89cu, 0x8007102cu, BE_R32(0x80062528u), 0x7f, 0x3c);
        }
        if (BE_R8(0x800594f8u) == 0)
            BE_W8(0x8005947cu, 0);
    }
    if (BE_R8(0x800594f8u) != 0) {
        BE_CALL(0x80028470u, 0x80071064u, 0x10, 2);
        BE_S0 = BE_CALL(0x8008abb8u, 0x80071070u, 4, 1);
        BE_S1 = BE_CALL(0x8008abb8u, 0x80071084u, BE_S0 - 0x801e0000u, 1);
        BE_CALL(0x800295d8u, 0x8007109cu, 1, 0x801e0000u, 0, 0x80);
        BE_CALL(0x80028a60u, 0x800710a4u, 0);
        BE_CALL(0x800320e8u, 0x800710acu, BE_S0);
        BE_CALL(0x800320e8u, 0x800710b4u, BE_S1);
        if (BE_R8(0x8005947cu) == 0) {
            BE_CALL0(0x801e0a34u, 0x800710d0u);
        } else {
            BE_W8(0x80059508u, BE_R8(0x8005947cu) - 1u);
            BE_W8(0x8005947cu, 0);
        }
    }
    if (BE_R32(BE_R32(0x8005917cu)) != UINT32_MAX) {
        BE_CALL(0x80028470u, 0x8007110cu, 0x10, 2);
        BE_CALL(0x800295d8u, 0x80071120u, 6, 0x80280000u, 0, 0x80);
        BE_CALL(0x80028a60u, 0x80071128u, 0);
    }
    BE_CALL(0x8003f99cu, 0x80071150u, 0x8006f9dcu,
            0x800658dcu + BE_R8(0x80059508u) * 0x20u, 0x20);
    BE_CALL(0x800b8098u, 0x80071160u, BE_R8(0x8005954cu));
    BE_CALL0(0x8007252cu, 0x80071168u);
    BE_W8(0x800c3e4cu, 2);
    BE_CALL(0x800b81bcu, 0x80071184u, BE_R32(0x800c3decu));
    BE_CALL(0x800b39c0u, 0x800711a0u, 0, 2, 0xff, 0xff, 0xff);
    if (BE_R8(0x800c3d48u) == 0)
        BE_CALL(0x800b39c0u, 0x800711c8u, 0x14, 2, 0, 0, 0);
    if (BE_R8(0x800594f8u) != 0)
        BE_W32(0x800c3e54u, BE_CALL(0x800397fcu, 0x800711ecu, 0x80062648u, 0x7f, 0));
    BE_W32(0x800d3364u, BE_R32(0x8005949cu));
    BE_W32(0x800c3eb0u, BE_R32(0x8005949cu));
    BE_CALL0(0x80077990u, 0x80071214u);
    BE_W8(0x800d3298u, 1);
    BE_CALL(0x800bc404u, 0x80071230u, BE_R16(0x800d39dcu));
    BE_CALL0(0x800716d8u, 0x80071238u);
    BE_CALL0(0x8007819cu, 0x80071240u);
    while (BE_R8(0x800ccc58u) == 0)
        BE_CALL0(0x800716d8u, 0x8007125cu);
    BE_CALL(0x800320e8u, 0x80071280u, BE_R32(0x800595d0u));
    if (BE_R8(0x8005954cu) != 4)
        BE_CALL(0x8003a094u, 0x800712a4u, BE_R32(0x800595d0u));
    BE_CALL(0x8003852cu, 0x800712b4u, BE_R32(0x800595d0u));
    BE_CALL(0x800320e8u, 0x800712c4u, BE_R32(0x80059480u));
    BE_CALL(0x800320e8u, 0x800712d4u, BE_R32(0x800594acu));
    BE_CALL0(0x80070e2cu, 0x800712dcu);
    BE_CALL(0x80070eb0u, 0x800712e4u, 1);
    if (BE_R8(0x800d2fc4u) == 0)
        BE_W8(0x800c3e4cu, 1);
    BE_CALL0(0x8009892cu, 0x80071308u);
    for (unsigned i = 0; i < 3; i++) {
        uint32_t id = BE_R8(0x800d2d24u + i);
        if (id != 0x7f) {
            BE_W16(0x800c3e0cu + i * 4u, BE_R16(0x8006ecf4u + id * 0x20u));
            id = BE_R8(0x800d2d24u + i);
            BE_W16(0x800c3e0eu + i * 4u, BE_R16(0x8006ecf6u + id * 0x20u));
        }
    }
    while (BE_R8(0x800c48eau) == 0 && BE_R8(0x800d2fc4u) == 0) {
        if (BE_R8(0x800ccc58u) != 0)
            BE_CALL0(0x800723e0u, 0x8007139cu);
        BE_CALL0(0x800716d8u, 0x800713a4u);
    }
    uint32_t outcome = BE_R8(0x800c48eau);
    if (!(outcome & 0xc0u)) {
        BE_S2 = 0;
    } else if (outcome & 0x40u) {
        BE_S2 = 1;
    } else if (BE_R8(0x800c3d48u) == 0) {
        BE_S2 = 2;
    } else if (BE_R8(0x800c3d5cu) != 0) {
        BE_S2 = 3;
        BE_W8(0x800594d0u, 3);
        BE_W8(0x800c48eau, 1);
    }
    /* Retail deliberately retains incoming s2 in the remaining branch.
     * Reading that register avoids the recovered C's uninitialized local. */
    switch (BE_S2 & 0xffu) {
    case 0:
        BE_W8(0x800594d0u, 0);
        BE_S3 = 0;
        /* fall through */
    case 3:
        if (BE_R8(0x800c3d48u) != 0) {
            uint32_t context = BE_R32(0x800d3278u);
            BE_W16(context + 0x394u, 0xff);
            if (BE_R8(0x800c3d44u) != 0 || BE_R8(0x800d2fc4u) == 0) {
                BE_W8(context + 0x800u, 0);
                context = BE_R32(0x800d3278u);
                BE_S0 = BE_R8(0x800c48eau);
                BE_W8(0x800c48eau, 0);
                for (unsigned i = 0; i < 3; i++) {
                    uint32_t status = BE_R16(0x800ccd64u + i * 0x170u) & 0x8000u;
                    BE_W16(context + 0x3b4u + i * 2u, status);
                    status |= BE_R16(0x800cce08u + i * 0x170u) & 0x8000u;
                    BE_W16(context + 0x3b4u + i * 2u, status);
                }
                BE_CALL0(0x800c0f70u, 0x80071540u);
                BE_CALL(0x80070eb0u, 0x80071548u, 1);
                BE_W8(0x800c48eau, BE_S0);
            }
        }
        break;
    case 1:
        BE_W8(0x800594d0u, 2);
        BE_S3 = 2;
        break;
    case 2:
        BE_W8(0x800594d0u, 1);
        BE_S3 = 1;
        break;
    }
    BE_CALL0(0x800b8d7cu, 0x80071584u);
    if (BE_R8(0x800d2d50u) != 0)
        BE_S3 = 1;
    BE_CALL(0x80028470u, 0x800715a4u, 0x10, 0);
    BE_W32(0x800d2d3cu, BE_CALL(0x8008abb8u, 0x800715b0u, 4, 1));
    BE_W32(0x800d2f60u, BE_CALL(0x8008abb8u, 0x800715ccu,
                               BE_R32(0x800d2d3cu) - 0x801de000u, 1));
    BE_CALL(0x800295d8u, 0x800715ecu, 4, 0x801de000u, 0, 0x80);
    BE_CALL(0x800b853cu, 0x800715f4u, BE_S3);
    while (BE_R8(0x800ccc58u) != 0)
        BE_CALL0(0x800716d8u, 0x80071610u);
    if (BE_R8(0x800c3d48u) == 0 && !(BE_R8(0x800c48eau) & 0x40u) &&
        !(BE_R8(0x8006f9ddu) & 8u))
        BE_CALL(0x800b39c0u, 0x80071680u, 0x40, 2, 0x40, 0x40, 0x40);
    BE_CALL0(0x80070edcu, 0x80071688u);
    BE_CALL0(0x801e252cu, 0x80071690u);
    BE_CALL(0x800320e8u, 0x800716a0u, BE_R32(0x800d2d3cu));
    BE_CALL(0x800320e8u, 0x800716b0u, BE_R32(0x800d2f60u));
    entry->cpu->gpr[31] = BE_R32(sp + 0x28u);
    BE_S3 = BE_R32(sp + 0x24u);
    BE_S2 = BE_R32(sp + 0x20u);
    BE_S1 = BE_R32(sp + 0x1cu);
    BE_S0 = BE_R32(sp + 0x18u);
    entry->cpu->gpr[29] = sp + 0x30u;
}

#undef BE_R8
#undef BE_R16
#undef BE_R32
#undef BE_W8
#undef BE_W16
#undef BE_W32
#undef BE_CALL0
#undef BE_CALL
#undef BE_S0
#undef BE_S1
#undef BE_S2
#undef BE_S3

static int battle_entry_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                  uint32_t target)
{
    if (target != BATTLE_ENTRY || runtime->force_interpret ||
        !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0))
        return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_ENTRY");
    if (control != NULL && strcmp(control, "0") == 0)
        return 0;
    uint8_t digest[32];
    const char *expected = BATTLE_ENTRY_SHA256;
    file1_digest(PSX_ADDR(BATTLE_ENTRY), BATTLE_ENTRY_BYTES, digest);
    for (unsigned i = 0; i < sizeof(digest); i++) {
        if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
            file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1])
            return 0;
    }
    BattleEntryCall entry = { .runtime = runtime, .cpu = cpu };
    if (setjmp(entry.failure) != 0)
        return -1;
    battle_entry_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

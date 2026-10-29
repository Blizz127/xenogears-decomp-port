/* Native packed-window boundary. Nested values are already native compact
 * pointers written by the host constructor/queue; translating them again
 * would silently reinterpret corrupt guest tokens as valid host storage. */
typedef struct BattleWindowBinding {
    BattleMipsRuntime *runtime;
    jmp_buf failure;
    const char *volatile *reason;
} BattleWindowBinding;

static void battle_window_binding_reject(void *opaque, const char *site)
{
    BattleWindowBinding *call = opaque;
    *call->reason = site;
    longjmp(call->failure, 1);
}

static void battle_window_binding_check(void *opaque, uintptr_t address,
    size_t bytes, unsigned alignment, int write, const char *site)
{
    BattleWindowBinding *call = opaque;
    uintptr_t ram = (uintptr_t)g_PsxRam;
    if (!alignment || (alignment & (alignment - 1u)) ||
        (address & (alignment - 1u)) || bytes > UINT_MAX ||
        address > UINTPTR_MAX - bytes ||
        (address <= UINT32_MAX && bytes > (uint64_t)UINT32_MAX + 1u - address))
        battle_window_binding_reject(opaque, site);
    if (address < ram && bytes > ram - address)
        battle_window_binding_reject(opaque, site);
    if (address >= ram && address < ram + PSX_RAM_SIZE) {
        size_t offset = address - ram;
        if (offset >= 0x200000u || bytes > 0x200000u - offset)
            battle_window_binding_reject(opaque, site);
    } else {
        /* Raw nested guest tokens are not native pointers. Scalars never
         * reach this callback. Zero-byte cursor checks still require a
         * mapped address, rather than accepting a null/unbound word. */
        if (address <= UINT32_MAX &&
            ((address < 0x200000u) ||
             (address >= 0x80000000u && address < 0x80800000u) ||
             (address >= 0xa0000000u && address < 0xa0800000u) ||
             (address >= 0x1f800000u && address < 0x1f801000u)))
            battle_window_binding_reject(opaque, site);
        if (!host_address_valid(call->runtime, address,
                                (unsigned)(bytes ? bytes : 1), write))
            battle_window_binding_reject(opaque, site);
    }
    /* Direct packed writes must retire overlapping module authority just
     * as stores through the guest bus do. Validation is conservative over
     * the checked write span; no rollback or CPU fallback follows failure. */
    if (write && bytes)
        runtime_note_store(call->runtime, (const uint8_t *)address, (unsigned)bytes);
}

static void battle_window_binding_packet(void *opaque,
                                         const void *ot, const void *prim)
{
    uintptr_t ram = (uintptr_t)g_PsxRam;
    uintptr_t a = (uintptr_t)ot, b = (uintptr_t)prim;
    if (a >= ram && a < ram + 0x200000u &&
        !(b >= ram && b < ram + 0x200000u))
        battle_window_binding_reject(opaque, "guest OT with foreign primitive");
}

static int bridge_window_render(BattleMipsRuntime *runtime,
    PcPortMipsCpu *cpu, const ResolvedFunction *resolved)
{
    if (resolved->address != 0x80034888u ||
        cpu->gpr[31] != 0x80074ffcu) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_WINDOW_RENDER");
    if (!control || strcmp(control, "1") != 0) return 0;
    if (!runtime->initialized || runtime->force_interpret ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0))
        return -1;
    if (!PcPort_SystemWindowGuardSwap || !resolved->host) return -1;
    void *window = module_memory_pointer(runtime, cpu->gpr[4], 0x90u, 1);
    void *ot = module_memory_pointer(runtime, cpu->gpr[5], 4, 1);
    if (!window || (cpu->gpr[4] & 3u) || cpu->gpr[6] > 1u) return -1;
    /* An empty, inactive window returns without reading the OT. Preserve
     * that behavior; an invalid OT is rejected when linkage actually occurs. */
    if (!ot) ot = (void *)(uintptr_t)cpu->gpr[5];
    const char *volatile reason = NULL;
    BattleWindowBinding call = {.runtime = runtime, .reason = &reason};
    PcPortSystemWindowGuard guard = {
        &call, battle_window_binding_check,
        battle_window_binding_packet, battle_window_binding_reject
    };
    const PcPortSystemWindowGuard *previous = PcPort_SystemWindowGuardSwap(&guard);
    int failed = setjmp(call.failure);
    if (!failed)
        ((void (*)(void *, void *, int32_t))resolved->host)(
            window, ot, (int32_t)cpu->gpr[6]);
    PcPort_SystemWindowGuardSwap(previous);
    if (failed) {
        fprintf(stderr, "[xeno-port][battle-window] native span refused: %s\n",
                reason ? reason : "unknown");
        return -1;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

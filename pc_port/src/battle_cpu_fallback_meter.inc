/* Runtime CPU-fallback meter for the battle runtime (diagnostic only).
 * Included by battle_mips_runtime.c; not a separate translation unit.
 *
 * XENO_CPU_FALLBACK_STATS=1 (any non-empty value but "0") counts, per
 * battle (func_80070F40 entry to exit), how much of the battle still runs on
 * the MIPS interpreter -- the disclosed CPU fallback -- against what the
 * bridge dispatches to native code:
 *
 *   interpreted_instructions  guest instructions the interpreter executed
 *                             (one per bridge consultation that returned
 *                             "interpret"; every interpreted instruction
 *                             passes the bridge first);
 *   interpreted_calls         guest calls that ran interpreted: a JAL/JALR
 *                             (link register ra) whose target the bridge did
 *                             not take natively, plus every guest entry made
 *                             from native code (battle entry, guest calls,
 *                             work-list callbacks); also counted by target;
 *   native_main_exe_calls     bridged calls into native main-executable code;
 *   native_overlay_calls      battle.bin / overlay addresses dispatched to
 *                             native host C (the overlay allowlist and the
 *                             typed overlay bridges);
 *   native_results_calls      adopted RESULTS module bodies
 *                             (battle_results_gate.inc);
 *   native_file1_calls        the adopted file-1 controller 801E6CE8.
 *   disabled_dispatches       bridge misses refused by the disabled CPU gate;
 *                             excluded from interpreted instructions/calls.
 *
 * At battle exit it prints one summary line and the top interpreted call
 * targets to stderr.  When the variable is unset nothing is counted or
 * printed and dispatch is unchanged: every hook is one test of a cached
 * flag.  When it is set the hooks only read guest memory (the JAL/JALR word
 * behind ra, through resolve_memory's read path, which has no side effect for
 * guest code addresses) and update these counters; no guest state, register,
 * dispatch decision or timing input changes.
 *
 * Call detection is a heuristic: a bridge consultation at PC X is a call
 * entry when the previous consultation on the same CPU was at ra-4 (the
 * delay slot) and the word at ra-8 is a JAL to X or a JALR with rd = ra.
 * A JALR linking through another register is not counted as a call (its
 * instructions still are). */

#define CPU_METER_TARGETS 4096u
#define CPU_METER_TOP 16u

enum {
    CPU_METER_KIND_NONE = 0,
    CPU_METER_KIND_RESULTS = 1,
    CPU_METER_KIND_GENERIC = 2
};

typedef struct CpuMeterTarget {
    uint32_t target;
    uint64_t calls;
} CpuMeterTarget;

typedef struct CpuFallbackMeter {
    int enabled; /* -1 until the environment is read */
    int active;  /* inside a metered battle */
    unsigned battle;
    int kind;    /* classification of the bridge call in progress */
    const PcPortMipsCpu *last_cpu;
    uint32_t last_pc;
    const PcPortMipsCpu *entry_cpu;
    uint32_t entry_pc;
    uint64_t interpreted_instructions;
    uint64_t interpreted_calls;
    uint64_t disabled_dispatches;
    uint64_t untracked_calls; /* targets that did not fit the table */
    uint64_t native_main_exe_calls;
    uint64_t native_overlay_calls;
    uint64_t native_results_calls;
    uint64_t native_file1_calls;
    uint64_t legacy_boundary_calls; /* audited legacy C at recorded boundaries */
    unsigned distinct_targets;
    CpuMeterTarget targets[CPU_METER_TARGETS];
} CpuFallbackMeter;

static CpuFallbackMeter g_CpuFallbackMeter = {.enabled = -1};

static int cpu_meter_enabled(void)
{
    if (g_CpuFallbackMeter.enabled < 0) {
        const char *value = getenv("XENO_CPU_FALLBACK_STATS");
        g_CpuFallbackMeter.enabled =
            value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
    }
    return g_CpuFallbackMeter.enabled;
}

static int cpu_meter_active(void)
{
    return cpu_meter_enabled() && g_CpuFallbackMeter.active;
}

static void cpu_meter_begin_battle(void)
{
    CpuFallbackMeter *m = &g_CpuFallbackMeter;
    unsigned battle;

    if (!cpu_meter_enabled())
        return;
    battle = m->battle + 1u;
    memset(m, 0, sizeof(*m));
    m->enabled = 1;
    m->active = 1;
    m->battle = battle;
}

/* The next bridge consultation of `cpu` at `pc` is a guest entry made from
 * native code (it has no JAL behind it). */
static void cpu_meter_mark_entry(const PcPortMipsCpu *cpu, uint32_t pc)
{
    if (!cpu_meter_active())
        return;
    g_CpuFallbackMeter.entry_cpu = cpu;
    g_CpuFallbackMeter.entry_pc = pc;
}

static void cpu_meter_note_legacy_boundary(void)
{
    if (cpu_meter_active())
        g_CpuFallbackMeter.legacy_boundary_calls++;
}

static void cpu_meter_note_kind(int kind)
{
    if (cpu_meter_active())
        g_CpuFallbackMeter.kind = kind;
}

static int cpu_meter_is_call(BattleMipsRuntime *runtime, const PcPortMipsCpu *cpu,
                             uint32_t target)
{
    CpuFallbackMeter *m = &g_CpuFallbackMeter;
    uint32_t ra = cpu->gpr[31];
    uint32_t word, op;
    const uint8_t *p;

    if (m->entry_cpu == cpu && m->entry_pc == target) {
        m->entry_cpu = NULL;
        return 1;
    }
    /* Guest code inside the 2 MiB of RAM only: no main-exe .text read (that
     * would reach note_unshared_access) and nothing past the RAM image. */
    if (m->last_cpu != cpu || m->last_pc + 4u != ra || (ra & 3u) != 0 ||
        !target_is_guest_code(ra - 8u) || ra - 8u >= 0x80200000u)
        return 0;
    p = resolve_memory(runtime, ra - 8u, 4, 0);
    if (p == NULL)
        return 0;
    word = load_le(p, 4);
    op = word >> 26;
    if (op == 3u)
        return ((((ra - 4u) & 0xF0000000u) | ((word & 0x03FFFFFFu) << 2)) == target);
    return op == 0u && (word & 0x3Fu) == 9u && ((word >> 11) & 31u) == 31u;
}

static void cpu_meter_count_target(uint32_t target)
{
    CpuFallbackMeter *m = &g_CpuFallbackMeter;
    unsigned slot = (unsigned)((target * 2654435761u) >> 20) & (CPU_METER_TARGETS - 1u);
    unsigned probe;

    for (probe = 0; probe < 64u; probe++) {
        CpuMeterTarget *t = &m->targets[(slot + probe) & (CPU_METER_TARGETS - 1u)];
        if (t->calls != 0 && t->target == target) {
            t->calls++;
            return;
        }
        if (t->calls == 0) {
            t->target = target;
            t->calls = 1;
            m->distinct_targets++;
            return;
        }
    }
    m->untracked_calls++;
}

typedef struct CpuMeterBridgeState {
    int on;
    int call;
    int saved_kind;
} CpuMeterBridgeState;

static void cpu_meter_bridge_enter(BattleMipsRuntime *runtime, const PcPortMipsCpu *cpu,
                                   uint32_t target, CpuMeterBridgeState *state)
{
    state->on = cpu_meter_active();
    if (!state->on)
        return;
    state->call = cpu_meter_is_call(runtime, cpu, target);
    state->saved_kind = g_CpuFallbackMeter.kind;
    g_CpuFallbackMeter.kind = CPU_METER_KIND_NONE;
}

static void cpu_meter_bridge_leave(const PcPortMipsCpu *cpu, uint32_t target,
                                   int result, const CpuMeterBridgeState *state)
{
    CpuFallbackMeter *m = &g_CpuFallbackMeter;
    int kind;

    if (!state->on)
        return;
    kind = m->kind;
    m->kind = state->saved_kind;
    m->last_cpu = cpu;
    m->last_pc = target;
    if (result == 0) {
        /* The adapter consults the bridge before its fail-closed gate. A
         * refused dispatch has not fetched/executed an instruction. */
        if (PcPortMipsInterpreterDisabled()) {
            m->disabled_dispatches++;
        } else {
            m->interpreted_instructions++;
            if (state->call) {
                m->interpreted_calls++;
                cpu_meter_count_target(target);
            }
        }
    } else if (result > 0) {
        if (kind == CPU_METER_KIND_RESULTS)
            m->native_results_calls++;
        else if (kind == CPU_METER_KIND_NONE && target == 0x801e6ce8u)
            m->native_file1_calls++;
        else if (target_is_guest_code(target))
            m->native_overlay_calls++;
        else
            m->native_main_exe_calls++;
    }
}

static int cpu_meter_target_compare(const void *lhs, const void *rhs)
{
    const CpuMeterTarget *a = lhs;
    const CpuMeterTarget *b = rhs;
    if (a->calls != b->calls)
        return a->calls < b->calls ? 1 : -1;
    return a->target < b->target ? -1 : a->target > b->target;
}

static void cpu_meter_end_battle(const char *how)
{
    CpuFallbackMeter *m = &g_CpuFallbackMeter;
    uint64_t native;
    unsigned i, n = 0, shown;
    double share;

    if (!cpu_meter_active())
        return;
    m->active = 0;
    native = m->native_main_exe_calls + m->native_overlay_calls +
             m->native_results_calls + m->native_file1_calls;
    share = native + m->interpreted_calls == 0
                ? 0.0
                : 100.0 * (double)native / (double)(native + m->interpreted_calls);
    fprintf(stderr,
            "[xeno-port][cpu-fallback] battle=%u end=%s "
            "interpreted_instructions=%llu interpreted_calls=%llu "
            "disabled_dispatches=%llu "
            "interpreted_targets=%u untracked_calls=%llu "
            "native_main_exe_calls=%llu native_overlay_calls=%llu "
            "native_results_calls=%llu native_file1_calls=%llu "
            "native_call_share=%.1f%%\n",
            m->battle, how, (unsigned long long)m->interpreted_instructions,
            (unsigned long long)m->interpreted_calls,
            (unsigned long long)m->disabled_dispatches, m->distinct_targets,
            (unsigned long long)m->untracked_calls,
            (unsigned long long)m->native_main_exe_calls,
            (unsigned long long)m->native_overlay_calls,
            (unsigned long long)m->native_results_calls,
            (unsigned long long)m->native_file1_calls, share);
    fprintf(stderr,
            "[xeno-port][cpu-fallback] battle=%u legacy_boundary_calls=%llu\n",
            m->battle, (unsigned long long)m->legacy_boundary_calls);
    /* Compact the table in place for the report; it is reset at the next
     * battle anyway. */
    for (i = 0; i < CPU_METER_TARGETS; i++) {
        if (m->targets[i].calls != 0)
            m->targets[n++] = m->targets[i];
    }
    qsort(m->targets, n, sizeof(m->targets[0]), cpu_meter_target_compare);
    /* XENO_CPU_FALLBACK_TOP=N lists N targets instead of the default 16
     * (0 or "all" lists every distinct target); diagnostics only. */
    {
        const char *top = getenv("XENO_CPU_FALLBACK_TOP");
        unsigned limit = CPU_METER_TOP;
        if (top != NULL && top[0] != '\0')
            limit = strcmp(top, "all") == 0 ? 0u : (unsigned)strtoul(top, NULL, 10);
        if (limit == 0u || limit > n)
            limit = n;
        shown = limit;
    }
    fprintf(stderr,
            "[xeno-port][cpu-fallback] top %u of %u interpreted call targets:\n",
            shown, n);
    for (i = 0; i < shown; i++) {
        uint32_t t = m->targets[i].target;
        const char *where = t >= BATTLE_BASE && t < BATTLE_END ? "battle.bin"
                            : t >= 0x801de000u && t < 0x801e4500u
                                ? "results/intro window"
                                : "overlay";
        fprintf(stderr,
                "[xeno-port][cpu-fallback]   %2u. 0x%08x calls=%llu (%.1f%%) %s\n",
                i + 1u, t, (unsigned long long)m->targets[i].calls,
                m->interpreted_calls == 0
                    ? 0.0
                    : 100.0 * (double)m->targets[i].calls /
                          (double)m->interpreted_calls,
                where);
    }
    memset(m->targets, 0, sizeof(m->targets));
}

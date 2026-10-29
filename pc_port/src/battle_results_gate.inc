/* Identity gate and dispatch for the adopted battle RESULTS module routines.
 * Included by battle_mips_runtime.c after battle_file1_controller.inc (it
 * reuses file1_digest), not compiled as a second translation unit.
 *
 * The results module is linked at 0x801DE000, but that range is shared: the
 * battle intro module (0x801E0000) and other overlays load over it, and the
 * per-target bridge cache keys on the address alone.  A native body may only
 * stand in for the retail bytes while those bytes are provably the retail
 * results module, so every dispatch checks identity first:
 *
 *  - The verdict is computed from a snapshot of the immutable range
 *    0x801DE000..0x801E44C0 (RESULTS_IMMUTABLE_BYTES; the trailing module data
 *    is mutable, see battle_results_gate.h) hashed with SHA-256 against the
 *    pinned retail digest below.
 *  - A VERIFIED verdict is reused only while guest RAM still equals that
 *    snapshot byte-for-byte (one memcmp per adopted call).  This holds whoever
 *    writes the range -- interpreted stores, native archive loads, DMA, an
 *    unobserved native poke -- so the gate does not depend on hooks seeing
 *    every writer.  Any difference re-hashes.
 *  - The observable writers additionally drop the verdict outright, like the
 *    file-1 gate: interpreted/adopted stores overlapping the range
 *    (runtime_note_store), the archive read entry points, and battle entry/exit.
 *  - A REJECTED verdict (some other code resident) is kept while the first
 *    0x40 bytes of the range and the 0x40 bytes around the target still match
 *    the rejected snapshot, so another overlay that executes an instruction at
 *    an adopted address does not pay a full re-hash each time.  This can only
 *    keep interpreting something that has meanwhile become the results
 *    module; it never adopts anything.
 *
 * Fail closed: an empty allowlist, an unbound body, a pending delay-slot load,
 * a runtime/CPU context mismatch or any identity doubt returns 0 and the
 * interpreter runs the retail bytes. */

#include "battle_results_host.h"

#define RESULTS_BASE 0x801de000u
#define RESULTS_RAM_OFFSET (RESULTS_BASE - 0x80000000u)
#define RESULTS_UNKNOWN 0
#define RESULTS_VERIFIED 1
#define RESULTS_REJECTED 2
#define RESULTS_REJECT_PROBE 0x40u
/* SHA-256 of retail guest 0x801DE000..0x801E44C0: the first 0x64C0 bytes of
 * disc archive 0x10, file 4 (tools/scripts/extract_battle_results_module.py).
 * An identity constant only; it contains no retail data. */
#define RESULTS_IMMUTABLE_SHA256 \
    "80c31ea1289a0ce11e2440ee9759494503347c5a27cbb325d33a4492e4d74f67"

typedef struct BattleResultsLeaf {
    uint32_t address; /* real jal target */
    const char *name; /* func_XXXXXXXX; host body is battle_results_<name> */
} BattleResultsLeaf;

static const BattleResultsLeaf kResultsLeaves[] = {
#include "battle_results_host_leaves.inc"
    {0u, NULL}
};
#define RESULTS_LEAF_SLOTS (sizeof(kResultsLeaves) / sizeof(kResultsLeaves[0]))

static BattleResultsHostBody g_ResultsHosts[RESULTS_LEAF_SLOTS];
BattleResultsExitRegs g_BattleResultsExit;
BattleResultsEntryRegs g_BattleResultsEntry;
static unsigned char g_ResultsLogged[RESULTS_LEAF_SLOTS];
static int g_ResultsBound;

/* setjmp frame for the accessor's fail-closed path (PcPort_BattleMipsResolve). */
typedef struct ResultsCall {
    struct ResultsCall *previous;
    jmp_buf failure;
} ResultsCall;
static ResultsCall *results_call;
static const char *results_failure_reason;
static uint32_t results_failure_address;
static unsigned results_failure_width;

/* XENO_BATTLE_RESULTS_DIAG=1: log identity verdicts and the first native run
 * of each adopted routine.  Diagnostic only. */
static int results_diag_enabled(void)
{
    static int enabled = -1;
    if (enabled < 0) {
        const char *value = getenv("XENO_BATTLE_RESULTS_DIAG");
        enabled = value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
    }
    return enabled;
}

/* Resolve the allowlist to host bodies once.  An entry whose name does not
 * spell its address, lies outside the immutable range, or binds to nothing or
 * to a generated stub stays interpreted. */
static void results_bind(void)
{
    size_t i;

    if (g_ResultsBound)
        return;
    g_ResultsBound = 1;
    for (i = 0; kResultsLeaves[i].name != NULL; i++) {
        const BattleResultsLeaf *leaf = &kResultsLeaves[i];
        char expected[24];
        char symbol[48];

        snprintf(expected, sizeof(expected), "func_%08X", leaf->address);
        if (strcmp(expected, leaf->name) != 0 || (leaf->address & 3u) != 0 ||
            leaf->address - RESULTS_BASE >= RESULTS_IMMUTABLE_BYTES) {
            fprintf(stderr,
                    "[xeno-port][battle-results] allowlist entry %s does not "
                    "name results code at 0x%08x; it stays interpreted\n",
                    leaf->name, leaf->address);
            continue;
        }
        snprintf(symbol, sizeof(symbol), "battle_results_%s", leaf->name);
        if (xeno_port_is_generated_stub(symbol))
            continue;
        g_ResultsHosts[i] = (BattleResultsHostBody)dlsym(RTLD_DEFAULT, symbol);
    }
}

static int results_snapshot_matches_retail(const BattleResultsIdentity *identity)
{
    static const char expected[] = RESULTS_IMMUTABLE_SHA256;
    uint8_t digest[32];
    unsigned i;

    file1_digest(identity->snapshot, RESULTS_IMMUTABLE_BYTES, digest);
    for (i = 0; i < 32; i++) {
        if (file1_hex_digit(digest[i] >> 4) != expected[i * 2] ||
            file1_hex_digit(digest[i] & 15u) != expected[i * 2 + 1])
            return 0;
    }
    return 1;
}

static void results_invalidate(BattleMipsRuntime *runtime)
{
    runtime->results.state = RESULTS_UNKNOWN;
    runtime->results.generation++;
}

static int results_identity_current(BattleMipsRuntime *runtime, uint32_t target)
{
    BattleResultsIdentity *identity = &runtime->results;
    const uint8_t *live = g_PsxRam + RESULTS_RAM_OFFSET;

    if (identity->state == RESULTS_VERIFIED &&
        memcmp(live, identity->snapshot, RESULTS_IMMUTABLE_BYTES) == 0)
        return 1;
    if (identity->state == RESULTS_REJECTED) {
        size_t at = (target - RESULTS_BASE) & ~(size_t)(RESULTS_REJECT_PROBE - 1u);
        size_t length = RESULTS_IMMUTABLE_BYTES - at;
        if (length > RESULTS_REJECT_PROBE)
            length = RESULTS_REJECT_PROBE;
        if (memcmp(live, identity->snapshot, RESULTS_REJECT_PROBE) == 0 &&
            memcmp(live + at, identity->snapshot + at, length) == 0)
            return 0;
    }
    /* A verified image that changed under us was written by someone no hook
     * saw; record that as an invalidation before re-deciding. */
    if (identity->state == RESULTS_VERIFIED)
        identity->generation++;
    memcpy(identity->snapshot, live, RESULTS_IMMUTABLE_BYTES);
    identity->state = results_snapshot_matches_retail(identity)
                          ? RESULTS_VERIFIED : RESULTS_REJECTED;
    if (results_diag_enabled())
        fprintf(stderr,
                "[xeno-port][battle-results] identity %s at target 0x%08x "
                "(generation %llu)\n",
                identity->state == RESULTS_VERIFIED ? "verified" : "rejected",
                target, (unsigned long long)identity->generation);
    return identity->state == RESULTS_VERIFIED;
}

/* Called by runtime_note_store for every interpreted or adopted guest store,
 * with the resolved host pointer, so KSEG0/KSEG1 aliases are both seen. */
static void results_note_store(BattleMipsRuntime *runtime, const uint8_t *p,
                               unsigned width)
{
    uintptr_t begin = (uintptr_t)g_PsxRam + RESULTS_RAM_OFFSET;

    if (runtime->results.state != RESULTS_UNKNOWN &&
        (uintptr_t)p + width > begin &&
        (uintptr_t)p < begin + RESULTS_IMMUTABLE_BYTES)
        results_invalidate(runtime);
}

/* ArchiveReadFileFromCdSector, ArchiveReadFileToBuffer and ArchiveReadFile
 * are how a module reaches 0x801DE000; drop the verdict on every one of them
 * (wherever it writes) instead of reproducing the file-1 size decode.  The
 * content check would catch the load anyway; this keeps the cached verdict
 * from outliving a load by even one call. */
static void results_note_archive(BattleMipsRuntime *runtime, uint32_t target)
{
    if (target == 0x8002954cu || target == 0x800295d8u ||
        target == 0x80029690u)
        results_invalidate(runtime);
}

/* DIAGNOSTIC / EVIDENCE TOOLING: XENO_RESULTS_CAPTURE_3A18=<dir> writes the
 * complete call-site state of the first four calls of one RESULTS routine
 * (0x801E3A18, or XENO_RESULTS_CAPTURE_TARGET=<hex address>) -- guest RAM,
 * the native g_GameState copy, the scratchpad and the CPU registers -- before
 * the dispatch decides native vs interpreted, so a reference (interpreted)
 * run and an adopted run capture the same thing.  The files are
 * retail-derived game state: they stay with the run (runner job out/), never
 * in the tree or a package.  Replayed by
 * battle_results_host_differential_test.c --replay-3a18.  Unset => nothing. */
static void results_capture_write(const char *dir, unsigned n, const char *what,
                                  const void *data, size_t size)
{
    char path[512];
    FILE *f;

    snprintf(path, sizeof(path), "%s/3a18-%u-%s.bin", dir, n, what);
    f = fopen(path, "wb");
    if (f == NULL || fwrite(data, 1, size, f) != size) {
        fprintf(stderr, "[xeno-port][battle-results] capture write failed: %s\n", path);
        if (f != NULL)
            fclose(f);
        return;
    }
    fclose(f);
}

static uint32_t results_capture_target(void)
{
    static uint32_t target;
    if (target == 0) {
        const char *e = getenv("XENO_RESULTS_CAPTURE_TARGET");
        target = (e != NULL && e[0] != '\0') ? (uint32_t)strtoul(e, NULL, 16) : 0x801E3A18u;
    }
    return target;
}

static void results_capture_3a18(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    static unsigned taken;
    const char *dir = getenv("XENO_RESULTS_CAPTURE_3A18");
    uint32_t regs[36];
    size_t d;

    if (dir == NULL || dir[0] == '\0' || taken >= 4)
        return;
    memcpy(regs, cpu->gpr, sizeof(cpu->gpr));
    regs[32] = cpu->hi;
    regs[33] = cpu->lo;
    regs[34] = cpu->load_valid ? (0x100u | cpu->load_reg) : 0u;
    /* The captured routine.  Identity is not evaluated here (it caches
     * verdict state); the replay verifies the captured RAM. */
    regs[35] = results_capture_target();
    results_capture_write(dir, taken, "ram", g_PsxRam, PSX_RAM_SIZE);
    results_capture_write(dir, taken, "pad", g_PsxScratchpad, 4096);
    results_capture_write(dir, taken, "regs", regs, sizeof(regs));
    for (d = 0; d < runtime->data_count; d++) {
        if (strcmp(runtime->data[d].name, "g_GameState") == 0)
            results_capture_write(dir, taken, "gs", runtime->data[d].host,
                                  runtime->data[d].size);
    }
    fprintf(stderr, "[xeno-port][battle-results] captured 0x%08X call %u "
            "(sp=%08x ra=%08x) to %s\n", regs[35], taken, cpu->gpr[29], cpu->gpr[31], dir);
    taken++;
}

static int results_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                            uint32_t target)
{
    ResultsCall call;
    /* volatile: read in the setjmp failure path; GCC >= 14 -Wclobbered
     * cannot see that it is never written after setjmp. */
    volatile size_t index;
    uint32_t v0;

    /* Empty allowlist: nothing below runs and dispatch is what it was before
     * results adoption existed.  The bridge sees every guest PC, so the range
     * check comes before anything else. */
    if (kResultsLeaves[0].name == NULL)
        return 0;
    if (target - RESULTS_BASE >= RESULTS_IMMUTABLE_BYTES)
        return 0;
    if (target == results_capture_target())
        results_capture_3a18(runtime, cpu);
    for (index = 0; kResultsLeaves[index].name != NULL; index++) {
        if (kResultsLeaves[index].address == target)
            break;
    }
    if (kResultsLeaves[index].name == NULL)
        return 0;
    /* Route reference control: disable ONLY RESULTS adoption. Other overlay
     * and main-executable bridges keep their ordinary production behavior. */
    {
        const char *control = getenv("XENO_BATTLE_RESULTS_FORCE_INTERPRET");
        if (control && control[0] && strcmp(control, "0") != 0)
            return 0;
    }
    results_bind();
    if (g_ResultsHosts[index] == NULL)
        return 0;
    /* A load issued in the jal delay slot has not reached its register yet;
     * the callee's first instruction would see it land.  A native body would
     * read the stale register and the load would then land after the return,
     * on top of v0 or whatever the body produced.  Let retail run it. */
    if (cpu->load_valid && cpu->load_reg != 0)
        return 0;
    /* The accessor resolves through g_ActiveBattleRuntime. */
    if (!runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu)
        return 0;
    if (!results_identity_current(runtime, target))
        return 0;
    if (results_diag_enabled() && !g_ResultsLogged[index]) {
        g_ResultsLogged[index] = 1;
        fprintf(stderr,
                "[xeno-port][battle-results] native %s at 0x%08x "
                "(results module identity verified)\n",
                kResultsLeaves[index].name, target);
    }
    call.previous = results_call;
    results_call = &call;
    if (setjmp(call.failure) != 0) {
        results_call = call.previous;
        /* Guest writes made before the fault stand, as they would have under
         * the interpreter, which faults on the same access. */
        fprintf(stderr,
                "[xeno-port][battle-results] native %s stopped: %s "
                "addr=0x%08x width=%u\n",
                kResultsLeaves[index].name, results_failure_reason,
                results_failure_address, results_failure_width);
        return -1;
    }
    g_BattleResultsExit.mask = 0;
    g_BattleResultsExit.hilo = 0;
    g_BattleResultsEntry.sp = cpu->gpr[29];
    memcpy(g_BattleResultsEntry.gpr, cpu->gpr, sizeof(g_BattleResultsEntry.gpr));
    v0 = g_ResultsHosts[index](cpu->gpr[4], cpu->gpr[5], cpu->gpr[6],
                               cpu->gpr[7]);
    results_call = call.previous;
    runtime->results.native_calls++;
    cpu->gpr[2] = v0;
    /* r0 never; v0 is the return; sp/ra/s* are callee-saved by the retail
     * body's own prologue and never in an exit set. */
    {
        unsigned reg;
        for (reg = 1; reg < 32; reg++) {
            if (reg != 2 && (g_BattleResultsExit.mask & (1u << reg)))
                cpu->gpr[reg] = g_BattleResultsExit.gpr[reg];
        }
    }
    if (g_BattleResultsExit.hilo) {
        cpu->hi = g_BattleResultsExit.hi;
        cpu->lo = g_BattleResultsExit.lo;
    }
    return 1;
}

uint8_t *PcPort_BattleMipsResolve(uint32_t address, unsigned width, int write)
{
    BattleMipsRuntime *runtime = g_ActiveBattleRuntime;
    const char *reason = NULL;
    uint8_t *p = NULL;

    if (runtime == NULL || !runtime->initialized)
        reason = "no active battle runtime";
    else if (width != 1 && width != 2 && width != 4)
        reason = "invalid access width";
    else if ((address & (width - 1u)) != 0)
        reason = "unaligned access"; /* retail raises an address error */
    else if ((p = resolve_memory(runtime, address, width, write != 0)) == NULL)
        reason = "unmapped guest address";
    if (reason != NULL) {
        results_failure_reason = reason;
        results_failure_address = address;
        results_failure_width = width;
        if (results_call != NULL)
            longjmp(results_call->failure, 1);
        fprintf(stderr,
                "[xeno-port][battle-results] guest access outside an adopted "
                "call: %s addr=0x%08x width=%u\n", reason, address, width);
        abort();
    }
    if (write)
        runtime_note_store(runtime, p, width);
    return p;
}

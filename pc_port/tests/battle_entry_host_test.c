/* The retail entry is the oracle; callees are deterministic boundary models.
 * This proves entry control flow and ABI, not the callees or playable battle.
 * Compare every call's declared arguments, SP/RA, saved registers, frame and
 * mutable global state, then all RAM/shared data and preserved registers.
 */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
unsigned int MFC2(int r) { (void)r; abort(); }
unsigned int CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned int v, int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned int v, int r) { (void)v; (void)r; abort(); }
int doCOP2(int op) { (void)op; abort(); }
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
int PcPort_QuickCheckpointFieldIsActive(void) { return 0; }
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }

static BattleMipsRuntime rt;
static uint8_t retail[BATTLE_ENTRY_BYTES], shared[0x20000];
static uint8_t expected_ram[PSX_RAM_SIZE], expected_shared[sizeof(shared)];
static unsigned case_id, calls, allocations, ready_frames, fight_frames, result_frames;
static unsigned compare_calls, stop_target, pending_return;
static uint32_t finish_flags;
static int native_entry, failed;

typedef struct CallTrace {
    uint32_t target, ra, sp, args[5], saved[4];
    uint8_t frame[0x30], shared_hash[32], battle_hash[32];
} CallTrace;
static CallTrace traces[128];

static void fail(const char *label, uint32_t detail)
{
    fprintf(stderr, "BATTLE ENTRY FAIL case=%u call=%u %s 0x%08x\n",
            case_id, calls, label, detail);
    failed = 1;
}

static uint32_t get(uint32_t a, unsigned w)
{
    uint32_t v;
    assert(runtime_read(&rt, a, w, &v) == 0);
    return v;
}
static void put(uint32_t a, unsigned w, uint32_t v)
{
    assert(runtime_write(&rt, a, w, v) == 0);
}

static int arg_count(uint32_t target)
{
    switch (target) {
    case 0x8008abb8u: case 0x8003f8e8u: case 0x80028470u: return 2;
    case 0x8003a89cu: case 0x8003f99cu: case 0x800397fcu: return 3;
    case 0x800295d8u: return 4;
    case 0x800b39c0u: return 5;
    case 0x80028a60u: case 0x800320e8u: case 0x800b8098u:
    case 0x800b81bcu: case 0x800bc404u: case 0x8003a094u:
    case 0x8003852cu: case 0x80070eb0u: case 0x800b853cu: return 1;
    case 0x801e0a34u: case 0x8007252cu: case 0x80077990u:
    case 0x800716d8u: case 0x8007819cu: case 0x80070e2cu:
    case 0x8009892cu: case 0x800723e0u: case 0x800c0f70u:
    case 0x800b8d7cu: case 0x80070edcu: case 0x801e252cu: return 0;
    default: return -1;
    }
}

static int model_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target == BATTLE_ENTRY && native_entry)
        return runtime_bridge(opaque, cpu, target);
    if (target >= BATTLE_ENTRY && target < BATTLE_ENTRY + BATTLE_ENTRY_BYTES)
        return 0;
    if (stop_target != 0 && target - stop_target < 12u)
        return 0;
    int argc = arg_count(target);
    if (argc < 0 || calls >= sizeof(traces) / sizeof(traces[0])) {
        fail("unexpected callee", target);
        return -1;
    }
    CallTrace trace = { .target = target, .ra = cpu->gpr[31], .sp = cpu->gpr[29] };
    for (int i = 0; i < argc; i++)
        trace.args[i] = i < 4 ? cpu->gpr[4 + i] : get(cpu->gpr[29] + 0x10, 4);
    memcpy(trace.saved, cpu->gpr + 16, sizeof(trace.saved));
    memcpy(trace.frame, PSX_ADDR(cpu->gpr[29]), sizeof(trace.frame));
    /* Hashing the full boundary state is test bookkeeping. OpenSSL is an
     * existing port dependency and keeps O0/UBSan runs bounded; identity
     * still uses the unmodified production digest implementation. */
    assert(SHA256(shared, sizeof(shared), trace.shared_hash));
    assert(SHA256(PSX_ADDR(0x800c3a6cu), 0x11000, trace.battle_hash));
    if (compare_calls) {
        if (memcmp(&trace, &traces[calls], sizeof(trace)) != 0) {
            fail("call/state differs", target);
            uint32_t actual[13], expected[13];
            memcpy(actual, &trace, sizeof(actual));
            memcpy(expected, &traces[calls], sizeof(expected));
            for (unsigned i = 0; i < 13; i++)
                if (actual[i] != expected[i])
                    fprintf(stderr, "  word[%u]=%08x expected=%08x\n", i, actual[i], expected[i]);
            return -1;
        }
    } else {
        traces[calls] = trace;
    }
    ++calls;
    cpu->gpr[2] = 0x5000u + calls;
    cpu->gpr[3] = 0x6000u + calls;
    switch (target) {
    case 0x8008abb8u:
        cpu->gpr[2] = allocations < 3 ? 0x80100000u + allocations * 0x10000u
                                     : 0x801f0000u - (allocations - 3u) * 0x1000u;
        allocations++;
        break;
    case 0x8003f8e8u:
        assert(cpu->gpr[5] <= 0xa2b4u);
        memset(PSX_ADDR(cpu->gpr[4]), 0, cpu->gpr[5]);
        break;
    case 0x8003f99cu:
        for (unsigned i = 0; i < cpu->gpr[6]; i++)
            put(cpu->gpr[4] + i, 1, get(cpu->gpr[5] + i, 1));
        break;
    case 0x8007819cu:
        put(0x800ccc58u, 1, case_id & 1u);
        break;
    case 0x800716d8u:
        if (cpu->gpr[31] == 0x80071264u && ++ready_frames == 2)
            put(0x800ccc58u, 1, 1);
        if (cpu->gpr[31] == 0x800713acu && ++fight_frames == 3) {
            put(0x800c48eau, 1, finish_flags);
            put(0x800d2fc4u, 1, (case_id >> 3) & 1u);
        }
        if (cpu->gpr[31] == 0x80071618u && ++result_frames == 2)
            put(0x800ccc58u, 1, 0);
        break;
    case 0x8009892cu:
        put(0x800ccc58u, 1, (case_id >> 1) & 1u);
        break;
    case 0x800b853cu:
        put(0x800ccc58u, 1, (case_id >> 2) & 1u);
        break;
    default:
        break;
    }
    if (target == pending_return) {
        cpu->load_valid = 1;
        cpu->load_reg = 2;
        cpu->load_value = 0x1234;
        cpu->load_mask = UINT32_MAX;
    }
    return 1;
}

static void setup(PcPortMipsCpu *cpu)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0xa5, sizeof(g_PsxRam));
    memset(shared, 0x5a, sizeof(shared));
    memcpy(PSX_ADDR(BATTLE_ENTRY), retail, sizeof(retail));
    rt.initialized = 1;
    rt.data[0] = (ResolvedData){0x80050000u, 0x10000u, shared, "shared0"};
    rt.data[1] = (ResolvedData){0x80060000u, 0xfaf0u, shared + 0x10000, "shared1"};
    rt.data_count = 2;
    g_ActiveBattleRuntime = &rt;
    initialize_cpu(cpu, &rt);
    cpu->bus.bridge = model_bridge;
    for (unsigned r = 1; r < 29; r++)
        cpu->gpr[r] = 0x13570000u + r;
    cpu->gpr[18] = 0xdead0000u | ((case_id / 32) % 5);
    cpu->hi = 0x11223344u; cpu->lo = 0x55667788u;
    cpu->cp0[12] = 0x12345678u;
    calls = allocations = ready_frames = fight_frames = result_frames = 0;
    stop_target = pending_return = 0;
    put(0x8005947cu, 1, case_id % 3 == 0 ? 0 : case_id % 3 == 1 ? 1 : 0xff);
    put(0x80059180u, 1, (case_id >> 1) & 1u);
    put(0x800594f8u, 1, (case_id >> 2) & 1u);
    put(0x80059508u, 1, 2);
    put(0x80062528u, 4, 0xfedc1234u);
    put(0x8005949cu, 4, 0xabcd5678u);
    put(0x8005917cu, 4, case_id & 1 ? 0xa00e0000u : 0x800e0000u);
    put(0x800e0000u, 4, case_id & 1 ? 7 : UINT32_MAX);
    put(0x8005954cu, 1, (case_id & 1u) ? 4 : 0);
    put(0x800c3decu, 4, 0x80000001u);
    put(0x800c3d48u, 1, (case_id >> 4) & 1u);
    put(0x800c3d5cu, 1, (case_id >> 5) & 1u);
    put(0x800c3d44u, 1, (case_id >> 6) & 1u);
    put(0x800d2fc4u, 1, 0);
    put(0x800c48eau, 1, 0);
    put(0x800d2d50u, 1, (case_id >> 7) & 1u);
    put(0x800d3278u, 4, 0x800e1000u);
    put(0x800d39dcu, 2, 0xff80);
    put(0x800595d0u, 4, 0x80130000u);
    put(0x80059480u, 4, 0x80131000u);
    put(0x800594acu, 4, 0x80132000u);
    for (unsigned i = 0; i < 0x2000; i++)
        put(0x800658dcu + i, 1, i * 7u + case_id);
    for (unsigned i = 0; i < 3; i++) {
        uint32_t id = (case_id + i) % 3 ? i + 1 : 0x7f;
        put(0x800d2d24u + i, 1, id);
        put(0x8006ecf4u + id * 0x20u, 2, 0x8100u + i);
        put(0x8006ecf6u + id * 0x20u, 2, 0x4200u + i);
        put(0x800ccd64u + i * 0x170u, 2, ((case_id >> i) & 1u) ? 0x8123 : 0x4567);
        put(0x800cce08u + i * 0x170u, 2, ((case_id >> (i + 1)) & 1u) ? 0x8abc : 0x1234);
    }
    const uint32_t outcomes[] = {1, 0x40, 0x80, 0xc0};
    finish_flags = outcomes[(case_id / 8) % 4];
    if (case_id >= 160 && case_id < 165) {
        /* Exercise every retained-s2 outcome and an out-of-switch value. */
        cpu->gpr[18] = 0xdead0000u | (case_id - 160);
        put(0x800c3d48u, 1, 1);
        put(0x800c3d5cu, 1, 0);
        finish_flags = 0x80;
    }
}

static int run_case(void)
{
    PcPortMipsCpu oracle, native;
    setup(&oracle);
    native_entry = compare_calls = 0;
    int rc = PcPortMipsRun(&oracle, BATTLE_ENTRY, BATTLE_HALT_PC, 10000);
    if (rc != PC_PORT_MIPS_HALTED) { fail(oracle.error, oracle.pc); return 0; }
    unsigned expected_calls = calls;
    assert(oracle.steps > 300);
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    memcpy(expected_shared, shared, sizeof(shared));
    setup(&native);
    native_entry = compare_calls = 1;
    /* The candidate must finish with the production interpreter gate closed. */
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    rc = PcPortMipsRun(&native, BATTLE_ENTRY, BATTLE_HALT_PC, 10000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    if (rc != PC_PORT_MIPS_HALTED) { fail(native.error, native.pc); return 0; }
    if (calls != expected_calls || native.steps != 0 || rt.bridge_cpu != NULL)
        fail("calls/steps/context", calls);
    for (unsigned r = 16; r < 32; r++)
        if (oracle.gpr[r] != native.gpr[r]) fail("preserved register", r);
    if (oracle.gpr[2] != native.gpr[2] || oracle.gpr[3] != native.gpr[3] ||
        oracle.hi != native.hi || oracle.lo != native.lo ||
        memcmp(oracle.cp0, native.cp0, sizeof(oracle.cp0))) fail("return registers", 0);
    if (memcmp(expected_ram, g_PsxRam, sizeof(g_PsxRam))) fail("final RAM", 0);
    if (memcmp(expected_shared, shared, sizeof(shared))) fail("final shared data", 0);
    return !failed;
}

static int controls(void)
{
    PcPortMipsCpu cpu;
    setup(&cpu); native_entry = 1; compare_calls = 0;
    rt.bridge_cpu = &cpu;
    for (unsigned i = 0; i < 3; i++) {
        size_t at = i == 0 ? 0 : i == 1 ? BATTLE_ENTRY_BYTES / 2 : BATTLE_ENTRY_BYTES - 1;
        ((uint8_t *)PSX_ADDR(BATTLE_ENTRY))[at] ^= 1;
        if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("identity accepted", i);
        memcpy(PSX_ADDR(BATTLE_ENTRY), retail, sizeof(retail));
    }
    cpu.load_valid = 1; cpu.load_reg = 4;
    if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("entry load accepted", 0);
    cpu.load_valid = 0;
    rt.force_interpret = 1;
    if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("reference control ignored", 0);
    rt.force_interpret = 0;
    assert(setenv("XENO_BATTLE_NATIVE_ENTRY", "0", 1) == 0);
    if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("entry control ignored", 0);
    assert(unsetenv("XENO_BATTLE_NATIVE_ENTRY") == 0);
    rt.initialized = 0;
    if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("uninitialized context accepted", 0);
    rt.initialized = 1;
    g_ActiveBattleRuntime = NULL;
    if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("inactive context accepted", 0);
    g_ActiveBattleRuntime = &rt;
    rt.bridge_cpu = NULL;
    if (battle_entry_try_host(&rt, &cpu, BATTLE_ENTRY) != 0) fail("wrong CPU accepted", 0);
    if (calls != 0) fail("rejected entry had effects", calls);

    /* A test-only three-instruction callee proves that a native entry still
     * exposes downstream fallback in both the outer and process receipts. */
    setup(&cpu); stop_target = 0x800b8098u;
    put(stop_target, 4, 0x24020007u); /* addiu v0,zero,7 */
    put(stop_target + 4u, 4, 0x03e00008u); /* jr ra */
    put(stop_target + 8u, 4, 0); /* delay slot */
    PcPortMipsExecutionStats interpreted_before = PcPortMipsExecutionStatsGet();
    if (PcPortMipsRun(&cpu, BATTLE_ENTRY, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_HALTED || cpu.steps != 3 ||
        PcPortMipsExecutionStatsGet().interpreted_instructions != interpreted_before.interpreted_instructions + 3)
        fail("nested interpretation hidden", (uint32_t)cpu.steps);

    setup(&cpu); pending_return = 0x8008abb8u;
    if (PcPortMipsRun(&cpu, BATTLE_ENTRY, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 1)
        fail("pending return load accepted", calls);
    setup(&cpu); stop_target = 0x800b8098u;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int rc = PcPortMipsRun(&cpu, BATTLE_ENTRY, BATTLE_HALT_PC, 10000);
    PcPortMipsExecutionStats after = PcPortMipsExecutionStatsGet();
    if (rc != PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps != 0 ||
        after.interpreted_instructions != before.interpreted_instructions ||
        after.disabled_refusals != before.disabled_refusals + 1 || rt.bridge_cpu != NULL)
        fail("unconverted callee not refused", (uint32_t)rc);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    return !failed;
}

int main(void)
{
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image && fseek(image, BATTLE_ENTRY - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(retail, 1, sizeof(retail), image) == sizeof(retail));
    assert(fclose(image) == 0);
    assert(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0);
    for (case_id = 0; case_id < 165; case_id++)
        if (!run_case()) return 1;
    if (!controls()) return 1;
    puts("BATTLE ENTRY PASS 165 retail differential cases, native steps=0, identity/disabled/fault controls");
    return 0;
}

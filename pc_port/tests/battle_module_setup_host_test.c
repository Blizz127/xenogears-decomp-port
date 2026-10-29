/* Retail module callers vs production native bodies, with modeled callees.
 * This checks the callers' complete contracts, not decompression/SDK behavior
 * or acceptance of an executed complete transition or battle. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>

uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
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
static uint8_t module[0x56c0], shared[0x20000], expected_shared[sizeof(shared)];
static uint8_t expected_ram[PSX_RAM_SIZE];
static unsigned variant, calls, decompressions, allocations;
static uint32_t frame_base, stop_target, pending_return;
static int candidate, compare, compose, failed;
typedef struct Trace {
    uint32_t target, ra, sp, saved[9], args[7], hi, lo;
    uint8_t frame[0x80], shared_hash[32], ram_hash[32];
} Trace;
static Trace traces[128];

static void fail(const char *label, uint32_t value)
{
    fprintf(stderr, "BATTLE MODULE SETUP FAIL variant=%u call=%u %s 0x%08x\n",
            variant, calls, label, value);
    failed = 1;
}
static uint32_t read_at(uint32_t address, unsigned width)
{
    uint32_t value;
    assert(runtime_read(&rt, address, width, &value) == 0);
    return value;
}
static void write_at(uint32_t address, unsigned width, uint32_t value)
{
    assert(runtime_write(&rt, address, width, value) == 0);
}
static int is_body(uint32_t target)
{
    return target - BATTLE_MODULE_DISPATCH_ADDRESS < BATTLE_MODULE_DISPATCH_BYTES ||
           (compose && target - BATTLE_MODULE_SETUP_ADDRESS < BATTLE_MODULE_SETUP_BYTES);
}
static int model(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (stop_target && target - stop_target < 12u) return 0;
    if (candidate && (target == BATTLE_MODULE_DISPATCH_ADDRESS ||
                      (compose && target == BATTLE_MODULE_SETUP_ADDRESS)))
        return runtime_bridge(opaque, cpu, target);
    if (is_body(target)) return 0;
    unsigned argc;
    switch (target) {
    case BATTLE_MODULE_SETUP_ADDRESS:
    case 0x801e4048u: case 0x801e4160u: case 0x801e4870u: case 0x801e4ac0u:
    case 0x801e4cd0u: case 0x801e4e7cu: case 0x801e5014u:
    case 0x801e6290u: case 0x801e62b8u: argc = 0; break;
    case 0x8003342cu: case 0x800320e8u: case 0x800288ecu: argc = 1; break;
    case 0x80089c9cu: case 0x80032e88u: case 0x80033698u: case 0x80078310u:
    case 0x80028470u: case 0x8008abb8u: case 0x8003f8e8u: argc = 2; break;
    case 0x8003f99cu: case 0x80029afcu: argc = 3; break;
    case 0x8002dde4u: argc = 7; break;
    default: fail("unexpected callee", target); return -1;
    }
    if (calls >= sizeof(traces) / sizeof(traces[0])) { fail("trace capacity", calls); return -1; }
    Trace trace = {.target = target, .ra = cpu->gpr[31], .sp = cpu->gpr[29],
                   .hi = cpu->hi, .lo = cpu->lo};
    memcpy(trace.saved, cpu->gpr + 16, 8 * sizeof(uint32_t));
    trace.saved[8] = cpu->gpr[30];
    memcpy(trace.frame, PSX_ADDR(frame_base), sizeof(trace.frame));
    for (unsigned i = 0; i < argc; i++)
        trace.args[i] = i < 4 ? cpu->gpr[4 + i] : read_at(cpu->gpr[29] + 0x10u + (i - 4u) * 4u, 4);
    assert(SHA256(shared, sizeof(shared), trace.shared_hash));
    assert(SHA256(g_PsxRam, sizeof(g_PsxRam), trace.ram_hash));
    if (compare && memcmp(&trace, &traces[calls], sizeof(trace))) {
        Trace *want = &traces[calls];
        if (trace.target != want->target) fail("target", want->target);
        if (trace.sp != want->sp || trace.ra != want->ra) fail("SP/RA", trace.sp);
        for (unsigned i = 0; i < 9; i++) if (trace.saved[i] != want->saved[i]) fail("saved register", i);
        for (unsigned i = 0; i < 7; i++) if (trace.args[i] != want->args[i]) fail("argument", i);
        if (memcmp(trace.frame, want->frame, sizeof(trace.frame))) fail("guest frame", frame_base);
        fail("call/frame/memory differs", target); return -1;
    }
    if (!compare) traces[calls] = trace;
    calls++;
    cpu->gpr[2] = 0x90000000u + calls;
    cpu->gpr[3] = 0x60000000u + calls;
    cpu->hi = 0x12300000u + calls;
    cpu->lo = 0x45600000u + calls;
    write_at(0x80059470u, 4, calls);
    switch (target) {
    case 0x80089c9cu:
        cpu->gpr[2] = 0xabcd0000u | ((trace.args[0] >> (trace.args[1] & 31u)) & 1u);
        if (variant & 16u) write_at(0x8006f368u + (calls - 1u), 1, (calls - 1u) * 2u);
        break;
    case 0x8003342cu:
        if (variant & 8u) write_at(0x800595a8u, 4, 0xa0151000u);
        break;
    case 0x8003f99cu: {
        uint8_t *dest = resolve_memory(&rt, trace.args[0], trace.args[2], 1);
        uint8_t *src = resolve_memory(&rt, trace.args[1], trace.args[2], 0);
        assert(dest && src && trace.args[2] <= 0x2000);
        memcpy(dest, src, trace.args[2]);
        if (trace.ra == 0x801e5520u && (variant & 8u))
            write_at(0x800d3294u, 1, read_at(0x800d3294u, 1) ^ 1u);
        if (trace.ra == 0x801e5598u && (variant & 16u))
            write_at(cpu->gpr[19], 1, (read_at(cpu->gpr[19], 1) + 1u) % 11u);
        break;
    }
    case 0x80032e88u: {
        uint32_t buffer = (variant & 4u ? 0xa0100000u : 0x80100000u) + decompressions++ * 0x2000u;
        assert(decompressions < 20 && trace.args[1] <= 1);
        uint8_t *p = PSX_ADDR(buffer);
        for (unsigned i = 0; i < 0x2000; i++) p[i] = (uint8_t)(trace.args[0] + i + calls * 13u + variant);
        cpu->gpr[2] = buffer;
        break;
    }
    case 0x800320e8u: write_at(trace.args[0], 4, 0xdead0000u | calls); break;
    case 0x800288ecu:
        cpu->gpr[2] = 0x100u + trace.args[0] * 8u;
        if (variant & 8u) write_at(0x8006f9dcu, 1, read_at(0x8006f9dcu, 1) + 17u);
        break;
    case 0x8008abb8u:
        cpu->gpr[2] = (variant & 4u ? 0xa0160000u : 0x80160000u) + allocations++ * 0x2000u;
        if (variant & 16u) write_at(0x8006f9dcu, 1, read_at(0x8006f9dcu, 1) ^ 0x80u);
        break;
    case 0x8003f8e8u:
        memset(PSX_ADDR(trace.args[0]), 0, trace.args[1]);
        break;
    case 0x8002dde4u:
        for (unsigned i = 1; i < 7; i++) if (trace.args[i]) fail("upload argument", i);
        break;
    default:
        write_at(0x800c3e80u + calls * 4u, 4, target);
        break;
    }
    if (pending_return == target) {
        cpu->load_valid = 1; cpu->load_reg = 2;
        cpu->load_value = 0xfeed0100u; cpu->load_mask = 0;
    }
    return 1;
}

static void setup(PcPortMipsCpu *cpu, uint32_t stage)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0xa5, sizeof(g_PsxRam));
    memset(shared, 0x5a, sizeof(shared));
    memcpy(PSX_ADDR(0x801e4000u), module, sizeof(module));
    rt.data[0] = (ResolvedData){0x80050000u, sizeof(shared), shared, "shared"};
    rt.data_count = 1; rt.initialized = 1;
    g_ActiveBattleRuntime = &rt;
    initialize_cpu(cpu, &rt); cpu->bus.bridge = model;
    for (unsigned r = 1; r < 29; r++) cpu->gpr[r] = 0x12340000u + variant * 256u + r;
    cpu->gpr[30] = 0x87650000u + variant;
    cpu->gpr[4] = stage;
    if (variant & 2u) cpu->gpr[29] |= 0x20000000u;
    frame_base = cpu->gpr[29] - 0x70u;
    cpu->hi = 0x98765432u; cpu->lo = 0xfedcba98u;
    calls = decompressions = allocations = 0;
    stop_target = pending_return = 0;
    write_at(0x8006f9dcu, 1, variant * 31u);
    write_at(0x8006f9ddu, 1, (variant & 1u ? 0x10u : 0) | 3u);
    static const uint32_t masks[] = {0, 5, 0xffff, 0x400};
    write_at(0x8006f364u, 2, masks[(variant >> 1) & 3u]);
    write_at(0x8006f366u, 2, 0xffff);
    write_at(0x8006f368u, 1, variant & 16u ? 0x80 : 0);
    write_at(0x8006f369u, 1, 2);
    write_at(0x8006f36au, 1, 10);
    write_at(0x800595a8u, 4, 0x80150000u);
    for (unsigned table = 0; table < 2; table++)
        for (unsigned i = 0; i < 64; i++)
            write_at(0x80150000u + table * 0x1000u + i * 4u, 4, 0x10000u + i * 0x101u + table * 0x7000u);
    for (unsigned actor = 0; actor < 11; actor++) {
        uint32_t address = 0x8006d8a0u + actor * 0xa4u;
        for (unsigned i = 0; i < 0xa4; i++) write_at(address + i, 1, actor * 7u + i);
        static const uint32_t gears[] = {0, 0xff, 3, 17};
        write_at(address + 0xa0u, 1, gears[(variant + actor) & 3u]);
    }
}

static int run_case(uint32_t stage, uint32_t entry)
{
    PcPortMipsCpu oracle, native;
    setup(&oracle, stage); candidate = compare = 0;
    if (PcPortMipsRun(&oracle, entry, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_HALTED) {
        fail(oracle.error, oracle.pc); return 0;
    }
    unsigned expected_calls = calls;
    if (!oracle.steps) fail("oracle did not execute", 0);
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    memcpy(expected_shared, shared, sizeof(shared));
    setup(&native, stage); candidate = compare = 1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int rc = PcPortMipsRun(&native, entry, BATTLE_HALT_PC, 10000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    if (rc != PC_PORT_MIPS_HALTED) { fail(native.error, native.pc); return 0; }
    if (calls != expected_calls || native.steps || rt.bridge_cpu ||
        PcPortMipsExecutionStatsGet().interpreted_instructions != before.interpreted_instructions)
        fail("calls/context/instructions", calls);
    for (unsigned r = 16; r < 32; r++)
        if (native.gpr[r] != oracle.gpr[r]) fail("preserved register", r);
    if (native.gpr[2] != oracle.gpr[2] || native.gpr[3] != oracle.gpr[3] ||
        native.hi != oracle.hi || native.lo != oracle.lo) fail("return values", 0);
    if (memcmp(g_PsxRam, expected_ram, sizeof(g_PsxRam)) ||
        memcmp(shared, expected_shared, sizeof(shared))) fail("final memory", 0);
    return !failed;
}

static int controls(void)
{
    PcPortMipsCpu cpu;
    variant = 0; compose = candidate = 1; compare = 0;
    setup(&cpu, 0); rt.bridge_cpu = &cpu;
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    const uint32_t addresses[] = {BATTLE_MODULE_SETUP_ADDRESS, BATTLE_MODULE_DISPATCH_ADDRESS};
    const unsigned sizes[] = {BATTLE_MODULE_SETUP_BYTES, BATTLE_MODULE_DISPATCH_BYTES};
    for (unsigned target = 0; target < 2; target++) {
        for (unsigned region = 0; region < 2; region++) {
            unsigned offsets[] = {0, sizes[region] / 2, sizes[region] - 1};
            for (unsigned i = 0; i < 3; i++) {
                uint8_t *p = PSX_ADDR(addresses[region] + offsets[i]); *p ^= 1;
                if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) {
                    fail("modified module accepted", region);
                    return 0;
                }
                *p ^= 1;
            }
        }
        cpu.load_valid = 1; cpu.load_reg = 4;
        if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) fail("pending entry load", target);
        cpu.load_valid = 0; rt.force_interpret = 1;
        if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) fail("reference hook", target);
        rt.force_interpret = 0;
        assert(setenv("XENO_BATTLE_NATIVE_MODULE_SETUP", "0", 1) == 0);
        if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) fail("reference switch", target);
        assert(unsetenv("XENO_BATTLE_NATIVE_MODULE_SETUP") == 0);
        g_ActiveBattleRuntime = NULL;
        if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) fail("inactive runtime", target);
        g_ActiveBattleRuntime = &rt; rt.bridge_cpu = NULL;
        if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) fail("wrong CPU", target);
        rt.bridge_cpu = &cpu; rt.initialized = 0;
        if (battle_module_setup_try_host(&rt, &cpu, addresses[target])) fail("uninitialized runtime", target);
        rt.initialized = 1;
    }
    if (calls || memcmp(g_PsxRam, expected_ram, sizeof(expected_ram))) fail("rejection had effects", calls);
    setup(&cpu, 0); pending_return = 0x8003342cu;
    if (PcPortMipsRun(&cpu, BATTLE_MODULE_DISPATCH_ADDRESS, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 4)
        fail("pending callee load", calls);
    const uint32_t stops[] = {BATTLE_MODULE_SETUP_ADDRESS, 0x8003342cu, 0x80078310u};
    for (unsigned i = 0; i < 3; i++) {
        setup(&cpu, 0); stop_target = stops[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
        PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
        if (PcPortMipsRun(&cpu, BATTLE_MODULE_DISPATCH_ADDRESS, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_UNRESOLVED_CALL ||
            cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().disabled_refusals != before.disabled_refusals + 1)
            fail("missing callee/context", i);
        if (i == 2 && read_at(0x800d2f5cu, 4) == 0xa5a5a5a5u) fail("completed writes lost", i);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    }
    return !failed;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    assert(setvbuf(stdout, NULL, _IOLBF, 0) == 0);
    FILE *image = fopen(argv[1], "rb");
    assert(image && fread(module, 1, sizeof(module), image) == sizeof(module));
    assert(fclose(image) == 0);
    assert(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0);
    const uint32_t stages[] = {0, 1, 2, 3, 4, 255, 256, 257, 258, 259, 0xffffffffu, 0x80000000u};
    for (variant = 0; variant < 48; variant++)
        if (!run_case(stages[variant % 12u], BATTLE_MODULE_DISPATCH_ADDRESS)) return 1;
    puts("BATTLE MODULE dispatch PASS 48 cases");
    compose = 1;
    for (variant = 0; variant < 32; variant++)
        if (!run_case(0, BATTLE_MODULE_SETUP_ADDRESS)) return 1;
    puts("BATTLE MODULE setup PASS 32 cases");
    for (variant = 0; variant < 32; variant++)
        if (!run_case(variant << 8, BATTLE_MODULE_DISPATCH_ADDRESS)) return 1;
    if (!controls()) return 1;
    puts("BATTLE MODULE SETUP PASS 112 modeled-callee differential cases: 48 dispatch, 32 setup, 32 composed; identity, aliases, mutable callbacks, stack arguments and refusal controls; native steps=0");
    return 0;
}

/* Retail transition oracle against production native dispatch. Callees are
 * boundary models; this verifies the caller, not a rendered/playable battle. */
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
static uint8_t code[BATTLE_TRANSITION_BYTES], table[BATTLE_TRANSITION_TABLE_BYTES];
static uint8_t shared[0x10000], expected_shared[sizeof(shared)], expected_ram[PSX_RAM_SIZE];
static uint8_t expected_scratch[sizeof(g_PsxScratchpad)];
static unsigned calls, variant, frames, vsyncs, module_stage, stop_target, pending_return;
static uint32_t initial_sp, heap;
static int candidate, compare, failed;

typedef struct Trace {
    uint32_t target, ra, sp, saved[8], args[2];
    uint8_t frame[0x38], shared_hash[32], ram_hash[32], image_hash[32], scratch_hash[32];
} Trace;
static Trace traces[2048];

static void fail(const char *label, uint32_t detail)
{
    fprintf(stderr, "BATTLE TRANSITION FAIL variant=%u call=%u frame=%u %s 0x%08x\n",
            variant, calls, frames, label, detail);
    failed = 1;
}
static uint32_t read_at(uint32_t a, unsigned w)
{
    uint32_t value;
    assert(runtime_read(&rt, a, w, &value) == 0);
    return value;
}
static void write_at(uint32_t a, unsigned w, uint32_t value)
{
    assert(runtime_write(&rt, a, w, value) == 0);
}
static unsigned ready_first(void)
{
    const unsigned first[] = {1, 84, 91, 3};
    return first[variant % 4];
}
static unsigned ready_gap(void)
{
    const unsigned gaps[] = {1, 1, 3, 7};
    return gaps[variant % 4];
}
static int model(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (stop_target && target - stop_target < 12u) return 0;
    if (candidate && target == BATTLE_TRANSITION_ADDRESS)
        return runtime_bridge(opaque, cpu, target);
    if (target - BATTLE_TRANSITION_ADDRESS < BATTLE_TRANSITION_BYTES) return 0;
    unsigned argc;
    switch (target) {
    case 0x8001c944u: case 0x800b73ecu: case 0x800286ccu:
    case 0x8001bb0cu: case 0x80019ca0u: argc = 0; break;
    case 0x80031bdcu: case 0x800448f8u: case 0x80044894u:
    case 0x80044ad8u: case 0x80021ad8u: argc = 2; break;
    case 0x800445d0u: case 0x800320e8u: case 0x801e5840u:
    case 0x800b6f0cu: case 0x800b7160u: case 0x8004b54cu:
    case 0x80044e9cu: case 0x80044c44u: case 0x80044bd0u:
    case 0x800b7330u: case 0x80044534u: case 0x80028a60u: argc = 1; break;
    default: fail("unexpected callee", target); return -1;
    }
    if (calls >= sizeof(traces) / sizeof(traces[0])) {
        fail("too many calls", target); return -1;
    }
    Trace trace = { .target = target, .ra = cpu->gpr[31], .sp = cpu->gpr[29] };
    memcpy(trace.saved, cpu->gpr + 16, sizeof(trace.saved));
    memcpy(trace.frame, PSX_ADDR(initial_sp - 0x38u), sizeof(trace.frame));
    for (unsigned i = 0; i < argc; i++) trace.args[i] = cpu->gpr[4 + i];
    assert(SHA256(shared, sizeof(shared), trace.shared_hash));
    assert(SHA256(PSX_ADDR(0x800c3a6cu), 0x11000, trace.ram_hash));
    assert(SHA256(PSX_ADDR(heap), 0x30000, trace.image_hash));
    assert(SHA256(g_PsxScratchpad, sizeof(g_PsxScratchpad), trace.scratch_hash));
    if (compare && memcmp(&trace, &traces[calls], sizeof(trace))) {
        const Trace *want = &traces[calls];
        if (trace.target != want->target) fail("callee target differs", want->target);
        if (trace.ra != want->ra || trace.sp != want->sp) fail("SP/RA differs", trace.sp);
        if (memcmp(trace.saved, want->saved, sizeof(trace.saved))) fail("live saved registers differ", target);
        if (memcmp(trace.args, want->args, sizeof(trace.args))) fail("arguments differ", target);
        fail("call/frame/state differs", target); return -1;
    }
    if (!compare) traces[calls] = trace;
    calls++;
    cpu->gpr[2] = 0x85000100u + variant + calls;
    cpu->gpr[3] = 0x60000000u + calls;
    cpu->hi = 0x50000000u + calls;
    cpu->lo = 0x70000000u + calls;
    write_at(0x80059520u, 4, 0x80000000u | (variant * 32u + calls));
    switch (target) {
    case 0x80031bdcu:
        if (trace.args[0] != 0x30000 || trace.args[1] != 1) fail("heap contract", 0);
        cpu->gpr[2] = heap;
        break;
    case 0x800448f8u:
        if (read_at(trace.args[0], 4) || read_at(trace.args[0] + 4, 4) != (224u << 16 | 320u))
            fail("capture rectangle", trace.args[0]);
        for (unsigned i = 0; i < 320u * 224u; i++)
            write_at(trace.args[1] + i * 2u, 2, i * 17u + variant);
        break;
    case 0x80044894u:
        if (read_at(trace.args[0], 4) != (256u << 16 | 704u) ||
            read_at(trace.args[0] + 4, 4) != (224u << 16 | 320u))
            fail("upload rectangle", trace.args[0]);
        for (unsigned i = 0; i < 0x14000; i++)
            if (!(read_at(trace.args[1] + i * 2u, 2) & 0x8000u)) {
                fail("STP bit including padding", i); break;
            }
        break;
    case 0x80044ad8u:
        if (trace.args[1] != 0x1000) fail("ordering table count", trace.args[1]);
        /* Distinct writes expose incorrect buffer selection without modeling
         * the SDK's ordering-table implementation. */
        write_at(trace.args[0], 4, calls);
        write_at(trace.args[0] + 0x3ffcu, 4, calls ^ 0x80000000u);
        break;
    case 0x800b73ecu:
        cpu->gpr[2] = 0x80140000u;
        break;
    case 0x800286ccu:
        frames++;
        cpu->gpr[2] = frames >= ready_first() && (frames - ready_first()) % ready_gap() == 0 ? 0 : (variant & 1u ? UINT32_MAX : 1);
        break;
    case 0x801e5840u:
        if (trace.args[0] != module_stage++) fail("module stage", trace.args[0]);
        break;
    case 0x800b6f0cu: case 0x800b7160u:
        if (trace.sp != 0x1f8003f8u || read_at(0x1f8003fcu, 4) != initial_sp - 0x38u ||
            trace.args[0] != 0x80140000u) fail("scratchpad callback frame", trace.sp);
        write_at(0x1f800300u, 4, target ^ frames);
        break;
    case 0x8004b54cu:
        if (trace.args[0] != 2 || trace.sp != initial_sp - 0x38u) fail("Vsync/restored stack", trace.sp);
        vsyncs++;
        break;
    case 0x80021ad8u:
        if (trace.args[1] != 0xfffffff4u) fail("fade decrement", trace.args[1]);
        cpu->gpr[2] = trace.args[0] + trace.args[1] + variant;
        if (variant >= 8) write_at(0x800ccb34u, 4, 1u - read_at(0x800ccb34u, 4));
        break;
    case 0x80044e9cu: case 0x80044c44u:
        if (variant >= 8)
            write_at(0x800ccb00u, 4, read_at(0x800ccb00u, 4) == 0x800c4a20u ? 0x800c8a90u : 0x800c4a20u);
        break;
    }
    if (target == pending_return) {
        cpu->load_valid = 1; cpu->load_reg = 2;
        cpu->load_value = 0x12345678u; cpu->load_mask = 0;
    }
    return failed ? -1 : 1;
}
static void setup(PcPortMipsCpu *cpu)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0x35 + variant, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x69, sizeof(g_PsxScratchpad));
    memset(shared, 0x5a, sizeof(shared));
    memcpy(PSX_ADDR(BATTLE_TRANSITION_ADDRESS), code, sizeof(code));
    memcpy(PSX_ADDR(BATTLE_TRANSITION_TABLE), table, sizeof(table));
    rt.data[0] = (ResolvedData){0x80050000u, sizeof(shared), shared, "shared"};
    rt.data_count = 1; rt.initialized = 1;
    g_ActiveBattleRuntime = &rt;
    initialize_cpu(cpu, &rt);
    cpu->bus.bridge = model;
    for (unsigned r = 1; r < 29; r++) cpu->gpr[r] = 0x12340000u + variant * 256u + r;
    if (variant & 4u) cpu->gpr[29] = 0xa0000000u | (cpu->gpr[29] & 0x1fffffffu);
    initial_sp = cpu->gpr[29];
    heap = variant & 2u ? 0xa0100000u : 0x80100000u;
    calls = frames = vsyncs = module_stage = stop_target = pending_return = 0;
    const uint32_t buffers[] = {0x800c4a20u, 0x800c8a90u, 0, 0xa00c4a20u};
    write_at(0x800ccb00u, 4, buffers[variant % 4]);
    write_at(0x800ccb34u, 4, 7); /* caller must reset it before the loop */
}
static int run_case(void)
{
    PcPortMipsCpu oracle, native;
    setup(&oracle); candidate = compare = 0;
    if (PcPortMipsRun(&oracle, BATTLE_TRANSITION_ADDRESS, BATTLE_HALT_PC, 1000000) != PC_PORT_MIPS_HALTED) {
        fail(oracle.error, oracle.pc); return 0;
    }
    unsigned expected_calls = calls;
    unsigned expected_frames = ready_first() + 3 * ready_gap();
    if (expected_frames < 86) expected_frames = 86;
    if (frames != expected_frames || vsyncs != frames || module_stage != 4 || !oracle.steps)
        fail("oracle progression/cadence", frames);
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    memcpy(expected_shared, shared, sizeof(shared));
    memcpy(expected_scratch, g_PsxScratchpad, sizeof(expected_scratch));
    setup(&native); candidate = compare = 1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int rc = PcPortMipsRun(&native, BATTLE_TRANSITION_ADDRESS, BATTLE_HALT_PC, 1000000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    if (rc != PC_PORT_MIPS_HALTED) { fail(native.error, native.pc); return 0; }
    if (calls != expected_calls || native.steps || rt.bridge_cpu || frames != expected_frames || vsyncs != frames ||
        PcPortMipsExecutionStatsGet().interpreted_instructions != before.interpreted_instructions)
        fail("calls/context/instructions/cadence", calls);
    for (unsigned r = 16; r < 32; r++)
        if (native.gpr[r] != oracle.gpr[r]) fail("preserved register", r);
    if (native.gpr[2] != oracle.gpr[2] || native.gpr[3] != oracle.gpr[3] ||
        native.hi != oracle.hi || native.lo != oracle.lo) fail("return values", 0);
    if (memcmp(g_PsxRam, expected_ram, sizeof(g_PsxRam)) ||
        memcmp(shared, expected_shared, sizeof(shared)) ||
        memcmp(g_PsxScratchpad, expected_scratch, sizeof(g_PsxScratchpad))) fail("final memory", 0);
    return !failed;
}
static int controls(void)
{
    PcPortMipsCpu cpu;
    setup(&cpu); candidate = 1; compare = 0; rt.bridge_cpu = &cpu;
    const uint32_t locations[] = {BATTLE_TRANSITION_ADDRESS, BATTLE_TRANSITION_ADDRESS + 300,
        BATTLE_TRANSITION_ADDRESS + BATTLE_TRANSITION_BYTES - 1,
        BATTLE_TRANSITION_TABLE, BATTLE_TRANSITION_TABLE + 8, BATTLE_TRANSITION_TABLE + 19};
    for (unsigned i = 0; i < sizeof(locations) / sizeof(locations[0]); i++) {
        uint8_t *p = PSX_ADDR(locations[i]); *p ^= 1;
        if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS))
            fail("modified identity accepted", locations[i]);
        *p ^= 1;
    }
    cpu.load_valid = 1; cpu.load_reg = 4;
    if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS)) fail("pending entry load", 0);
    cpu.load_valid = 0; rt.force_interpret = 1;
    if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS)) fail("force interpret", 0);
    rt.force_interpret = 0;
    assert(setenv("XENO_BATTLE_NATIVE_TRANSITION", "0", 1) == 0);
    if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS)) fail("reference switch", 0);
    assert(unsetenv("XENO_BATTLE_NATIVE_TRANSITION") == 0);
    g_ActiveBattleRuntime = NULL;
    if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS)) fail("inactive runtime", 0);
    g_ActiveBattleRuntime = &rt; rt.bridge_cpu = NULL;
    if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS)) fail("wrong CPU", 0);
    rt.bridge_cpu = &cpu; rt.initialized = 0;
    if (battle_transition_try_host(&rt, &cpu, BATTLE_TRANSITION_ADDRESS)) fail("uninitialized runtime", 0);
    if (calls) fail("rejected identity/context had effects", calls);
    setup(&cpu); pending_return = 0x8001c944u;
    if (PcPortMipsRun(&cpu, BATTLE_TRANSITION_ADDRESS, BATTLE_HALT_PC, 1000000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 1)
        fail("pending callee load", calls);
    const uint32_t refused[] = {0x8001c944u, 0x800b6f0cu, 0x800b7160u};
    for (unsigned i = 0; i < sizeof(refused) / sizeof(refused[0]); i++) {
        setup(&cpu); stop_target = refused[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
        PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
        if (PcPortMipsRun(&cpu, BATTLE_TRANSITION_ADDRESS, BATTLE_HALT_PC, 1000000) != PC_PORT_MIPS_UNRESOLVED_CALL ||
            cpu.steps || PcPortMipsExecutionStatsGet().disabled_refusals != before.disabled_refusals + 1 || rt.bridge_cpu)
            fail("missing callee not refused/context", calls);
        if (i && (cpu.gpr[29] != 0x1f8003f8u || read_at(0x1f8003fcu, 4) != initial_sp - 0x38u))
            fail("scratchpad refusal state", cpu.gpr[29]);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    }
    return !failed;
}
int main(void)
{
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image && fseek(image, BATTLE_TRANSITION_ADDRESS - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(code, 1, sizeof(code), image) == sizeof(code));
    assert(fseek(image, BATTLE_TRANSITION_TABLE - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(table, 1, sizeof(table), image) == sizeof(table));
    assert(fclose(image) == 0);
    assert(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0);
    for (variant = 0; variant < 12; variant++) if (!run_case()) return 1;
    if (!controls()) return 1;
    puts("BATTLE TRANSITION PASS 12 retail/native modeled-callee cases; delayed loaders, double buffers, STP padding, callback mutations, scratchpad stack, cadence, identity/refusal controls; native steps=0");
    return 0;
}

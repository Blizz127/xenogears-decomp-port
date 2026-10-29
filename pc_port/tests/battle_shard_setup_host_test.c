/* Compare retail shard callers with production native dispatch. Default
 * cases model SDK boundaries; --sdk executes the real native SDK/RNG bodies
 * on both sides. This is not hardware graphics or playable-battle acceptance. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "psx/gtereg.h"
#include <openssl/sha.h>

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
int PcPort_QuickCheckpointFieldIsActive(void) { return 0; }
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
extern uint32_t g_RandomSeed;
extern VECTOR *Square0(VECTOR *, VECTOR *);
extern MATRIX *RotMatrixZYX(SVECTOR *, MATRIX *);

static BattleMipsRuntime rt;
static uint8_t alloc_code[BATTLE_SHARD_ALLOC_BYTES], init_code[BATTLE_SHARD_INIT_BYTES];
static uint8_t shared[0x10000], expected_shared[sizeof(shared)], expected_ram[PSX_RAM_SIZE];
static GTERegisters expected_gte;
static unsigned calls, variant, random_calls, stop_target, pending_return;
static uint32_t heap, initial_sp, frame_base, model_seed;
static int candidate, compare, failed, actual_sdk, isolate_alloc;

typedef struct Trace {
    uint32_t target, ra, sp, saved[9], args[4], hi, lo, seed;
    uint8_t frame[0xd0], vector[12], shared_hash[32], heap_hash[32], gte_hash[32];
} Trace;
static Trace traces[10100];

static void fail(const char *label, uint32_t detail)
{
    fprintf(stderr, "BATTLE SHARD SETUP FAIL variant=%u call=%u %s 0x%08x\n",
            variant, calls, label, detail);
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
static int model(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (stop_target && target - stop_target < 12u) return 0;
    if (candidate && (target == BATTLE_SHARD_ALLOC_ADDRESS ||
                      (!isolate_alloc && target == BATTLE_SHARD_INIT_ADDRESS)))
        return runtime_bridge(opaque, cpu, target);
    if (target - BATTLE_SHARD_ALLOC_ADDRESS < BATTLE_SHARD_ALLOC_BYTES ||
        (!isolate_alloc && target - BATTLE_SHARD_INIT_ADDRESS < BATTLE_SHARD_INIT_BYTES)) return 0;
    unsigned argc;
    switch (target) {
    case 0x8003fa38u: argc = 0; break;
    case BATTLE_SHARD_INIT_ADDRESS: case 0x80048c4cu: case 0x80043c60u: argc = 1; break;
    case 0x80031bdcu: case 0x8004a414u: case 0x8004b32cu:
    case 0x8004abbcu: case 0x80043c24u: argc = 2; break;
    case 0x8004947cu: argc = 3; break;
    case 0x80043a1cu: argc = 4; break;
    default: fail("unexpected callee", target); return -1;
    }
    if (calls >= sizeof(traces) / sizeof(traces[0])) {
        fail("too many calls", target); return -1;
    }
    Trace trace = {.target=target, .ra=cpu->gpr[31], .sp=cpu->gpr[29],
                   .hi=cpu->hi, .lo=cpu->lo, .seed=actual_sdk ? g_RandomSeed : model_seed};
    memcpy(trace.saved, cpu->gpr + 16, 8 * sizeof(uint32_t)); trace.saved[8] = cpu->gpr[30];
    for (unsigned i = 0; i < argc; i++) trace.args[i] = cpu->gpr[4 + i];
    memcpy(trace.frame, PSX_ADDR(frame_base), sizeof(trace.frame));
    memcpy(trace.vector, PSX_ADDR(0x800c35c4u), sizeof(trace.vector));
    assert(SHA256(shared, sizeof(shared), trace.shared_hash));
    assert(SHA256(PSX_ADDR(heap) - 16, 0x10f7c + 32, trace.heap_hash));
    assert(SHA256((uint8_t *)&gteRegs, sizeof(gteRegs), trace.gte_hash));
    if (compare && memcmp(&trace, &traces[calls], sizeof(trace))) {
        const Trace *want = &traces[calls];
        if (trace.target != want->target) fail("callee target", want->target);
        if (trace.sp != want->sp || trace.ra != want->ra) fail("SP/RA", trace.sp);
        for (unsigned i = 0; i < 9; i++) if (trace.saved[i] != want->saved[i]) fail("live saved register", i);
        if (memcmp(trace.args, want->args, sizeof(trace.args))) fail("arguments", target);
        if (trace.hi != want->hi || trace.lo != want->lo) fail("HI/LO", trace.lo);
        if (memcmp(trace.frame, want->frame, sizeof(trace.frame))) {
            for (unsigned i = 0; i < sizeof(trace.frame); i++)
                if (trace.frame[i] != want->frame[i]) { fail("frame byte", frame_base + i); break; }
        }
        fail("call/frame/state differs", target); return -1;
    }
    if (!compare) traces[calls] = trace;
    calls++;
    if (actual_sdk && target != 0x80031bdcu && target != BATTLE_SHARD_INIT_ADDRESS) {
        if (target == 0x8003fa38u) random_calls++;
        return runtime_bridge(opaque, cpu, target);
    }
    cpu->gpr[2] = 0x85000100u + calls;
    cpu->gpr[3] = 0x60000000u + calls;
    write_at(0x80059520u, 4, 0x80000000u | calls);
    switch (target) {
    case 0x80031bdcu:
        if (trace.args[0] != 0x10f7c || trace.args[1] != 1) fail("allocation size/flags", trace.args[0]);
        cpu->gpr[2] = heap;
        break;
    case BATTLE_SHARD_INIT_ADDRESS:
        if (!isolate_alloc || trace.args[0] != heap || read_at(heap + 4, 4) != heap ||
            read_at(heap + 0x20, 4) != heap) fail("allocation pointer slots", trace.args[0]);
        cpu->gpr[2] = heap + 0x80u; /* expose an allocator that discards the callee result */
        break;
    case 0x80048c4cu:
        if (trace.ra == 0x800b7460u) {
            if (trace.args[0] != 0x9500) fail("initial radius", trace.args[0]);
            cpu->gpr[2] = variant == 5 ? 0 : 195;
        } else {
            cpu->gpr[2] = variant == 7 ? 0x00100000u :
                          variant >= 4 ? (trace.args[0] ^ 0xe2345678u) : 2048u + trace.args[0] % 4096u;
        }
        break;
    case 0x8003fa38u:
        model_seed = model_seed * 1664525u + 1013904223u;
        random_calls++;
        cpu->gpr[2] = variant >= 4 ? model_seed : (model_seed >> 16) & 0x7fffu;
        break;
    case 0x8004a414u:
        for (unsigned i = 0; i < 3; i++) {
            uint32_t x = read_at(trace.args[0] + i * 4u, 4);
            write_at(trace.args[1] + i * 4u, 4, x * x);
        }
        cpu->gpr[2] = trace.args[1];
        break;
    case 0x8004b32cu:
        cpu->gpr[2] = (trace.args[0] ^ trace.args[1]) & 0xfffu;
        if (variant == 7) write_at(trace.sp + 0x48u, 4, UINT32_MAX);
        break;
    case 0x8004abbcu:
        for (unsigned i = 0; i < 32; i++)
            write_at(trace.args[1] + i, 1, read_at(trace.args[0] + (i % 6), 1) + calls + i);
        cpu->gpr[2] = trace.args[1];
        break;
    case 0x8004947cu:
        for (unsigned i = 0; i < 3; i++)
            write_at(trace.args[2] + i * 4u, 4,
                     read_at(trace.args[1] + i * 4u, 4) ^ read_at(trace.args[0] + i * 4u, 4));
        cpu->gpr[2] = trace.args[2];
        break;
    case 0x80043c60u:
        write_at(trace.args[0] + 3, 1, 7);
        write_at(trace.args[0] + 7, 1, 0x24);
        break;
    case 0x80043c24u:
        write_at(trace.args[0] + 7, 1, (read_at(trace.args[0] + 7, 1) & ~1u) | (trace.args[1] & 1u));
        break;
    case 0x80043a1cu:
        cpu->gpr[2] = 0x100u + calls;
        break;
    }
    if (target == pending_return) {
        cpu->load_valid = 1; cpu->load_reg = 2;
        cpu->load_value = 0x12345678u; cpu->load_mask = 0;
    }
    return failed ? -1 : 1;
}
static void setup(PcPortMipsCpu *cpu, uint32_t entry)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0x35 + variant, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x69, sizeof(g_PsxScratchpad));
    memset(shared, 0x5a, sizeof(shared));
    memset(&gteRegs, 0, sizeof(gteRegs));
    g_RandomSeed = model_seed = 0x12345678u + variant * 31337u;
    memcpy(PSX_ADDR(BATTLE_SHARD_ALLOC_ADDRESS), alloc_code, sizeof(alloc_code));
    memcpy(PSX_ADDR(BATTLE_SHARD_INIT_ADDRESS), init_code, sizeof(init_code));
    rt.data[0] = (ResolvedData){0x80050000u, sizeof(shared), shared, "shared"};
    rt.data_count = 1; rt.initialized = 1;
    if (actual_sdk) {
        const ResolvedFunction sdk[] = {
            {0x8003fa38u, (void *)rand, "rand"},
            {0x80043a1cu, (void *)GetTPage, "GetTPage"},
            {0x80043c24u, (void *)SetShadeTex, "SetShadeTex"},
            {0x80043c60u, (void *)SetPolyFT3, "SetPolyFT3"},
            {0x80048c4cu, (void *)SquareRoot0, "SquareRoot0"},
            {0x8004947cu, (void *)ApplyMatrixLV, "ApplyMatrixLV"},
            {0x8004a414u, (void *)Square0, "Square0"},
            {0x8004abbcu, (void *)RotMatrixZYX, "RotMatrixZYX"},
            {0x8004b32cu, (void *)ratan2, "ratan2"},
        };
        memcpy(rt.functions, sdk, sizeof(sdk)); rt.function_count = sizeof(sdk) / sizeof(sdk[0]);
    }
    g_ActiveBattleRuntime = &rt;
    initialize_cpu(cpu, &rt); cpu->bus.bridge = model;
    for (unsigned r = 1; r < 31; r++) if (r != 29) cpu->gpr[r] = 0x12340000u + variant * 256u + r;
    if (variant & 2u) cpu->gpr[29] = 0xa0000000u | (cpu->gpr[29] & 0x1fffffffu);
    initial_sp = cpu->gpr[29]; frame_base = initial_sp - (entry == BATTLE_SHARD_ALLOC_ADDRESS ? 0xb8u : 0xa0u);
    heap = variant & 4u ? 0xa0100000u : 0x80100000u;
    cpu->gpr[4] = heap;
    cpu->hi = 0xabcdef01u; cpu->lo = 0x12345678u;
    calls = random_calls = stop_target = pending_return = 0;
    write_at(0x800c35c4u, 4, 0);
    write_at(0x800c35c8u, 4, 0);
}
static int run_case(uint32_t entry)
{
    PcPortMipsCpu oracle, native;
    setup(&oracle, entry); candidate = compare = 0;
    if (PcPortMipsRun(&oracle, entry, BATTLE_HALT_PC, 1000000) != PC_PORT_MIPS_HALTED) {
        fail(oracle.error, oracle.pc); return 0;
    }
    unsigned expected_calls = calls;
    uint32_t expected_seed = actual_sdk ? g_RandomSeed : model_seed;
    if (random_calls != (isolate_alloc ? 0u : 560u * 7u)) fail("RNG call count", random_calls);
    if (!oracle.steps) fail("oracle did not execute", 0);
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    memcpy(expected_shared, shared, sizeof(shared));
    memcpy(&expected_gte, &gteRegs, sizeof(gteRegs));
    setup(&native, entry); candidate = compare = 1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int rc = PcPortMipsRun(&native, entry, BATTLE_HALT_PC, 1000000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    if (rc != PC_PORT_MIPS_HALTED) { fail(native.error, native.pc); return 0; }
    if (calls != expected_calls || native.steps || rt.bridge_cpu ||
        PcPortMipsExecutionStatsGet().interpreted_instructions != before.interpreted_instructions)
        fail("call count/context/instructions", calls);
    for (unsigned r = 16; r < 32; r++)
        if (native.gpr[r] != oracle.gpr[r]) fail("preserved register", r);
    if (native.gpr[2] != oracle.gpr[2] || native.gpr[3] != oracle.gpr[3] ||
        native.hi != oracle.hi || native.lo != oracle.lo) fail("return values", native.gpr[2]);
    if (memcmp(g_PsxRam, expected_ram, sizeof(g_PsxRam)) ||
        memcmp(shared, expected_shared, sizeof(shared)) ||
        memcmp(&gteRegs, &expected_gte, sizeof(gteRegs))) fail("final memory/GTE", 0);
    if ((actual_sdk ? g_RandomSeed : model_seed) != expected_seed) fail("RNG state", 0);
    if (!failed) fprintf(stderr, "BATTLE SHARD case PASS variant=%u entry=%08x sdk=%d isolated=%d calls=%u\n",
                         variant, entry, actual_sdk, isolate_alloc, calls);
    return !failed;
}
static int controls(void)
{
    PcPortMipsCpu cpu;
    setup(&cpu, BATTLE_SHARD_ALLOC_ADDRESS); candidate = 1; compare = 0; rt.bridge_cpu = &cpu;
    const uint32_t addresses[] = {BATTLE_SHARD_ALLOC_ADDRESS, BATTLE_SHARD_INIT_ADDRESS};
    const unsigned sizes[] = {BATTLE_SHARD_ALLOC_BYTES, BATTLE_SHARD_INIT_BYTES};
    for (unsigned unit = 0; unit < 2; unit++) {
        const unsigned offsets[] = {0, sizes[unit] / 2, sizes[unit] - 1};
        for (unsigned i = 0; i < 3; i++) {
            uint8_t *p = PSX_ADDR(addresses[unit] + offsets[i]); *p ^= 1;
            if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("modified identity", addresses[unit] + offsets[i]);
            *p ^= 1;
        }
        cpu.load_valid = 1; cpu.load_reg = 4;
        if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("pending entry load", unit);
        cpu.load_valid = 0; rt.force_interpret = 1;
        if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("force interpret", unit);
        rt.force_interpret = 0;
        assert(setenv("XENO_BATTLE_NATIVE_SHARD_SETUP", "0", 1) == 0);
        if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("reference control", unit);
        assert(unsetenv("XENO_BATTLE_NATIVE_SHARD_SETUP") == 0);
        g_ActiveBattleRuntime = NULL;
        if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("inactive runtime", unit);
        g_ActiveBattleRuntime = &rt; rt.bridge_cpu = NULL;
        if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("wrong CPU", unit);
        rt.bridge_cpu = &cpu; rt.initialized = 0;
        if (battle_shard_setup_try_host(&rt, &cpu, addresses[unit])) fail("uninitialized runtime", unit);
        rt.initialized = 1;
    }
    if (calls) fail("rejection had effects", calls);
    setup(&cpu, BATTLE_SHARD_ALLOC_ADDRESS); pending_return = 0x80031bdcu;
    if (PcPortMipsRun(&cpu, BATTLE_SHARD_ALLOC_ADDRESS, BATTLE_HALT_PC, 1000000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 1)
        fail("pending callee load", calls);
    const uint32_t refused[] = {BATTLE_SHARD_INIT_ADDRESS, 0x80048c4cu, 0x8004abbcu};
    for (unsigned i = 0; i < 3; i++) {
        setup(&cpu, BATTLE_SHARD_ALLOC_ADDRESS); stop_target = refused[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
        PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
        if (PcPortMipsRun(&cpu, BATTLE_SHARD_ALLOC_ADDRESS, BATTLE_HALT_PC, 1000000) != PC_PORT_MIPS_UNRESOLVED_CALL ||
            cpu.steps || PcPortMipsExecutionStatsGet().disabled_refusals != before.disabled_refusals + 1 || rt.bridge_cpu)
            fail("nested refusal/context", calls);
        if (read_at(heap + 4, 4) != heap || read_at(heap + 0x20, 4) != heap) fail("lost writes on refusal", i);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    }
    return !failed;
}
int main(int argc, char **argv)
{
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image && fseek(image, BATTLE_SHARD_ALLOC_ADDRESS - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(alloc_code, 1, sizeof(alloc_code), image) == sizeof(alloc_code));
    assert(fread(init_code, 1, sizeof(init_code), image) == sizeof(init_code));
    assert(fclose(image) == 0);
    assert(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0);
    if (argc == 2 && strcmp(argv[1], "--sdk") == 0) {
        actual_sdk = 1;
        for (variant = 0; variant < 4; variant++)
            if (!run_case(variant & 1u ? BATTLE_SHARD_ALLOC_ADDRESS : BATTLE_SHARD_INIT_ADDRESS)) return 1;
        puts("BATTLE SHARD SDK PASS 4 retail/native comparisons through actual native SDK/RNG and production bridge; full 560-shard initialization, native steps=0");
        return 0;
    }
    isolate_alloc = 1;
    for (variant = 0; variant < 4; variant++) if (!run_case(BATTLE_SHARD_ALLOC_ADDRESS)) return 1;
    isolate_alloc = 0;
    for (variant = 0; variant < 8; variant++)
        if (!run_case(variant & 1u ? BATTLE_SHARD_ALLOC_ADDRESS : BATTLE_SHARD_INIT_ADDRESS)) return 1;
    if (!controls()) return 1;
    puts("BATTLE SHARD SETUP PASS 12 retail/native modeled-callee cases: 4 allocation, 4 initialization, 4 composed; signed arithmetic, aliases, frames, identity/refusal controls; native steps=0");
    return 0;
}

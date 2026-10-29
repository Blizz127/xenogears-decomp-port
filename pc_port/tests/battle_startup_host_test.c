/* Retail startup/display oracle vs production native dispatch. Default cases
 * use modeled callees; --sdk uses the actual native graphics initializers.
 * This does not prove a rendered or playable battle. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>
#include "psx/gtereg.h"

GTERegisters gteRegs;
static GTERegisters expected_gte;
static int actual_sdk;
int GetVideoMode(void) { return 0; } /* NTSC for the actual SDK initializer. */

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
unsigned int MFC2(int r) { (void)r; abort(); }
unsigned int CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned int v, int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned int v, int r)
{
    /* The packed SetGeomOffset bridge writes these two raw control words.
     * This initializer fixture records register storage only; the shard SDK
     * suite separately exercises the actual native GTE backend. */
    assert(r == 24 || r == 25);
    gteRegs.CP2C.r[r] = v;
}
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
static uint8_t code[BATTLE_STARTUP_BYTES], table[BATTLE_STARTUP_TABLE_BYTES];
static uint8_t display_code[BATTLE_DISPLAY_BYTES];
static uint8_t shared[0x10000], expected_shared[sizeof(shared)], expected_ram[PSX_RAM_SIZE];
static unsigned calls, variant, stop_target, pending_return;
static int candidate, compare, failed, compose_display;

typedef struct Trace {
    uint32_t target, ra, sp, saved[8], args[6];
    int32_t geometry[2];
    uint8_t frame[0x28], shared_hash[32], ram_hash[32];
} Trace;
static Trace traces[16];

static void fail(const char *label, uint32_t detail)
{
    fprintf(stderr, "BATTLE STARTUP FAIL variant=%u call=%u %s 0x%08x\n",
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
    if (stop_target && target - stop_target < 12u)
        return 0;
    if (candidate && (target == BATTLE_STARTUP_ADDRESS ||
                      (compose_display && target == BATTLE_DISPLAY_ADDRESS)))
        return runtime_bridge(opaque, cpu, target);
    if (target - BATTLE_STARTUP_ADDRESS < BATTLE_STARTUP_BYTES ||
        (compose_display && target - BATTLE_DISPLAY_ADDRESS < BATTLE_DISPLAY_BYTES))
        return 0;
    unsigned argc;
    switch (target) {
    case 0x800b8284u: case 0x8001bbacu: case 0x800b7870u:
    case 0x801e8588u: case 0x801e91e8u: case 0x801e9594u:
    case 0x801e893cu: case 0x800a8b0cu: argc = 0; break;
    case 0x80028a60u: argc = 1; break;
    case 0x801e7210u: argc = 6; break;
    case 0x800a5e9cu: argc = 2; break;
    case 0x8004a12cu: argc = 2; break;
    case 0x800439e0u: case 0x80043928u: argc = 5; break;
    default: fail("unexpected callee", target); return -1;
    }
    if (calls >= sizeof(traces) / sizeof(traces[0])) {
        fail("too many calls", target); return -1;
    }
    Trace trace = { .target = target, .ra = cpu->gpr[31], .sp = cpu->gpr[29] };
    trace.geometry[0] = C2_OFX; trace.geometry[1] = C2_OFY;
    memcpy(trace.saved, cpu->gpr + 16, sizeof(trace.saved));
    memcpy(trace.frame, PSX_ADDR(cpu->gpr[29]), sizeof(trace.frame));
    for (unsigned i = 0; i < argc; i++)
        trace.args[i] = i < 4 ? cpu->gpr[4 + i] : read_at(cpu->gpr[29] + 0x10u + 4u * (i - 4u), 4);
    assert(SHA256(shared, sizeof(shared), trace.shared_hash));
    assert(SHA256(PSX_ADDR(0x800c3a6cu), 0x11000, trace.ram_hash));
    if (compare && memcmp(&trace, &traces[calls], sizeof(trace))) {
        fail("call/frame/state differs", target); return -1;
    }
    if (!compare) traces[calls] = trace;
    calls++;
    if (actual_sdk && (target == 0x8004a12cu || target == 0x800439e0u || target == 0x80043928u))
        return runtime_bridge(opaque, cpu, target);
    cpu->gpr[2] = 0xa5000100u + variant + calls;
    cpu->gpr[3] = 0x60000000u + calls;
    cpu->hi = 0x50000000u + calls;
    cpu->lo = 0x70000000u + calls;
    /* Callee writes must be visible at later boundaries through the canonical
     * shared-data mapping, including values that look like pointers. */
    write_at(0x80059520u, 4, 0x80000000u | (variant * 32u + calls));
    write_at(0x80059470u, 4, 0xffff0000u | calls);
    if (target == 0x801e7210u) {
        write_at(cpu->gpr[7], 1, 0x5a);
        write_at(trace.args[4], 1, 0xa5);
        write_at(trace.args[5], 1, 0x42);
    }
    if (target == 0x800439e0u || target == 0x80043928u) {
        /* Model only the SDK memory boundary, with nonzero contents to expose
         * incorrect environment addresses, sizes and later screen stores. */
        unsigned bytes = target == 0x800439e0u ? 0x14u : 0x5cu;
        for (unsigned i = 0; i < bytes; i++)
            write_at(trace.args[0] + i, 1, variant * 7u + calls + i);
    }
    if (target == pending_return) {
        cpu->load_valid = 1; cpu->load_reg = 2;
        cpu->load_value = 0x12345678u; cpu->load_mask = 0;
    }
    return 1;
}
static void setup(PcPortMipsCpu *cpu, uint32_t mode)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0xa5, sizeof(g_PsxRam));
    memset(shared, 0x5a, sizeof(shared));
    memset(&gteRegs, 0x5a, sizeof(gteRegs));
    memcpy(PSX_ADDR(BATTLE_STARTUP_ADDRESS), code, sizeof(code));
    memcpy(PSX_ADDR(BATTLE_STARTUP_TABLE), table, sizeof(table));
    memcpy(PSX_ADDR(BATTLE_DISPLAY_ADDRESS), display_code, sizeof(display_code));
    rt.data[0] = (ResolvedData){0x80050000u, sizeof(shared), shared, "shared"};
    rt.data_count = 1; rt.initialized = 1;
    if (actual_sdk) {
        rt.functions[0] = (ResolvedFunction){0x80043928u, (void *)SetDefDrawEnv, "SetDefDrawEnv"};
        rt.functions[1] = (ResolvedFunction){0x800439e0u, (void *)SetDefDispEnv, "SetDefDispEnv"};
        rt.functions[2] = (ResolvedFunction){0x8004a12cu, (void *)SetGeomOffset, "SetGeomOffset"};
        rt.function_count = 3;
    }
    g_ActiveBattleRuntime = &rt;
    initialize_cpu(cpu, &rt);
    cpu->bus.bridge = model;
    for (unsigned r = 1; r < 29; r++) cpu->gpr[r] = 0x12340000u + variant * 256u + r;
    cpu->gpr[4] = mode;
    if ((variant / 12u) & 1u) cpu->gpr[29] = 0xa0000000u | (cpu->gpr[29] & 0x1fffffffu);
    calls = stop_target = pending_return = 0;
    write_at(0x80059520u, 4, 0xfedcba98u);
    write_at(0x80059470u, 4, 0x87654321u);
}
static int run_case(uint32_t mode, uint32_t entry)
{
    PcPortMipsCpu oracle, native;
    setup(&oracle, mode); candidate = compare = 0;
    if (PcPortMipsRun(&oracle, entry, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED) {
        fail(oracle.error, oracle.pc); return 0;
    }
    unsigned expected_calls = calls;
    assert(oracle.steps > 0);
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    memcpy(expected_shared, shared, sizeof(shared));
    memcpy(&expected_gte, &gteRegs, sizeof(gteRegs));
    setup(&native, mode); candidate = compare = 1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int rc = PcPortMipsRun(&native, entry, BATTLE_HALT_PC, 1000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    if (rc != PC_PORT_MIPS_HALTED) { fail(native.error, native.pc); return 0; }
    if (calls != expected_calls || native.steps || rt.bridge_cpu ||
        PcPortMipsExecutionStatsGet().interpreted_instructions != before.interpreted_instructions)
        fail("calls/context/instruction count", calls);
    for (unsigned r = 16; r < 32; r++)
        if (native.gpr[r] != oracle.gpr[r]) fail("preserved register", r);
    if (native.gpr[2] != oracle.gpr[2] || native.gpr[3] != oracle.gpr[3] ||
        native.hi != oracle.hi || native.lo != oracle.lo) fail("return values", 0);
    if (memcmp(g_PsxRam, expected_ram, sizeof(g_PsxRam)) ||
        memcmp(shared, expected_shared, sizeof(shared))) fail("final memory", 0);
    if (memcmp(&gteRegs, &expected_gte, sizeof(gteRegs))) fail("GTE register state", 0);
    if (actual_sdk) {
        if (C2_OFX != 160 * 65536 || C2_OFY != 164 * 65536)
            fail("actual SDK geometry offset", 0);
        const uint32_t disp[] = {0x800c4a7cu, 0x800c8aecu};
        const uint32_t draw[] = {0x800c4a20u, 0x800c8a90u};
        for (unsigned i = 0; i < 2; i++) {
            if (read_at(disp[i], 2) != 0 || read_at(disp[i] + 2, 2) != (i ? 0u : 224u) ||
                read_at(disp[i] + 4, 2) != 320 || read_at(disp[i] + 6, 2) != 224 ||
                read_at(disp[i] + 8, 2) != 0 || read_at(disp[i] + 10, 2) != 10 ||
                read_at(disp[i] + 12, 2) != 256 || read_at(disp[i] + 14, 2) != 216)
                fail("actual SDK display environment", i);
            if (read_at(draw[i], 2) != 0 || read_at(draw[i] + 2, 2) != (i ? 224u : 0u) ||
                read_at(draw[i] + 4, 2) != 320 || read_at(draw[i] + 6, 2) != 224 ||
                read_at(draw[i] + 8, 2) != 0 || read_at(draw[i] + 10, 2) != (i ? 224u : 0u) ||
                read_at(draw[i] + 0x18, 1) != 0)
                fail("actual SDK draw environment", i);
        }
    }
    return !failed;
}
static int controls(void)
{
    PcPortMipsCpu cpu;
    setup(&cpu, 0); candidate = 1; compare = 0; rt.bridge_cpu = &cpu;
    const uint32_t locations[] = {BATTLE_STARTUP_ADDRESS, BATTLE_STARTUP_ADDRESS + 140,
        BATTLE_STARTUP_ADDRESS + BATTLE_STARTUP_BYTES - 1,
        BATTLE_STARTUP_TABLE, BATTLE_STARTUP_TABLE + 12, BATTLE_STARTUP_TABLE + 23};
    for (unsigned i = 0; i < sizeof(locations) / sizeof(locations[0]); i++) {
        uint8_t *p = PSX_ADDR(locations[i]); *p ^= 1;
        if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS) != 0)
            fail("modified identity accepted", locations[i]);
        *p ^= 1;
    }
    cpu.load_valid = 1; cpu.load_reg = 4;
    if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS)) fail("pending entry load", 0);
    cpu.load_valid = 0; rt.force_interpret = 1;
    if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS)) fail("force interpret", 0);
    rt.force_interpret = 0;
    assert(setenv("XENO_BATTLE_NATIVE_STARTUP", "0", 1) == 0);
    if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS)) fail("reference switch", 0);
    assert(unsetenv("XENO_BATTLE_NATIVE_STARTUP") == 0);
    g_ActiveBattleRuntime = NULL;
    if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS)) fail("inactive runtime", 0);
    g_ActiveBattleRuntime = &rt; rt.bridge_cpu = NULL;
    if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS)) fail("wrong CPU", 0);
    rt.bridge_cpu = &cpu; rt.initialized = 0;
    if (battle_startup_try_host(&rt, &cpu, BATTLE_STARTUP_ADDRESS)) fail("uninitialized runtime", 0);
    if (calls) fail("rejected identity/context had effects", calls);

    setup(&cpu, 0); pending_return = 0x800b8284u;
    if (PcPortMipsRun(&cpu, BATTLE_STARTUP_ADDRESS, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 1)
        fail("pending callee load", calls);
    setup(&cpu, 0); stop_target = 0x800b8284u;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    if (PcPortMipsRun(&cpu, BATTLE_STARTUP_ADDRESS, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_UNRESOLVED_CALL ||
        cpu.steps || PcPortMipsExecutionStatsGet().disabled_refusals != before.disabled_refusals + 1)
        fail("missing callee was not refused", calls);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    return !failed;
}
static int display_controls(void)
{
    PcPortMipsCpu cpu;
    setup(&cpu, 0); candidate = compose_display = 1; compare = 0; rt.bridge_cpu = &cpu;
    const unsigned offsets[] = {0, BATTLE_DISPLAY_BYTES / 2, BATTLE_DISPLAY_BYTES - 1};
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        uint8_t *p = PSX_ADDR(BATTLE_DISPLAY_ADDRESS + offsets[i]); *p ^= 1;
        if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS))
            fail("modified display identity accepted", offsets[i]);
        *p ^= 1;
    }
    cpu.load_valid = 1; cpu.load_reg = 4;
    if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS)) fail("display entry load", 0);
    cpu.load_valid = 0; rt.force_interpret = 1;
    if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS)) fail("display reference hook", 0);
    rt.force_interpret = 0;
    assert(setenv("XENO_BATTLE_NATIVE_DISPLAY", "0", 1) == 0);
    if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS)) fail("display reference switch", 0);
    assert(unsetenv("XENO_BATTLE_NATIVE_DISPLAY") == 0);
    g_ActiveBattleRuntime = NULL;
    if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS)) fail("display inactive runtime", 0);
    g_ActiveBattleRuntime = &rt; rt.bridge_cpu = NULL;
    if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS)) fail("display wrong CPU", 0);
    rt.bridge_cpu = &cpu; rt.initialized = 0;
    if (battle_display_try_host(&rt, &cpu, BATTLE_DISPLAY_ADDRESS)) fail("display uninitialized runtime", 0);
    if (calls) fail("display rejection had effects", calls);

    setup(&cpu, 0); pending_return = 0x8004a12cu;
    if (PcPortMipsRun(&cpu, BATTLE_DISPLAY_ADDRESS, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 1)
        fail("display pending callee load", calls);
    setup(&cpu, 0); stop_target = 0x8004a12cu;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    if (PcPortMipsRun(&cpu, BATTLE_STARTUP_ADDRESS, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_UNRESOLVED_CALL ||
        cpu.steps || PcPortMipsExecutionStatsGet().disabled_refusals != before.disabled_refusals + 1 || rt.bridge_cpu)
        fail("nested display refusal/context", calls);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    return !failed;
}
int main(int argc, char **argv)
{
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image && fseek(image, BATTLE_STARTUP_ADDRESS - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(code, 1, sizeof(code), image) == sizeof(code));
    assert(fseek(image, BATTLE_STARTUP_TABLE - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(table, 1, sizeof(table), image) == sizeof(table));
    assert(fseek(image, BATTLE_DISPLAY_ADDRESS - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(display_code, 1, sizeof(display_code), image) == sizeof(display_code));
    assert(fclose(image) == 0);
    assert(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0);
    if (argc == 2 && strcmp(argv[1], "--sdk") == 0) {
        actual_sdk = compose_display = 1;
        for (variant = 0; variant < 24; variant++)
            if (!run_case(0, BATTLE_DISPLAY_ADDRESS)) return 1;
        puts("BATTLE DISPLAY SDK PASS 24 retail/native comparisons through production bridge and actual SDK initializers; native steps=0");
        return 0;
    }
    const uint32_t modes[] = {0, 1, 2, 3, 4, 5, 6, 255, 256, 0x7fffffffu, 0x80000000u, 0xffffffffu};
    for (variant = 0; variant < 48; variant++)
        if (!run_case(modes[variant % (sizeof(modes) / sizeof(modes[0]))], BATTLE_STARTUP_ADDRESS)) return 1;
    if (!controls()) return 1;
    compose_display = 1;
    for (variant = 0; variant < 48; variant++)
        if (!run_case(modes[variant % (sizeof(modes) / sizeof(modes[0]))], BATTLE_STARTUP_ADDRESS)) return 1;
    for (variant = 0; variant < 24; variant++)
        if (!run_case(0, BATTLE_DISPLAY_ADDRESS)) return 1;
    if (!display_controls()) return 1;
    puts("BATTLE STARTUP PASS 120 retail differential cases: 48 startup, 48 startup+display, 24 display; native steps=0; identity/refusal controls");
    return 0;
}

/* Retail 80078310 versus production native dispatch. The default suite models
 * callees; --sdk uses the real TIM parser, atlas getter and SDK upload/sync
 * wrappers with a recording VRAM backend. Neither is GPU/hardware acceptance. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>

uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
int g_GPUDisabledState, g_splitIndex, g_xeno_swap_index;
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
extern void func_80026338(uint8_t *, int32_t, uint32_t *, int32_t *, int32_t *,
                          int32_t *, int32_t *, int32_t *);

/* The SDK translation unit requires GCC's permissive C++ compilation for old
 * pointer casts. These exact-signature forwarders let Clang's function-type
 * sanitizer check the bridge boundary while executing the real SDK bodies. */
static int sdk_load_image(RECT16 *rect, u_long *data) { return LoadImage(rect, data); }
static int sdk_draw_sync(int mode) { return DrawSync(mode); }

static BattleMipsRuntime rt;
static uint8_t code[BATTLE_TEXTURE_BYTES], shared[0x10000], expected_shared[sizeof(shared)];
static uint8_t expected_ram[PSX_RAM_SIZE];
static unsigned variant, calls, uploads, syncs;
static uint32_t frame_base, opened, stop_target, pending_return;
static int candidate, compare, failed, actual_sdk;
typedef struct Trace {
    uint32_t target, ra, sp, saved[9], args[8], hi, lo;
    uint8_t frame[0xd0], shared_hash[32], ram_hash[32];
} Trace;
typedef struct Upload {
    int x, y, width, height, dest_x, dest_y;
    uint32_t source;
    uint8_t pixels[32];
} Upload;
static Trace traces[20];
static Upload upload_trace[6], expected_uploads[6];

static void fail(const char *label, uint32_t detail)
{
    fprintf(stderr, "BATTLE TEXTURE FAIL variant=%u sdk=%d call=%u %s 0x%08x\n",
            variant, actual_sdk, calls, label, detail);
    failed = 1;
}
void GR_CopyVRAM(unsigned short *src, int x, int y, int w, int h, int dx, int dy)
{
    assert(uploads < 6 && w * h * 2 <= 32 && w > 0 && h > 0);
    Upload *out = &upload_trace[uploads++];
    *out = (Upload){.x=x, .y=y, .width=w, .height=h, .dest_x=dx, .dest_y=dy,
                    .source=PsxMemory_GuestAddr(src)};
    memcpy(out->pixels, src, (unsigned)(w * h * 2));
}
void GR_UpdateVRAM(void) { syncs++; }
static uint32_t read_at(uint32_t a, unsigned width)
{
    uint32_t value;
    assert(runtime_read(&rt, a, width, &value) == 0);
    return value;
}
static void write_at(uint32_t a, unsigned width, uint32_t value)
{
    assert(runtime_write(&rt, a, width, value) == 0);
}
static int model(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (stop_target && target - stop_target < 12u) return 0;
    if (candidate && target == BATTLE_TEXTURE_ADDRESS) return runtime_bridge(opaque, cpu, target);
    if (target - BATTLE_TEXTURE_ADDRESS < BATTLE_TEXTURE_BYTES) return 0;
    unsigned argc;
    switch (target) {
    case 0x800471b4u: case 0x800471c4u: case 0x800445d0u: argc = 1; break;
    case 0x80026338u: argc = 8; break;
    case 0x80044894u: argc = 2; break;
    default: fail("unexpected callee", target); return -1;
    }
    if (calls >= 20) { fail("trace capacity", calls); return -1; }
    Trace trace = {.target=target, .ra=cpu->gpr[31], .sp=cpu->gpr[29], .hi=cpu->hi, .lo=cpu->lo};
    memcpy(trace.saved, cpu->gpr + 16, 8 * sizeof(uint32_t)); trace.saved[8] = cpu->gpr[30];
    memcpy(trace.frame, PSX_ADDR(frame_base), sizeof(trace.frame));
    for (unsigned i = 0; i < argc; i++)
        trace.args[i] = i < 4 ? cpu->gpr[4 + i] : read_at(cpu->gpr[29] + 0x10u + (i - 4u) * 4u, 4);
    assert(SHA256(shared, sizeof(shared), trace.shared_hash));
    assert(SHA256(g_PsxRam, sizeof(g_PsxRam), trace.ram_hash));
    if (compare && memcmp(&trace, &traces[calls], sizeof(trace))) {
        Trace *want = &traces[calls];
        if (trace.target != want->target) fail("callee", want->target);
        if (trace.sp != want->sp || trace.ra != want->ra) fail("SP/RA", trace.ra);
        for (unsigned i = 0; i < 9; i++) if (trace.saved[i] != want->saved[i]) fail("saved register", i);
        for (unsigned i = 0; i < 8; i++) if (trace.args[i] != want->args[i]) fail("argument", i);
        for (unsigned i = 0; i < sizeof(trace.frame); i++)
            if (trace.frame[i] != want->frame[i]) { fail("frame byte", frame_base + i); break; }
        fail("call/memory differs", target); return -1;
    }
    if (!compare) traces[calls] = trace;
    calls++;
    if (actual_sdk) return runtime_bridge(opaque, cpu, target);
    cpu->gpr[2] = 0x91000000u + calls;
    cpu->gpr[3] = 0x62000000u + calls;
    cpu->hi = 0x12340000u + calls; cpu->lo = 0x56780000u + calls;
    write_at(0x80059510u, 4, calls);
    unsigned slot = cpu->gpr[19];
    switch (target) {
    case 0x800471b4u: opened = trace.args[0]; break;
    case 0x800471c4u: {
        uint32_t base = opened < 0x200000u ? opened | 0x80000000u : opened;
        const uint32_t fields[] = {8, base + 12u, base + 20u, base + 56u, base + 64u};
        for (unsigned i = 0; i < 5; i++) write_at(trace.args[0] + i * 4u, 4, fields[i]);
        if (variant & 32u) {
            write_at(0x800d2f5cu, 4, 0xa0161000u);
            write_at(cpu->gpr[29] + 0x88u, 1, 255u - slot);
        }
        break;
    }
    case 0x80026338u:
        for (unsigned i = 0; i < 6; i++)
            write_at(trace.args[i + 2], 4, 0x8badfff0u + i * 7u + slot * 3u);
        break;
    case 0x80044894u:
        uploads++;
        /* The first upload can replace the second upload's pointers. */
        if ((variant & 32u) && trace.ra == 0x8007847cu) {
            write_at(cpu->gpr[29] + 0x2cu, 4, 0xa0170000u + slot * 8u);
            write_at(cpu->gpr[29] + 0x30u, 4, 0xa0170100u + slot * 0x20u);
        }
        break;
    case 0x800445d0u:
        if (trace.args[0]) fail("DrawSync mode", trace.args[0]);
        syncs++;
        if (variant & 32u) {
            write_at(0x800d3294u, 1, read_at(0x800d3294u, 1) ^ 1u);
            if (slot < 2) write_at(0x800d2d25u + slot, 1, 0xffu - slot);
            write_at(cpu->gpr[29] + 0x90u, 4, 0xfffffffeu + slot);
            write_at(cpu->gpr[29] + 0x80u, 4, 0xa0100400u);
        }
        break;
    }
    if (target == pending_return) {
        cpu->load_valid = 1; cpu->load_reg = 2; cpu->load_value = 0x87654321u; cpu->load_mask = 0;
    }
    return failed ? -1 : 1;
}
static void setup(PcPortMipsCpu *cpu)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0xa5, sizeof(g_PsxRam));
    memset(shared, 0x5a, sizeof(shared));
    memset(upload_trace, 0, sizeof(upload_trace));
    memcpy(PSX_ADDR(BATTLE_TEXTURE_ADDRESS), code, sizeof(code));
    rt.data[0] = (ResolvedData){0x80050000u, sizeof(shared), shared, "shared"};
    rt.data_count = 1; rt.initialized = 1; g_ActiveBattleRuntime = &rt;
    if (actual_sdk) {
        const ResolvedFunction funcs[] = {
            {0x80026338u, (void *)func_80026338, "func_80026338"},
            {0x800445d0u, (void *)sdk_draw_sync, "DrawSync"},
            {0x80044894u, (void *)sdk_load_image, "LoadImage"},
            {0x800471b4u, (void *)OpenTIM, "OpenTIM"},
            {0x800471c4u, (void *)ReadTIM, "ReadTIM"},
        };
        memcpy(rt.functions, funcs, sizeof(funcs)); rt.function_count = sizeof(funcs) / sizeof(funcs[0]);
    }
    initialize_cpu(cpu, &rt); cpu->bus.bridge = model;
    for (unsigned r = 1; r < 31; r++) if (r != 29) cpu->gpr[r] = 0x12340000u + variant * 256u + r;
    if (variant & 16u) cpu->gpr[29] |= 0x20000000u;
    cpu->gpr[4] = variant & 16u ? 0x100000u : 0x80100000u;
    cpu->gpr[5] = 0xfedc00fdu + (variant % 3u);
    cpu->hi = 0x87654321u; cpu->lo = 0xabcdef01u;
    frame_base = cpu->gpr[29] - 0xc8u;
    calls = uploads = syncs = opened = stop_target = pending_return = 0;
    for (unsigned slot = 0; slot < 3; slot++)
        write_at(0x800d2d24u + slot, 1, variant & (1u << slot) ? slot * 5u : 0x7f);
    write_at(0x800d3294u, 1, variant & 8u ? 0x80 : 0);
    write_at(0x800d2f5cu, 4, variant & 16u ? 0x160000u : 0x80160000u);
    for (unsigned index = 0; index < 258; index++) {
        uint32_t record = 0x80160400u + index * 0x20u;
        write_at(0x80160004u + index * 2u, 2, record - 0x80160000u);
        write_at(record, 2, 0xfffbu);
        write_at(record + 4u, 2, (variant & 8u) ? 0xffc0u : 0x40u);
        write_at(record + 6u, 2, 3);
        write_at(record + 0x14u, 2, variant & 1u);
        write_at(record + 0x16u, 2, 0x300u + (index % 3u) * 16u);
        write_at(record + 0x18u, 2, 0x1e0u);
        write_at(record + 0x1au, 2, 0x100);
        write_at(record + 0x1cu, 2, 0x100);
    }
    for (unsigned kind = 0; kind < 12; kind++) {
        uint32_t base = 0x80100000u + kind * 0x460u;
        write_at(base, 4, 0x10); write_at(base + 4u, 4, 8);
        write_at(base + 8u, 4, 44); write_at(base + 12u, 4, 0);
        write_at(base + 16u, 4, 0x00010010);
        write_at(base + 52u, 4, 28); write_at(base + 56u, 4, 0);
        write_at(base + 60u, 4, 0x00020004);
        for (unsigned i = 0; i < 16; i++) write_at(base + 20u + i * 2u, 2, kind * 100u + i);
        for (unsigned i = 0; i < 8; i++) write_at(base + 64u + i * 2u, 2, kind * 200u + i);
    }
}
static int run_case(void)
{
    PcPortMipsCpu oracle, native;
    setup(&oracle); candidate = compare = 0;
    if (PcPortMipsRun(&oracle, BATTLE_TEXTURE_ADDRESS, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_HALTED) {
        fail(oracle.error, oracle.pc); return 0;
    }
    if (!oracle.steps) fail("oracle did not execute", 0);
    unsigned expected_calls = calls, wanted_uploads = uploads, wanted_syncs = syncs;
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    memcpy(expected_shared, shared, sizeof(shared));
    memcpy(expected_uploads, upload_trace, sizeof(upload_trace));
    setup(&native); candidate = compare = 1;
    assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int rc = PcPortMipsRun(&native, BATTLE_TEXTURE_ADDRESS, BATTLE_HALT_PC, 10000);
    assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    if (rc != PC_PORT_MIPS_HALTED) { fail(native.error, native.pc); return 0; }
    if (calls != expected_calls || uploads != wanted_uploads || syncs != wanted_syncs || native.steps ||
        rt.bridge_cpu || PcPortMipsExecutionStatsGet().interpreted_instructions != before.interpreted_instructions)
        fail("calls/context/instructions", calls);
    for (unsigned r = 16; r < 32; r++) if (native.gpr[r] != oracle.gpr[r]) fail("preserved register", r);
    if (native.gpr[2] != oracle.gpr[2] || native.gpr[3] != oracle.gpr[3] ||
        native.hi != oracle.hi || native.lo != oracle.lo) fail("return boundary", native.gpr[3]);
    if (memcmp(g_PsxRam, expected_ram, sizeof(g_PsxRam)) || memcmp(shared, expected_shared, sizeof(shared)) ||
        memcmp(upload_trace, expected_uploads, sizeof(upload_trace))) fail("final memory/upload", 0);
    return !failed;
}
static int controls(void)
{
    PcPortMipsCpu cpu;
    variant = 7; candidate = 1; compare = actual_sdk = 0;
    setup(&cpu); rt.bridge_cpu = &cpu;
    memcpy(expected_ram, g_PsxRam, sizeof(expected_ram));
    unsigned offsets[] = {0, BATTLE_TEXTURE_BYTES / 2, BATTLE_TEXTURE_BYTES - 1};
    for (unsigned i = 0; i < 3; i++) {
        uint8_t *p = PSX_ADDR(BATTLE_TEXTURE_ADDRESS + offsets[i]); *p ^= 1;
        if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) { fail("identity", offsets[i]); return 0; }
        *p ^= 1;
    }
    cpu.load_valid = 1; cpu.load_reg = 2;
    if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) fail("pending entry load", 0);
    cpu.load_valid = 0; rt.force_interpret = 1;
    if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) fail("reference hook", 0);
    rt.force_interpret = 0;
    assert(setenv("XENO_BATTLE_NATIVE_TEXTURE", "0", 1) == 0);
    if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) fail("reference switch", 0);
    assert(unsetenv("XENO_BATTLE_NATIVE_TEXTURE") == 0);
    g_ActiveBattleRuntime = NULL;
    if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) fail("inactive runtime", 0);
    g_ActiveBattleRuntime = &rt; rt.bridge_cpu = NULL;
    if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) fail("wrong CPU", 0);
    rt.bridge_cpu = &cpu; rt.initialized = 0;
    if (battle_texture_try_host(&rt, &cpu, BATTLE_TEXTURE_ADDRESS)) fail("uninitialized", 0);
    rt.initialized = 1;
    if (calls || memcmp(g_PsxRam, expected_ram, sizeof(expected_ram))) fail("rejection effects", 0);
    setup(&cpu); pending_return = 0x800471c4u;
    if (PcPortMipsRun(&cpu, BATTLE_TEXTURE_ADDRESS, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_UNRESOLVED_CALL || calls != 2)
        fail("pending callee load", calls);
    const uint32_t stops[] = {0x800471b4u, 0x800471c4u, 0x80026338u, 0x80044894u, 0x800445d0u};
    for (unsigned i = 0; i < 5; i++) {
        setup(&cpu); stop_target = stops[i];
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0);
        PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
        if (PcPortMipsRun(&cpu, BATTLE_TEXTURE_ADDRESS, BATTLE_HALT_PC, 10000) != PC_PORT_MIPS_UNRESOLVED_CALL ||
            cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().disabled_refusals != before.disabled_refusals + 1)
            fail("missing callee/context", i);
        if (i >= 3 && read_at(0x8010000cu, 2) != 0xfffeu) fail("retained rectangle write", i);
        assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0);
    }
    return !failed;
}
int main(int argc, char **argv)
{
    assert(setvbuf(stdout, NULL, _IOLBF, 0) == 0);
    actual_sdk = argc == 2 && strcmp(argv[1], "--sdk") == 0;
    assert(argc == 1 || actual_sdk);
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image && fseek(image, BATTLE_TEXTURE_ADDRESS - BATTLE_BASE, SEEK_SET) == 0);
    assert(fread(code, 1, sizeof(code), image) == sizeof(code) && fclose(image) == 0);
    assert(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0);
    unsigned count = actual_sdk ? 32 : 64;
    for (variant = 0; variant < count; variant++) if (!run_case()) return 1;
    if (!actual_sdk && !controls()) return 1;
    printf("BATTLE TEXTURE %s PASS %u cases; guest frames, eight arguments, memory and return boundaries; native steps=0\n",
           actual_sdk ? "SDK" : "MODEL", count);
    return 0;
}

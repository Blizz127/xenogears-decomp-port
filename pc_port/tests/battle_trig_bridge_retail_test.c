/* Dynamic trig bridge regression. Production initialize_runtime resolves the
 * real exported PsyCross functions with dlsym; no function map is hand-filled.
 * The oracle executes the pinned retail SLUS instructions and 4096-entry table.
 *
 * Separate controls first compare the conventional PsyCross trig table against
 * retail across a complete cycle, so a bridge failure cannot be attributed to
 * an assumed table difference. The bridge then receives raw 32-bit angles,
 * including values which resemble guest pointers. Only the defined scalar
 * result v0 is compared, not scratch/caller-saved registers or rendering.
 */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* Deliberately non-page-aligned backing addresses (fixed by the runner) expose
 * accidental scalar-to-pointer translation even when its low bits would happen
 * to agree in a normally page-aligned executable. No production layout changes. */
uint8_t g_PsxRam[PSX_RAM_SIZE] __attribute__((section(".trig_psx_ram"), aligned(16)));
uint8_t g_PsxScratchpad[4096] __attribute__((section(".trig_scratch"), aligned(16)));
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames; /* printf-only OT diag counter (runtime) */
void PcPort_PadVblankPump(void) {}

char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *packet, int single)
{ (void)packet; (void)single; abort(); }

static const uint32_t entries[] = {0x8003f8b0u, 0x8003f8ccu};
static const char *const retail_names[] = {"rcos", "rsin"};
static BattleMipsRuntime initialized_runtime;
static BattleMipsRuntime retail_runtime;
static unsigned long tested, mismatches, by_entry[2], printed;

static uint32_t retail_result(unsigned leaf, uint32_t angle)
{
    PcPortMipsCpu cpu;
    initialize_cpu(&cpu, &retail_runtime);
    cpu.bus.bridge = NULL;
    cpu.gpr[4] = angle;
    int status = PcPortMipsRun(&cpu, entries[leaf], BATTLE_HALT_PC, 64);
    if (status != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE TRIG BRIDGE FAIL retail entry=%08x angle=%08x: %s\n",
                entries[leaf], angle, cpu.error);
        exit(2);
    }
    return cpu.gpr[2];
}

static void compare_angle(uint32_t angle, const char *range)
{
    for (unsigned leaf = 0; leaf < 2; ++leaf) {
        uint32_t expected = retail_result(leaf, angle);
        PcPortMipsCpu cpu;
        initialize_cpu(&cpu, &initialized_runtime);
        cpu.gpr[4] = angle;
        int status = PcPortMipsRun(&cpu, entries[leaf], BATTLE_HALT_PC, 64);
        ++tested;
        if (status != PC_PORT_MIPS_HALTED || cpu.gpr[2] != expected) {
            ++mismatches;
            ++by_entry[leaf];
            /* Keep complete counts while retaining a few legible examples. */
            if (printed < 8 || (angle & 0xfff) == 0) {
                fprintf(stderr, "BATTLE TRIG BRIDGE FAIL range=%s entry=%08x symbol=%s "
                        "angle=%08x native=%d retail=%d status=%d\n", range,
                        entries[leaf], retail_names[leaf], angle,
                        (int32_t)cpu.gpr[2], (int32_t)expected, status);
                ++printed;
            }
        }
        if (initialized_runtime.bridge_cpu != NULL) {
            fputs("BATTLE TRIG BRIDGE FAIL active bridge CPU was not restored\n", stderr);
            exit(2);
        }
    }
}

static void read_slice(FILE *image, uint32_t address, unsigned bytes)
{
    assert(!fseek(image, address - 0x8000f800u, SEEK_SET));
    assert(fread(PSX_ADDR(address), 1, bytes, image) == bytes);
}

int main(void)
{
    assert(((uintptr_t)g_PsxRam & 0xfff) == 0x20);
    assert(((uintptr_t)g_PsxScratchpad & 0xfff) == 0x40);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image);
    read_slice(image, 0x8003f8b0u, 0x38);
    read_slice(image, 0x800523f0u, 0x4000);
    assert(!fclose(image));

    /* String lookup intentionally bypasses the game's source-level shim.
     * These are the same conventional symbols visible to production dlsym. */
    int (*host_sine)(int) = (int (*)(int))dlsym(RTLD_DEFAULT, "rsin");
    int (*host_cosine)(int) = (int (*)(int))dlsym(RTLD_DEFAULT, "rcos");
    if (!host_sine || !host_cosine) {
        fputs("BATTLE TRIG BRIDGE FAIL actual PsyCross trig exports missing\n", stderr);
        return 2;
    }
    unsigned table_mismatches = 0;
    for (unsigned angle = 0; angle < 4096; ++angle) {
        int actual[2] = {host_sine((int)angle), host_cosine((int)angle)};
        for (unsigned leaf = 0; leaf < 2; ++leaf) {
            int32_t expected = (int32_t)retail_result(leaf, angle);
            if (actual[leaf] != expected) {
                if (table_mismatches++ < 8)
                    fprintf(stderr, "BATTLE TRIG TABLE FAIL entry=%08x angle=%u native=%d retail=%d\n",
                            entries[leaf], angle, actual[leaf], expected);
            }
        }
    }
    if (table_mismatches) return 2;
    puts("BATTLE TRIG TABLE PASS 8192 comparisons: real PsyCross sine/cosine exports exactly match the retail lookup table");
    fflush(stdout);

    initialize_runtime(&initialized_runtime);
    if (!initialized_runtime.initialized ||
        !find_function(&initialized_runtime, entries[0]) ||
        !find_function(&initialized_runtime, entries[1])) {
        fputs("BATTLE TRIG BRIDGE FAIL real initialization did not resolve both retail trig entries\n", stderr);
        return 2;
    }
    /* Each page exhausts all 4096 low-angle values. Signed pages and physical,
     * KSEG0/KSEG1/scratch-like patterns exercise masking before pointer logic. */
    static const struct { uint32_t prefix; const char *name; } pages[] = {
        {0x00000000u, "cycle"}, {0x00001000u, "positive-wrap"},
        {0xfffff000u, "negative-cycle"}, {0x7ffff000u, "int-max"},
        {0x80000000u, "int-min-kseg0"}, {0x80001000u, "kseg0"},
        {0x807ff000u, "kseg0-end"}, {0x80800000u, "after-kseg0"},
        {0xa0000000u, "kseg1"}, {0xa0001000u, "kseg1-wrap"},
        {0xa07ff000u, "kseg1-end"}, {0xa0800000u, "after-kseg1"},
        {0x1f800000u, "scratch"}, {0x1f801000u, "after-scratch"},
        {0x00800000u, "large-positive"}, {0xff800000u, "large-negative"},
    };
    for (unsigned page = 0; page < sizeof(pages) / sizeof(pages[0]); ++page)
        for (uint32_t angle = 0; angle < 4096; ++angle)
            compare_angle(pages[page].prefix | angle, pages[page].name);
    uint32_t seed = 0x9e3779b9u;
    for (unsigned i = 0; i < 4096; ++i) {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        compare_angle(seed, "deterministic-32-bit-sample");
    }
    if (mismatches) {
        fprintf(stderr, "BATTLE TRIG BRIDGE FAIL %lu/%lu comparisons; rcos-entry=%lu rsin-entry=%lu; "
                "real initialization, dispatch, full cycles and raw 32-bit scalar inputs\n",
                mismatches, tested, by_entry[0], by_entry[1]);
        return 1;
    }
    printf("BATTLE TRIG BRIDGE PASS %lu comparisons: real dlsym initialization/dispatch, "
           "16 full angle cycles including signed and pointer-like ranges, 4096 deterministic 32-bit samples\n", tested);
    return 0;
}

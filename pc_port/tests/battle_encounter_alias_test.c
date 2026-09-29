/* Regression for the retail battle encounter record at 8006F9DC..FB.
 *
 * Retail func_80070F40 copies one 0x20-byte D_800658DC record to guest
 * 8006F9DC.  Native func_8001BB0C then consumes the byte at guest +2.  The
 * native port must therefore consume that byte from the same guest-RAM
 * record.  The standalone generated D_8006F9DE symbol is poisoned so it
 * cannot accidentally substitute for the retail guest storage.
 */
#include "../src/battle_mips_runtime.c"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames; /* printf-only OT diag counter (runtime) */
void PcPort_PadVblankPump(void) {}

uint8_t D_80059508;
uint8_t D_800658DC[0x200];
uint8_t *D_80059470;
uint8_t *D_8005949C;
uint8_t *D_80059520;

char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *packet, int single)
{ (void)packet; (void)single; abort(); }
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
void CTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }

extern uint8_t D_8006F9DE;

static uint8_t observed_index;
static unsigned loader_calls;

int func_800379D8(int index, int variant, uint8_t **first, uint8_t **second,
                 uint8_t **third)
{
    observed_index = (uint8_t)index;
    loader_calls++;
    if (variant != 0 || first != &D_80059470 || second != &D_80059520 ||
        third != &D_8005949C)
        abort();
    return 0x12345678;
}

extern int func_8001BB0C(void);

static void read_retail(FILE *image, uint32_t address, unsigned size)
{
    assert(address >= BATTLE_BASE);
    assert(fseek(image, (long)(address - BATTLE_BASE), SEEK_SET) == 0);
    assert(fread(PSX_ADDR(address), 1, size, image) == size);
}

static int execute_retail_copy(BattleMipsRuntime *runtime, uint8_t selector)
{
    PcPortMipsCpu cpu;
    D_80059508 = selector;
    initialize_cpu(&cpu, runtime);
    cpu.gpr[29] = 0x801ff000u;
    /* Stop immediately after the retail memmove call returns. */
    int status = PcPortMipsRun(&cpu, 0x80071130u, 0x80071158u, 64);
    if (status != PC_PORT_MIPS_HALTED)
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS retail status=%d error=%s pc=%08x\n",
                status, cpu.error, cpu.pc);
    return status;
}

static int call_native_consumer(BattleMipsRuntime *runtime)
{
    PcPortMipsCpu cpu;
    initialize_cpu(&cpu, runtime);
    cpu.gpr[29] = 0x801ff000u;
    loader_calls = 0;
    observed_index = 0;
    if (runtime_bridge_call(runtime, &cpu, 0x8001bb0cu) != 1 ||
        cpu.gpr[2] != 0x12345678u || loader_calls != 1)
        return 0;
    return 1;
}

static unsigned check_selector(BattleMipsRuntime *runtime, uint8_t selector)
{
    uint8_t expected[0x20];
    unsigned failures = 0;
    for (unsigned i = 0; i < sizeof(expected); ++i) {
        expected[i] = (uint8_t)(selector * 29u + i * 7u + 3u);
        D_800658DC[(unsigned)selector * 0x20u + i] = expected[i];
    }
    memset(PSX_ADDR(0x8006f9dcu), 0xa5, sizeof(expected));
    if (execute_retail_copy(runtime, selector) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL selector=%u retail copy did not halt\n",
                selector);
        return 1;
    }
    for (unsigned i = 0; i < sizeof(expected); ++i) {
        uint32_t k0 = 0, k1 = 0;
        if (runtime_read(runtime, 0x8006f9dcu + i, 1, &k0) != 0 ||
            runtime_read(runtime, 0xa006f9dcu + i, 1, &k1) != 0 ||
            k0 != expected[i] || k1 != expected[i]) {
            fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL selector=%u byte=%u k0=%02x k1=%02x expected=%02x\n",
                    selector, i, k0, k1, expected[i]);
            failures++;
        }
    }
    static const struct { uint8_t offset, width; } spans[] = {
        {0, 1}, {0, 2}, {0, 4}, {1, 1}, {1, 2}, {1, 4},
        {2, 1}, {2, 2}, {2, 4}, {28, 4}, {30, 2}, {31, 1},
    };
    for (unsigned i = 0; i < sizeof(spans) / sizeof(spans[0]); ++i) {
        uint32_t k0 = 0, k1 = 0;
        uint32_t want = load_le(expected + spans[i].offset, spans[i].width);
        if (runtime_read(runtime, 0x8006f9dcu + spans[i].offset,
                         spans[i].width, &k0) != 0 ||
            runtime_read(runtime, 0xa006f9dcu + spans[i].offset,
                         spans[i].width, &k1) != 0 || k0 != want || k1 != want) {
            fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL selector=%u offset=%u width=%u k0=%08x k1=%08x expected=%08x\n",
                    selector, spans[i].offset, spans[i].width, k0, k1, want);
            failures++;
        }
    }
    if (!call_native_consumer(runtime) || observed_index != expected[2]) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL selector=%u native-index=%02x expected=%02x\n",
                selector, observed_index, expected[2]);
        failures++;
    }
    return failures;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image != NULL);
    read_retail(image, 0x80071130u, 0x30u);
    assert(fclose(image) == 0);

    BattleMipsRuntime runtime = {0};
    initialize_runtime(&runtime);
    if (!runtime.initialized || !find_function(&runtime, 0x8001bb0cu) ||
        !find_function(&runtime, 0x8003f99cu)) {
        fputs("BATTLE ENCOUNTER ALIAS FAIL production initialization missed func_8001BB0C or memmove\n",
              stderr);
        return 2;
    }

    unsigned failures = 0;
    /* D_800658DC contains sixteen retail-sized records.  Boundaries plus the
     * observed Map 2 selector exercise representative legal indices. */
    static const uint8_t selectors[] = {0, 1, 7, 15};
    for (unsigned i = 0; i < sizeof(selectors); ++i)
        failures += check_selector(&runtime, selectors[i]);

    /* Actual Map 2 selector-1 record, decompressed from the pinned disc map. */
    FILE *record = fopen(argv[1], "rb");
    assert(record != NULL);
    assert(fread(D_800658DC + 0x20u, 1, 0x20u, record) == 0x20u);
    assert(fgetc(record) == EOF);
    assert(fclose(record) == 0);
    assert(D_800658DC[0x22u] == 0x12u);
    if (execute_retail_copy(&runtime, 1) != PC_PORT_MIPS_HALTED ||
        !call_native_consumer(&runtime) || observed_index != 0x12u) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL map2 selector=1 native-index=%02x expected=12\n",
                observed_index);
        failures++;
    }

    if (failures) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS RED failures=%u: guest record writes and native +2 read are not coherent\n",
                failures);
        return 1;
    }
    puts("BATTLE ENCOUNTER ALIAS PASS retail 32-byte copies and native +2 reads are coherent through KSEG0/KSEG1 aliases");
    return 0;
}

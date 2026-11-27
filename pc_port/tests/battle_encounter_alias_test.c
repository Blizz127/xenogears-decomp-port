/* Regression for the retail battle encounter record at 8006F9DC..FB.
 *
 * Retail func_80070F40 copies one 0x20-byte D_800658DC record to guest
 * 8006F9DC.  Native func_8001BB0C then consumes the byte at guest +2.  The
 * native port must therefore consume that byte from the same guest-RAM
 * record.  The standalone generated D_8006F9DE symbol is poisoned so it
 * cannot accidentally substitute for the retail guest storage.
 */
#include "../src/battle_mips_runtime.c"
#include <assert.h>

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

/* Compare the field-selected part of the cold entry up to the return from
 * memmove. The reference executes the owner's overlay with the same real
 * typed memmove as the production bridge. */
static int check_formation_copy(BattleMipsRuntime *runtime, uint8_t selector)
{
    const uint32_t entry = 0x80070fc8u, resume = 0x80071158u;
    const uint32_t sentinel = 0x801a0400u, active = 0x8006f9dcu;
    static const struct { uint32_t address; unsigned size; } outputs[] = {
        {0x8005959cu, 1}, {0x800c3e29u, 1}, {0x800c3e28u, 1},
        {0x800d366cu, 1}, {0x800c3e54u, 4}
    };
    const size_t count = sizeof(outputs) / sizeof(outputs[0]);
    uint32_t observed[sizeof(outputs) / sizeof(outputs[0])];
    PcPortMipsCpu reference;
    uint8_t record[32];
    for (unsigned native = 0; native <= 1; ++native) {
        PcPortMipsCpu cpu;
        initialize_battle_entry_cpu(&cpu, runtime);
        for (unsigned i = 1; i < 28; ++i)
            cpu.gpr[i] = 0x12340000u + i; /* detect clobbered registers */
        cpu.gpr[30] = 0x1234001eu;
        cpu.gpr[19] = 1;
        cpu.gpr[29] -= 48;
        cpu.gpr[31] = entry;
        cpu.hi = 0x9876u;
        cpu.lo = 0x1357u;
        D_80059508 = selector;
        assert(runtime_write(runtime, 0x8005947cu, 1, 0) == 0);
        assert(runtime_write(runtime, 0x800594f8u, 1, 0) == 0);
        assert(runtime_write(runtime, 0x80062528u, 4, 0x54001234u) == 0);
        assert(runtime_write(runtime, 0x8005917cu, 4, sentinel) == 0);
        assert(runtime_write(runtime, sentinel, 4, UINT32_MAX) == 0);
        for (size_t i = 0; i < count; ++i)
            assert(runtime_write(runtime, outputs[i].address, outputs[i].size, 0x75757575u) == 0);
        memset(PSX_ADDR(active - 1), 0xa5, sizeof(record) + 2);
        runtime->force_interpret = !native;
        int status = PcPortMipsRun(&cpu, entry, resume, 128);
        runtime->force_interpret = 0;
        if (status != PC_PORT_MIPS_HALTED || (cpu.steps == 0) != native ||
            cpu.pc != resume || cpu.next_pc != resume + 4 || cpu.load_valid ||
            *(uint8_t *)PSX_ADDR(active - 1) != 0xa5 ||
            *(uint8_t *)PSX_ADDR(active + sizeof(record)) != 0xa5) {
            fprintf(stderr, "BATTLE ENCOUNTER ALIAS copy selector=%u native=%u pc=%08x steps=%llu error=%s\n",
                    selector, native, cpu.pc, (unsigned long long)cpu.steps, cpu.error);
            return 0;
        }
        for (size_t i = 0; i < count; ++i) {
            uint32_t value;
            assert(runtime_read(runtime, outputs[i].address, outputs[i].size, &value) == 0);
            if (!native)
                observed[i] = value;
            else if (value != observed[i])
                return 0;
        }
        if (!native) {
            reference = cpu;
            memcpy(record, PSX_ADDR(active), sizeof(record));
        } else if (memcmp(cpu.gpr, reference.gpr, sizeof(cpu.gpr)) ||
                   cpu.hi != reference.hi || cpu.lo != reference.lo ||
                   memcmp(cpu.cp0, reference.cp0, sizeof(cpu.cp0)) ||
                   memcmp(record, PSX_ADDR(active), sizeof(record))) {
            return 0;
        }
    }
    return 1;
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
    if (!check_formation_copy(runtime, selector)) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL selector=%u formation state differs\n",
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
    read_retail(image, 0x80070fc8u, 0x80071158u - 0x80070fc8u);
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
     * observed Map 2 selector and Mountain Path slot and its neighbors are covered. */
    static const uint8_t selectors[] = {0, 1, 5, 6, 7, 15};
    for (unsigned i = 0; i < sizeof(selectors); ++i)
        failures += check_selector(&runtime, selectors[i]);

    /* Actual Map 2 selector-1 record, decompressed from the pinned disc map. */
    FILE *record = fopen(argv[1], "rb");
    assert(record != NULL);
    assert(fread(D_800658DC + 0x20u, 1, 0x20u, record) == 0x20u);
    assert(fgetc(record) == EOF);
    assert(fclose(record) == 0);
    assert(D_800658DC[0x22u] == 0x12u);
    if (!check_formation_copy(&runtime, 1) ||
        !call_native_consumer(&runtime) || observed_index != 0x12u) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS FAIL map2 selector=1 native-index=%02x expected=12\n",
                observed_index);
        failures++;
    }

    /* The table can also live entirely in guest RAM. Remove only its test
     * binding and place the owner's selected record in that domain. A stray
     * direct read of the native symbol then sees deliberately different data. */
    ResolvedData *table = find_data(&runtime, 0x800658dcu, sizeof(D_800658DC));
    assert(table != NULL && table->host == D_800658DC);
    uint32_t table_size = table->size;
    table->size = 0;
    uint8_t *guest_row = PSX_ADDR(0x800658dcu + 0x20u);
    assert(module_memory_pointer(&runtime, 0x800658dcu + 0x20u, 32, 0) == guest_row);
    memcpy(guest_row, D_800658DC + 0x20u, 32);
    memset(D_800658DC + 0x20u, 0xe7, 32);
    if (!check_formation_copy(&runtime, 1) ||
        memcmp(PSX_ADDR(0x8006f9dcu), guest_row, 32) ||
        !call_native_consumer(&runtime) || observed_index != guest_row[2]) {
        fputs("BATTLE ENCOUNTER ALIAS FAIL guest-backed Map 2 record\n", stderr);
        failures++;
    }
    table->size = table_size;

    if (failures) {
        fprintf(stderr, "BATTLE ENCOUNTER ALIAS RED failures=%u: guest record writes and native +2 read are not coherent\n",
                failures);
        return 1;
    }
    puts("BATTLE ENCOUNTER ALIAS PASS owner and native formation states agree; 32-byte copies and native +2 reads are coherent through KSEG0/KSEG1 aliases");
    return 0;
}

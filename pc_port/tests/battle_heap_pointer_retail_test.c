/* Return-pointer boundary regression. The production runtime initializes its
 * map via real dlsym and executes the original battle reservation fragment and
 * allocation helper. Only HeapAlloc and HeapChangeCurrentUser are controlled.
 * The oracle uses the same retail instructions with guest-returning allocator
 * boundaries; the native boundary returns pointers into real g_PsxRam.
 *
 * This proves allocation requests, returned guest pointers and the two stored
 * reservation records. It does not prove allocator policy or battle rendering.
 */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* Match the observed run's backing base, making the original 6480104-byte
 * request reproducible. Other guest addresses exercise the general contract. */
uint8_t g_PsxRam[PSX_RAM_SIZE] __attribute__((section(".heap_test_ram"), aligned(16)));
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames; /* printf-only OT diag counter (runtime) */
void PcPort_PadVblankPump(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *packet, int single)
{ (void)packet; (void)single; abort(); }

static BattleMipsRuntime candidate_runtime, oracle_runtime;
static uint32_t allocation_plan[2];
static struct Allocation { uint32_t size, flags, raw_size; } calls[2], expected_calls[2];
static unsigned call_count, user_calls, printed;
static unsigned free_calls;
static uintptr_t freed_pointer;
#define FREE_SCALAR_RESULT 0x00623450u

void HeapChangeCurrentUser(uint32_t user, void *names)
{
    assert(user == 2 && names == NULL);
    ++user_calls;
}

void *HeapAlloc(uint32_t size, uint32_t flags)
{
    assert(candidate_runtime.bridge_cpu && call_count < 2);
    unsigned index = call_count++;
    calls[index] = (struct Allocation){size, flags, candidate_runtime.bridge_cpu->gpr[4]};
    return allocation_plan[index] ? PSX_ADDR(allocation_plan[index]) : NULL;
}

uint32_t HeapFree(void *pointer)
{
    ++free_calls;
    freed_pointer = (uintptr_t)pointer;
    /* A scalar deliberately numerically inside the RAM backing range must
     * retain its value. Normalizing every native return would corrupt it. */
    return FREE_SCALAR_RESULT;
}

static int oracle_bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    if (target == 0x80032498u) {
        HeapChangeCurrentUser(cpu->gpr[4], (void *)(uintptr_t)cpu->gpr[5]);
        return 1;
    }
    if (target == 0x80031bdcu) {
        assert(call_count < 2);
        unsigned index = call_count++;
        calls[index] = (struct Allocation){cpu->gpr[4], cpu->gpr[5], cpu->gpr[4]};
        cpu->gpr[2] = allocation_plan[index];
        return 1;
    }
    return target >= 0x8006faf0u && target < 0x800c3a6cu ? 0 : -1;
}

static void reset_calls(void)
{
    call_count = user_calls = 0;
    memset(calls, 0, sizeof(calls));
}

static void run_cpu(PcPortMipsCpu *cpu, int native, uint32_t entry, uint32_t halt)
{
    initialize_cpu(cpu, native ? &candidate_runtime : &oracle_runtime);
    if (!native) cpu->bus.bridge = oracle_bridge;
    int result = PcPortMipsRun(cpu, entry, halt, 1000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE HEAP POINTER FAIL execution native=%d entry=%08x pc=%08x: %s\n",
                native, entry, cpu->pc, cpu->error);
        exit(2);
    }
}

static int compare_fragment(uint32_t first, uint32_t second)
{
    enum { RECORD_BYTES = 0x30 };
    const uint32_t records = 0x800d3278u;
    uint8_t expected_records[RECORD_BYTES];
    PcPortMipsCpu cpu;
    allocation_plan[0] = first; allocation_plan[1] = second;
    reset_calls();
    memset(PSX_ADDR(records), 0xa5, RECORD_BYTES);
    run_cpu(&cpu, 0, 0x80070e48u, 0x80070e88u);
    assert(call_count == 2 && user_calls == 2);
    assert(calls[0].size == 4 && calls[0].flags == 1);
    assert(calls[1].size == first - 0x801e5000u && calls[1].flags == 1);
    assert(cpu.gpr[2] == second);
    assert(load_le(PSX_ADDR(0x800d3284u), 4) == first);
    assert(load_le(PSX_ADDR(0x800d328cu), 4) == second);
    memcpy(expected_calls, calls, sizeof(calls));
    memcpy(expected_records, PSX_ADDR(records), RECORD_BYTES);
    uint32_t expected_result = cpu.gpr[2];

    reset_calls();
    memset(PSX_ADDR(records), 0xa5, RECORD_BYTES);
    run_cpu(&cpu, 1, 0x80070e48u, 0x80070e88u);
    if (call_count != 2 || user_calls != 2 ||
        memcmp(calls, expected_calls, sizeof(calls)) ||
        memcmp(PSX_ADDR(records), expected_records, RECORD_BYTES) ||
        cpu.gpr[2] != expected_result || candidate_runtime.bridge_cpu != NULL) {
        if (printed++ < 8)
            fprintf(stderr, "BATTLE HEAP POINTER FAIL reservation first=%08x second=%08x "
                    "second_request=%u/%u raw=%08x/%08x stored=%08x,%08x expected=%08x,%08x result=%08x/%08x\n",
                    first, second, calls[1].size, expected_calls[1].size,
                    calls[1].raw_size, expected_calls[1].raw_size,
                    load_le(PSX_ADDR(0x800d3284u), 4), load_le(PSX_ADDR(0x800d328cu), 4),
                    first, second, cpu.gpr[2], expected_result);
        return 1;
    }
    return 0;
}

static int compare_direct(uint32_t result_pointer, uint32_t size, uint32_t flags)
{
    PcPortMipsCpu cpu;
    allocation_plan[0] = result_pointer;
    reset_calls();
    initialize_cpu(&cpu, &oracle_runtime);
    cpu.bus.bridge = oracle_bridge;
    cpu.gpr[4] = size; cpu.gpr[5] = flags;
    assert(PcPortMipsRun(&cpu, 0x80031bdcu, BATTLE_HALT_PC, 64) == PC_PORT_MIPS_HALTED);
    uint32_t expected = cpu.gpr[2];
    memcpy(expected_calls, calls, sizeof(calls));
    reset_calls();
    initialize_cpu(&cpu, &candidate_runtime);
    cpu.gpr[4] = size; cpu.gpr[5] = flags;
    assert(PcPortMipsRun(&cpu, 0x80031bdcu, BATTLE_HALT_PC, 64) == PC_PORT_MIPS_HALTED);
    if (call_count != 1 || user_calls != 0 || cpu.gpr[2] != expected ||
        memcmp(calls, expected_calls, sizeof(calls)) || candidate_runtime.bridge_cpu != NULL) {
        if (printed++ < 10)
            fprintf(stderr, "BATTLE HEAP POINTER FAIL direct size=%u flags=%u returned=%08x expected=%08x\n",
                    size, flags, cpu.gpr[2], expected);
        return 1;
    }
    return 0;
}

static int compare_roundtrip(uint32_t guest_pointer)
{
    PcPortMipsCpu cpu;
    allocation_plan[0] = guest_pointer;
    reset_calls();
    free_calls = 0; freed_pointer = UINTPTR_MAX;
    initialize_cpu(&cpu, &candidate_runtime);
    cpu.gpr[4] = 4; cpu.gpr[5] = 1;
    assert(PcPortMipsRun(&cpu, 0x80031bdcu, BATTLE_HALT_PC, 64) == PC_PORT_MIPS_HALTED);
    uint32_t pointer = cpu.gpr[2];
    initialize_cpu(&cpu, &candidate_runtime);
    cpu.gpr[4] = pointer;
    assert(PcPortMipsRun(&cpu, 0x800320e8u, BATTLE_HALT_PC, 64) == PC_PORT_MIPS_HALTED);
    uintptr_t expected = guest_pointer ? (uintptr_t)PSX_ADDR(guest_pointer) : 0;
    if (free_calls != 1 || freed_pointer != expected || cpu.gpr[2] != FREE_SCALAR_RESULT ||
        candidate_runtime.bridge_cpu != NULL) {
        fprintf(stderr, "BATTLE HEAP POINTER FAIL roundtrip guest=%08x free_pointer=%lx/%lx "
                "scalar=%08x/%08x calls=%u\n", guest_pointer, (unsigned long)freed_pointer,
                (unsigned long)expected, cpu.gpr[2], FREE_SCALAR_RESULT, free_calls);
        return 1;
    }
    return 0;
}

int main(void)
{
    assert((uintptr_t)g_PsxRam == 0x0060fe20u);
    FILE *image = fopen("disc/battle.bin", "rb");
    assert(image);
    assert(!fseek(image, 0x80070e48u - BATTLE_BASE, SEEK_SET));
    assert(fread(PSX_ADDR(0x80070e48u), 1, 0x40, image) == 0x40);
    assert(!fseek(image, 0x8008abb8u - BATTLE_BASE, SEEK_SET));
    assert(fread(PSX_ADDR(0x8008abb8u), 1, 0x48, image) == 0x48);
    assert(!fclose(image));
    initialize_runtime(&candidate_runtime);
    if (!candidate_runtime.initialized || !find_function(&candidate_runtime, 0x80031bdcu) ||
        !find_function(&candidate_runtime, 0x80032498u) || !find_function(&candidate_runtime, 0x800320e8u)) {
        fputs("BATTLE HEAP POINTER FAIL actual runtime initialization did not resolve heap boundaries\n", stderr);
        return 2;
    }
    unsigned failures = 0, cases = 0;
    const uint32_t firsts[] = {0x801f34a8u, 0x801e5000u, 0x801e5004u, 0x801efff0u, 0x801f0000u, 0x801ffff4u};
    const uint32_t seconds[] = {0x801e4fe0u, 0x801e4000u};
    for (unsigned i = 0; i < sizeof(firsts) / sizeof(firsts[0]); ++i)
        for (unsigned j = 0; j < sizeof(seconds) / sizeof(seconds[0]); ++j) {
            failures += compare_fragment(firsts[i], seconds[j]);
            ++cases;
        }
    const uint32_t pointers[] = {0, 0x80000000u, 0x80000004u, 0x80100000u, 0x801f34a8u, 0x801fffffu};
    const uint32_t sizes[] = {0, 4, 128};
    for (unsigned p = 0; p < sizeof(pointers) / sizeof(pointers[0]); ++p)
        for (unsigned s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s)
            for (uint32_t flags = 0; flags <= 1; ++flags) {
                failures += compare_direct(pointers[p], sizes[s], flags);
                ++cases;
            }
    for (unsigned p = 0; p < sizeof(pointers) / sizeof(pointers[0]); ++p) {
        failures += compare_roundtrip(pointers[p]);
        ++cases;
    }
    if (failures) {
        fprintf(stderr, "BATTLE HEAP POINTER FAIL %u/%u cases; actual observed guest pointer 801F34A8 "
                "requires second allocation 58536 (E4A8), including complete guarded reservation records\n", failures, cases);
        return 1;
    }
    printf("BATTLE HEAP POINTER PASS %u cases: real runtime initialization/dispatch, retail reservation fragment/helper, "
           "allocation requests and guest pointer records, direct NULL/edge-address results, "
           "HeapFree pointer roundtrip and unchanged scalar result; allocator boundaries controlled\n", cases);
    return 0;
}

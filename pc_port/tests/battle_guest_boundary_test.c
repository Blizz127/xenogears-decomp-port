/* A component test of the production battle bus and bridge. No device services
 * are needed by the selected guest calls. Unexpected device calls fail. */
#include "battle_mips_runtime.c"
#include <assert.h>

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
static unsigned pump_count;

void PcPort_PadVblankPump(void) { ++pump_count; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int size) { (void)p; (void)size; abort(); }
char PsyX_BeginScene(void) { abort(); }
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int val, int reg) { (void)val; (void)reg; abort(); }
void CTC2(unsigned int val, int reg) { (void)val; (void)reg; abort(); }
int doCOP2(int cmd) { (void)cmd; abort(); }
void PushMatrix(void) { abort(); }
void PopMatrix(void) { abort(); }

static BattleMipsRuntime runtime;
static const PcPortMipsCpu *observed_cpu;
static unsigned fetch_count;

static int observe_read(void *context, uint32_t address, unsigned width, uint32_t *value)
{
    /* Neither selected routine reads code as data. */
    if (width == 4 && address == observed_cpu->pc)
        ++fetch_count;
    return runtime_read(context, address, width, value);
}

static void load_at(const char *path, uint32_t address, size_t bytes)
{
    FILE *in = fopen(path, "rb");
    assert(in);
    assert(fread(PSX_ADDR(address), 1, bytes, in) == bytes);
    assert(fgetc(in) == EOF && !ferror(in));
    assert(fclose(in) == 0);
}

static void begin_call(PcPortMipsCpu *cpu, uint32_t halt)
{
    initialize_cpu(cpu, &runtime);
    cpu->gpr[29] = 0x801fe000;
    cpu->gpr[31] = halt;
}

static void check_guest(uint32_t start, uint32_t halt, unsigned expected_steps,
                        unsigned expected_result, int verify_return)
{
    PcPortMipsCpu cpu;
    begin_call(&cpu, halt);
    observed_cpu = &cpu;
    fetch_count = 0;
    cpu.bus.read = observe_read;
    const unsigned pumps_before = pump_count;
    const PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    const int status = PcPortMipsRun(&cpu, start, halt, 32);
    const PcPortMipsExecutionStats after = PcPortMipsExecutionStatsGet();
    assert(after.native_bridges == before.native_bridges);
    assert(after.unresolved_bridges == before.unresolved_bridges);
    if (PcPortMipsInterpreterDisabled()) {
        assert(status == PC_PORT_MIPS_INTERPRETER_DISABLED && cpu.steps == 0 && fetch_count == 0);
        assert(after.disabled_refusals == before.disabled_refusals + 1);
        assert(after.interpreted_instructions == before.interpreted_instructions);
        assert(pump_count - pumps_before == 1);
    } else {
        assert(status == PC_PORT_MIPS_HALTED && cpu.pc == halt);
        assert(cpu.steps == expected_steps && fetch_count == expected_steps);
        assert(after.interpreted_instructions - before.interpreted_instructions == expected_steps);
        assert(after.disabled_refusals == before.disabled_refusals);
        assert(pump_count - pumps_before == expected_steps);
        if (verify_return) {
            assert(cpu.gpr[2] == expected_result);
            assert(cpu.gpr[29] == 0x801fe000 && cpu.gpr[31] == halt);
        }
    }
    observed_cpu = NULL;
}

static void check_archive_table(void)
{
    const uint32_t table = 0x80180000;
    const uint32_t caller = 0x80010000;
    const uint32_t resolve_archive_entries = 0x8003342c;
    PcPortMipsCpu cpu;
    begin_call(&cpu, caller);
    cpu.gpr[4] = table;
    uint8_t *words = PSX_ADDR(table);
    store_le(words, 4, 2);
    store_le(words + 4, 4, 0x18);
    store_le(words + 8, 4, 0x24);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    assert(PcPortMipsRun(&cpu, resolve_archive_entries, caller, 4) == PC_PORT_MIPS_HALTED);
    PcPortMipsExecutionStats after = PcPortMipsExecutionStatsGet();
    assert(after.native_bridges == before.native_bridges + 1);
    assert(after.interpreted_instructions == before.interpreted_instructions);
    assert(cpu.gpr[2] == 2 && load_le(words + 4, 4) == table + 0x18);
    assert(load_le(words + 8, 4) == table + 0x24);

    /* An overflowing count must fail before relocating even the first offset. */
    store_le(words, 4, UINT32_MAX);
    begin_call(&cpu, caller);
    cpu.gpr[4] = table;
    before = PcPortMipsExecutionStatsGet();
    assert(PcPortMipsRun(&cpu, resolve_archive_entries, caller, 4) == PC_PORT_MIPS_UNRESOLVED_CALL);
    after = PcPortMipsExecutionStatsGet();
    assert(after.unresolved_bridges == before.unresolved_bridges + 1);
    assert(after.interpreted_instructions == before.interpreted_instructions);
    assert(load_le(words + 4, 4) == table + 0x18 && load_le(words + 8, 4) == table + 0x24);
}

static void reject_legacy_sprite(void) { abort(); }

static void check_popup_ingress(void)
{
    const uint32_t direction = 0x800223b0u;
    const uint32_t sprite = 0x80150000u;
    uint8_t *animation = PSX_ADDR(sprite + 0x58u);
    const uint32_t saved_word = load_le(animation, 4);
    const size_t count = runtime.function_count;
    /* Install a real resolvable target so a missing domain guard calls the trap. */
    ResolvedFunction *entry = find_function(&runtime, direction);
    ResolvedFunction previous;
    if (entry) {
        previous = *entry;
    } else {
        assert(count < sizeof runtime.functions / sizeof runtime.functions[0]);
        runtime.functions[runtime.function_count++] =
            (ResolvedFunction){direction, NULL, "sprite"};
        qsort(runtime.functions, runtime.function_count, sizeof runtime.functions[0], function_compare);
        entry = find_function(&runtime, direction);
        assert(entry);
    }
    entry->host = (void *)reject_legacy_sprite;
    PcPortMipsCpu cpu;
    begin_call(&cpu, 0x801e68f8u);
    cpu.gpr[4] = sprite;
    uint32_t initial[32];
    memcpy(initial, cpu.gpr, sizeof initial);
    store_le(animation, 4, 0x800c3c00u);
    assert(runtime_bridge(&runtime, &cpu, direction) < 0);
    assert(!memcmp(cpu.gpr, initial, sizeof initial));
    uint8_t *compact = PSX_ADDR(sprite + 0x100u);
    assert((uintptr_t)compact <= UINT32_MAX);
    store_le(animation, 4, (uint32_t)(uintptr_t)compact);
    assert(battle_resident_boundary(&runtime, &cpu, direction) == BRIDGE_HOOKS_NONE);
    store_le(animation, 4, saved_word);
    if (count == runtime.function_count) {
        *entry = previous;
    } else {
        size_t index = (size_t)(entry - runtime.functions);
        memmove(entry, entry + 1, (count - index) * sizeof *entry);
        runtime.function_count = count;
    }

    begin_call(&cpu, 0x800805acu);
    assert(runtime_bridge(&runtime, &cpu, 0x8007fec4u) == 0);
    begin_call(&cpu, 0x8002c86cu);
    assert(runtime_bridge(&runtime, &cpu, 0x80020000u) < 0);
    puts("popup ingress: guest sprite animation refused, compact shape accepted, menu yielded to guest, packet callback refused");
}

#ifdef TEST_RETAINED_SERVICES
#include "battle_retained_services_test.inc"
#endif

int main(int argc, char **argv)
{
    assert(argc == 3 || argc == 4);
    load_at(argv[1], BATTLE_BASE, 343936);
    load_at(argv[2], 0x801e4000, 22208);
    initialize_runtime(&runtime);
    g_ActiveBattleRuntime = &runtime;
    assert(!runtime.force_interpret);
    PcPortMipsExecutionStatsInit();
    check_archive_table();
    check_popup_ingress();
    /* The overlay entry is a one-instruction checkpoint. The module call
     * returns through its own guest frame and reads the selected state word. */
    check_guest(0x80075938, 0x8007593c, 1, 0, 0);
    store_le(PSX_ADDR(0x800c35d8), 4, 11);
    check_guest(0x801e6d34, 0x80010000, 9, 11, 1);
#ifdef TEST_RETAINED_SERVICES
    if (argc == 4 && !PcPortMipsInterpreterDisabled())
        check_retained_services(argv[3]);
#endif
    puts(PcPortMipsInterpreterDisabled()
         ? "strict: two pre-execution refusals; typed table accepted and overflow rejected"
         : "hybrid: one overlay instruction and complete nine-instruction module return; typed table accepted and overflow rejected");
    return 0;
}

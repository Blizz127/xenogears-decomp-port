/* Focused production-linked certificate for world helper 0x80090A84. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include "world_map_helper_90a84.h"

#define SLOT_ADDR       0x800D7000u
#define OBJECT_ADDR     0x800D7800u
#define HEADING         0x8009CD4Cu
#define ANGLE           0x8009BD3Au
#define FLAGS           0x8009BD10u
#define BYTE_STATE      0x8009D738u
#define SELECTION       0x8009BD24u
#define OBJECT_GLOBAL   0x8009D7D8u
#define ALT_SELECTION   0x8009CE68u
#define STATE_PUBLISH   0x8009D804u
#define EDGE_STATE      0x8009CEC0u
#define PREVIOUS_STATE  0x8009C7E8u
#define CHANGED_STATE   0x8009BD34u

static unsigned g_rcos_calls;
static unsigned g_rsin_calls;

typedef struct TraceEvent {
    u32 kind;
    u32 address;
    u32 value;
} TraceEvent;

static TraceEvent g_trace[64];
static unsigned g_trace_count;

void wm_90a84_test_trace(u32 kind, u32 address, u32 value)
{
    if (g_trace_count < (sizeof(g_trace) / sizeof(g_trace[0]))) {
        g_trace[g_trace_count].kind = kind;
        g_trace[g_trace_count].address = address;
        g_trace[g_trace_count].value = value;
    }
    g_trace_count++;
}

int rcos(int angle)
{
    (void)angle;
    g_rcos_calls++;
    return 0x11223344;
}

int rsin(int angle)
{
    (void)angle;
    g_rsin_calls++;
    return (int)0xF2345678u;
}

static void store_u8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0, 4096u);
    store_u32(OBJECT_GLOBAL, OBJECT_ADDR);
    store_u16(OBJECT_ADDR + 0x0Eu, 2u);
    store_u16(ANGLE, 0x1234u);
    store_u16(SELECTION, 0xFFFFu);
    store_u16(ALT_SELECTION, 0u);
    store_u16(FLAGS, 0u);
    store_u8(BYTE_STATE, 0u);
    store_u16(SLOT_ADDR + 0x48u, 0x7777u);
    store_u32(SLOT_ADDR + 0x38u, 0xAAAAAAAAu);
    store_u32(SLOT_ADDR + 0x40u, 0xBBBBBBBBu);
    g_rcos_calls = 0u;
    g_rsin_calls = 0u;
    g_trace_count = 0u;
}

static int expect_u32(const char* name, u32 actual, u32 expected)
{
    if (actual == expected)
        return 0;
    fprintf(stderr, "ASSERTION %s actual=0x%08X expected=0x%08X\n",
            name, (unsigned)actual, (unsigned)expected);
    return 1;
}

static int expect_result(const char* name, s32 actual, s32 expected)
{
    if (actual == expected)
        return 0;
    fprintf(stderr, "ASSERTION %s actual=%d expected=%d\n",
            name, (int)actual, (int)expected);
    return 1;
}

static int expect_event(unsigned index, u32 kind, u32 address, u32 value)
{
    if (index < g_trace_count && index < (sizeof(g_trace) / sizeof(g_trace[0])) &&
        g_trace[index].kind == kind && g_trace[index].address == address &&
        g_trace[index].value == value)
        return 0;
    fprintf(stderr,
            "ASSERTION ordered event[%u] actual=(%u,0x%08X,0x%08X) "
            "expected=(%u,0x%08X,0x%08X)\n",
            index,
            index < g_trace_count ? g_trace[index].kind : 0u,
            index < g_trace_count ? g_trace[index].address : 0u,
            index < g_trace_count ? g_trace[index].value : 0u,
            kind, address, value);
    return 1;
}

static int expect_trace_count(unsigned expected)
{
    if (g_trace_count == expected)
        return 0;
    fprintf(stderr, "ASSERTION ordered event count actual=%u expected=%u\n",
            g_trace_count, expected);
    return 1;
}

static int test_heading_table(void)
{
    static const u16 expected[16] = {
        0x7777u, 0x1234u, 0x0634u, 0x0434u, 0x0A34u,
        0x7777u, 0x0834u, 0x7777u, 0x0E34u, 0x0034u,
        0x7777u, 0x7777u, 0x0C34u, 0x7777u, 0x7777u,
        0x7777u
    };
    unsigned phase;
    int failures = 0;

    for (phase = 0u; phase < 16u; phase++) {
        s32 result;

        reset_fixture();
        store_u16(HEADING, (u16)(phase << 12));
        result = wm_80090A84(SLOT_ADDR);
        failures += expect_result("heading result", result, 0);
        failures += expect_u32("heading selection",
                               (u32)load_u16(SLOT_ADDR + 0x48u),
                               (u32)expected[phase]);
        if (phase == 0u) {
            failures += expect_u32("heading rcos calls", g_rcos_calls, 0u);
            failures += expect_u32("heading rsin calls", g_rsin_calls, 0u);
        } else {
            failures += expect_u32("heading rcos calls", g_rcos_calls, 1u);
            failures += expect_u32("heading rsin calls", g_rsin_calls, 1u);
            failures += expect_u32("heading rcos delay-slot store", load_u32(SLOT_ADDR + 0x38u),
                                   0x11223344u);
            failures += expect_u32("heading negated rsin slot", load_u32(SLOT_ADDR + 0x40u),
                                   0x0DCBA988u);
        }
    }
    return failures;
}

static int test_return_branches(void)
{
    int failures = 0;

    reset_fixture();
    store_u16(FLAGS, 0x20u);
    store_u8(BYTE_STATE, 1u);
    failures += expect_result("flag-byte return", wm_80090A84(SLOT_ADDR), 3);

    reset_fixture();
    store_u16(FLAGS, 0x20u);
    store_u16(SELECTION, 0u);
    failures += expect_result("flag-selection return", wm_80090A84(SLOT_ADDR), 1);

    reset_fixture();
    store_u16(FLAGS, 0x20u);
    store_u16(SELECTION, 0u);
    store_u16(ALT_SELECTION, 0xFFFFu);
    failures += expect_result("selection source", wm_80090A84(SLOT_ADDR), 1);

    reset_fixture();
    store_u16(SELECTION, 0u);
    store_u16(OBJECT_ADDR + 0x0Eu, 1u);
    failures += expect_result("object-state return", wm_80090A84(SLOT_ADDR), 1);

    reset_fixture();
    store_u16(SELECTION, 0u);
    store_u16(OBJECT_ADDR + 0x0Eu, 2u);
    store_u32(PREVIOUS_STATE, 0xA5A5A5A5u);
    failures += expect_result("update return", wm_80090A84(SLOT_ADDR), 0);
    failures += expect_u32("update edge state", load_u32(EDGE_STATE), 0u);
    failures += expect_u32("update changed state", load_u32(CHANGED_STATE), 0u);
    failures += expect_u32("update previous state", load_u32(PREVIOUS_STATE), 0u);

    reset_fixture();
    store_u16(HEADING, 3u);
    store_u32(PREVIOUS_STATE, 1u);
    failures += expect_result("edge return", wm_80090A84(SLOT_ADDR), 0);
    failures += expect_u32("edge active state", load_u32(EDGE_STATE), 1u);
    failures += expect_u32("edge changed state", load_u32(CHANGED_STATE), 0u);
    failures += expect_u32("edge previous state", load_u32(PREVIOUS_STATE), 1u);

    reset_fixture();
    store_u16(FLAGS, 0x10u);
    store_u16(ALT_SELECTION, 0xFFFFu);
    store_u16(SELECTION, 0xFFFFu);
    failures += expect_result("publish return", wm_80090A84(SLOT_ADDR), 0);
    failures += expect_u32("publish state", load_u32(STATE_PUBLISH), 1u);

    return failures;
}

static int test_ordered_event_oracle(void)
{
    static const TraceEvent expected[] = {
        { WM_90A84_TRACE_ENTRY, SLOT_ADDR, 9u },
        { WM_90A84_TRACE_LOAD_HEADING, HEADING, 0x9000u },
        { WM_90A84_TRACE_LOAD_ANGLE, ANGLE, 0x1234u },
        { WM_90A84_TRACE_STORE_ANGLE, SLOT_ADDR + 0x48u, 0x0034u },
        { WM_90A84_TRACE_CALL_RCOS, 0x80090B6Cu, 0x0034u },
        /* jal rsin delay slot executes before rsin replaces v0. */
        { WM_90A84_TRACE_STORE_COS, SLOT_ADDR + 0x38u, 0x11223344u },
        { WM_90A84_TRACE_CALL_RSIN, 0x80090B78u, 0x0034u },
        { WM_90A84_TRACE_STORE_SIN, SLOT_ADDR + 0x40u, 0x0DCBA988u },
        { WM_90A84_TRACE_LOAD_FLAGS, FLAGS, 0u },
        { WM_90A84_TRACE_LOAD_SELECTION, SELECTION, 0xFFFFu },
        { WM_90A84_TRACE_CALL_90A18, 0x80090C48u, 0u },
        { WM_90A84_TRACE_LOAD_HEADING, HEADING, 0x9000u },
        { WM_90A84_TRACE_LOAD_PREVIOUS_STATE, PREVIOUS_STATE, 1u },
        { WM_90A84_TRACE_STORE_EDGE_STATE, EDGE_STATE, 0u },
        { WM_90A84_TRACE_STORE_CHANGED_STATE, CHANGED_STATE, 0u },
        { WM_90A84_TRACE_STORE_PREVIOUS_STATE, PREVIOUS_STATE, 0u },
        { WM_90A84_TRACE_RETURN, 0x80090C50u, 0u }
    };
    unsigned index;
    int failures = 0;

    reset_fixture();
    store_u16(HEADING, 0x9000u);
    store_u32(PREVIOUS_STATE, 1u);
    failures += expect_result("ordered return", wm_80090A84(SLOT_ADDR), 0);
    failures += expect_trace_count((unsigned)(sizeof(expected) /
                                               sizeof(expected[0])));
    for (index = 0u; index < (unsigned)(sizeof(expected) / sizeof(expected[0]));
         index++)
        failures += expect_event(index, expected[index].kind,
                                 expected[index].address, expected[index].value);
    return failures;
}

/* Execute the actual disc instructions, including the jal delay slot.
 * The trig functions are deterministic boundary doubles, shared with native. */
static int retail_read(void* opaque, uint32_t address, unsigned width, uint32_t* value)
{
    (void)opaque;
    if (address < 0x80000000u || address + width > 0x80200000u) return -1;
    *value = 0;
    memcpy(value, PSX_ADDR(address), width);
    return 0;
}
static int retail_write(void* opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque;
    if (address < 0x80000000u || address + width > 0x80200000u) return -1;
    memcpy(PSX_ADDR(address), &value, width);
    return 0;
}
static int retail_trig(void* opaque, PcPortMipsCpu* cpu, uint32_t target)
{
    (void)opaque;
    if (target == 0x8003F8B0u) cpu->gpr[2] = (u32)rcos((int)cpu->gpr[4]);
    else if (target == 0x8003F8CCu) {
        /* The delay-slot store must already have committed before entry. */
        if (load_u32(SLOT_ADDR + 0x38u) != 0x11223344u) return -1;
        cpu->gpr[2] = (u32)rsin((int)cpu->gpr[4]);
    } else return 0;
    return 1;
}
static int test_disc_delay_slot(void)
{
    FILE* file = fopen("disc/world_map.bin", "rb");
    PcPortMipsBus bus = {0};
    PcPortMipsCpu cpu;
    u32 native_x, native_z;
    int failures = 0;
    if (!file) return expect_result("open retail disc", 0, 1);
    reset_fixture();
    store_u16(HEADING, 0x1000u);
    (void)wm_80090A84(SLOT_ADDR);
    native_x = load_u32(SLOT_ADDR + 0x38u);
    native_z = load_u32(SLOT_ADDR + 0x40u);
    if (fseek(file, 0x80090B50u - 0x8006FAF0u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80090B50u), 1, 0x38u, file) != 0x38u) {
        fclose(file);
        return expect_result("read retail code", 0, 1);
    }
    fclose(file);
    store_u32(SLOT_ADDR + 0x38u, 0u);
    store_u32(SLOT_ADDR + 0x40u, 0u);
    bus.read = retail_read; bus.write = retail_write; bus.bridge = retail_trig;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[16] = SLOT_ADDR;
    failures += expect_result("disc delay-slot execution",
        PcPortMipsRun(&cpu, 0x80090B50u, 0x80090B88u, 100u), PC_PORT_MIPS_HALTED);
    failures += expect_u32("native versus disc cosine", native_x, load_u32(SLOT_ADDR + 0x38u));
    failures += expect_u32("native versus disc negated sine", native_z, load_u32(SLOT_ADDR + 0x40u));
    return failures;
}

int main(void)
{
    int failures;

    PsxMemory_Init();
    failures = test_heading_table();
    failures += test_return_branches();
    failures += test_ordered_event_oracle();
    failures += test_disc_delay_slot();
    if (failures != 0) {
        fprintf(stderr, "0x80090A84 certificate FAILED (%d assertions)\n",
                failures);
        return 1;
    }
    puts("0x80090A84 certificate PASS");
    return 0;
}

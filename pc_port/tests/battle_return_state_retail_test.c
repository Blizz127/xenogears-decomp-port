#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"

/*
 * Exact full-entry oracle for SLUS_006.64 0x8001B6C4..0x8001B844.
 * Calls outside that interval are explicit boundary spies.  The loader gate is
 * fixed to -1 because the source FontLoadFont declaration has a separate,
 * pre-existing 10-vs-11 argument ABI defect; this regression owns the normal
 * post-battle state-selection path only.
 */
enum {
    RAM_SIZE = 0x200000,
    ENTRY_PC = 0x8001b6c4u,
    END_PC = 0x8001b844u,
    RETAIL_SIZE = END_PC - ENTRY_PC,
    HALT_PC = 0xfffffffcu,
    STACK_PC = 0x801fff00u,
    GP_VALUE = 0x80059170u,
    LOADER_WORD_PC = 0x80050000u,

    ARCHIVE_SYNC_PC = 0x80028a60u,
    ARCHIVE_INDEX_PC = 0x80028470u,
    LOAD_SETUP_PC = 0x8003747cu,
    FONT_LOAD_PC = 0x800374e8u,
    BATTLE_SETUP_PC = 0x8001b844u,
    BATTLE_RETURN_PC = 0x80070f40u,
    PARTY_REINIT_PC = 0x8001ac94u,
    CHANGE_STATE_PC = 0x8001996cu,
    MAIN_LOOP_PC = 0x80019accu,

    RESULT_PC = 0x800c48eau,
    GUEST_GUARD_PC = 0x800d3338u,
    RETURN_MODE_PC = 0x8005947cu,
    RETURN_FLAG_PC = 0x800594f8u,
    ENTRY_FLAG_PC = 0x8005959cu,
    MAP0_PC = 0x8006f94eu,
    MAP1_PC = 0x8006f950u,
    MAP2_PC = 0x8006f952u,
    MAP3_PC = 0x8006f954u,
    LOADER_PTR_PC = 0x8005917cu,
};

uint8_t g_PsxRam[RAM_SIZE];
#define PSX_ADDR(address) ((void *)(g_PsxRam + ((uintptr_t)(address) & 0x1fffffu)))

typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;

/* Host symbols used by the included production body.  RESULT and GUARD are
 * deliberately poisoned away from their guest bytes to expose overlay/global
 * split reads. */
s32 *D_8005917C;
u8 D_800C48EA;
u8 D_800D3338;
u8 D_8005947C;
u8 D_800594F8;
u16 D_8006F94E;
u16 D_8006F950;
u16 D_8006F952;
u16 D_8006F954;
u8 D_8005959C;

typedef enum EventKind {
    EV_ARCHIVE_SYNC,
    EV_ARCHIVE_INDEX,
    EV_LOAD_SETUP,
    EV_FONT_LOAD,
    EV_BATTLE_SETUP,
    EV_BATTLE_RETURN,
    EV_PARTY_REINIT,
    EV_CHANGE_STATE,
    EV_MAIN_LOOP,
} EventKind;

typedef struct Event {
    EventKind kind;
    uint32_t arg[11];
} Event;

typedef struct Trace {
    Event events[12];
    unsigned count;
} Trace;

static Trace *active_trace;

static void trace_event(EventKind kind, const uint32_t *args, unsigned count)
{
    Event *event;
    if (active_trace == NULL || active_trace->count >= 12) {
        fputs("BATTLE RETURN STATE FAIL trace overflow/no active trace\n", stderr);
        abort();
    }
    event = &active_trace->events[active_trace->count++];
    memset(event, 0, sizeof(*event));
    event->kind = kind;
    if (count != 0)
        memcpy(event->arg, args, count * sizeof(args[0]));
}

void ArchiveCdDataSync(int mode)
{
    uint32_t args[] = {(uint32_t)mode};
    trace_event(EV_ARCHIVE_SYNC, args, 1);
}

void ArchiveSetIndex(int index, int offset)
{
    uint32_t args[] = {(uint32_t)index, (uint32_t)offset};
    trace_event(EV_ARCHIVE_INDEX, args, 2);
}

void func_8003747C(void *destination)
{
    uint32_t args[] = {(uint32_t)(uintptr_t)destination};
    trace_event(EV_LOAD_SETUP, args, 1);
}

void *FontLoadFont(int a0, int a1, int a2, int a3, s32 a4, s32 a5,
                   s32 a6, s32 a7, s32 a8, s32 a9, s32 a10)
{
    uint32_t args[] = {(uint32_t)a0, (uint32_t)a1, (uint32_t)a2,
                       (uint32_t)a3, (uint32_t)a4, (uint32_t)a5,
                       (uint32_t)a6, (uint32_t)a7, (uint32_t)a8,
                       (uint32_t)a9, (uint32_t)a10};
    trace_event(EV_FONT_LOAD, args, 11);
    return (void *)(uintptr_t)0x5a5aa5a5u;
}

void func_8001B844(void) { trace_event(EV_BATTLE_SETUP, NULL, 0); }
void func_80070F40(void) { trace_event(EV_BATTLE_RETURN, NULL, 0); }
void GamePartySignalReinitialize(void) { trace_event(EV_PARTY_REINIT, NULL, 0); }

void ChangeGameState(unsigned int state)
{
    uint32_t args[] = {state};
    trace_event(EV_CHANGE_STATE, args, 1);
}

void MainLoop(int error_code)
{
    uint32_t args[] = {(uint32_t)error_code};
    trace_event(EV_MAIN_LOOP, args, 1);
}

#ifndef BATTLE_RETURN_BODY
#error "BATTLE_RETURN_BODY must name the extracted production function"
#endif
#include BATTLE_RETURN_BODY

typedef struct TestCase {
    uint8_t result;
    uint8_t guard;
    uint8_t return_mode;
    uint16_t map[4];
    const char *label;
} TestCase;

typedef struct Outcome {
    Trace trace;
    uint8_t result;
    uint8_t guard;
    uint8_t return_mode;
    uint8_t return_flag;
    uint8_t entry_flag;
    uint16_t map[4];
} Outcome;

static uint16_t read16(uint32_t address)
{
    uint16_t value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static uint32_t read32(uint32_t address)
{
    uint32_t value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void write16(uint32_t address, uint16_t value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(uint32_t address, uint32_t value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int bus_read(void *opaque, uint32_t address, unsigned width,
                    uint32_t *value)
{
    const uint8_t *retail = opaque;
    const uint8_t *source;
    if (address >= ENTRY_PC && (uint64_t)address + width <= END_PC)
        source = retail + (address - ENTRY_PC);
    else if (address >= 0x80000000u &&
             (uint64_t)address + width <= 0x80200000u)
        source = PSX_ADDR(address);
    else
        return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)source[i] << (i * 8);
    return 0;
}

static int bus_write(void *opaque, uint32_t address, unsigned width,
                     uint32_t value)
{
    uint8_t *destination;
    (void)opaque;
    if (address < 0x80000000u ||
        (uint64_t)address + width > 0x80200000u)
        return -1;
    destination = PSX_ADDR(address);
    for (unsigned i = 0; i < width; ++i)
        destination[i] = (uint8_t)(value >> (8 * i));
    return 0;
}

static uint32_t stack_arg(const PcPortMipsCpu *cpu, unsigned index)
{
    return read32(cpu->gpr[29] + 16u + 4u * (index - 4u));
}

static int retail_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    uint32_t args[11] = {0};
    EventKind kind;
    (void)opaque;
    switch (target) {
    case ARCHIVE_SYNC_PC: kind = EV_ARCHIVE_SYNC; args[0] = cpu->gpr[4]; break;
    case ARCHIVE_INDEX_PC:
        kind = EV_ARCHIVE_INDEX; args[0] = cpu->gpr[4]; args[1] = cpu->gpr[5]; break;
    case LOAD_SETUP_PC: kind = EV_LOAD_SETUP; args[0] = cpu->gpr[4]; break;
    case FONT_LOAD_PC:
        kind = EV_FONT_LOAD;
        for (unsigned i = 0; i < 4; ++i) args[i] = cpu->gpr[4 + i];
        for (unsigned i = 4; i < 11; ++i) args[i] = stack_arg(cpu, i);
        cpu->gpr[2] = 0x5a5aa5a5u;
        break;
    case BATTLE_SETUP_PC: kind = EV_BATTLE_SETUP; break;
    case BATTLE_RETURN_PC: kind = EV_BATTLE_RETURN; break;
    case PARTY_REINIT_PC: kind = EV_PARTY_REINIT; break;
    case CHANGE_STATE_PC: kind = EV_CHANGE_STATE; args[0] = cpu->gpr[4]; break;
    case MAIN_LOOP_PC: kind = EV_MAIN_LOOP; args[0] = cpu->gpr[4]; break;
    default: return 0;
    }
    trace_event(kind, args, kind == EV_FONT_LOAD ? 11 :
                (kind == EV_ARCHIVE_INDEX ? 2 :
                 (kind == EV_ARCHIVE_SYNC || kind == EV_LOAD_SETUP ||
                  kind == EV_CHANGE_STATE || kind == EV_MAIN_LOOP ? 1 : 0)));
    return 1;
}

static void initialize_guest(const TestCase *test)
{
    memset(g_PsxRam, 0xa5, sizeof(g_PsxRam));
    *(uint8_t *)PSX_ADDR(RESULT_PC) = test->result;
    *(uint8_t *)PSX_ADDR(GUEST_GUARD_PC) = test->guard;
    *(uint8_t *)PSX_ADDR(RETURN_MODE_PC) = test->return_mode;
    *(uint8_t *)PSX_ADDR(RETURN_FLAG_PC) = 0x6d;
    *(uint8_t *)PSX_ADDR(ENTRY_FLAG_PC) = 0x7e;
    for (unsigned i = 0; i < 4; ++i)
        write16(MAP0_PC + 2u * i, test->map[i]);
    write32(LOADER_PTR_PC, LOADER_WORD_PC);
    write32(LOADER_WORD_PC, UINT32_MAX); /* normal debug-disabled path */
}

static void initialize_native(const TestCase *test)
{
    static s32 loader_word;
    loader_word = -1;
    D_8005917C = &loader_word;
    D_800C48EA = (uint8_t)(test->result + 0x5bu); /* always unequal */
    D_800D3338 = (uint8_t)(test->guard == 0 ? 0xa7 : 0);
    D_8005947C = test->return_mode;
    D_800594F8 = 0x6d;
    D_8005959C = 0x7e;
    D_8006F94E = test->map[0];
    D_8006F950 = test->map[1];
    D_8006F952 = test->map[2];
    D_8006F954 = test->map[3];
}

static void capture_guest(Outcome *out)
{
    out->result = *(uint8_t *)PSX_ADDR(RESULT_PC);
    out->guard = *(uint8_t *)PSX_ADDR(GUEST_GUARD_PC);
    out->return_mode = *(uint8_t *)PSX_ADDR(RETURN_MODE_PC);
    out->return_flag = *(uint8_t *)PSX_ADDR(RETURN_FLAG_PC);
    out->entry_flag = *(uint8_t *)PSX_ADDR(ENTRY_FLAG_PC);
    for (unsigned i = 0; i < 4; ++i)
        out->map[i] = read16(MAP0_PC + 2u * i);
}

static void capture_native(Outcome *out)
{
    /* RESULT/GUARD remain poisoned host placeholders by design.  The logical
     * inputs are the unchanged guest bytes, while all owned native outputs are
     * captured from their production globals. */
    out->result = *(uint8_t *)PSX_ADDR(RESULT_PC);
    out->guard = *(uint8_t *)PSX_ADDR(GUEST_GUARD_PC);
    out->return_mode = D_8005947C;
    out->return_flag = D_800594F8;
    out->entry_flag = D_8005959C;
    out->map[0] = D_8006F94E;
    out->map[1] = D_8006F950;
    out->map[2] = D_8006F952;
    out->map[3] = D_8006F954;
}

static int run_retail(const uint8_t *retail, const TestCase *test, Outcome *out)
{
    PcPortMipsBus bus = {.opaque = (void *)retail, .read = bus_read,
                         .write = bus_write, .bridge = retail_bridge};
    PcPortMipsCpu cpu;
    memset(out, 0, sizeof(*out));
    initialize_guest(test);
    active_trace = &out->trace;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[28] = GP_VALUE;
    cpu.gpr[29] = STACK_PC;
    cpu.gpr[31] = HALT_PC;
    if (PcPortMipsRun(&cpu, ENTRY_PC, HALT_PC, 512) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE RETURN STATE FAIL retail %s r=%02x: %s\n",
                test->label, test->result, cpu.error);
        return 0;
    }
    capture_guest(out);
    active_trace = NULL;
    return 1;
}

static void run_native(const TestCase *test, Outcome *out)
{
    memset(out, 0, sizeof(*out));
    initialize_guest(test);
    initialize_native(test);
    active_trace = &out->trace;
    func_8001B6C4();
    capture_native(out);
    active_trace = NULL;
}

static int outcomes_equal(const Outcome *retail, const Outcome *native)
{
    return memcmp(retail, native, sizeof(*retail)) == 0;
}

static const Event *find_event(const Trace *trace, EventKind kind)
{
    for (unsigned i = 0; i < trace->count; ++i)
        if (trace->events[i].kind == kind) return &trace->events[i];
    return NULL;
}

static int check_case(const uint8_t *retail, const TestCase *test,
                      unsigned *mismatches)
{
    Outcome oracle, native;
    if (!run_retail(retail, test, &oracle)) return 0;
    run_native(test, &native);
    if (outcomes_equal(&oracle, &native)) return 1;
    if ((*mismatches)++ < 12) {
        const Event *oc = find_event(&oracle.trace, EV_CHANGE_STATE);
        const Event *nc = find_event(&native.trace, EV_CHANGE_STATE);
        fprintf(stderr,
                "BATTLE RETURN STATE FAIL %s result=%02x guard=%02x mode=%02x "
                "map0=%04x calls=%u/%u change=%s%u/%s%u flags=%02x,%02x/%02x,%02x\n",
                test->label, test->result, test->guard, test->return_mode,
                test->map[0], oracle.trace.count, native.trace.count,
                oc ? "" : "none:", oc ? oc->arg[0] : 0,
                nc ? "" : "none:", nc ? nc->arg[0] : 0,
                oracle.entry_flag, oracle.return_flag,
                native.entry_flag, native.return_flag);
    }
    return 1;
}

int main(void)
{
    uint8_t retail[RETAIL_SIZE];
    FILE *file = fopen("disc/SLUS_006.64", "rb");
    unsigned mismatches = 0, cases = 0;
    static const uint8_t special[] = {0x01, 0x21, 0x40, 0x81};
    static const uint16_t maps[] = {0x0000, 0x03ff, 0x0400, 0x07ff,
                                    0x0800, 0x83ff, 0x8400, 0xffff};
    if (file == NULL || fseek(file, ENTRY_PC - 0x8000f800u, SEEK_SET) != 0 ||
        fread(retail, 1, sizeof(retail), file) != sizeof(retail) ||
        fclose(file) != 0) {
        fputs("BATTLE RETURN STATE FAIL cannot read pinned retail slice\n", stderr);
        return 1;
    }

    /* Every possible result byte, with host result/guard poison active. */
    for (unsigned result = 0; result < 256; ++result) {
        TestCase test = {(uint8_t)result, 0, 0,
                         {(uint16_t)(0x1200u | result), 0x1111, 0x2222, 0x3333},
                         "all-results"};
        ++cases;
        if (!check_case(retail, &test, &mismatches)) return 1;
    }

    /* Branch cross-section: zero/nonzero guard and return mode, mask boundary
     * and high-bit aliases, plus the complete 0x81 reset tuple. */
    for (unsigned s = 0; s < sizeof(special); ++s)
        for (unsigned guard = 0; guard < 2; ++guard)
            for (unsigned mode = 0; mode < 2; ++mode)
                for (unsigned m = 0; m < sizeof(maps) / sizeof(maps[0]); ++m) {
                    TestCase test = {special[s], guard ? 0x7d : 0,
                                     mode ? 0x43 : 0,
                                     {maps[m], 0x1111, 0x2222, 0x3333},
                                     "branch-cross-section"};
                    ++cases;
                    if (!check_case(retail, &test, &mismatches)) return 1;
                }

    if (mismatches != 0) {
        fprintf(stderr, "BATTLE RETURN STATE FAIL mismatches=%u cases=%u\n",
                mismatches, cases);
        return 1;
    }
    printf("BATTLE RETURN STATE GREEN cases=%u full retail instructions + boundary spies\n",
           cases);
    return 0;
}

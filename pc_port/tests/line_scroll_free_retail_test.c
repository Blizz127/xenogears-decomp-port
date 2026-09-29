#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "battle_mips_adapter.h"

enum { PSX_RAM_SIZE_FOR_TEST = 2 * 1024 * 1024 + 1024 * 1024 };
uint8_t g_PsxRam[PSX_RAM_SIZE_FOR_TEST];
uint8_t g_PsxScratchpad[1024];

#ifndef LINE_SCROLL_SOURCE
#define LINE_SCROLL_SOURCE "src/slus_006.64/graphics/line_scroll.c"
#endif
#include LINE_SCROLL_SOURCE

enum { RECORD_GUEST = 0x800c3db8u, LEAF_PC = 0x8002800cu,
       HEAP_FREE_PC = 0x800320e8u, HALT_PC = 0xfffffffcu,
       LEAF_SIZE = 0x40 };

typedef struct {
    unsigned calls;
    void *host_pointer;
    uint32_t guest_pointer;
    uint32_t field_at_call;
    uint8_t poison_at_call[8];
} FreeTrace;

static uint8_t *active_record;
static FreeTrace native_trace;
static uint8_t native_low_block[32];

static void copy_bytes(void *destination, const void *source, size_t count)
{
    uint8_t *d = destination;
    const uint8_t *s = source;
    for (size_t i = 0; i < count; ++i)
        d[i] = s[i];
}

static void fill_bytes(void *destination, uint8_t value, size_t count)
{
    uint8_t *d = destination;
    for (size_t i = 0; i < count; ++i)
        d[i] = value;
}

static int compare_bytes(const void *left, const void *right, size_t count)
{
    const uint8_t *a = left;
    const uint8_t *b = right;
    for (size_t i = 0; i < count; ++i) {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

u_int HeapFree(void *pMem)
{
    native_trace.calls++;
    native_trace.host_pointer = pMem;
    copy_bytes(&native_trace.field_at_call, active_record + 0x14, 4);
    copy_bytes(native_trace.poison_at_call, active_record + 0x18, 8);
    return 0;
}

static uint32_t read32(const uint8_t *p)
{
    uint32_t value;
    copy_bytes(&value, p, sizeof(value));
    return value;
}

static void write32(uint8_t *p, uint32_t value)
{
    copy_bytes(p, &value, sizeof(value));
}

static void *resolve_guest(uint32_t value)
{
    uint32_t segment = value & ~0x001fffffu;
    if (segment == 0x80000000u || segment == 0xa0000000u)
        return g_PsxRam + (value & 0x001fffffu);
    return (void *)(uintptr_t)value;
}

static int retail_read(void *opaque, uint32_t address, unsigned width,
                       uint32_t *value)
{
    const uint8_t *leaf = opaque;
    const uint8_t *source;
    if (address >= LEAF_PC && (uint64_t)address + width <= LEAF_PC + LEAF_SIZE)
        source = leaf + (address - LEAF_PC);
    else if (address >= 0x80000000u &&
             (uint64_t)address + width <= 0x80200000u)
        source = g_PsxRam + (address & 0x001fffffu);
    else
        return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)source[i] << (i * 8);
    return 0;
}

static int retail_write(void *opaque, uint32_t address, unsigned width,
                        uint32_t value)
{
    (void)opaque;
    if (address < 0x80000000u ||
        (uint64_t)address + width > 0x80200000u)
        return -1;
    uint8_t *destination = g_PsxRam + (address & 0x001fffffu);
    for (unsigned i = 0; i < width; ++i)
        destination[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static FreeTrace retail_trace;

static int retail_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    if (target != HEAP_FREE_PC)
        return 0;
    retail_trace.calls++;
    retail_trace.guest_pointer = cpu->gpr[4];
    retail_trace.field_at_call = read32(g_PsxRam +
                                        (RECORD_GUEST & 0x001fffffu) + 0x14);
    copy_bytes(retail_trace.poison_at_call,
               g_PsxRam + (RECORD_GUEST & 0x001fffffu) + 0x18, 8);
    cpu->gpr[2] = 0;
    return 1;
}

static int run_retail(uint8_t *leaf, uint32_t pointer_value,
                      FreeTrace *trace, uint8_t *after)
{
    fill_bytes(g_PsxRam, 0xa5, 0x200000);
    fill_bytes(trace, 0, sizeof(*trace));
    uint8_t *record = g_PsxRam + (RECORD_GUEST & 0x001fffffu);
    write32(record + 0x14, pointer_value);
    write32(record + 0x18, 0x800e97dcu);
    fill_bytes(&retail_trace, 0, sizeof(retail_trace));

    PcPortMipsBus bus = {
        .opaque = leaf,
        .read = retail_read,
        .write = retail_write,
        .bridge = retail_bridge,
    };
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = RECORD_GUEST;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = HALT_PC;
    if (PcPortMipsRun(&cpu, LEAF_PC, HALT_PC, 128) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "LINE SCROLL FREE FAIL retail leaf: %s\n", cpu.error);
        return 0;
    }
    copy_bytes(trace, &retail_trace, sizeof(*trace));
    copy_bytes(after, record, 0x20);
    return 1;
}

static int native_case(uint32_t pointer_value, const uint8_t *retail_after,
                       const FreeTrace *retail)
{
    struct {
        uint8_t before[8];
        uint8_t record[0x20];
        uint8_t after[8];
    } fixture;
    fill_bytes(&fixture, 0xa5, sizeof(fixture));
    write32(fixture.record + 0x14, pointer_value);
    write32(fixture.record + 0x18, 0x800e97dcu);
    active_record = fixture.record;
    fill_bytes(&native_trace, 0, sizeof(native_trace));
    GfxLineScrollFree((LineScroll *)fixture.record);
    active_record = NULL;

    uint8_t expected_record[0x20];
    copy_bytes(expected_record, retail_after, sizeof(expected_record));
    if (compare_bytes(fixture.record, expected_record,
                      sizeof(expected_record)) != 0) {
        fprintf(stderr, "LINE SCROLL FREE FAIL record bytes pointer=%08x\n",
                pointer_value);
        return 0;
    }
    if (native_trace.calls != retail->calls ||
        native_trace.field_at_call != retail->field_at_call ||
        compare_bytes(native_trace.poison_at_call, retail->poison_at_call, 8) != 0) {
        fprintf(stderr, "LINE SCROLL FREE FAIL boundary ordering pointer=%08x "
                        "calls=%u/%u field=%08x/%08x\n",
                pointer_value, native_trace.calls, retail->calls,
                native_trace.field_at_call, retail->field_at_call);
        return 0;
    }
    if (retail->calls != 0 && retail->guest_pointer != pointer_value) {
        fprintf(stderr, "LINE SCROLL FREE FAIL retail argument pointer=%08x/%08x\n",
                retail->guest_pointer, pointer_value);
        return 0;
    }
    if (pointer_value != 0 &&
        (native_trace.host_pointer != resolve_guest(pointer_value) ||
         native_trace.field_at_call != pointer_value)) {
        fprintf(stderr, "LINE SCROLL FREE FAIL host resolution pointer=%08x\n",
                pointer_value);
        return 0;
    }
    uint8_t guard[8];
    fill_bytes(guard, 0xa5, sizeof(guard));
    if (compare_bytes(fixture.before, guard, sizeof(guard)) != 0 ||
        compare_bytes(fixture.after, guard, sizeof(guard)) != 0) {
        fprintf(stderr, "LINE SCROLL FREE FAIL adjacent sentinel pointer=%08x\n",
                pointer_value);
        return 0;
    }
    return 1;
}

int main(void)
{
    uint8_t leaf[LEAF_SIZE];
    FILE *disc = fopen("disc/SLUS_006.64", "rb");
    if (disc == NULL) {
        fputs("LINE SCROLL FREE FAIL missing retail image\n", stderr);
        return 1;
    }
    if (fseek(disc, 0x8002800cL - 0x8000f800L, SEEK_SET) != 0 ||
        fread(leaf, 1, sizeof(leaf), disc) != sizeof(leaf) || fclose(disc) != 0) {
        fputs("LINE SCROLL FREE FAIL reading retail leaf\n", stderr);
        return 1;
    }
    if ((uintptr_t)native_low_block > UINT32_MAX) {
        fputs("LINE SCROLL FREE FAIL native low pointer fixture is not low\n",
              stderr);
        return 1;
    }

    const uint32_t pointers[] = {
        0u, 0x80004020u, 0xa0004050u,
        (uint32_t)(uintptr_t)native_low_block,
    };
    unsigned cases = 0;
    for (unsigned i = 0; i < sizeof(pointers) / sizeof(*pointers); ++i) {
        FreeTrace retail;
        uint8_t retail_after[0x20];
        if (!run_retail(leaf, pointers[i], &retail, retail_after) ||
            !native_case(pointers[i], retail_after, &retail))
            return 1;
        ++cases;
    }
    printf("LINE SCROLL FREE PASS %u cases: raw retail leaf, packed +14, "
           "KSEG0/KSEG1/native-low resolution, null poison, ordered free/clear\n",
           cases);
    return 0;
}

/* Retail loader/caller boundary tests. Dependency doubles record archive and
 * heap calls; they do not prove the contents of the disc files or rendering. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t D_8006F9DE;
uint8_t *D_80059470, *D_8005949C, *D_80059520, *D_800658C8;
extern int32_t func_8001BB0C(void);

#ifdef TEST_WRAPPER_ONLY
static uint8_t expected_index;

int32_t func_800379D8(int32_t index, int32_t variant,
                    uint8_t **first, uint8_t **second, uint8_t **third)
{
    if (index != expected_index || variant != 0 || first != &D_80059470 ||
        second != &D_80059520 || third != &D_8005949C) {
        fprintf(stderr, "BATTLE ASSET CALLER FAIL index=%08x expected=%02x or wrong output arguments\n",
                (uint32_t)index, expected_index);
        return -99;
    }
    *first = PSX_ADDR(0x80100000u);
    *second = NULL;
    *third = PSX_ADDR(0x80110004u);
    return -7;
}

int main(void)
{
    for (unsigned index = 0; index < 256; index++) {
        /* Retail 80071130 copies the active encounter into guest RAM;
         * 8001BB24 then reads its third byte. Poison the old separate stub. */
        expected_index = index;
        *(uint8_t*)PSX_ADDR(0x8006F9DEu) = index;
        D_8006F9DE = index ^ 0xFFu;
        D_80059470 = D_80059520 = D_8005949C = NULL;
        int32_t result = func_8001BB0C();
        if (result != -7 || D_80059470 != PSX_ADDR(0x80100000u) ||
            D_80059520 != NULL || D_8005949C != PSX_ADDR(0x80110004u))
            return 1;
    }
    puts("BATTLE ASSET CALLER PASS all 256 byte indices, output order and return");
    return 0;
}
#else
extern int32_t func_800379D8(int32_t, int32_t, uint8_t **, uint8_t **, uint8_t **);
#define QUEUE 0x8005a1dcu
#define OUTPUT 0x801f1000u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

typedef struct Event { uint32_t kind, a, b; } Event;
static struct {
    Event events[32];
    unsigned count, allocs;
    int32_t size;
} state;

static void event(uint32_t kind, uint32_t a, uint32_t b)
{
    assert(state.count < 32);
    state.events[state.count++] = (Event){kind, a, b};
}

static uint32_t read_word(uint32_t address, unsigned width)
{
    const uint8_t *p = PSX_ADDR(address);
    uint32_t result = 0;
    for (unsigned i = 0; i < width; i++) result |= (uint32_t)p[i] << (8 * i);
    return result;
}

static void write_word(uint32_t address, unsigned width, uint32_t value)
{
    uint8_t *p = PSX_ADDR(address);
    for (unsigned i = 0; i < width; i++) p[i] = value >> (8 * i);
}

int ArchiveGetArchiveOffsetIndices(int *directory, int *entry)
{ event(1, 0, 0); *directory = 0x18; *entry = 2; return 0; }
int ArchiveSetIndex(int directory, int entry)
{ event(2, directory, entry); return 37; }
void HeapChangeCurrentUser(unsigned user, char **types)
{ assert(types == NULL); event(3, user, 0); }
int ArchiveDecodeSizeAbsolute(int index)
{ event(4, index, 0); return state.size; }
int ArchiveDecodeAlignedSize(int index)
{ event(5, index, 0); return 0x800 + ((uint32_t)index & 7) * 0x800; }
void *HeapAlloc(unsigned size, unsigned flags)
{
    event(6, size, flags);
    assert(state.allocs < 2);
    return PSX_ADDR(0x80100000u + state.allocs++ * 0x10000u);
}
void HeapPinBlock(void *block)
{ event(7, PsxMemory_GuestAddr(block), 0); }

int func_80029AFC(void *queue, int arg1, int arg2)
{
    assert(queue == PSX_ADDR(QUEUE) && arg1 == 0 && arg2 == 0);
    event(8, 0, 0);
    for (unsigned i = 0; i < 3; i++)
        event(9, read_word(QUEUE + i * 8, 2), read_word(QUEUE + i * 8 + 4, 4));
    /* The real archive routine sorts nonzero entries by unsigned file ID.
     * Preserve the third-entry terminator and the untouched padding shorts. */
    uint32_t first = read_word(QUEUE, 2), second = read_word(QUEUE + 8, 2);
    if (first && second && first > second) {
        uint32_t data = read_word(QUEUE + 4, 4);
        write_word(QUEUE, 2, second);
        write_word(QUEUE + 8, 2, first);
        write_word(QUEUE + 4, 4, read_word(QUEUE + 12, 4));
        write_word(QUEUE + 12, 4, data);
    }
    return -3; /* Retail loader does not branch on the queue call's result. */
}

static int bus_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    if (address < 0x80000000u || (uint64_t)address + width > 0x80200000u) return -1;
    *value = read_word(address, width);
    return 0;
}

static int bus_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque;
    if (address < 0x80000000u || (uint64_t)address + width > 0x80200000u) return -1;
    write_word(address, width, value);
    return 0;
}

static int bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    uint32_t a = cpu->gpr[4], b = cpu->gpr[5];
    switch (target) {
    case 0x800284b4u: {
        int directory, entry;
        cpu->gpr[2] = ArchiveGetArchiveOffsetIndices(&directory, &entry);
        write_word(a, 4, directory); write_word(b, 4, entry);
        break;
    }
    case 0x80028470u: cpu->gpr[2] = ArchiveSetIndex(a, b); break;
    case 0x80032498u: assert(b == 0); HeapChangeCurrentUser(a, NULL); break;
    case 0x80028928u: cpu->gpr[2] = ArchiveDecodeSizeAbsolute(a); break;
    case 0x800288ecu: cpu->gpr[2] = ArchiveDecodeAlignedSize(a); break;
    case 0x80031bdcu: cpu->gpr[2] = PsxMemory_GuestAddr(HeapAlloc(a, b)); break;
    case 0x800320a4u: HeapPinBlock(PSX_ADDR(a)); break;
    case 0x80029afcu: cpu->gpr[2] = func_80029AFC(PSX_ADDR(a), b, cpu->gpr[6]); break;
    default: return 0;
    }
    return 1;
}

static void reset_case(int32_t size)
{
    memset(&state, 0, sizeof(state)); state.size = size;
    memset(PSX_ADDR(QUEUE - 4), 0xa5, 32);
    memset(PSX_ADDR(OUTPUT), 0xa5, 24);
    D_800658C8 = PSX_ADDR(0x80180000u);
    write_word(0x800658c8u, 4, PsxMemory_GuestAddr(D_800658C8));
}

static int compare_case(int32_t index, int32_t variant, int32_t size)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = bus_read, .write = bus_write, .bridge = bridge};
    uint8_t expected_queue[32];
    uint32_t expected_outputs[3], expected_shared;
    Event expected_events[32];
    reset_case(size);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = index; cpu.gpr[5] = variant;
    cpu.gpr[6] = OUTPUT; cpu.gpr[7] = OUTPUT + 8; cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    write_word(STACK + 16, 4, OUTPUT + 16);
    if (PcPortMipsRun(&cpu, 0x800379d8u, HALT, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "retail asset loader execution failed: %s\n", cpu.error);
        return 0;
    }
    uint32_t expected_return = cpu.gpr[2];
    unsigned expected_count = state.count;
    memcpy(expected_events, state.events, sizeof(expected_events));
    memcpy(expected_queue, PSX_ADDR(QUEUE - 4), sizeof(expected_queue));
    for (unsigned i = 0; i < 3; i++) expected_outputs[i] = read_word(OUTPUT + i * 8, 4);
    expected_shared = read_word(0x800658c8u, 4);

    reset_case(size);
    uint8_t *outputs[3] = {NULL, NULL, NULL};
    uint32_t actual_return = func_800379D8(index, variant, &outputs[0], &outputs[1], &outputs[2]);
    if (actual_return != expected_return || expected_count != state.count ||
        memcmp(expected_events, state.events, sizeof(expected_events)) ||
        memcmp(expected_queue, PSX_ADDR(QUEUE - 4), sizeof(expected_queue)) ||
        PsxMemory_GuestAddr(D_800658C8) != expected_shared) {
        fprintf(stderr, "asset loader state/calls mismatch index=%d variant=%d size=%08x\n",
                index, variant, (uint32_t)size);
        return 0;
    }
    for (unsigned i = 0; i < 3; i++) {
        if (PsxMemory_GuestAddr(outputs[i]) != expected_outputs[i]) {
            fprintf(stderr, "asset loader output %u mismatch index=%d variant=%d size=%08x\n",
                    i, index, variant, (uint32_t)size);
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    FILE *retail = fopen("disc/SLUS_006.64", "rb");
    if (!retail || fseek(retail, 0x800379d8u - 0x8000f800u, SEEK_SET) ||
        fread(PSX_ADDR(0x800379d8u), 1, 0x1b0, retail) != 0x1b0) return 1;
    fclose(retail);
    unsigned cases = 0;
    for (unsigned index = 0; index < 256; index++) {
        for (unsigned variant = 0; variant < 2; variant++) {
            if (!compare_case(index, variant, 0x1fe)) return 1;
            cases++;
        }
    }
    const int32_t sizes[] = {0, 1, 2, 3, 0x7fff, 0x8000, 0x8003, 0xffff, 0x7abc0005};
    const int32_t indices[] = {0, 1, 2, -1, -16383, INT32_MIN, INT32_MAX};
    const int32_t variants[] = {0, 1, -1, -2, -7, INT32_MIN, INT32_MAX};
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(*sizes); i++)
        for (unsigned j = 0; j < sizeof(indices) / sizeof(*indices); j++)
            for (unsigned k = 0; k < sizeof(variants) / sizeof(*variants); k++) {
                if (!compare_case(indices[j], variants[k], sizes[i])) return 1;
                cases++;
            }
    printf("BATTLE ASSET LOADER RETAIL PASS cases=%u, calls/queue/padding/outputs/return\n", cases);
    return 0;
}
#endif

/* Compare the production sprite wrapper with the supplied retail instructions.
 * Only the callee boundary is doubled: these arguments are ABI test fixtures,
 * not replacement sprites or game state. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
int32_t D_800591B8;
extern void *func_800242F4(void *, void *, int16_t, int16_t, int16_t,
                          int16_t, int16_t, int32_t);

#define ENTRY 0x800242f4u
#define CALLEE 0x8002435cu
#define FLAGS 0x800591b8u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu
static uint32_t observed[8], returned;
static unsigned calls;

void *func_8002435C(void *destination, void *package, int16_t tx, int16_t ty,
                    int16_t cx, int16_t cy, int16_t extra)
{
    observed[0] = (uintptr_t)destination;
    observed[1] = (uintptr_t)package;
    observed[2] = (int32_t)tx;
    observed[3] = (int32_t)ty;
    observed[4] = (int32_t)cx;
    observed[5] = (int32_t)cy;
    observed[6] = (int32_t)extra;
    observed[7] = D_800591B8;
    calls++;
    return (void *)(uintptr_t)returned;
}

static int read_bus(void *context, uint32_t address, unsigned width, uint32_t *value)
{
    (void)context;
    if (address < 0x80000000u || (uint64_t)address + width > 0x80200000u)
        return -1;
    const uint8_t *p = PSX_ADDR(address);
    *value = 0;
    for (unsigned i = 0; i < width; i++) *value |= (uint32_t)p[i] << (8 * i);
    return 0;
}

static int write_bus(void *context, uint32_t address, unsigned width, uint32_t value)
{
    (void)context;
    if (address < 0x80000000u || (uint64_t)address + width > 0x80200000u)
        return -1;
    uint8_t *p = PSX_ADDR(address);
    for (unsigned i = 0; i < width; i++) p[i] = value >> (8 * i);
    return 0;
}

static uint32_t word(uint32_t address)
{ uint32_t value; assert(read_bus(NULL, address, 4, &value) == 0); return value; }
static void put(uint32_t address, uint32_t value)
{ assert(write_bus(NULL, address, 4, value) == 0); }

static int callee_bridge(void *context, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)context;
    if (target != CALLEE) return 0;
    for (unsigned i = 0; i < 4; i++) observed[i] = cpu->gpr[4 + i];
    for (unsigned i = 4; i < 7; i++) observed[i] = word(cpu->gpr[29] + i * 4);
    observed[7] = word(FLAGS);
    calls++;
    cpu->gpr[2] = returned;
    return 1;
}

static void compare(const uint32_t arguments[8])
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = callee_bridge};
    uint32_t expected[8];
    uint8_t caller_bytes[0x30];
    memset(PSX_ADDR(STACK - 0x30), 0xa5, 0x60);
    memset(PSX_ADDR(FLAGS - 4), 0xa5, 12);
    put(FLAGS, 0x76543210u);
    PcPortMipsCpuInit(&cpu, &bus);
    for (unsigned i = 0; i < 4; i++) cpu.gpr[4 + i] = arguments[i];
    for (unsigned i = 4; i < 8; i++) put(STACK + i * 4, arguments[i]);
    memcpy(caller_bytes, PSX_ADDR(STACK), sizeof(caller_bytes));
    cpu.gpr[28] = 0x80059170u; cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    calls = 0;
    if (PcPortMipsRun(&cpu, ENTRY, HALT, 100) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE WRAPPER retail failed: %s\n", cpu.error);
        assert(0);
    }
    assert(calls == 1 && cpu.gpr[2] == returned && cpu.gpr[29] == STACK);
    assert(word(FLAGS) == 0);
    assert(word(FLAGS - 4) == 0xa5a5a5a5u && word(FLAGS + 4) == 0xa5a5a5a5u);
    assert(word(STACK - 0x30) == 0xa5a5a5a5u && word(STACK - 0x2c) == 0xa5a5a5a5u);
    assert(memcmp(caller_bytes, PSX_ADDR(STACK), sizeof(caller_bytes)) == 0);
    memcpy(expected, observed, sizeof(expected));
    memset(observed, 0x5a, sizeof(observed));
    calls = 0;
    D_800591B8 = 0x76543210;
    void *result = func_800242F4((void *)(uintptr_t)arguments[0],
        (void *)(uintptr_t)arguments[1], (int16_t)arguments[2],
        (int16_t)arguments[3], (int16_t)arguments[4], (int16_t)arguments[5],
        (int16_t)arguments[6], (int32_t)arguments[7]);
    assert(calls == 1 && (uintptr_t)result == returned && D_800591B8 == 0);
    for (unsigned i = 0; i < 8; i++) {
        if (observed[i] != expected[i]) {
            fprintf(stderr, "SPRITE WRAPPER arg[%u] native=%08x retail=%08x\n",
                    i, observed[i], expected[i]);
            assert(0);
        }
    }
}

int main(void)
{
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image);
    assert(fseek(image, ENTRY - 0x8000f800u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(ENTRY), 1, CALLEE - ENTRY, image) == CALLEE - ENTRY);
    fclose(image);
    /* Exact arguments preserved in the visible-run-08 core. */
    uint32_t arguments[8] = {0x7187c8, 0x713f58, 0, 0x1c3, 0x140, 0x100, 0x20, 0};
    returned = 0x719000;
    compare(arguments);
    unsigned cases = 1;
    /* Each of the five halfword slots visits every 16-bit value, with dirty
     * upper halves at the retail entry to verify its sign-extension rules. */
    for (uint32_t value = 0; value <= UINT16_MAX; value++, cases++) {
        arguments[0] = (value & 1) ? 0 : 0x711234;
        arguments[1] = (value & 2) ? 0 : 0x725678;
        for (unsigned i = 2; i < 7; i++)
            arguments[i] = 0xa53c0000u | ((value + i * 997u) & 0xffffu);
        arguments[7] = value * 65537u;
        returned = (value & 4) ? 0 : 0x734567;
        compare(arguments);
    }
    /* Every bit of the full-width eighth argument independently set/clear. */
    for (unsigned bit = 0; bit < 32; bit++) {
        arguments[7] = 1u << bit;
        compare(arguments);
        arguments[7] = ~(1u << bit);
        compare(arguments);
        cases += 2;
    }
    printf("SPRITE CREATE WRAPPER RETAIL PASS cases=%u, both pointers/five halfwords/flags/return/guards\n", cases);
    return 0;
}

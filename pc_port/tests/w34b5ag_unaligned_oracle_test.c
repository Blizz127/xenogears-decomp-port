/* Exercise unaligned word operations through the running CPU using a synthetic bus. */
#include "battle_mips_adapter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { CODE = 0x1000, DATA = 0x2000, LWL = 0x22, LWR = 0x26, SWL = 0x2a, SWR = 0x2e };
typedef struct {
    uint32_t code[3];
    uint8_t data[12];
} TestBus;

static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write_word(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (8 * i));
}

static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    TestBus *memory = opaque;
    if (width != 4 || (address & 3u)) return -1;
    if (address >= CODE && address < CODE + sizeof memory->code) {
        *value = memory->code[(address - CODE) / 4u];
        return 0;
    }
    if (address < DATA || address > DATA + sizeof memory->data - 4u) return -1;
    *value = read_word(memory->data + (address - DATA));
    return 0;
}

static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    TestBus *memory = opaque;
    if (width != 4 || (address & 3u) || address < DATA ||
        address > DATA + sizeof memory->data - 4u) return -1;
    write_word(memory->data + (address - DATA), value);
    return 0;
}

/* The base address is in r1 and the source or destination word is in r2. */
static uint32_t instruction(unsigned opcode, unsigned offset)
{
    return (uint32_t)opcode << 26 | 1u << 21 | 2u << 16 | offset;
}

static uint32_t execute(TestBus *memory, uint32_t address, uint32_t reg, unsigned steps)
{
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .opaque = memory};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[1] = address;
    cpu.gpr[2] = reg;
    assert(PcPortMipsRun(&cpu, CODE, CODE + steps * 4u, 8) == PC_PORT_MIPS_HALTED);
    assert(cpu.steps == steps);
    return cpu.gpr[2];
}

static void check_single(unsigned op, unsigned offset, uint32_t reg, uint32_t pattern)
{
    TestBus bus = {0};
    uint8_t expected_data[sizeof bus.data], expected_reg[4];
    memset(bus.data, 0xa5, sizeof bus.data);
    write_word(bus.data + 4u, pattern);
    write_word(expected_reg, reg);
    memcpy(expected_data, bus.data, sizeof expected_data);
    int left = op == LWL || op == SWL;
    int store = op == SWL || op == SWR;
    unsigned count = left ? offset + 1 : 4 - offset;
    unsigned first_ram = left ? 0 : offset;
    unsigned first_reg = left ? 3 - offset : 0;
    if (store)
        memcpy(expected_data + 4 + first_ram, expected_reg + first_reg, count);
    else
        memcpy(expected_reg + first_reg, expected_data + 4 + first_ram, count);
    bus.code[0] = instruction(op, 0);
    assert(execute(&bus, DATA + 4 + offset, reg, 2) == read_word(expected_reg));
    assert(!memcmp(bus.data, expected_data, sizeof expected_data));
}

static void check_pair(unsigned offset, uint32_t reg)
{
    TestBus bus = {0};
    uint8_t original[sizeof bus.data], expected[sizeof bus.data];
    for (unsigned i = 0; i < sizeof bus.data; ++i) bus.data[i] = (uint8_t)(7u + 17u * i);
    memcpy(original, bus.data, sizeof original);
    uint32_t word = read_word(original + 4 + offset);
    bus.code[0] = instruction(LWL, 3);
    bus.code[1] = instruction(LWR, 0);
    assert(execute(&bus, DATA + 4 + offset, reg, 3) == word);
    assert(!memcmp(original, bus.data, sizeof original));

    memcpy(expected, original, sizeof expected);
    write_word(expected + 4 + offset, reg);
    bus.code[0] = instruction(SWL, 3);
    bus.code[1] = instruction(SWR, 0);
    assert(execute(&bus, DATA + 4 + offset, reg, 3) == reg);
    assert(!memcmp(expected, bus.data, sizeof expected));
}

int main(void)
{
    static const uint32_t regs[] = {0xdeadbeef, 0, UINT32_MAX, 0x01234567, 0x89abcdef};
    static const uint32_t patterns[] = {0x88442211, 0xeeddbbaa, 0, UINT32_MAX, 0x67452301};
    static const unsigned ops[] = {LWL, LWR, SWL, SWR};
    for (unsigned offset = 0; offset < 4; ++offset) {
        for (unsigned i = 0; i < sizeof regs / sizeof regs[0]; ++i) {
            for (unsigned j = 0; j < sizeof patterns / sizeof patterns[0]; ++j) {
                for (unsigned op = 0; op < sizeof ops / sizeof ops[0]; ++op)
                    check_single(ops[op], offset, regs[i], patterns[j]);
            }
            check_pair(offset, regs[i]);
        }
    }
    puts("Unaligned CPU operations passed 400 single-operation inputs and 20 load/store pairs");
    return 0;
}

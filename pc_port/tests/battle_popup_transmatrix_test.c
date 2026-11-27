#include "battle_mips_adapter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RAM_SIZE 0x200000u
#define SPAD_BASE 0x1f800000u
#define SPAD_SIZE 0x400u
#define ENTRY 0x80049d9cu
#define RETURN_PC 0x800bceccu

static struct FixtureMemory {
    uint8_t ram[RAM_SIZE];
    uint8_t scratch[SPAD_SIZE];
} memory, expected;

static uint8_t *address_bytes(uint32_t address, unsigned width)
{
    const uintptr_t base = (uintptr_t)memory.ram;
    if (width > 4) return NULL;
    if ((uint64_t)address >= base && (uint64_t)address + width <= base + RAM_SIZE)
        return memory.ram + ((uintptr_t)address - base);
    if (address >= SPAD_BASE && (uint64_t)address + width <= SPAD_BASE + SPAD_SIZE)
        return memory.scratch + (address - SPAD_BASE);
    uint32_t segment = address & 0xffe00000u;
    uint32_t offset = address & (RAM_SIZE - 1);
    if ((segment == 0x80000000u || segment == 0xa0000000u) && offset <= RAM_SIZE - width)
        return memory.ram + offset;
    return NULL;
}

static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    const uint8_t *bytes = address_bytes(address, width);
    if (!bytes) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)bytes[i] << (8 * i);
    return 0;
}

static void write_word_bytes(uint8_t *bytes, unsigned width, uint32_t value)
{
    for (unsigned i = 0; i < width; ++i) bytes[i] = (uint8_t)(value >> (8 * i));
}

static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque;
    uint8_t *bytes = address_bytes(address, width);
    if (!bytes) return -1;
    write_word_bytes(bytes, width, value);
    return 0;
}

static void check_mode(unsigned mode)
{
    const uint32_t matrix = mode == 1 ? SPAD_BASE + 0x100 : 0x80150000u;
    uint32_t vector = 0x80150080u;
    if (mode == 1) {
        uintptr_t low_host_pointer = (uintptr_t)(memory.ram + 0x150080);
        assert(low_host_pointer <= UINT32_MAX);
        vector = (uint32_t)low_host_pointer;
    } else if (mode == 2) {
        vector = matrix + 24; /* Input aliases two output components. */
    }

    memset(memory.ram + 0x150000, 0x5a, 128);
    memset(memory.scratch, 0x33, sizeof memory.scratch);
    const uint32_t translation[3] = {0x10203040u, 0x10203041u, 0x10203042u};
    for (unsigned i = 0; i < 3; ++i)
        assert(write_bus(NULL, vector + 4 * i, 4, translation[i]) == 0);

    expected = memory;
    uint8_t *expected_matrix = mode == 1 ? expected.scratch + 0x100 : expected.ram + 0x150000;
    for (unsigned i = 0; i < 3; ++i)
        write_word_bytes(expected_matrix + 20 + 4 * i, 4, translation[i]);

    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus};
    PcPortMipsCpuInit(&cpu, &bus);
    for (unsigned i = 1; i < 32; ++i) cpu.gpr[i] = 0x31400000u + i;
    cpu.gpr[4] = matrix;
    cpu.gpr[5] = vector;
    cpu.gpr[31] = RETURN_PC;
    uint32_t expected_regs[32];
    memcpy(expected_regs, cpu.gpr, sizeof expected_regs);
    expected_regs[2] = matrix;
    for (unsigned i = 0; i < 3; ++i) expected_regs[8 + i] = translation[i];

    assert(PcPortMipsRun(&cpu, ENTRY, RETURN_PC, 200) == PC_PORT_MIPS_HALTED);
    assert(cpu.pc == RETURN_PC && cpu.steps == 9);
    assert(!memcmp(cpu.gpr, expected_regs, sizeof expected_regs));
    assert(cpu.hi == 0 && cpu.lo == 0);
    assert(!memcmp(&memory, &expected, sizeof memory));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    FILE *file = fopen(argv[1], "rb");
    assert(file);
    assert(fseek(file, 0x3a59c, SEEK_SET) == 0);
    assert(fread(memory.ram + (ENTRY & (RAM_SIZE - 1)), 1, 36, file) == 36);
    assert(fclose(file) == 0);

    /* The final historical input repeats the normal guest mapping. */
    for (unsigned mode = 0; mode < 4; ++mode) check_mode(mode);
    puts("TransMatrix guest checks passed for four inputs, including scratch and aliasing");
    return 0;
}

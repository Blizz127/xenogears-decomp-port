/* Memory bus for isolated scalar routines from the US Disc 1 executable and
 * ordinary battle overlay. Only the two mapped images and test RAM are used. */
#ifndef XENO_TEST_OWNED_BATTLE_BUS_H
#define XENO_TEST_OWNED_BATTLE_BUS_H

#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_RAM_BASE 0x80000000u
#define TEST_RAM_SIZE 0x200000u
static uint8_t test_ram[TEST_RAM_SIZE];

static int test_range(uint32_t address, unsigned width)
{
    return address >= TEST_RAM_BASE && width <= TEST_RAM_SIZE &&
           address - TEST_RAM_BASE <= TEST_RAM_SIZE - width;
}

static int test_read(void *unused, uint32_t address, unsigned width, uint32_t *value)
{
    (void)unused;
    if ((width != 1 && width != 2 && width != 4) || !test_range(address, width))
        return -1;
    const uint8_t *src = test_ram + (address - TEST_RAM_BASE);
    *value = 0;
    for (unsigned i = 0; i < width; i++)
        *value |= (uint32_t)src[i] << (8 * i);
    return 0;
}

static int test_write(void *unused, uint32_t address, unsigned width, uint32_t value)
{
    (void)unused;
    if ((width != 1 && width != 2 && width != 4) || !test_range(address, width))
        return -1;
    uint8_t *dst = test_ram + (address - TEST_RAM_BASE);
    for (unsigned i = 0; i < width; i++)
        dst[i] = (uint8_t)(value >> (8 * i));
    return 0;
}

static inline uint32_t test_get(uint32_t address, unsigned width)
{
    uint32_t result;
    assert(test_read(NULL, address, width, &result) == 0);
    return result;
}

static inline void test_put(uint32_t address, unsigned width, uint32_t value)
{
    assert(test_write(NULL, address, width, value) == 0);
}

static void test_image(const char *name, unsigned offset, uint32_t base, unsigned size)
{
    FILE *image = fopen(name, "rb");
    assert(image && test_range(base, size));
    assert(fseek(image, offset, SEEK_SET) == 0);
    assert(fread(test_ram + (base - TEST_RAM_BASE), 1, size, image) == size);
    assert(fclose(image) == 0);
}

static void test_init(PcPortMipsCpu *cpu)
{
    const PcPortMipsBus bus = {.read = test_read, .write = test_write};
    PcPortMipsCpuInit(cpu, &bus);
    cpu->gpr[29] = 0x801ff000u;
    cpu->gpr[31] = 0xfffffffcu;
}

static inline void test_run(PcPortMipsCpu *cpu, uint32_t entry)
{
    uint32_t halt = cpu->gpr[31];
    int status = PcPortMipsRun(cpu, entry, halt, 250000);
    if (status != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "Guest entry %08x failed: %s\n", entry, cpu->error);
        exit(1);
    }
    assert(cpu->pc == halt && cpu->steps > 0);
}

#endif

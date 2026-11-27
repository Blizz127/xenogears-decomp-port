#ifndef XENO_TEST_BATTLE_GUEST_NUMERIC_BUS_H
#define XENO_TEST_BATTLE_GUEST_NUMERIC_BUS_H

#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GUEST_RAM_BASE UINT32_C(0x80000000)
#define GUEST_RAM_SIZE UINT32_C(0x200000)
#define BATTLE_OVERLAY_BASE UINT32_C(0x8006faf0)
#define SLUS_TEXT_BASE UINT32_C(0x80010000)
#define SLUS_HEADER_BYTES 0x800u

static uint8_t guest_ram[GUEST_RAM_SIZE];

static int guest_address(uint32_t address, unsigned width, uint32_t *offset)
{
    uint32_t segment = address >> 29;
    if (segment != 0 && segment != 4 && segment != 5) return -1;
    *offset = address & UINT32_C(0x1fffffff);
    if (width > GUEST_RAM_SIZE || *offset > GUEST_RAM_SIZE - width) return -1;
    return 0;
}

static int guest_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    uint32_t offset;
    (void)opaque;
    if (width > sizeof *value || guest_address(address, width, &offset)) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)guest_ram[offset + i] << (8 * i);
    return 0;
}

static int guest_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    uint32_t offset;
    (void)opaque;
    if (width > sizeof value || guest_address(address, width, &offset)) return -1;
    for (unsigned i = 0; i < width; ++i)
        guest_ram[offset + i] = (uint8_t)(value >> (8 * i));
    return 0;
}

static void guest_load(const char *image, uint32_t load_base, long header,
                       uint32_t address, unsigned size)
{
    uint32_t offset;
    assert(address >= load_base && !guest_address(address, size, &offset));
    FILE *fp = fopen(image, "rb");
    assert(fp);
    assert(fseek(fp, header + (long)(address - load_base), SEEK_SET) == 0);
    assert(fread(guest_ram + offset, 1, size, fp) == size);
    assert(fclose(fp) == 0);
}
#endif

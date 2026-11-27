#ifndef XENO_TEST_WORLD_GUEST_MEMORY_H
#define XENO_TEST_WORLD_GUEST_MEMORY_H
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t guest[0x200000], scratch[1024];

static uint8_t *region(uint32_t address, size_t bytes)
{
    if (address >= 0x1f800000u && address < 0x1f800400u) {
        size_t offset = address - 0x1f800000u;
        return bytes <= sizeof scratch - offset ? scratch + offset : NULL;
    }
    size_t offset = address & 0x1fffffffu;
    return offset <= sizeof guest && bytes <= sizeof guest - offset ? guest + offset : NULL;
}

static int read_guest(void *ctx, uint32_t address, unsigned width, uint32_t *value)
{
    (void)ctx;
    const uint8_t *p = region(address, width);
    if (!p || width > sizeof *value) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int write_guest(void *ctx, uint32_t address, unsigned width, uint32_t value)
{
    (void)ctx;
    uint8_t *p = region(address, width);
    if (!p || width > sizeof value) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static void put(uint32_t address, unsigned width, uint32_t value)
{
    assert(write_guest(NULL, address, width, value) == 0);
}

static void load_overlay(const char *path)
{
    FILE *file = fopen(path, "rb");
    assert(file);
    assert(fread(guest + 0x6faf0, 1, 180422, file) == 180422);
    assert(fgetc(file) == EOF && !ferror(file));
    assert(fclose(file) == 0);
}
#endif

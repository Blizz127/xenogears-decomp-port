/* Focused production-linked certificate for retail helper 0x80094364. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_94364.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POS       0x800B1000u
#define TABLE     0x800B2000u
#define LIST0     0x800B3000u
#define LIST1     0x800B3200u
#define EMPTY     0x800B3400u
#define TABLE_PTR 0x8009BD00u
#define RECORD_PTR 0x8009D7D8u
#define CANARY    0xA55A3CC3u

static uint8_t before_ram[PSX_RAM_SIZE];
static int failures;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void record(u32 address,
                   s16 x,
                   s16 z,
                   s16 width,
                   s16 depth,
                   s16 marker,
                   s16 type)
{
    write16(address + 0u, (u16)x);
    write16(address + 2u, (u16)z);
    write16(address + 4u, (u16)width);
    write16(address + 6u, (u16)depth);
    write16(address + 8u, (u16)marker);
    write16(address + 0xEu, (u16)type);
}

static void seed(void)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));

    write32(TABLE_PTR, TABLE);
    write32(TABLE + 0u, LIST0);
    write32(TABLE + 4u, LIST1);
    write32(TABLE + 8u, EMPTY);
    /* Safe decoy for M1's deliberately doubled table scale at index 2. */
    write32(TABLE + 16u, EMPTY);

    record(LIST0 + 0x00u, 10, 20, 5, 6, 1, 2);
    record(LIST0 + 0x10u, 10, 20, 5, 6, 1, 3);
    record(LIST0 + 0x20u, 0, 0, 0, 0, -1, 0);

    record(LIST1 + 0x00u, (s16)0x2345, 0, 0, 0, 1, -1);
    record(LIST1 + 0x10u, 0, 0, 0, 0, -1, 0);
    record(EMPTY, 0, 0, 0, 0, -1, 0);
    write32(RECORD_PTR, CANARY);
}

static void set_position(u32 x, u32 z)
{
    write32(POS + 0u, x);
    write32(POS + 8u, z);
}

static int offset_is_record_ptr(size_t offset)
{
    size_t start = (size_t)(RECORD_PTR & 0x1FFFFFu);
    return offset >= start && offset < start + 4u;
}

static void snapshot(void)
{
    memcpy(before_ram, g_PsxRam, sizeof(before_ram));
}

static void check_write_set(int hit, const char* name)
{
    size_t i;
    for (i = 0u; i < sizeof(g_PsxRam); i++) {
        if (before_ram[i] != g_PsxRam[i] &&
            !(hit && offset_is_record_ptr(i))) {
            fprintf(stderr,
                    "ASSERTION %s FAILED offset=0x%zx before=%02x after=%02x\n",
                    name, i, before_ram[i], g_PsxRam[i]);
            failures++;
            return;
        }
    }
}

static s32 run(u32 x,
               u32 z,
               u32 list_index,
               s32 requested_type,
               int expect_hit,
               const char* write_name)
{
    s32 result;
    set_position(x, z);
    write32(RECORD_PTR, CANARY);
    snapshot();
    result = wm_80094364(POS, list_index, requested_type);
    check_write_set(expect_hit, write_name);
    return result;
}

int main(void)
{
    s32 result;

    seed();
    result = run(10u << 12, 20u << 12, 0u, 3, 1,
                 "hit.write_set");
    check(result == 1 && read32(RECORD_PTR) == LIST0 + 0x10u,
          "type.match.scan");
    check(read32(RECORD_PTR) == LIST0 + 0x10u, "stride.0x10");

    result = run(10u << 12, 20u << 12, 0u, 2, 1,
                 "lower.write_set");
    check(result == 1 && read32(RECORD_PTR) == LIST0,
          "bounds.inclusive");

    result = run(15u << 12, 26u << 12, 0u, 2, 1,
                 "upper.write_set");
    check(result == 1 && read32(RECORD_PTR) == LIST0,
          "bounds.inclusive");

    result = run(0x12345000u, 0u, 1u, -1, 1,
                 "masked.write_set");
    check(result == 1 && read32(RECORD_PTR) == LIST1,
          "coord.mask");
    check(result == 1 && read32(RECORD_PTR) == LIST1,
          "type.signed");
    check(result == 1 && read32(RECORD_PTR) == LIST1,
          "list.index.scale");

    result = run(10u << 12, 20u << 12, 0u, 4, 0,
                 "miss.write_set");
    check(result == 0 && read32(RECORD_PTR) == CANARY,
          "miss.no_publish");

    result = run(10u << 12, 20u << 12, 2u, 2, 0,
                 "empty.write_set");
    check(result == 0 && read32(RECORD_PTR) == CANARY,
          "empty.no_publish");

    if (failures != 0) {
        fprintf(stderr, "W34N26 94364 CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N26 94364 CERTIFICATE PASS");
    return 0;
}

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_cold_defaults.h"

#define GS_BASE   UINT32_C(0x8006D634)
#define HOST_SIZE UINT32_C(0xB000)

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static u8 s_host[HOST_SIZE];
static u8 s_guest_before[PSX_RAM_SIZE];
static u8 s_host_before[HOST_SIZE];
static u8 s_guest_mask[PSX_RAM_SIZE];
static u8 s_host_mask[HOST_SIZE];
static int s_passes;
static int s_total;

typedef struct ExpectedStore {
    u32 address;
    u16 value;
    u8 width;
} ExpectedStore;

static const ExpectedStore s_fixed[] = {
    { UINT32_C(0x8006F952), UINT16_C(0x0FFF), 2 },
    { UINT32_C(0x8006F950), UINT16_C(0x0C00), 2 },
    { UINT32_C(0x8006F94E), UINT16_C(0x0400), 2 },
    { UINT32_C(0x8006F954), UINT16_C(1), 2 },
    { UINT32_C(0x8006EE68), UINT16_C(0x4003), 2 },
    { UINT32_C(0x8006EE60), UINT16_C(0x6680), 2 },
    { UINT32_C(0x8006EE62), UINT16_C(0xFF00), 2 },
    { UINT32_C(0x8006EE64), UINT16_C(0x2A00), 2 },
    { UINT32_C(0x8006EE56), UINT16_C(0x2C00), 2 },
    { UINT32_C(0x8006F369), UINT16_C(10), 1 },
    { UINT32_C(0x8006D940), UINT16_C(15), 1 },
    { UINT32_C(0x8006D9E4), UINT16_C(2), 1 },
    { UINT32_C(0x8006DB2C), UINT16_C(4), 1 },
    { UINT32_C(0x8006F36A), UINT16_C(5), 1 },
    { UINT32_C(0x8006DBD0), UINT16_C(5), 1 },
    { UINT32_C(0x8006DC74), UINT16_C(6), 1 },
    { UINT32_C(0x8006DDBC), UINT16_C(7), 1 },
    { UINT32_C(0x8006DE60), UINT16_C(8), 1 },
    { UINT32_C(0x8006DA88), UINT16_C(3), 1 },
    { UINT32_C(0x8006DF04), UINT16_C(3), 1 },
    { UINT32_C(0x8006EE6A), UINT16_C(1), 2 },
    { UINT32_C(0x8006EE66), UINT16_C(0), 2 },
    { UINT32_C(0x8006EE54), UINT16_C(0x7580), 2 },
    { UINT32_C(0x8006EE58), UINT16_C(0), 2 },
    { UINT32_C(0x8006F368), UINT16_C(0), 1 },
    { UINT32_C(0x8006F8E5), UINT16_C(0), 1 },
    { UINT32_C(0x8006F8E6), UINT16_C(0), 1 },
    { UINT32_C(0x8006F8E7), UINT16_C(0), 1 },
    { UINT32_C(0x8006DD18), UINT16_C(9), 1 },
    { UINT32_C(0x8006DFA8), UINT16_C(9), 1 },
    { UINT32_C(0x8006EF8E), UINT16_C(0x0400), 2 },
    { UINT32_C(0x8006EF90), UINT16_C(0x7500), 2 },
    { UINT32_C(0x8006EF92), UINT16_C(0x2E58), 2 },
    { UINT32_C(0x8006EF94), UINT16_C(0x0400), 2 },
    { UINT32_C(0x8006EF96), UINT16_C(0x7580), 2 },
    { UINT32_C(0x8006EF98), UINT16_C(0x2E58), 2 },
    { UINT32_C(0x8006EF9A), UINT16_C(0x0400), 2 },
    { UINT32_C(0x8006EF9C), UINT16_C(0x7600), 2 },
    { UINT32_C(0x8006EF9E), UINT16_C(0x2E58), 2 },
    { UINT32_C(0x8006EE7C), UINT16_C(1), 2 },
};

static void check(const char* name, int condition)
{
    s_total++;
    if (condition) {
        s_passes++;
        printf("PASS [cold]: %s\n", name);
    } else {
        printf("FAIL [cold]: %s\n", name);
    }
}

static void store_u16(void* destination, u16 value)
{
    memcpy(destination, &value, sizeof(value));
}

static u16 load_u16(const void* source)
{
    u16 value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static u32 load_u32(const void* source)
{
    u32 value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static void mark_store(u32 address, u8 width)
{
    u32 guest_offset = address & UINT32_C(0x1FFFFF);
    u32 host_offset = address - GS_BASE;
    u32 byte;

    for (byte = 0; byte < (u32)width; byte++) {
        s_guest_mask[guest_offset + byte] = 1u;
        s_host_mask[host_offset + byte] = 1u;
    }
}

static int unchanged_outside_mask(const u8* after, const u8* before,
                                  const u8* mask, u32 size)
{
    u32 offset;
    for (offset = 0; offset < size; offset++)
        if (mask[offset] == 0u && after[offset] != before[offset])
            return 0;
    return 1;
}

static void run_cold_case(void)
{
    const u16 table_index = UINT16_C(2);
    const u16 table_x = UINT16_C(0x1234);
    const u16 table_z = UINT16_C(0xFEDC);
    u32 index;
    int result;
    int values_exact = 1;
    int aliases_exact = 1;

    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(s_host, 0x5A, sizeof(s_host));
    memset(s_guest_mask, 0, sizeof(s_guest_mask));
    memset(s_host_mask, 0, sizeof(s_host_mask));

    store_u16(s_host + (UINT32_C(0x8006EE7A) - GS_BASE), table_index);
    store_u16(PSX_ADDR(UINT32_C(0x8009AF80) + (u32)table_index * 2u),
              table_x);
    store_u16(PSX_ADDR(UINT32_C(0x8009AF90) + (u32)table_index * 2u),
              table_z);
    memcpy(s_guest_before, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_host_before, s_host, sizeof(s_host));

    result = wm_80070D58_cold_defaults(s_host);
    check("returns success", result == 0);
    check("entrance coerced to one",
          load_u16(s_host + (UINT32_C(0x8006F954) - GS_BASE)) == 1u);
    check("channel two default is five",
          s_host[UINT32_C(0x8006F36A) - GS_BASE] == 5u);
    check("host/guest alias mirrors exact",
          load_u16(s_host + (UINT32_C(0x8006F94E) - GS_BASE)) ==
          load_u16(PSX_ADDR(UINT32_C(0x8006F94E))));
    check("lookup pair uses incoming EE7A",
          load_u16(PSX_ADDR(UINT32_C(0x8006EE78))) == table_x &&
          load_u16(PSX_ADDR(UINT32_C(0x8006EE7A))) == table_z);
    check("D160 fixed upper bound",
          load_u32(PSX_ADDR(UINT32_C(0x8009D160))) ==
          UINT32_C(0x07FFFFFF));

    for (index = 0; index < (u32)(sizeof(s_fixed) / sizeof(s_fixed[0]));
         index++) {
        const ExpectedStore* expected = &s_fixed[index];
        u32 host_offset = expected->address - GS_BASE;
        if (expected->width == 1u) {
            if (s_host[host_offset] != (u8)expected->value ||
                *(u8*)PSX_ADDR(expected->address) != (u8)expected->value)
                values_exact = 0;
        } else {
            if (load_u16(s_host + host_offset) != expected->value ||
                load_u16(PSX_ADDR(expected->address)) != expected->value)
                values_exact = 0;
        }
        mark_store(expected->address, expected->width);
    }

    mark_store(UINT32_C(0x8006EE78), 2u);
    mark_store(UINT32_C(0x8006EE7A), 2u);
    for (index = 0; index < (u32)(sizeof(s_fixed) / sizeof(s_fixed[0]));
         index++) {
        const ExpectedStore* expected = &s_fixed[index];
        u32 host_offset = expected->address - GS_BASE;
        if (memcmp(s_host + host_offset, PSX_ADDR(expected->address),
                   expected->width) != 0)
            aliases_exact = 0;
    }
    check("all fixed retail stores exact", values_exact);
    check("all GameState stores mirrored", aliases_exact);

    s_guest_mask[UINT32_C(0x8009D160) & UINT32_C(0x1FFFFF)] = 1u;
    s_guest_mask[(UINT32_C(0x8009D160) & UINT32_C(0x1FFFFF)) + 1u] = 1u;
    s_guest_mask[(UINT32_C(0x8009D160) & UINT32_C(0x1FFFFF)) + 2u] = 1u;
    s_guest_mask[(UINT32_C(0x8009D160) & UINT32_C(0x1FFFFF)) + 3u] = 1u;
    check("guest write set exact",
          unchanged_outside_mask(g_PsxRam, s_guest_before, s_guest_mask,
                                 (u32)sizeof(g_PsxRam)));
    check("host write set exact",
          unchanged_outside_mask(s_host, s_host_before, s_host_mask,
                                 (u32)sizeof(s_host)));
    check("lookup tables read-only",
          load_u16(PSX_ADDR(UINT32_C(0x8009AF80) +
                            (u32)table_index * 2u)) == table_x &&
          load_u16(PSX_ADDR(UINT32_C(0x8009AF90) +
                            (u32)table_index * 2u)) == table_z);
}

static void run_null_case(void)
{
    u32 before = load_u32(PSX_ADDR(UINT32_C(0x8009D160)));
    check("null host rejected",
          wm_80070D58_cold_defaults(NULL) == -1);
    check("null host leaves guest unchanged",
          load_u32(PSX_ADDR(UINT32_C(0x8009D160))) == before);
}

int main(void)
{
    run_cold_case();
    run_null_case();
    printf("W34N118 COLD DEFAULTS CERTIFICATE %s (%d/%d)\n",
           s_passes == s_total ? "PASS" : "FAIL", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}

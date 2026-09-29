#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_7565c.h"

#define POOL_PTR 0x8009BE24u
#define POOL     0x80120000u

u8 D_8005A4E4[0x10000u];

static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_source_before[0x2300u];
static int s_failures;

typedef struct WriteEvent {
    u32 destination;
    u32 source_offset;
    u32 size;
} WriteEvent;

static WriteEvent s_events[32];
static u32 s_event_count;

static size_t ram_offset(u32 address)
{
    return (size_t)(address & 0x1FFFFFu);
}

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        s_failures++;
    }
}

void wm_7565c_test_write(u32 destination, u32 source_offset, u32 size)
{
    if (s_event_count < (u32)(sizeof(s_events) / sizeof(s_events[0]))) {
        s_events[s_event_count].destination = destination;
        s_events[s_event_count].source_offset = source_offset;
        s_events[s_event_count].size = size;
        s_event_count++;
    }
}

static void sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void expected_copy(u32 destination, u32 source_offset, u32 size)
{
    memcpy(s_expected_ram + ram_offset(destination),
           D_8005A4E4 + source_offset, (size_t)size);
}

static void expected_sh(u32 destination, u32 source_offset)
{
    memcpy(s_expected_ram + ram_offset(destination),
           D_8005A4E4 + source_offset, 2u);
}

static void expected_sw(u32 destination, u32 source_offset)
{
    memcpy(s_expected_ram + ram_offset(destination),
           D_8005A4E4 + source_offset, 4u);
}

static void build_expected(void)
{
    expected_copy(POOL, 0u, 0x2000u);
    expected_copy(0x8009C5ACu, 0x2000u, 0x10u);
    expected_copy(0x8009D55Cu, 0x2000u, 0x10u);
    expected_sh(0x8009D52Cu, 0x2010u);
    expected_sw(0x8009BE40u, 0x2014u);
    expected_sw(0x8009BCC4u, 0x2018u);
    expected_sw(0x8009D64Cu, 0x201Cu);
    expected_copy(0x8009C854u, 0x2020u, 0x20u);
    expected_copy(0x8009CEC4u, 0x2040u, 0x280u);
    expected_sh(0x8009D154u, 0x22C0u);
    expected_copy(0x8009BD38u, 0x22C4u, 0x08u);
    expected_sw(0x8009D3F0u, 0x22CCu);
    expected_sw(0x8009BE0Cu, 0x22D0u);
    expected_copy(0x8009BBB4u, 0x22D4u, 0x10u);
    expected_copy(0x8009C838u, 0x22E4u, 0x08u);
    expected_copy(0x8009BE28u, 0x22ECu, 0x10u);
}

static int range_equal(u32 destination, u32 source_offset, u32 size)
{
    return memcmp(PSX_ADDR(destination), D_8005A4E4 + source_offset,
                  (size_t)size) == 0;
}

static void seed(void)
{
    u32 i;

    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    for (i = 0u; i < (u32)sizeof(D_8005A4E4); i++)
        D_8005A4E4[i] = (u8)((i * 73u + (i >> 3) + 0x31u) & 0xFFu);
    /* A deliberately different guest mirror detects the pointer-domain
     * split without relying only on the final full-RAM comparison. */
    memset(PSX_ADDR(0x8005A4E4u), 0x5Cu, 0x2300u);
    sw(POOL_PTR, POOL);
    memcpy(s_expected_ram, g_PsxRam, PSX_RAM_SIZE);
    memcpy(s_source_before, D_8005A4E4, sizeof(s_source_before));
    s_event_count = 0u;
    build_expected();
}

static void check_write_order(void)
{
    static const WriteEvent expected[] = {
        { POOL, 0x0000u, 0x2000u },
        { 0x8009C5ACu, 0x2000u, 0x10u },
        { 0x8009D55Cu, 0x2000u, 0x10u },
        { 0x8009C854u, 0x2020u, 0x20u },
        { 0x8009D52Cu, 0x2010u, 0x02u },
        { 0x8009BE40u, 0x2014u, 0x04u },
        { 0x8009BCC4u, 0x2018u, 0x04u },
        { 0x8009D64Cu, 0x201Cu, 0x04u },
        { 0x8009CEC4u, 0x2040u, 0x280u },
        { 0x8009BD38u, 0x22C4u, 0x08u },
        { 0x8009BBB4u, 0x22D4u, 0x10u },
        { 0x8009C838u, 0x22E4u, 0x08u },
        { 0x8009BE28u, 0x22ECu, 0x10u },
        { 0x8009D154u, 0x22C0u, 0x02u },
        { 0x8009D3F0u, 0x22CCu, 0x04u },
        { 0x8009BE0Cu, 0x22D0u, 0x04u }
    };
    u32 i;

    check(s_event_count == (u32)(sizeof(expected) / sizeof(expected[0])),
          "restore.write.order");
    if (s_event_count != (u32)(sizeof(expected) / sizeof(expected[0])))
        return;
    for (i = 0u; i < s_event_count; i++) {
        if (memcmp(&s_events[i], &expected[i], sizeof(expected[i])) != 0) {
            check(0, "restore.write.order");
            return;
        }
    }
}

int main(void)
{
    seed();
    wm_8007565C();

    check(range_equal(POOL, 0u, 0x10u) &&
              *(const u8*)PSX_ADDR(POOL) != 0x5Cu,
          "source.native.authority");
    check(range_equal(POOL + 0x1F80u, 0x1F80u, 0x80u),
          "pool.full.0x2000");
    check(range_equal(0x8009C5ACu, 0x2000u, 0x10u) &&
              range_equal(0x8009D55Cu, 0x2000u, 0x10u),
          "runtime.record.dual.publish");
    check(range_equal(0x8009C854u, 0x2020u, 0x20u),
          "timers.full.0x20");
    check(range_equal(0x8009CEC4u, 0x2040u, 0x280u),
          "ring.full.0x280");
    check(range_equal(0x8009BE28u, 0x22ECu, 0x10u),
          "camera.position.four.words");
    check(memcmp(D_8005A4E4, s_source_before,
                 sizeof(s_source_before)) == 0,
          "snapshot.source.read.only");
    check(memcmp(g_PsxRam, s_expected_ram, PSX_RAM_SIZE) == 0,
          "guest.write.set.exact");
    check_write_order();

    if (s_failures != 0)
        return 1;
    puts("W34N22 7565C RESTORE CERTIFICATE PASS");
    return 0;
}

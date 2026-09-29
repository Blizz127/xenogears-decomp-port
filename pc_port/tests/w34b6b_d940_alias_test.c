/*
 * W34B6-B production-linked focused test for the D940 GameState gear alias.
 *
 * The production helper is the system under test.  Verifies that
 * wm_sync_gamestate_channel_aliases() now also synchronizes the gear/resource
 * byte for each channel's primary character from host GameState to guest PSX RAM.
 *
 * Example build from the repository root:
 *
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Wall -Wextra -Wconversion -Wsign-conversion \
 *     -Ipc_port/tests/include -Ipc_port/include_shim -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b6b_d940_alias_test.c \
 *     pc_port/src/world_map_gamestate_alias.c \
 *     -o /tmp/w34b6b_d940_alias_test
 *
 * Replace -O0 with -O2 for the optimized run.  For the UB gate add:
 *
 *   -fsanitize=undefined -fno-sanitize-recover=undefined
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_gamestate_alias.h"

#define U8(a)  (*(u8*)PSX_ADDR(a))

#define MAX_CHARACTERS         11u
#define GEAR_OFFSET_IN_CHAR    0xA0u
#define GEAR_HOST_BASE         WM_GAMESTATE_GEAR_HOST_BASE
#define GEAR_GUEST_BASE        WM_GAMESTATE_GEAR_GUEST_BASE
#define CHARACTER_STRIDE       WM_GAMESTATE_CHARACTER_STRIDE

/* Buffer must cover host offsets for any u8 character ID (0..0xFE). */
#define HOST_BUFFER_SIZE       (WM_GAMESTATE_GEAR_HOST_BASE + \
                                0xFFu * CHARACTER_STRIDE + 16u)

_Static_assert(WM_GAMESTATE_GEAR_HOST_BASE == 0x030Cu,
               "host gear base changed");
_Static_assert(WM_GAMESTATE_GEAR_GUEST_BASE == 0x8006D940u,
               "guest gear base changed");
_Static_assert(WM_GAMESTATE_CHARACTER_STRIDE == 0xA4u,
               "character stride changed");
_Static_assert(WM_GAMESTATE_CHANNEL_HOST_OFFSET == 0x1D34u,
               "host channel offset changed");
_Static_assert(WM_GAMESTATE_CHANNEL_GUEST_BASE == 0x8006F368u,
               "guest channel base changed");
_Static_assert(WM_GAMESTATE_CHANNEL_COUNT == 3u,
               "channel count changed");

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static u8 s_ram_before[PSX_RAM_SIZE];
static int s_passes;
static int s_total;

static void check(const char* case_name, const char* assertion, int ok)
{
    s_total++;
    if (ok) {
        s_passes++;
        printf("  PASS [%s]: %s\n", case_name, assertion);
    } else {
        printf("  FAIL [%s]: %s\n", case_name, assertion);
    }
}

static void fill_pattern(u8* bytes, size_t count, u32 seed)
{
    size_t i;

    for (i = 0u; i < count; i++) {
        bytes[i] = (u8)(((u32)i * 37u + seed) & 0xFFu);
    }
}

static u32 gear_host_offset(u8 character_id)
{
    return GEAR_HOST_BASE + (u32)character_id * CHARACTER_STRIDE;
}

static u32 gear_guest_addr(u8 character_id)
{
    return GEAR_GUEST_BASE + (u32)character_id * CHARACTER_STRIDE;
}

/* --- Test: Asymmetric channel-to-character mapping --- */
static void test_asymmetric_channels(void)
{
    const char* name = "asymmetric";
    u8 host[HOST_BUFFER_SIZE];
    u32 changed_count = 0u;
    u32 i;

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x41u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x93u);

    /* channel 0 -> char 5, channel 1 -> char 1, channel 2 -> char 7 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 5;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 1;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 7;

    /* Set host gear values */
    host[gear_host_offset(5)] = 0x21u;
    host[gear_host_offset(1)] = 0xB4u;
    host[gear_host_offset(7)] = 0xFEu;

    /* Pre-fill guest gear with unrelated values */
    U8(gear_guest_addr(5)) = 0x00u;
    U8(gear_guest_addr(1)) = 0x00u;
    U8(gear_guest_addr(7)) = 0x00u;

    memcpy(s_ram_before, g_PsxRam, PSX_RAM_SIZE);

    wm_sync_gamestate_channel_aliases(host);

    check(name, "guest char5 gear == 0x21",
          U8(gear_guest_addr(5)) == 0x21u);
    check(name, "guest char1 gear == 0xB4",
          U8(gear_guest_addr(1)) == 0xB4u);
    check(name, "guest char7 gear == 0xFE",
          U8(gear_guest_addr(7)) == 0xFEu);

    /* Check unrelated characters unchanged */
    check(name, "char0 gear unchanged",
          U8(gear_guest_addr(0)) == s_ram_before[gear_guest_addr(0) & 0x1FFFFF]);
    check(name, "char2 gear unchanged",
          U8(gear_guest_addr(2)) == s_ram_before[gear_guest_addr(2) & 0x1FFFFF]);
    check(name, "char3 gear unchanged",
          U8(gear_guest_addr(3)) == s_ram_before[gear_guest_addr(3) & 0x1FFFFF]);
    check(name, "char4 gear unchanged",
          U8(gear_guest_addr(4)) == s_ram_before[gear_guest_addr(4) & 0x1FFFFF]);
    check(name, "char6 gear unchanged",
          U8(gear_guest_addr(6)) == s_ram_before[gear_guest_addr(6) & 0x1FFFFF]);

    /* Count total changed bytes — should be exactly 3 (channels) + 3 (gear) = 6 */
    for (i = 0u; i < PSX_RAM_SIZE; i++) {
        if (g_PsxRam[i] != s_ram_before[i]) {
            changed_count++;
        }
    }
    check(name, "exactly 6 bytes changed (3 channel + 3 gear)",
          changed_count == 6u);
}

/* --- Test: Nonzero sentinel --- */
static void test_nonzero_sentinel(void)
{
    const char* name = "nonzero";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x55u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xAAu);

    /* channel 0 -> char 2 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 2;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* Host gear = 0x5A, guest pre-set to 0x00 */
    host[gear_host_offset(2)] = 0x5Au;
    U8(gear_guest_addr(2)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "guest char2 gear == 0x5A",
          U8(gear_guest_addr(2)) == 0x5Au);

    /* Now overwrite guest with 0xC3 and re-sync */
    U8(gear_guest_addr(2)) = 0xC3u;
    host[gear_host_offset(2)] = 0x5Au;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "authoritative overwrite 0xC3 -> 0x5A",
          U8(gear_guest_addr(2)) == 0x5Au);
}

/* --- Test: Valid character + gear 0xFF --- */
static void test_gear_ff(void)
{
    const char* name = "gear-ff";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x77u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xCCu);

    /* channel 0 -> char 3 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 3;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* Host gear = 0xFF (no gear), guest pre-set to 0x00 */
    host[gear_host_offset(3)] = 0xFFu;
    U8(gear_guest_addr(3)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "guest char3 gear == 0xFF",
          U8(gear_guest_addr(3)) == 0xFFu);
}

/* --- Test: Primary ID 0xFF skips gear sync --- */
static void test_primary_ff(void)
{
    const char* name = "primary-ff";
    u8 host[HOST_BUFFER_SIZE];
    u8 canary0, canary2;

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x88u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xDDu);

    /* channel 0 -> 0xFF (disabled), channel 1 -> char 4, channel 2 -> 0xFF */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 4;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* Set host gear for char 0 and char 2 to something distinct */
    host[gear_host_offset(0)] = 0xABu;
    host[gear_host_offset(2)] = 0xCDu;

    /* Set host gear for char 4 */
    host[gear_host_offset(4)] = 0x42u;

    /* Record canary values at char 0 and char 2 gear positions */
    canary0 = U8(gear_guest_addr(0));
    canary2 = U8(gear_guest_addr(2));

    wm_sync_gamestate_channel_aliases(host);

    check(name, "char0 gear unchanged (channel 0 disabled)",
          U8(gear_guest_addr(0)) == canary0);
    check(name, "char2 gear unchanged (channel 2 disabled)",
          U8(gear_guest_addr(2)) == canary2);
    check(name, "char4 gear == 0x42 (channel 1 active)",
          U8(gear_guest_addr(4)) == 0x42u);
}

/* --- Test: Duplicate primary IDs --- */
static void test_duplicate_ids(void)
{
    const char* name = "duplicate";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x99u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xEEu);

    /* channel 0 -> char 2, channel 1 -> char 2, channel 2 -> char 6 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 2;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 2;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 6;

    host[gear_host_offset(2)] = 0x77u;
    host[gear_host_offset(6)] = 0x88u;

    U8(gear_guest_addr(2)) = 0x00u;
    U8(gear_guest_addr(6)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "char2 gear == 0x77 (written twice, same value)",
          U8(gear_guest_addr(2)) == 0x77u);
    check(name, "char6 gear == 0x88",
          U8(gear_guest_addr(6)) == 0x88u);
}

/* --- Test: Character stride exactness --- */
static void test_character_stride(void)
{
    const char* name = "stride";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0xBBu);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x11u);

    /* Use char 0 and char 1 (adjacent) with distinct values */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 1;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    host[gear_host_offset(0)] = 0x10u;
    host[gear_host_offset(1)] = 0x20u;

    /* Set bytes at wrong strides to detect off-by-one */
    /* +0xA0 (wrong), +0xA8 (wrong) */
    host[GEAR_HOST_BASE + 0 * CHARACTER_STRIDE + 0xA0u - 1u] = 0xEEu;
    host[GEAR_HOST_BASE + 1 * CHARACTER_STRIDE + 0xA0u + 1u] = 0xFFu;

    U8(gear_guest_addr(0)) = 0x00u;
    U8(gear_guest_addr(1)) = 0x00u;

    memcpy(s_ram_before, g_PsxRam, PSX_RAM_SIZE);

    wm_sync_gamestate_channel_aliases(host);

    check(name, "char0 gear == 0x10",
          U8(gear_guest_addr(0)) == 0x10u);
    check(name, "char1 gear == 0x20",
          U8(gear_guest_addr(1)) == 0x20u);

    /* Verify neighbor bytes at -1 and +1 from gear position unchanged */
    check(name, "char0 gear-1 unchanged",
          U8(gear_guest_addr(0) - 1u) ==
              s_ram_before[(gear_guest_addr(0) - 1u) & 0x1FFFFF]);
    check(name, "char0 gear+1 unchanged",
          U8(gear_guest_addr(0) + 1u) ==
              s_ram_before[(gear_guest_addr(0) + 1u) & 0x1FFFFF]);
    check(name, "char1 gear-1 unchanged",
          U8(gear_guest_addr(1) - 1u) ==
              s_ram_before[(gear_guest_addr(1) - 1u) & 0x1FFFFF]);
    check(name, "char1 gear+1 unchanged",
          U8(gear_guest_addr(1) + 1u) ==
              s_ram_before[(gear_guest_addr(1) + 1u) & 0x1FFFFF]);
}

/* --- Test: Channel-index mutant detection --- */
static void test_channel_index_mutant(void)
{
    const char* name = "ch-index-mutant";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0xCCu);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x33u);

    /* channel 0 -> char 5 (not char 0!) */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 5;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    host[gear_host_offset(5)] = 0x55u;
    host[gear_host_offset(0)] = 0x01u;  /* canary at char 0 */

    U8(gear_guest_addr(5)) = 0x00u;
    U8(gear_guest_addr(0)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    /* If mutant uses channel index instead of character ID, char 0 gets 0x01 */
    check(name, "MUTANT DETECTED: char5 gear == 0x55 (not channel 0)",
          U8(gear_guest_addr(5)) == 0x55u);
    check(name, "MUTANT DETECTED: char0 gear unchanged (not char 0)",
          U8(gear_guest_addr(0)) == 0x00u);
}

/* --- Test: Fixed characters 0/1/2 mutant detection --- */
static void test_fixed_012_mutant(void)
{
    const char* name = "fixed-012";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0xDDu);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x44u);

    /* channel 0 -> char 8, channel 1 -> char 3, channel 2 -> char 10 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 8;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 3;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 10;

    host[gear_host_offset(8)] = 0x88u;
    host[gear_host_offset(3)] = 0x33u;
    host[gear_host_offset(10)] = 0xAAu;

    /* Set canary at char 0, 1, 2 */
    host[gear_host_offset(0)] = 0x01u;
    host[gear_host_offset(1)] = 0x02u;
    host[gear_host_offset(2)] = 0x03u;

    U8(gear_guest_addr(8)) = 0x00u;
    U8(gear_guest_addr(3)) = 0x00u;
    U8(gear_guest_addr(10)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "MUTANT DETECTED: char8 gear == 0x88",
          U8(gear_guest_addr(8)) == 0x88u);
    check(name, "MUTANT DETECTED: char3 gear == 0x33",
          U8(gear_guest_addr(3)) == 0x33u);
    check(name, "MUTANT DETECTED: char10 gear == 0xAA",
          U8(gear_guest_addr(10)) == 0xAAu);
}

/* --- Test: Wrong stride mutant detection --- */
static void test_wrong_stride_mutant(void)
{
    const char* name = "wrong-stride";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0xEEu);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x55u);

    /* channel 0 -> char 1 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 1;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* Set gear at correct offset 0x030C + 1*0xA4 = 0x03B0 */
    host[gear_host_offset(1)] = 0x42u;

    /* Set canary at wrong strides: 0xA0 and 0xA8 */
    /* char 1 with stride 0xA0: 0x030C + 0xA0 = 0x03AC */
    host[GEAR_HOST_BASE + 1 * 0xA0u] = 0xA0u;
    /* char 1 with stride 0xA8: 0x030C + 0xA8 = 0x03B4 */
    host[GEAR_HOST_BASE + 1 * 0xA8u] = 0xA8u;

    U8(gear_guest_addr(1)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "MUTANT DETECTED: char1 gear == 0x42 (stride 0xA4)",
          U8(gear_guest_addr(1)) == 0x42u);
}

/* --- Test: Wrong host offset mutant detection --- */
static void test_wrong_host_offset_mutant(void)
{
    const char* name = "wrong-host-off";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0xFFu);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x66u);

    /* channel 0 -> char 0 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* Correct: host offset 0x030C */
    host[gear_host_offset(0)] = 0x42u;

    /* Canary at -1 and +1 from host offset */
    host[GEAR_HOST_BASE - 1u] = 0xB1u;
    host[GEAR_HOST_BASE + 1u] = 0xB2u;

    U8(gear_guest_addr(0)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "MUTANT DETECTED: char0 gear == 0x42 (offset 0x030C)",
          U8(gear_guest_addr(0)) == 0x42u);
}

/* --- Test: Wrong guest base mutant detection --- */
static void test_wrong_guest_base_mutant(void)
{
    const char* name = "wrong-guest-base";
    u8 host[HOST_BUFFER_SIZE];
    u32 wrong_base = 0x8007D940u;  /* +0x10000 error */

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x11u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x22u);

    /* channel 0 -> char 0 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    host[gear_host_offset(0)] = 0x42u;

    /* Set canary at wrong guest base */
    U8(wrong_base + 0 * CHARACTER_STRIDE) = 0xABu;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "correct D940 updated: char0 gear == 0x42",
          U8(gear_guest_addr(0)) == 0x42u);
    check(name, "wrong +0x10000 address unchanged",
          U8(wrong_base + 0 * CHARACTER_STRIDE) == 0xABu);
}

/* --- Test: Neighbor canaries --- */
static void test_neighbor_canaries(void)
{
    const char* name = "canaries";
    u8 host[HOST_BUFFER_SIZE];
    u32 ch;

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x33u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x77u);

    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 1;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 2;

    host[gear_host_offset(0)] = 0x10u;
    host[gear_host_offset(1)] = 0x20u;
    host[gear_host_offset(2)] = 0x30u;

    memcpy(s_ram_before, g_PsxRam, PSX_RAM_SIZE);

    wm_sync_gamestate_channel_aliases(host);

    for (ch = 0; ch < 3; ch++) {
        u32 ga = gear_guest_addr((u8)ch);
        char label[64];

        snprintf(label, sizeof(label), "char%u gear-1 preserved", ch);
        check(name, label,
              U8(ga - 1u) == s_ram_before[(ga - 1u) & 0x1FFFFF]);

        snprintf(label, sizeof(label), "char%u gear+1 preserved", ch);
        check(name, label,
              U8(ga + 1u) == s_ram_before[(ga + 1u) & 0x1FFFFF]);
    }
}

/* --- Test: Restored/nonzero state fixture --- */
static void test_nonzero_fixture(void)
{
    const char* name = "nonzero-fixture";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x44u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x88u);

    /* Nonsequential character IDs with nonzero gear values */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 6;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 2;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 9;

    host[gear_host_offset(6)] = 0x60u;
    host[gear_host_offset(2)] = 0x20u;
    host[gear_host_offset(9)] = 0x90u;

    /* Pre-fill guest with stale values */
    U8(gear_guest_addr(6)) = 0x01u;
    U8(gear_guest_addr(2)) = 0x02u;
    U8(gear_guest_addr(9)) = 0x03u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "char6 gear == 0x60",
          U8(gear_guest_addr(6)) == 0x60u);
    check(name, "char2 gear == 0x20",
          U8(gear_guest_addr(2)) == 0x20u);
    check(name, "char9 gear == 0x90",
          U8(gear_guest_addr(9)) == 0x90u);
}

/* --- Test: Stale guest primary — hard gate --- */
static void test_stale_guest_primary(void)
{
    const char* name = "stale-guest";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x61u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0x92u);

    /* HOST channel mapping: ch0->5, ch1->1, ch2->7 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 5;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 1;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 7;

    /* STALE GUEST mapping before sync: ch0->0, ch1->2, ch2->FF */
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) = 0;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 1) = 2;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 2) = 0xFFu;

    /* Host gear values for host-selected characters */
    host[gear_host_offset(5)] = 0x21u;
    host[gear_host_offset(1)] = 0xB4u;
    host[gear_host_offset(7)] = 0xFEu;

    /* Stale-target records: char0 and char2 with distinctive values */
    host[gear_host_offset(0)] = 0xAAu;
    host[gear_host_offset(2)] = 0xBBu;

    /* Pre-fill guest gear for stale targets */
    U8(gear_guest_addr(0)) = 0x11u;
    U8(gear_guest_addr(2)) = 0x22u;
    U8(gear_guest_addr(5)) = 0x00u;
    U8(gear_guest_addr(1)) = 0x00u;
    U8(gear_guest_addr(7)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    /* F368/F369/F36A must be host values */
    check(name, "F368 == 5",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) == 5);
    check(name, "F369 == 1",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 1) == 1);
    check(name, "F36A == 7",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 2) == 7);

    /* Gear must be host-selected characters, not stale guest */
    check(name, "guest char5 gear == 0x21",
          U8(gear_guest_addr(5)) == 0x21u);
    check(name, "guest char1 gear == 0xB4",
          U8(gear_guest_addr(1)) == 0xB4u);
    check(name, "guest char7 gear == 0xFE",
          U8(gear_guest_addr(7)) == 0xFEu);

    /* Stale guest-selected char0/char2 gear must be UNCHANGED */
    check(name, "stale char0 gear unchanged (0x11)",
          U8(gear_guest_addr(0)) == 0x11u);
    check(name, "stale char2 gear unchanged (0x22)",
          U8(gear_guest_addr(2)) == 0x22u);
}

/* --- Test: Stale-primary mutant detection --- */
/* Models the wrong algorithm: read guest first, then update F368, then sync D940 */

static void mutant_sync(const u8* host_gamestate)
{
    u8* guest_channels =
        (u8*)PSX_ADDR(WM_GAMESTATE_CHANNEL_GUEST_BASE);
    u32 channel;

    for (channel = 0; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        /* WRONG: read stale guest primary_id BEFORE updating F368 */
        u8 stale_id = guest_channels[channel];

        /* Update F368 */
        guest_channels[channel] =
            host_gamestate[WM_GAMESTATE_CHANNEL_HOST_OFFSET + channel];

        /* WRONG: use stale guest ID for D940 sync */
        if (stale_id != 0xFFu) {
            u32 host_offset = WM_GAMESTATE_GEAR_HOST_BASE +
                              (u32)stale_id * WM_GAMESTATE_CHARACTER_STRIDE;
            u32 guest_addr = WM_GAMESTATE_GEAR_GUEST_BASE +
                             (u32)stale_id * WM_GAMESTATE_CHARACTER_STRIDE;
            *(u8*)PSX_ADDR(guest_addr) = host_gamestate[host_offset];
        }
    }
}

static void test_stale_primary_mutant(void)
{
    const char* name = "stale-mutant";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x71u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xA3u);

    /* HOST: ch0->5 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 5;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* STALE GUEST: ch0->0 */
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) = 0;

    /* Host gear: char5=0x21, char0=0xAA */
    host[gear_host_offset(5)] = 0x21u;
    host[gear_host_offset(0)] = 0xAAu;

    U8(gear_guest_addr(5)) = 0x00u;
    U8(gear_guest_addr(0)) = 0x00u;

    /* Run the MUTANT model */
    mutant_sync(host);

    /* The mutant syncs char0 (stale) instead of char5 (host) */
    int mutant_synced_char0 = (U8(gear_guest_addr(0)) == 0xAAu);
    int mutant_synced_char5 = (U8(gear_guest_addr(5)) == 0x21u);

    check(name, "MUTANT DETECTED: mutant syncs stale char0",
          mutant_synced_char0);
    check(name, "MUTANT DETECTED: mutant does NOT sync host char5",
          !mutant_synced_char5);

    /* Now run the CORRECT production sync on fresh state */
    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x71u);
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) = 0;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 1) = 0xFFu;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 2) = 0xFFu;
    U8(gear_guest_addr(5)) = 0x00u;
    U8(gear_guest_addr(0)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "production syncs host char5 (not stale char0)",
          U8(gear_guest_addr(5)) == 0x21u);
    check(name, "production leaves stale char0 unchanged",
          U8(gear_guest_addr(0)) == 0x00u);
}

/* --- Test: Host primary FF with stale guest valid --- */
static void test_primary_ff_stale_guest(void)
{
    const char* name = "host-ff-stale";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x82u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xD4u);

    /* HOST: ch0->FF (disabled) */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* STALE GUEST: ch0->3 (valid!) */
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) = 3;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 1) = 0xFFu;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 2) = 0xFFu;

    /* Host gear for char3 */
    host[gear_host_offset(3)] = 0xCDu;

    /* Guest char3 gear canary */
    U8(gear_guest_addr(3)) = 0x55u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "F368 == FF (host value)",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) == 0xFFu);
    check(name, "stale char3 gear NOT written (host ch0 is FF)",
          U8(gear_guest_addr(3)) == 0x55u);
}

/* --- Test: Host valid / stale guest FF --- */
static void test_host_valid_guest_ff(void)
{
    const char* name = "host-valid-stale-ff";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x93u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xE5u);

    /* HOST: ch0->5 (valid) */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 5;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0xFFu;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0xFFu;

    /* STALE GUEST: ch0->FF */
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) = 0xFFu;

    /* Host gear char5 = 0xFF (valid char, no gear) */
    host[gear_host_offset(5)] = 0xFFu;

    U8(gear_guest_addr(5)) = 0x00u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "F368 == 5 (host value)",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) == 5);
    check(name, "char5 gear == 0xFF (valid char, gear sentinel)",
          U8(gear_guest_addr(5)) == 0xFFu);
}

/* --- Test: Duplicate host IDs with stale guest IDs --- */
static void test_duplicate_stale(void)
{
    const char* name = "dup-stale";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0xA4u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xF6u);

    /* HOST: ch0->2, ch1->2, ch2->6 */
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 2;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 2;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 6;

    /* STALE GUEST: ch0->9, ch1->4, ch2->FF */
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) = 9;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 1) = 4;
    U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 2) = 0xFFu;

    /* Host gear */
    host[gear_host_offset(2)] = 0x77u;
    host[gear_host_offset(6)] = 0x88u;

    /* Stale-target host gear (should NOT be synced) */
    host[gear_host_offset(9)] = 0x99u;
    host[gear_host_offset(4)] = 0x44u;

    /* Guest gear canaries */
    U8(gear_guest_addr(2)) = 0x00u;
    U8(gear_guest_addr(6)) = 0x00u;
    U8(gear_guest_addr(9)) = 0x11u;
    U8(gear_guest_addr(4)) = 0x22u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "char2 gear == 0x77",
          U8(gear_guest_addr(2)) == 0x77u);
    check(name, "char6 gear == 0x88",
          U8(gear_guest_addr(6)) == 0x88u);
    check(name, "stale char9 gear unchanged",
          U8(gear_guest_addr(9)) == 0x11u);
    check(name, "stale char4 gear unchanged",
          U8(gear_guest_addr(4)) == 0x22u);
}

/* --- Test: F368-F36A still correct (regression) --- */
static void test_f368_regression(void)
{
    const char* name = "f368-regression";
    u8 host[HOST_BUFFER_SIZE];

    printf("\n=== %s ===\n", name);

    fill_pattern(g_PsxRam, PSX_RAM_SIZE, 0x55u);
    fill_pattern(host, HOST_BUFFER_SIZE, 0xAAu);

    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 0] = 0x12u;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 1] = 0x34u;
    host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + 2] = 0x56u;

    wm_sync_gamestate_channel_aliases(host);

    check(name, "F368 == 0x12",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 0) == 0x12u);
    check(name, "F369 == 0x34",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 1) == 0x34u);
    check(name, "F36A == 0x56",
          U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + 2) == 0x56u);
}

int main(void)
{
    printf("W34B6-B D940 GameState gear alias test\n");
    printf("=======================================\n");

    test_asymmetric_channels();
    test_nonzero_sentinel();
    test_gear_ff();
    test_primary_ff();
    test_duplicate_ids();
    test_character_stride();
    test_channel_index_mutant();
    test_fixed_012_mutant();
    test_wrong_stride_mutant();
    test_wrong_host_offset_mutant();
    test_wrong_guest_base_mutant();
    test_neighbor_canaries();
    test_nonzero_fixture();
    test_stale_guest_primary();
    test_stale_primary_mutant();
    test_primary_ff_stale_guest();
    test_host_valid_guest_ff();
    test_duplicate_stale();
    test_f368_regression();

    printf("\n=======================================\n");
    printf("RESULT: %d/%d PASS\n", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}

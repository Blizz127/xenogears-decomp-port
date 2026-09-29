/*
 * W34B5-V production-linked focused test for the narrow GameState channel
 * alias bridge.
 *
 * The production helper is the system under test.  The fake package
 * publication below is deliberately test-only: it models only mode-init's
 * channel-active decision after synchronization, and never substitutes for
 * the production world mode initializer or allocator.  Natural-route tests
 * cover that integration separately.
 *
 * Example build from the repository root:
 *
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Wall -Wextra -Wconversion -Wsign-conversion \
 *     -Ipc_port/tests/include -Ipc_port/include_shim -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5v_gamestate_channel_alias_test.c \
 *     pc_port/src/world_map_gamestate_alias.c \
 *     -o /tmp/w34b5v_gamestate_channel_alias_test
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
#define U32(a) (*(u32*)PSX_ADDR(a))

#define WM_MODE_FLAGS_BASE 0x8006F8E5u
#define WM_PACKAGE_BASE    0x8009CD34u
#define WM_GUEST_MASK      0x001FFFFFu
#define HOST_GUARD_BYTES   2u
#define GUEST_GUARD_BYTES  2u
/* Buffer must cover host offsets for any u8 character ID (0..0xFE) gear field. */
#define HOST_BUFFER_BYTES  (WM_GAMESTATE_GEAR_HOST_BASE + \
                            0xFFu * WM_GAMESTATE_CHARACTER_STRIDE + 16u)

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

typedef struct ChannelAliasCase {
    const char* name;
    u8 channel[WM_GAMESTATE_CHANNEL_COUNT];
} ChannelAliasCase;

static const ChannelAliasCase s_cases[] = {
    { "natural",      { 0x00u, 0x02u, 0xFFu } },
    { "all-disabled", { 0xFFu, 0xFFu, 0xFFu } },
    { "cold-default", { 0x00u, 0x0Au, 0x05u } },
    { "high-bits",    { 0x80u, 0xFEu, 0x7Fu } },
    { "ordinary",     { 0x12u, 0x34u, 0x56u } },
};

static const char* const s_guest_value_check[WM_GAMESTATE_CHANNEL_COUNT] = {
    "guest channel 0 exact",
    "guest channel 1 exact",
    "guest channel 2 exact",
};

static const char* const s_package_decision_check[WM_GAMESTATE_CHANNEL_COUNT] = {
    "fake package decision 0",
    "fake package decision 1",
    "fake package decision 2",
};

static const char* const s_package_coherence_check[WM_GAMESTATE_CHANNEL_COUNT] = {
    "guard/package coherence 0",
    "guard/package coherence 1",
    "guard/package coherence 2",
};

static void check_result(const char* case_name, const char* assertion, int ok)
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
        u32 index_bits = (u32)i;
        bytes[i] = (u8)((index_bits * 37u + seed) & 0xFFu);
    }
}

/* Controlled seam/model for only the package publication decision.  The
 * tokens are non-dereferenceable sentinels; no allocator or callback is
 * invoked.  Crucially, the decision reads the post-sync guest IDs. */
static void fake_publish_packages_from_guest(u8 published[3])
{
    u32 channel;

    for (channel = 0u; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        u8 guest_id = U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + channel);
        int active = guest_id != 0xFFu;

        published[channel] = active ? 1u : 0u;
        U32(WM_PACKAGE_BASE + channel * 4u) =
            active ? (0x81000000u | (channel << 16) | (u32)guest_id) : 0u;
    }
}

static void run_case(const ChannelAliasCase* test_case, u32 case_index)
{
    u8 host[HOST_BUFFER_BYTES];
    u8 host_before[HOST_BUFFER_BYTES];
    u8 fake_published[WM_GAMESTATE_CHANNEL_COUNT] = { 0u, 0u, 0u };
    const u32 guest_index = WM_GAMESTATE_CHANNEL_GUEST_BASE & WM_GUEST_MASK;
    const u32 flags_index = WM_MODE_FLAGS_BASE & WM_GUEST_MASK;
    const u32 package_index = WM_PACKAGE_BASE & WM_GUEST_MASK;
    size_t changed_count = 0u;
    size_t unauthorized_count = 0u;
    size_t i;
    u32 channel;

    fill_pattern(g_PsxRam, (size_t)PSX_RAM_SIZE,
                 0x41u + case_index * 0x13u);
    fill_pattern(host, (size_t)HOST_BUFFER_BYTES,
                 0x93u + case_index * 0x17u);

    for (channel = 0u; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        host[WM_GAMESTATE_CHANNEL_HOST_OFFSET + channel] =
            test_case->channel[channel];
        U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + channel) =
            (u8)(test_case->channel[channel] ^ 0xFFu);
        U32(WM_PACKAGE_BASE + channel * 4u) =
            0xD0000000u | (case_index << 12) | (channel << 4) | 0xDu;
    }

    U8(WM_MODE_FLAGS_BASE + 0u) = 0xA1u;
    U8(WM_MODE_FLAGS_BASE + 1u) = 0xB2u;
    U8(WM_MODE_FLAGS_BASE + 2u) = 0xC3u;

    memcpy(host_before, host, sizeof(host));
    memcpy(s_ram_before, g_PsxRam, sizeof(s_ram_before));

    wm_sync_gamestate_channel_aliases(host);

    for (channel = 0u; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        check_result(test_case->name, s_guest_value_check[channel],
                     U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + channel) ==
                         test_case->channel[channel]);
    }

    check_result(test_case->name, "host buffer entirely unchanged",
                 memcmp(host, host_before, sizeof(host)) == 0);
    check_result(test_case->name, "host leading adjacent canaries",
                 memcmp(host + WM_GAMESTATE_CHANNEL_HOST_OFFSET -
                            HOST_GUARD_BYTES,
                        host_before + WM_GAMESTATE_CHANNEL_HOST_OFFSET -
                            HOST_GUARD_BYTES,
                        HOST_GUARD_BYTES) == 0);
    check_result(test_case->name, "host trailing adjacent canaries",
                 memcmp(host + WM_GAMESTATE_CHANNEL_HOST_OFFSET +
                            WM_GAMESTATE_CHANNEL_COUNT,
                        host_before + WM_GAMESTATE_CHANNEL_HOST_OFFSET +
                            WM_GAMESTATE_CHANNEL_COUNT,
                        HOST_GUARD_BYTES) == 0);

    for (i = 0u; i < (size_t)PSX_RAM_SIZE; i++) {
        if (g_PsxRam[i] != s_ram_before[i]) {
            changed_count++;
            /* Authorized writes: channel triplet + gear bytes for valid IDs */
            if (i >= (size_t)guest_index &&
                i < (size_t)(guest_index + WM_GAMESTATE_CHANNEL_COUNT)) {
                continue; /* channel byte */
            }
            {
                int is_gear = 0;
                u32 ch;
                for (ch = 0u; ch < WM_GAMESTATE_CHANNEL_COUNT; ch++) {
                    u8 pid = test_case->channel[ch];
                    if (pid != 0xFFu) {
                        u32 gear_addr = WM_GAMESTATE_GEAR_GUEST_BASE +
                                        (u32)pid * WM_GAMESTATE_CHARACTER_STRIDE;
                        if (i == (size_t)(gear_addr & WM_GUEST_MASK)) {
                            is_gear = 1;
                            break;
                        }
                    }
                }
                if (!is_gear) {
                    unauthorized_count++;
                }
            }
        }
    }
    {
        size_t expected = (size_t)WM_GAMESTATE_CHANNEL_COUNT;
        u32 ch;
        for (ch = 0u; ch < WM_GAMESTATE_CHANNEL_COUNT; ch++) {
            if (test_case->channel[ch] != 0xFFu) {
                expected++;
            }
        }
        check_result(test_case->name, "correct number of guest bytes changed",
                     changed_count == expected);
    }
    check_result(test_case->name, "no unauthorized guest writes",
                 unauthorized_count == 0u);
    check_result(test_case->name, "guest leading adjacent canaries",
                 memcmp(g_PsxRam + guest_index - GUEST_GUARD_BYTES,
                        s_ram_before + guest_index - GUEST_GUARD_BYTES,
                        GUEST_GUARD_BYTES) == 0);
    check_result(test_case->name, "guest trailing adjacent canaries",
                 memcmp(g_PsxRam + guest_index +
                            WM_GAMESTATE_CHANNEL_COUNT,
                        s_ram_before + guest_index +
                            WM_GAMESTATE_CHANNEL_COUNT,
                        GUEST_GUARD_BYTES) == 0);
    check_result(test_case->name, "F8E5..F8E7 unchanged",
                 memcmp(g_PsxRam + flags_index,
                        s_ram_before + flags_index,
                        WM_GAMESTATE_CHANNEL_COUNT) == 0);
    check_result(test_case->name, "package table unchanged by bridge",
                 memcmp(g_PsxRam + package_index,
                        s_ram_before + package_index,
                        WM_GAMESTATE_CHANNEL_COUNT * sizeof(u32)) == 0);

    fake_publish_packages_from_guest(fake_published);
    for (channel = 0u; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        u32 package_bits = U32(WM_PACKAGE_BASE + channel * 4u);
        int expected_active = test_case->channel[channel] != 0xFFu;

        check_result(test_case->name, s_package_decision_check[channel],
                     (fake_published[channel] != 0u) == expected_active);
        check_result(test_case->name, s_package_coherence_check[channel],
                     (U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + channel) !=
                          0xFFu) == (package_bits != 0u));
    }
}

int main(void)
{
    u32 case_index;

    printf("W34B5-V GameState channel alias production test\n");
    printf("===============================================\n");

    for (case_index = 0u;
         case_index < (u32)(sizeof(s_cases) / sizeof(s_cases[0]));
         case_index++) {
        printf("\n=== %s ===\n", s_cases[case_index].name);
        run_case(&s_cases[case_index], case_index);
    }

    printf("\n===============================================\n");
    printf("RESULT: %d/%d PASS\n", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}

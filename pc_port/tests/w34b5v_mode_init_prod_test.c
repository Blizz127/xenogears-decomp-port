/*
 * W34B5-V production-linked package/guard coherence test.
 *
 * This test includes the actual port world_map_init.c translation unit so it
 * can exercise its static wm_80071CDC_mode_init body.  Function/data sections
 * plus linker garbage collection retain only that mode-init path and the
 * controlled seams below; the alias bridge itself is linked from its real
 * production source.
 *
 * Build from the repository root:
 *
 *   gcc -std=gnu17 -O0 -g -fpermissive -fno-builtin -include assert.h \
 *     -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -DUSE_EXTENDED_PRIM_POINTERS=0 \
 *     -ffunction-sections -fdata-sections -Wl,--gc-sections \
 *     -Ipc_port/include_shim -Iinclude \
 *     -Ipc_port/extern/PsyCross/include \
 *     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src \
 *     pc_port/tests/w34b5v_mode_init_prod_test.c \
 *     pc_port/src/world_map_gamestate_alias.c -o /tmp/w34b5v_mode_init
 */
#include <stdint.h>
#include <stdio.h>

#include "../src/world_map_init.c"

#define HOST_STATE_SIZE 0xB000u
#define TEST_HEAP_BASE  0x10000u
#define TEST_HEAP_STEP  0x100u

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void* g_pGameState;

static u8 s_host_state[HOST_STATE_SIZE];
static u32 s_decode_args[16];
static u32 s_alloc_sizes[16];
static u32 s_alloc_flags[16];
static int s_decode_count;
static int s_alloc_count;
static int s_queue_count;
static int s_passes;
static int s_total;

static void fill_bytes(void* destination, u8 value, size_t byte_count)
{
    u8* bytes = (u8*)destination;
    size_t index;

    for (index = 0u; index < byte_count; index++)
        bytes[index] = value;
}

typedef struct ModeCase {
    const char* name;
    u8 ids[WM_GAMESTATE_CHANNEL_COUNT];
} ModeCase;

static const ModeCase s_cases[] = {
    { "natural",      { 0x00u, 0x02u, 0xFFu } },
    { "all-disabled", { 0xFFu, 0xFFu, 0xFFu } },
    { "cold-default", { 0x00u, 0x0Au, 0x05u } },
    { "ordinary",     { 0x12u, 0x34u, 0x56u } },
    { "middle-empty", { 0x01u, 0xFFu, 0x03u } },
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

int ArchiveDecodeAlignedSize(unsigned int entry_index)
{
    if (s_decode_count < (int)(sizeof(s_decode_args) / sizeof(s_decode_args[0])))
        s_decode_args[s_decode_count] = (u32)entry_index;
    s_decode_count++;
    return (int)(0x20u + (u32)entry_index);
}

void* HeapAlloc(u_int alloc_size, u_int alloc_flags)
{
    u32 offset = TEST_HEAP_BASE + (u32)s_alloc_count * TEST_HEAP_STEP;

    if (s_alloc_count < (int)(sizeof(s_alloc_sizes) / sizeof(s_alloc_sizes[0]))) {
        s_alloc_sizes[s_alloc_count] = (u32)alloc_size;
        s_alloc_flags[s_alloc_count] = (u32)alloc_flags;
    }
    s_alloc_count++;
    return g_PsxRam + offset;
}

int func_80029AFC(void* entries, int arg1, int arg2)
{
    s_queue_count++;
    check_result("queue seam", "request pointer exact",
                 entries == PSX_ADDR(WM_REQ_D3F8));
    check_result("queue seam", "queue args zero", arg1 == 0 && arg2 == 0);
    return 7;
}

static void reset_case(const ModeCase* test_case)
{
    u32 channel;

    fill_bytes(g_PsxRam, 0xA5u, sizeof(g_PsxRam));
    fill_bytes(s_host_state, 0x5Au, sizeof(s_host_state));
    fill_bytes(s_decode_args, 0u, sizeof(s_decode_args));
    fill_bytes(s_alloc_sizes, 0u, sizeof(s_alloc_sizes));
    fill_bytes(s_alloc_flags, 0u, sizeof(s_alloc_flags));
    s_decode_count = 0;
    s_alloc_count = 0;
    s_queue_count = 0;
    g_pGameState = s_host_state;

    for (channel = 0u; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        u8 id = test_case->ids[channel];
        s_host_state[WM_GAMESTATE_CHANNEL_HOST_OFFSET + channel] = id;
        if (id != 0xFFu)
            s_host_state[GS_OFF_SEC_BASE + (u32)id * 164u] = 0xFFu;
    }

    WM_U8(WM_GAMESTATE_CHANNEL_GUEST_BASE - 1u) = 0xC7u;
    WM_U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + WM_GAMESTATE_CHANNEL_COUNT) = 0xD8u;
    WM_U8(0x8006F8E5u) = 0x31u;
    WM_U8(0x8006F8E6u) = 0x42u;
    WM_U8(0x8006F8E7u) = 0x53u;
}

static void run_case(const ModeCase* test_case)
{
    u32 expected_active = 0u;
    u32 request_index = 0u;
    u32 channel;
    int result;

    reset_case(test_case);
    result = wm_80071CDC_mode_init();

    check_result(test_case->name, "mode init return propagated", result == 7);
    check_result(test_case->name, "queue submitted exactly once",
                 s_queue_count == 1);

    for (channel = 0u; channel < WM_GAMESTATE_CHANNEL_COUNT; channel++) {
        u8 id = test_case->ids[channel];
        u8 guest_id = WM_U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + channel);
        u32 package = WM_U32(WM_PTR_CD34 + channel * 4u);
        char label[80];

        snprintf(label, sizeof(label), "channel %u guest ID exact", channel);
        check_result(test_case->name, label, guest_id == id);

        snprintf(label, sizeof(label), "channel %u guard/package coherent", channel);
        check_result(test_case->name, label,
                     (guest_id == 0xFFu) == (package == 0u));

        snprintf(label, sizeof(label), "channel %u secondary package absent", channel);
        check_result(test_case->name, label,
                     WM_U32(WM_PTR_BDF8 + channel * 4u) == 0u);

        if (id != 0xFFu) {
            u16 request_id = WM_U16(WM_REQ_D3F8 + request_index * 8u);
            u32 request_package = WM_U32(WM_REQ_D3F8 + request_index * 8u + 4u);

            snprintf(label, sizeof(label), "channel %u request ID", channel);
            check_result(test_case->name, label, request_id == (u16)(id + 2u));
            snprintf(label, sizeof(label), "channel %u request package", channel);
            check_result(test_case->name, label, request_package == package);
            expected_active++;
            request_index++;
        }
    }

    check_result(test_case->name, "primary allocation count",
                 s_alloc_count == (int)expected_active);
    check_result(test_case->name, "size-query count",
                 s_decode_count == (int)expected_active);
    check_result(test_case->name, "retail primary counter",
                 WM_U32(WM_CNT_C170) == expected_active);
    check_result(test_case->name, "request terminator ID",
                 WM_U16(WM_REQ_D3F8 + request_index * 8u) == 0u);
    check_result(test_case->name, "request terminator package",
                 WM_U32(WM_REQ_D3F8 + request_index * 8u + 4u) == 0u);
    check_result(test_case->name, "guest leading canary",
                 WM_U8(WM_GAMESTATE_CHANNEL_GUEST_BASE - 1u) == 0xC7u);
    check_result(test_case->name, "guest trailing canary",
                 WM_U8(WM_GAMESTATE_CHANNEL_GUEST_BASE +
                       WM_GAMESTATE_CHANNEL_COUNT) == 0xD8u);
    check_result(test_case->name, "F8E5 unchanged", WM_U8(0x8006F8E5u) == 0x31u);
    check_result(test_case->name, "F8E6 unchanged", WM_U8(0x8006F8E6u) == 0x42u);
    check_result(test_case->name, "F8E7 unchanged", WM_U8(0x8006F8E7u) == 0x53u);
}

int main(void)
{
    u32 case_index;

    printf("W34B5-V actual mode-init package coherence test\n");
    printf("================================================\n");
    for (case_index = 0u;
         case_index < (u32)(sizeof(s_cases) / sizeof(s_cases[0]));
         case_index++) {
        run_case(&s_cases[case_index]);
    }
    printf("================================================\n");
    printf("RESULT: %d/%d PASS\n", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}

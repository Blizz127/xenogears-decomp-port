/* W34N7 — retail base-world slot-2 teardown 0x8007299C certificate. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_7565c.h"
#include "world_map_teardown_7299c.h"

#define D7CC       0x8009D7CCu
#define POOL_PTR   0x8009BE24u
#define POOL_GUEST 0x80100000u
#define OBJ_GUEST  0x80110000u
#define ALLOC_BASE 0x80120000u

extern void wm_80071034_test_dispatch_slot2(void);

typedef struct Event {
    char kind;
    u32 value;
} Event;

void* D_80062528;
void* D_8006259C;
void* g_GfxWorkBuffers;
s32 D_80059190;
u16 D_8006F954;
u8 D_8005A4E4[0x10000u];

/* The dispatcher object also contains the independently tested slot-1 arm. */
int wm_80072238(void) { return 0; }
int wm_80077214(void) { return 0; }
void wm_80077480(void) {}
int wm_80077A64(void) { return 0; }
void wm_80077CC0(void) {}

static Event s_events[1024];
static u32 s_event_count;
static int s_failures;
static u32 s_poll_values[2];
static u32 s_poll_count;

typedef struct SnapshotEvent {
    u32 offset;
    u32 size;
} SnapshotEvent;

static SnapshotEvent s_snapshot_events[32];
static u32 s_snapshot_event_count;

#define ASSERT_MSG(condition, name, ...) do { \
    if (!(condition)) { \
        s_failures++; \
        fprintf(stderr, "ASSERTION %s FAILED: ", name); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } \
} while (0)

static void sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 snapshot_lw(u32 offset)
{
    u32 value;
    memcpy(&value, D_8005A4E4 + offset, sizeof(value));
    return value;
}

static void sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 pointer_value(const void* ptr)
{
    uintptr_t host = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (ptr != NULL && host >= base && host < base + (uintptr_t)PSX_RAM_SIZE)
        return 0x80000000u | (u32)(host - base);
    return (u32)host;
}

static void event(char kind, u32 value)
{
    if (s_event_count < (u32)(sizeof(s_events) / sizeof(s_events[0]))) {
        s_events[s_event_count].kind = kind;
        s_events[s_event_count].value = value;
        s_event_count++;
    }
}

void wm_7299c_test_snapshot_write(u32 offset, u32 size)
{
    if (s_snapshot_event_count <
        (u32)(sizeof(s_snapshot_events) / sizeof(s_snapshot_events[0]))) {
        s_snapshot_events[s_snapshot_event_count].offset = offset;
        s_snapshot_events[s_snapshot_event_count].size = size;
        s_snapshot_event_count++;
    }
}

void func_8003A89C(void* manager, s32 level, s32 steps)
{
    ASSERT_MSG(manager == D_80062528 && level == 0 && steps == 240,
               "audio_fade_arguments", "manager=%p level=%d steps=%d",
               manager, level, steps);
    event('A', pointer_value(manager));
}

void func_80039FF8(void)
{
    event('S', 0u);
}

void func_8003852C(void* seds)
{
    ASSERT_MSG(seds == D_8006259C, "seds_native_authority",
               "arg=%p authority=%p", seds, D_8006259C);
    event('E', pointer_value(seds));
}

unsigned int HeapFree(void* ptr)
{
    event('H', pointer_value(ptr));
    return 0u;
}

void func_800230A8(void* sprite)
{
    event('O', pointer_value(sprite));
}

void func_800346D4(void* window)
{
    event('W', pointer_value(window));
}

void func_8002CBBC(u8* model_data)
{
    event('M', pointer_value(model_data));
}

u32 func_8002C3D8(void)
{
    u32 result = s_poll_values[s_poll_count < 2u ? s_poll_count : 1u];
    s_poll_count++;
    event('P', result);
    return result;
}

static u32 alloc_value(u32 id)
{
    return ALLOC_BASE + id * 0x100u;
}

static void seed_common(u32 state)
{
    static const u32 globals[] = {
        0x8009D7ECu, 0x8009D7E8u, 0x8009CEB4u, 0x8009D150u,
        0x8009D7FCu, 0x8009D7F8u, 0x8009BE18u, 0x8009BE14u,
        0x8009D30Cu, 0x8009D780u, 0x8009D7D0u, 0x8009BDF4u,
        0x8009BE1Cu, 0x8009BE20u, 0x8009BC38u, 0x8009BCB0u,
        0x8009BC3Cu, 0x8009BCB4u, 0x8009C180u, 0x8009BE08u,
        0x8009D3C0u, 0x8009D7D4u
    };
    u32 i;

    PsxMemory_Init();
    memset(PSX_ADDR(POOL_GUEST), 0x5Au, 0x2000u);
    sw(D7CC, state);
    sw(POOL_PTR, POOL_GUEST);
    sh(0x8006F954u, 0x0456u);
    D_8006F954 = 0x0123u;
    D_80062528 = (void*)(uintptr_t)0x00225000u;
    D_8006259C = (void*)(uintptr_t)0x00226000u;
    g_GfxWorkBuffers = (void*)(uintptr_t)0x00227000u;
    D_80059190 = 77;
    s_event_count = 0u;
    s_poll_count = 0u;
    s_snapshot_event_count = 0u;
    s_poll_values[0] = 0u;
    s_poll_values[1] = 1u;

    /* Only slots 0 and 63 own native objects. */
    for (i = 0u; i < 64u; i++)
        sw(POOL_GUEST + i * 0x80u + 0x4Cu, 0u);
    sw(POOL_GUEST + 0x4Cu, 0x00310000u);
    sw(POOL_GUEST + 63u * 0x80u + 0x4Cu, 0x0031F000u);

    sh(0x8009D7E0u, 2u);
    sw(0x8009C620u, OBJ_GUEST);
    sw(OBJ_GUEST + 0x40u, alloc_value(70u));
    sw(OBJ_GUEST + 0x48u, alloc_value(71u));
    sw(OBJ_GUEST + 0x54u + 0x40u, alloc_value(72u));
    sw(OBJ_GUEST + 0x54u + 0x48u, alloc_value(73u));

    for (i = 0u; i < (u32)(sizeof(globals) / sizeof(globals[0])); i++)
        sw(globals[i], alloc_value(i + 1u));
    for (i = 0u; i < 256u; i++)
        sw(0x8009C184u + i * 4u, 0u);
    sw(0x8009C184u + 3u * 4u, alloc_value(80u));
    sw(0x8009C184u + 255u * 4u, alloc_value(81u));

    for (i = 0u; i < 3u; i++) {
        sw(0x8009CD34u + i * 4u, i == 1u ? 0u : alloc_value(90u + i));
        sw(0x8009BDF8u + i * 4u, alloc_value(100u + i));
    }

    sw(0x8009D55Cu, 0x11111111u);
    sw(0x8009D560u, 0x22222222u);
    sw(0x8009D564u, 0x33333333u);
    sh(0x8009D52Cu, 0xFF80u);
    sw(0x8009BE40u, 0x44444444u);
    sw(0x8009BCC4u, 0x55555555u);
    sw(0x8009D64Cu, 0x66666666u);
    memset(PSX_ADDR(0x8009C854u), 0xC8, 0x20u);
    memset(PSX_ADDR(0x8009CEC4u), 0xCE, 0x280u);
    sh(0x8009D154u, 0x007Fu);
    sw(0x8009BD38u, 0x38383838u);
    sw(0x8009BD3Cu, 0x3C3C3C3Cu);
    sw(0x8009D3F0u, 0xD3F0D3F0u);
    sw(0x8009BE0Cu, 0xBE0CBE0Cu);
    sw(0x8009BBB4u, 0xB4B4B4B4u);
    sw(0x8009BBB8u, 0xB8B8B8B8u);
    sw(0x8009BBBCu, 0xBCBCBCBCu);
    sw(0x8009C838u, 0xC838C838u);
    sw(0x8009C83Cu, 0xC83CC83Cu);
    sw(0x8009BE28u, 0xBE28BE28u);
    sw(0x8009BE2Cu, 0xBE2CBE2Cu);
    sw(0x8009BE30u, 0xBE30BE30u);
}

static int find_event(char kind, u32 value)
{
    u32 i;
    for (i = 0u; i < s_event_count; i++) {
        if (s_events[i].kind == kind && s_events[i].value == value)
            return (int)i;
    }
    return -1;
}

static void check_full_event_order(void)
{
    Event expected[64];
    u32 count = 0u;
    u32 i;

#define EXPECT(kind_, value_) do { \
    expected[count].kind = (kind_); \
    expected[count].value = (value_); \
    count++; \
} while (0)
    EXPECT('A', 0x00225000u);
    EXPECT('S', 0u);
    EXPECT('E', 0x00226000u);
    EXPECT('H', 0x00226000u);
    EXPECT('O', 0x00310000u);
    EXPECT('O', 0x0031F000u);
    EXPECT('W', 0x8009D498u);
    EXPECT('W', 0x8009BD64u);
    EXPECT('H', alloc_value(71u));
    EXPECT('M', alloc_value(70u));
    EXPECT('H', alloc_value(73u));
    EXPECT('M', alloc_value(72u));
    EXPECT('H', OBJ_GUEST);
    EXPECT('H', alloc_value(1u));
    EXPECT('H', alloc_value(2u));
    EXPECT('H', 0x00227000u);
    for (i = 3u; i <= 14u; i++)
        EXPECT('H', alloc_value(i));
    EXPECT('H', alloc_value(80u));
    EXPECT('H', alloc_value(81u));
    for (i = 15u; i <= 19u; i++)
        EXPECT('H', alloc_value(i));
    EXPECT('H', alloc_value(90u));
    EXPECT('H', alloc_value(100u));
    EXPECT('H', alloc_value(101u));
    EXPECT('H', alloc_value(92u));
    EXPECT('H', alloc_value(102u));
    EXPECT('H', POOL_GUEST);
    EXPECT('P', 0u);
    EXPECT('P', 1u);
    EXPECT('H', alloc_value(20u));
    EXPECT('H', alloc_value(22u));
#undef EXPECT

    ASSERT_MSG(s_event_count == count, "retail_full_event_count",
               "actual=%u expected=%u", s_event_count, count);
    if (s_event_count == count) {
        for (i = 0u; i < count; i++) {
            if (s_events[i].kind != expected[i].kind ||
                s_events[i].value != expected[i].value) {
                ASSERT_MSG(0, "retail_full_event_order",
                           "index=%u actual=%c:%08x expected=%c:%08x", i,
                           s_events[i].kind, s_events[i].value,
                           expected[i].kind, expected[i].value);
                break;
            }
        }
    }
}

static void check_state_zero_and_order(void)
{
    int first_window;
    int second_window;
    int object0;
    int object63;

    seed_common(0u);
    wm_80071034_test_dispatch_slot2();
    ASSERT_MSG(D_8006F954 == 0x0123u &&
                   (lw(0x8006F954u) & 0xFFFFu) == 0x0456u,
               "state0_transition_unchanged", "state zero changed entrance");

    ASSERT_MSG(s_event_count > 4u && s_events[0].kind == 'A' &&
                   s_events[1].kind == 'S' && s_events[2].kind == 'E' &&
                   s_events[3].kind == 'H' &&
                   s_events[3].value == 0x00226000u,
               "retail_audio_order", "first kinds=%c%c%c%c",
               s_events[0].kind, s_events[1].kind, s_events[2].kind,
               s_events[3].kind);

    object0 = find_event('O', 0x00310000u);
    object63 = find_event('O', 0x0031F000u);
    ASSERT_MSG(object0 >= 0 && object63 > object0, "pool_walk_64_order",
               "slot0=%d slot63=%d", object0, object63);
    ASSERT_MSG(lw(POOL_GUEST + 0x4Cu) == 0u &&
                   lw(POOL_GUEST + 63u * 0x80u + 0x4Cu) == 0u,
               "slot63_cleared", "slot0=0x%08x slot63=0x%08x",
               lw(POOL_GUEST + 0x4Cu),
               lw(POOL_GUEST + 63u * 0x80u + 0x4Cu));

    first_window = find_event('W', 0x8009D498u);
    second_window = find_event('W', 0x8009BD64u);
    ASSERT_MSG(first_window > object63 && second_window > first_window,
               "window_order", "D498=%d BD64=%d slot63=%d",
               first_window, second_window, object63);
    ASSERT_MSG(find_event('M', alloc_value(70u)) >= 0 &&
                   find_event('M', alloc_value(72u)) >= 0,
               "object_model_cleanup", "model0=%d model1=%d",
               find_event('M', alloc_value(70u)),
               find_event('M', alloc_value(72u)));
    ASSERT_MSG(find_event('H', alloc_value(80u)) >= 0 &&
                   find_event('H', alloc_value(81u)) >= 0,
               "asset_table_256", "entry3=%d entry255=%d",
               find_event('H', alloc_value(80u)),
               find_event('H', alloc_value(81u)));
    ASSERT_MSG(find_event('H', alloc_value(100u)) >= 0 &&
                   find_event('H', alloc_value(101u)) >= 0 &&
                   find_event('H', alloc_value(102u)) >= 0,
               "secondary_pair_freed", "secondary=%d/%d/%d",
               find_event('H', alloc_value(100u)),
               find_event('H', alloc_value(101u)),
               find_event('H', alloc_value(102u)));
    ASSERT_MSG(D_80059190 == 0 &&
                   find_event('H', 0x00227000u) >= 0,
               "gfx_work_buffers_real", "reset=%d free=%d", D_80059190,
               find_event('H', 0x00227000u));
    ASSERT_MSG(find_event('H', alloc_value(20u)) >= 0 &&
                   find_event('H', alloc_value(21u)) < 0 &&
                   find_event('H', alloc_value(22u)) >= 0,
               "archive_cleanup_selection", "BE08=%d D3C0=%d D7D4=%d",
               find_event('H', alloc_value(20u)),
               find_event('H', alloc_value(21u)),
               find_event('H', alloc_value(22u)));
    check_full_event_order();
}

static void check_state_one_snapshot(void)
{
    u8 expected_pool[0x2000];
    static const SnapshotEvent expected_snapshot_order[] = {
        { 0x0000u, 0x2000u },
        { 0x2000u, 4u }, { 0x2004u, 4u }, { 0x2008u, 4u },
        { 0x2010u, 4u }, { 0x2014u, 4u }, { 0x2018u, 4u },
        { 0x201Cu, 4u }, { 0x2020u, 0x20u }, { 0x2040u, 0x280u },
        { 0x22C0u, 4u }, { 0x22C4u, 4u }, { 0x22C8u, 4u },
        { 0x22CCu, 4u }, { 0x22D0u, 4u },
        { 0x22E4u, 4u }, { 0x22E8u, 4u },
        { 0x22D4u, 4u }, { 0x22D8u, 4u }, { 0x22DCu, 4u },
        { 0x22ECu, 4u }, { 0x22F0u, 4u }, { 0x22F4u, 4u }
    };
    u32 i;

    seed_common(1u);
    memcpy(expected_pool, PSX_ADDR(POOL_GUEST), sizeof(expected_pool));
    memset(expected_pool + 0x4Cu, 0, 4u);
    memset(expected_pool + 63u * 0x80u + 0x4Cu, 0, 4u);
    memset(D_8005A4E4, 0xCD, 0x2300u);
    memset(PSX_ADDR(0x8005A4E4u), 0xA5, 0x2300u);
    wm_8007299C();

    ASSERT_MSG(find_event('A', 0x00225000u) < 0, "state1_no_fade",
               "fade_event=%d", find_event('A', 0x00225000u));
    ASSERT_MSG(memcmp(D_8005A4E4, expected_pool,
                      sizeof(expected_pool)) == 0,
               "snapshot_pool_copy", "saved 0x2000-byte pool differs");
    {
        int guest_unchanged = 1;
        for (i = 0u; i < 0x2300u; i++) {
            if (*(const u8 *)PSX_ADDR(0x8005A4E4u + i) != 0xA5u) {
                guest_unchanged = 0;
                break;
            }
        }
        ASSERT_MSG(guest_unchanged, "snapshot_native_authority",
                   "guest mirror changed at offset=0x%x", i);
    }
    ASSERT_MSG(snapshot_lw(0x2000u) == 0x11111111u &&
                   snapshot_lw(0x2010u) == 0xFFFFFF80u &&
                   snapshot_lw(0x22C0u) == 0x0000007Fu &&
                   snapshot_lw(0x22F4u) == 0xBE30BE30u,
               "snapshot_scalar_layout", "values=%08x/%08x/%08x/%08x",
               snapshot_lw(0x2000u), snapshot_lw(0x2010u),
               snapshot_lw(0x22C0u), snapshot_lw(0x22F4u));
    ASSERT_MSG(memcmp(D_8005A4E4 + 0x2020u,
                      PSX_ADDR(0x8009C854u), 0x20u) == 0 &&
                   memcmp(D_8005A4E4 + 0x2040u,
                          PSX_ADDR(0x8009CEC4u), 0x280u) == 0,
               "snapshot_block_layout", "block copy differs");
    ASSERT_MSG(snapshot_lw(0x200Cu) == 0xCDCDCDCDu &&
                   snapshot_lw(0x22E0u) == 0xCDCDCDCDu &&
                   snapshot_lw(0x22F8u) == 0xCDCDCDCDu,
               "snapshot_sparse_holes_preserved", "holes=%08x/%08x/%08x",
               snapshot_lw(0x200Cu), snapshot_lw(0x22E0u),
               snapshot_lw(0x22F8u));
    ASSERT_MSG(s_snapshot_event_count ==
                   (u32)(sizeof(expected_snapshot_order) /
                         sizeof(expected_snapshot_order[0])) &&
                   memcmp(s_snapshot_events, expected_snapshot_order,
                          sizeof(expected_snapshot_order)) == 0,
               "snapshot_retail_store_order", "event_count=%u",
               s_snapshot_event_count);
    ASSERT_MSG(D_8006F954 == 0x8123u,
               "transition_flag", "flags=0x%04x",
               (unsigned)D_8006F954);
    ASSERT_MSG((lw(0x8006F954u) & 0xFFFFu) == 0x0456u,
               "transition_guest_unchanged", "guest flags changed");

    /* Exercise the real inverse in the same process.  The sparse words are
     * state carried by the native buffer even though 0x80075460 leaves them
     * untouched. */
    memset(PSX_ADDR(POOL_GUEST), 0xEE, 0x2000u);
    memset(PSX_ADDR(0x8009C5ACu), 0xEE, 0x10u);
    memset(PSX_ADDR(0x8009D55Cu), 0xEE, 0x10u);
    memset(PSX_ADDR(0x8009BBB4u), 0xEE, 0x10u);
    memset(PSX_ADDR(0x8009BE28u), 0xEE, 0x10u);
    wm_8007565C();
    ASSERT_MSG(memcmp(PSX_ADDR(POOL_GUEST), expected_pool,
                      sizeof(expected_pool)) == 0 &&
                   lw(0x8009D55Cu) == 0x11111111u &&
                   lw(0x8009D560u) == 0x22222222u &&
                   lw(0x8009D564u) == 0x33333333u,
               "snapshot_restore_round_trip", "pool/pose round trip differs");
    ASSERT_MSG(lw(0x8009C5B8u) == 0xCDCDCDCDu &&
                   lw(0x8009D568u) == 0xCDCDCDCDu &&
                   lw(0x8009BBC0u) == 0xCDCDCDCDu &&
                   lw(0x8009BE34u) == 0xCDCDCDCDu,
               "snapshot_sparse_holes_round_trip",
               "holes=%08x/%08x/%08x/%08x", lw(0x8009C5B8u),
               lw(0x8009D568u), lw(0x8009BBC0u), lw(0x8009BE34u));
}

int main(void)
{
    check_state_zero_and_order();
    check_state_one_snapshot();
    if (s_failures != 0) {
        fprintf(stderr, "W34N7 FAIL failures=%d\n", s_failures);
        return 1;
    }
    printf("W34N7 SLOT2 TEARDOWN CERTIFICATE PASS\n");
    return 0;
}

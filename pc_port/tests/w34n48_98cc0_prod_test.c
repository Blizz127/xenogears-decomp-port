/* W34N48 production-linked certificate for retail wm_80098CC0. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_98cc0.h"

#define SLOT_TABLE       UINT32_C(0x8009C184)
#define SELECT_INDICES   UINT32_C(0x8009BBAC)
#define ARCHIVE_PRIMARY  UINT32_C(0x8009BCD8)
#define ARCHIVE_SECOND   UINT32_C(0x8009BD08)
#define WORLD_WIDTH      UINT32_C(0x8009D160)
#define WORLD_HEIGHT     UINT32_C(0x8009D2B4)
#define OLD_WINDOW       UINT32_C(0x8009D318)
#define NEW_WINDOW       UINT32_C(0x8009D570)
#define PRIMARY_PATH     UINT32_C(0x80060000)
#define SECOND_PATH      UINT32_C(0x80061000)
#define TILE_BYTES       UINT32_C(0x710)

typedef struct QueueRecord {
    u32 kind;
    u32 word[4];
} QueueRecord;

uint8_t g_PsxRam[PSX_RAM_SIZE];

static int s_failures;
static u32 s_poll[2];
static u32 s_poll_count;
static u32 s_heap_next;
static u32 s_heap_count;
static u32 s_free_count;
static u32 s_freed_guest[8];
static u32 s_decode_entries[16];
static u32 s_decode_count;
static u32 s_path_entries[16];
static u32 s_path_count;
static QueueRecord s_queue[64];
static u32 s_queue_count;

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 ld16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

u32 func_8002C3D8(void)
{
    u32 result = s_poll[s_poll_count & 1u];
    s_poll_count++;
    return result;
}

void *HeapAlloc(u32 size, u32 flags)
{
    void *result = PSX_ADDR(s_heap_next);
    (void)flags;
    check("allocation-size", size == TILE_BYTES);
    s_heap_next += (size + 15u) & ~15u;
    s_heap_count++;
    return result;
}

void HeapFree(void *pointer)
{
    if (s_free_count < 8u)
        s_freed_guest[s_free_count] = PsxMemory_GuestAddr(pointer);
    s_free_count++;
}

int ArchiveDecodeSector(int entry_index)
{
    if (s_decode_count < 16u)
        s_decode_entries[s_decode_count] = (u32)entry_index;
    s_decode_count++;
    return entry_index == 11 ? 0x1000 : 0x2000;
}

char *ArchiveGetFilePath(int entry_index)
{
    if (s_path_count < 16u)
        s_path_entries[s_path_count] = (u32)entry_index;
    s_path_count++;
    return (char *)PSX_ADDR(entry_index == 11 ? PRIMARY_PATH : SECOND_PATH);
}

static void record_queue(u32 kind, u32 a, u32 b, u32 c, u32 d)
{
    if (s_queue_count < 64u) {
        s_queue[s_queue_count].kind = kind;
        s_queue[s_queue_count].word[0] = a;
        s_queue[s_queue_count].word[1] = b;
        s_queue[s_queue_count].word[2] = c;
        s_queue[s_queue_count].word[3] = d;
    }
    s_queue_count++;
}

s32 wm_8009623C(u32 a, u32 b, u32 c)
{
    record_queue(3u, a, b, c, 0u);
    return 0;
}

s32 wm_800962B0(u32 a, u32 b, u32 c, u32 d)
{
    record_queue(4u, a, b, c, d);
    return 0;
}

s32 wm_80096328(void)
{
    record_queue(8u, 0u, 0u, 0u, 0u);
    return 0;
}

s32 wm_800965A4(void)
{
    record_queue(5u, 0u, 0u, 0u, 0u);
    return 0;
}

static u32 slot_address(u32 tile)
{
    return SLOT_TABLE + tile * 4u;
}

static u32 fake_existing(u32 tile)
{
    return UINT32_C(0x800A0000) + tile * UINT32_C(0x800);
}

static void reset_traces(u32 first, u32 second)
{
    s_poll[0] = first;
    s_poll[1] = second;
    s_poll_count = 0u;
    s_heap_next = UINT32_C(0x80110000);
    s_heap_count = 0u;
    s_free_count = 0u;
    memset(s_freed_guest, 0, sizeof(s_freed_guest));
    memset(s_decode_entries, 0, sizeof(s_decode_entries));
    s_decode_count = 0u;
    memset(s_path_entries, 0, sizeof(s_path_entries));
    s_path_count = 0u;
    memset(s_queue, 0, sizeof(s_queue));
    s_queue_count = 0u;
}

static void seed(u32 first, u32 second)
{
    u32 i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    st32(ARCHIVE_PRIMARY, 11u);
    st32(ARCHIVE_SECOND, 22u);
    st32(WORLD_WIDTH, 16u);
    st32(WORLD_HEIGHT, 16u);
    for (i = 0u; i < 81u; i++) {
        st16(OLD_WINDOW + i * 2u, (u16)i);
        st16(NEW_WINDOW + i * 2u, (u16)i);
        st32(slot_address(i), fake_existing(i));
    }
    st16(SELECT_INDICES + 0u, 40u);
    st16(SELECT_INDICES + 2u, 41u);
    st16(SELECT_INDICES + 4u, 42u);
    st16(SELECT_INDICES + 6u, 43u);
    memset(PSX_ADDR(OLD_WINDOW + 162u), 0xC3, 32u);
    memset(PSX_ADDR(NEW_WINDOW + 162u), 0xC3, 32u);
    reset_traces(first, second);
}

static void clear_index(u32 index)
{
    u32 tile = (u32)(u16)ld16(NEW_WINDOW + index * 2u);
    st32(slot_address(tile), 0u);
}

static u32 transpose(u32 tile)
{
    return (tile % 16u) * 16u + tile / 16u;
}

static int record_is(u32 index, u32 kind, u32 a, u32 b)
{
    return index < s_queue_count && s_queue[index].kind == kind &&
           s_queue[index].word[0] == a && s_queue[index].word[1] == b;
}

static int buffer_matches_slot(u32 record, u32 tile, u32 word)
{
    return record < s_queue_count &&
           s_queue[record].word[word] == ld32(slot_address(tile));
}

static void clear_edge_fixture(void)
{
    static const u32 targets[] = {1u, 7u, 73u, 79u, 9u, 63u, 17u, 71u, 40u};
    u32 i;
    for (i = 0u; i < sizeof(targets) / sizeof(targets[0]); i++)
        clear_index(targets[i]);
}

static void check_unchanged_tables(void)
{
    u32 i;
    int good = 1;
    for (i = 0u; i < 81u; i++) {
        good = good && ld16(OLD_WINDOW + i * 2u) == (s16)i;
        good = good && ld16(NEW_WINDOW + i * 2u) == (s16)i;
    }
    good = good && ((const u8 *)PSX_ADDR(OLD_WINDOW + 162u))[0] == 0xC3u;
    good = good && ((const u8 *)PSX_ADDR(NEW_WINDOW + 162u))[31] == 0xC3u;
    check("input-read-only", good);
}

static void test_old_window_eviction(void)
{
    seed(0u, 5u);
    st16(OLD_WINDOW, 200u);
    st32(slot_address(200u), UINT32_C(0x800B0000));

    wm_80098CC0();

    check("old-window-eviction",
          s_free_count == 1u && s_freed_guest[0] == UINT32_C(0x800B0000) &&
          ld32(slot_address(200u)) == 0u && ld32(slot_address(0u)) != 0u);
}

static void test_ready_edges(void)
{
    static const u32 primary[] = {1u, 7u, 73u, 79u};
    static const u32 secondary[] = {9u, 63u, 17u, 71u};
    u32 i;
    int good;

    seed(0u, 5u);
    clear_edge_fixture();
    wm_80098CC0();

    check("ready-or-semantics",
          s_poll_count == 2u && s_queue_count == 11u &&
          s_queue[0].kind == 3u && s_decode_count == 3u && s_path_count == 0u);

    good = 1;
    for (i = 0u; i < 4u; i++) {
        good = good && record_is(i, 3u, UINT32_C(0x1000) + primary[i],
                                 TILE_BYTES);
        good = good && buffer_matches_slot(i, primary[i], 2u);
    }
    check("primary-edge-order", good);

    good = 1;
    for (i = 0u; i < 4u; i++) {
        good = good && record_is(4u + i, 3u,
                                 UINT32_C(0x2000) + transpose(secondary[i]),
                                 TILE_BYTES);
        good = good && buffer_matches_slot(4u + i, secondary[i], 2u);
    }
    check("secondary-edge-order", good);
    check("transposed-secondary",
          record_is(5u, 3u, UINT32_C(0x2000) + 243u, TILE_BYTES) &&
          record_is(7u, 3u, UINT32_C(0x2000) + 116u, TILE_BYTES));
    check("closure-pass", ld32(slot_address(40u)) != 0u &&
          buffer_matches_slot(8u, 40u, 2u));
    check("primary-source-priority",
          record_is(8u, 3u, UINT32_C(0x1000) + 40u, TILE_BYTES) &&
          s_decode_entries[0] == 11u && s_decode_entries[1] == 22u &&
          s_decode_entries[2] == 11u);
    check("ready-flush-backend",
          record_is(9u, 3u, 0u, 0u) && s_queue[10].kind == 8u);

    good = s_heap_count == 9u;
    for (i = 0u; i < 9u; i++) {
        u32 guest = s_queue[i].word[2];
        good = good && (guest & UINT32_C(0xFF000000)) == UINT32_C(0x80000000);
        good = good && guest >= UINT32_C(0x80110000) && guest < s_heap_next;
    }
    check("guest-pointer-publication", good);
    check_unchanged_tables();
}

static void test_minus_one_ready(void)
{
    seed(5u, UINT32_MAX);
    wm_80098CC0();
    check("minus-one-ready", s_decode_count == 2u && s_path_count == 0u &&
          s_queue_count == 2u && s_queue[0].kind == 3u &&
          s_queue[1].kind == 8u);
}

static void test_path_edges(void)
{
    static const u32 primary[] = {1u, 7u, 73u, 79u};
    static const u32 secondary[] = {9u, 63u, 17u, 71u};
    u32 i;
    int good = 1;

    seed(5u, 5u);
    clear_edge_fixture();
    wm_80098CC0();

    check("not-ready-selection", s_decode_count == 0u && s_path_count == 3u &&
          s_queue_count == 11u && s_queue[0].kind == 4u);
    for (i = 0u; i < 4u; i++) {
        good = good && record_is(i, 4u, PRIMARY_PATH, primary[i] << 11u);
        good = good && s_queue[i].word[2] == TILE_BYTES;
        good = good && buffer_matches_slot(i, primary[i], 3u);
    }
    check("path-primary-offsets", good);

    good = 1;
    for (i = 0u; i < 4u; i++) {
        good = good && record_is(4u + i, 4u, SECOND_PATH,
                                 transpose(secondary[i]) << 11u);
        good = good && s_queue[4u + i].word[2] == TILE_BYTES;
        good = good && buffer_matches_slot(4u + i, secondary[i], 3u);
    }
    check("path-secondary-shift", good);
    check("path-primary-priority",
          record_is(8u, 4u, PRIMARY_PATH, 40u << 11u) &&
          s_path_entries[0] == 11u && s_path_entries[1] == 22u &&
          s_path_entries[2] == 11u);
    check("path-flush-backend",
          record_is(9u, 4u, 0u, 0u) && s_queue[10].kind == 5u);
}

static void test_secondary_only_closure(void)
{
    seed(0u, 5u);
    clear_index(9u);
    clear_index(40u);
    wm_80098CC0();
    check("secondary-only-closure",
          s_queue_count == 4u &&
          record_is(0u, 3u, UINT32_C(0x2000) + transpose(9u), TILE_BYTES) &&
          record_is(1u, 3u, UINT32_C(0x2000) + transpose(40u), TILE_BYTES));
}

static void test_closure_without_edge_source(void)
{
    seed(0u, 5u);
    clear_index(40u);
    wm_80098CC0();
    check("closure-no-source",
          s_heap_count == 1u && ld32(slot_address(40u)) != 0u &&
          s_queue_count == 2u && record_is(0u, 3u, 0u, 0u) &&
          s_queue[1].kind == 8u);
}

int main(void)
{
    test_old_window_eviction();
    test_ready_edges();
    test_minus_one_ready();
    test_path_edges();
    test_secondary_only_closure();
    test_closure_without_edge_source();

    if (s_failures != 0) {
        fprintf(stderr, "W34N48 certificate: %d failure(s)\n", s_failures);
        return 1;
    }
    puts("W34N48 0x80098CC0 full-body certificate PASS");
    return 0;
}

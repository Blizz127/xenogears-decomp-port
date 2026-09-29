/* Focused production certificate for retail 0x80081C3C..0x80081FB4. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "world_map_callback_81c3c.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800B0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  5
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define BUFFER_A    UINT32_C(0x800A6000)
#define BUFFER_B    UINT32_C(0x800A8000)
#define WIDTH_TABLE UINT32_C(0x800AA000)
#define BUFFER_A_G  UINT32_C(0x8009D158)
#define BUFFER_B_G  UINT32_C(0x8009D15C)
#define TABLE_G     UINT32_C(0x8009D148)
#define MOVE_BASE   UINT32_C(0x8009D164)
#define STREAM_SIDE UINT32_C(0x8009D7F0)
#define DRAW_REC_G  UINT32_C(0x8009BE3C)
#define DRAW_RECORD UINT32_C(0x800AC000)
#define OT_ENTRY    UINT32_C(0x800AC100)
#define COUNT       UINT32_C(192)
#define STRIDE      UINT32_C(40)
#define BUFFER_SIZE (COUNT * STRIDE)

typedef struct AllocCall {
    u32 size;
    u32 flags;
} AllocCall;

static int failures;
static AllocCall alloc_calls[4];
static unsigned alloc_call_count;
static unsigned tpage_call_count;
static int tpage_args_ok;
static unsigned rand_call_count;
static unsigned move_call_count;
static u32 move_record;
static RECT move_rect;
static int move_x;
static int move_y;

static void check(int condition, const char *name)
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

static u8 read8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_trace(void)
{
    memset(alloc_calls, 0, sizeof(alloc_calls));
    alloc_call_count = 0u;
    tpage_call_count = 0u;
    tpage_args_ok = 1;
    rand_call_count = 0u;
    move_call_count = 0u;
    move_record = 0u;
    memset(&move_rect, 0, sizeof(move_rect));
    move_x = 0;
    move_y = 0;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(DRAW_REC_G, DRAW_RECORD);
    write32(DRAW_RECORD + 0x70u, OT_ENTRY);
    write32(OT_ENTRY, UINT32_C(0xAB011111));
    reset_trace();
}

void *HeapAlloc(u32 size, u32 flags)
{
    static const u32 allocation[] = {BUFFER_A, BUFFER_B, WIDTH_TABLE};
    unsigned call = alloc_call_count;

    check(call < 3u, "trace.alloc_capacity");
    if (call < 4u) {
        alloc_calls[call].size = size;
        alloc_calls[call].flags = flags;
    }
    alloc_call_count++;
    if (call >= 3u)
        return NULL;
    return PSX_ADDR(allocation[call]);
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    if (tp != 2 || abr != 0 || x != 640 || y != 256)
        tpage_args_ok = 0;
    tpage_call_count++;
    return UINT16_C(0x1234);
}

int rand(void)
{
    int result = 0;

    if (rand_call_count == 0u)
        result = 7;
    else if (rand_call_count == 1u)
        result = 8;
    rand_call_count++;
    return result;
}

void SetDrawMove(DR_MOVE *packet, RECT *rect, int x, int y)
{
    move_call_count++;
    move_record = PsxMemory_GuestAddr(packet);
    move_rect = *rect;
    move_x = x;
    move_y = y;
    write32(move_record, UINT32_C(0x05000000));
}

static void test_initializer(void)
{
    u32 index;

    seed();
    check(wm_80081C3C(SLOT_INDEX) == 1, "init.return");
    check(alloc_call_count == 3u &&
          alloc_calls[0].size == BUFFER_SIZE && alloc_calls[0].flags == 0u &&
          alloc_calls[1].size == BUFFER_SIZE && alloc_calls[1].flags == 0u &&
          alloc_calls[2].size == COUNT * 2u && alloc_calls[2].flags == 0u,
          "init.alloc_sizes");
    check(read32(BUFFER_A_G) == BUFFER_A &&
          read32(BUFFER_B_G) == BUFFER_B &&
          read32(TABLE_G) == WIDTH_TABLE,
          "init.alloc_publish");
    check(tpage_call_count == COUNT && tpage_args_ok != 0,
          "init.tpage_args");
    for (index = 0u; index < COUNT; index++) {
        u32 primitive = BUFFER_A + index * STRIDE;
        check(read8(primitive + 3u) == 9u &&
              read8(primitive + 4u) == 128u &&
              read8(primitive + 5u) == 128u &&
              read8(primitive + 6u) == 128u &&
              read8(primitive + 7u) == 45u &&
              read16(primitive + 22u) == UINT16_C(0x1234),
              "init.primitive_layout");
        check(read16(WIDTH_TABLE + index * 2u) == 1u,
              "init.width_seed");
    }
    check(memcmp(PSX_ADDR(BUFFER_A), PSX_ADDR(BUFFER_B), BUFFER_SIZE) == 0,
          "init.buffer_mirror");
}

static void test_update(void)
{
    u32 primitive0;
    u32 primitive1;
    u32 primitive_last;
    u32 move;

    seed();
    check(wm_80081C3C(SLOT_INDEX) == 1, "update.init");
    reset_trace();
    write16(WIDTH_TABLE + 0u, 4u);
    write16(WIDTH_TABLE + 2u, 5u);
    write32(STREAM_SIDE, 1u);
    write32(OT_ENTRY, UINT32_C(0xAB011111));
    check(wm_80081D80(SLOT_INDEX) == 1, "update.return");
    primitive0 = BUFFER_B;
    primitive1 = BUFFER_B + STRIDE;
    primitive_last = BUFFER_B + (COUNT - 1u) * STRIDE;
    move = MOVE_BASE + 24u;
    check(rand_call_count == COUNT &&
          read16(primitive0 + 8u) == 65u &&
          read16(primitive0 + 16u) == 257u &&
          read16(primitive0 + 24u) == 65u &&
          read16(primitive0 + 32u) == 257u &&
          read16(primitive0 + 10u) == 0u &&
          read16(primitive0 + 18u) == 0u &&
          read16(primitive0 + 26u) == 1u &&
          read16(primitive0 + 34u) == 1u &&
          read8(primitive0 + 12u) == 0u &&
          read8(primitive0 + 13u) == 0u &&
          read8(primitive0 + 20u) == 192u &&
          read8(primitive0 + 21u) == 0u &&
          read8(primitive0 + 28u) == 0u &&
          read8(primitive0 + 29u) == 1u &&
          read8(primitive0 + 36u) == 192u &&
          read8(primitive0 + 37u) == 1u,
          "update.strip_geometry");
    check(read16(primitive1 + 8u) == 65u &&
          read16(primitive1 + 16u) == 257u &&
          read16(BUFFER_A + 8u) == 0u,
          "update.selected_buffer");
    check((read32(primitive0) & UINT32_C(0x00FFFFFF)) ==
              UINT32_C(0x00011111) &&
          (read32(primitive1) & UINT32_C(0x00FFFFFF)) ==
              (primitive0 & UINT32_C(0x00FFFFFF)) &&
          (read32(move) & UINT32_C(0x00FFFFFF)) ==
              (primitive_last & UINT32_C(0x00FFFFFF)) &&
          (read32(OT_ENTRY) & UINT32_C(0x00FFFFFF)) ==
              (move & UINT32_C(0x00FFFFFF)),
          "update.ot_chain");
    check(move_call_count == 1u && move_record == move &&
          move_rect.x == 64 && move_rect.y == 216 &&
          move_rect.w == 192 && move_rect.h == 216 &&
          move_x == 640 && move_y == 256,
          "update.move_packet");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x80081C3C));
    write32(SLOT + 0x1Cu, UINT32_C(0x80081D80));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

    reset_trace();
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          rand_call_count == COUNT && move_call_count == 1u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_update();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N107 MODE16 DISTORTION STRIP CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N107 MODE16 DISTORTION STRIP CERTIFICATE PASS");
    return 0;
}

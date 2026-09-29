/* Focused production certificate for retail mode-9 lifecycle 0x80077A64. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_mode9_lifecycle.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

enum EventKind {
    EV_FRAMEBUFFER = 1,
    EV_MOVE_IMAGE,
    EV_DRAW_SYNC,
    EV_TRANSITION,
    EV_SECOND_FINISH,
    EV_OBJECT_POOL,
    EV_CROSS_PRODUCTS,
    EV_ARCHIVE_CD_SYNC,
    EV_DECODE_SIZE,
    EV_HEAP_ALLOC,
    EV_ARCHIVE_READ,
    EV_OBJECT_MATRIX,
    EV_GPU_A,
    EV_GPU_B,
    EV_BSS_CONSTANTS,
    EV_HEAP_TABLE,
    EV_UPLOAD_A,
    EV_UPLOAD_B,
    EV_DRAW_PACKETS,
    EV_TABLES,
    EV_SOUND_ADD,
    EV_ARCHIVE_INDEX,
    EV_TERRAIN_POSITION,
    EV_CD_WORK,
    EV_VSYNC,
    EV_DISTANCE,
    EV_REGISTER,
    EV_978FC,
    EV_8901C,
    EV_865A0,
    EV_SOUND_SHUTDOWN,
    EV_SOUND_RELEASE,
    EV_HEAP_FREE,
    EV_84818,
    EV_86568,
    EV_866C8,
    EV_74F04,
    EV_750DC,
    EV_88FF4,
    EV_89128,
    EV_97D64,
    EV_976A0
};

typedef struct Event {
    int kind;
    uintptr_t a0;
    uintptr_t a1;
    uintptr_t a2;
    uintptr_t a3;
} Event;

#define MAX_EVENTS 96
#define ALLOCATION 0x800B8000u

static Event events[MAX_EVENTS];
static int event_count;
static int failures;
static u32 distance_values[4];
static int distance_count;
static int distance_index;
void* D_8006259C;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void record_event(int kind, uintptr_t a0, uintptr_t a1,
                         uintptr_t a2, uintptr_t a3)
{
    if (event_count < MAX_EVENTS) {
        events[event_count].kind = kind;
        events[event_count].a0 = a0;
        events[event_count].a1 = a1;
        events[event_count].a2 = a2;
        events[event_count].a3 = a3;
    }
    event_count++;
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
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

static void reset_test(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(events, 0, sizeof(events));
    event_count = 0;
    distance_count = 0;
    distance_index = 0;
    D_8006259C = NULL;
}

static void check_event_kinds(const int* expected, int count,
                              const char* name)
{
    int index;
    if (event_count != count) {
        check(0, name);
        return;
    }
    for (index = 0; index < count; index++) {
        if (events[index].kind != expected[index]) {
            check(0, name);
            return;
        }
    }
}

void wm_80072BB0(void)
{
    record_event(EV_FRAMEBUFFER, 0u, 0u, 0u, 0u);
}

int MoveImage(void* rect, int x, int y)
{
    s16 values[4];
    memcpy(values, rect, sizeof(values));
    record_event(EV_MOVE_IMAGE,
                 ((uintptr_t)(u16)values[0] << 16) | (uintptr_t)(u16)values[1],
                 ((uintptr_t)(u16)values[2] << 16) | (uintptr_t)(u16)values[3],
                 (uintptr_t)(unsigned int)x, (uintptr_t)(unsigned int)y);
    return 0;
}

int DrawSync(int mode)
{
    record_event(EV_DRAW_SYNC, (uintptr_t)(unsigned int)mode, 0u, 0u, 0u);
    return 0;
}

int wm_80072DB4(s32 a0, s32 a1, s32 a2, s32 a3)
{
    record_event(EV_TRANSITION, (uintptr_t)(u32)a0, (uintptr_t)(u32)a1,
                 (uintptr_t)(u32)a2, (uintptr_t)(u32)a3);
    return 0;
}

int wm_mode9_stage_second_wave_finish(void)
{
    record_event(EV_SECOND_FINISH, 0u, 0u, 0u, 0u);
    return 0;
}

#define STAGE(name, event_id) \
    int name(void) { record_event((event_id), 0u, 0u, 0u, 0u); return 0; }

STAGE(wm_72238_stage_object_pool, EV_OBJECT_POOL)
STAGE(wm_72238_stage_cross_products, EV_CROSS_PRODUCTS)
STAGE(wm_72238_stage_object_matrix, EV_OBJECT_MATRIX)
STAGE(wm_72238_stage_gpu_asset_a, EV_GPU_A)
STAGE(wm_72238_stage_gpu_asset_b, EV_GPU_B)
STAGE(wm_72238_stage_bss_constants, EV_BSS_CONSTANTS)
STAGE(wm_72238_stage_heap_table, EV_HEAP_TABLE)
STAGE(wm_72238_stage_upload_a, EV_UPLOAD_A)
STAGE(wm_72238_stage_upload_b, EV_UPLOAD_B)
STAGE(wm_72238_stage_draw_packets, EV_DRAW_PACKETS)
STAGE(wm_72238_stage_88f64, EV_TABLES)

void ArchiveCdDataSync(int mode)
{
    record_event(EV_ARCHIVE_CD_SYNC, (uintptr_t)(unsigned int)mode,
                 0u, 0u, 0u);
}

int ArchiveDecodeAlignedSize(unsigned int entry_index)
{
    record_event(EV_DECODE_SIZE, (uintptr_t)entry_index, 0u, 0u, 0u);
    return 0x100;
}

void* HeapAlloc(unsigned int size, unsigned int flags)
{
    record_event(EV_HEAP_ALLOC, (uintptr_t)size, (uintptr_t)flags,
                 0u, 0u);
    return PSX_ADDR(ALLOCATION);
}

int ArchiveReadFileToBuffer(int entry_index, void* destination,
                            unsigned int offset, unsigned int flags)
{
    record_event(EV_ARCHIVE_READ, (uintptr_t)(u32)entry_index,
                 (uintptr_t)destination, (uintptr_t)offset,
                 (uintptr_t)flags);
    return 0;
}

void SoundAddSedsEntry(void* seds)
{
    record_event(EV_SOUND_ADD, (uintptr_t)seds, 0u, 0u, 0u);
}

int ArchiveSetIndex(int directory_index, int entry_index)
{
    record_event(EV_ARCHIVE_INDEX, (uintptr_t)(u32)directory_index,
                 (uintptr_t)(u32)entry_index, 0u, 0u);
    return 0;
}

void wm_80097BC0(u32 position)
{
    record_event(EV_TERRAIN_POSITION, (uintptr_t)position, 0u, 0u, 0u);
}

u32 wm_800967E4(void)
{
    record_event(EV_CD_WORK, 0u, 0u, 0u, 0u);
    return 0u;
}

int Vsync(int mode)
{
    record_event(EV_VSYNC, (uintptr_t)(unsigned int)mode, 0u, 0u, 0u);
    return 0;
}

u32 wm_80096668_circular_distance(void)
{
    u32 value = 0u;
    if (distance_index < distance_count)
        value = distance_values[distance_index];
    distance_index++;
    record_event(EV_DISTANCE, (uintptr_t)value, 0u, 0u, 0u);
    return value;
}

void wm_pool_register(u32 init_cb, u32 update_cb)
{
    record_event(EV_REGISTER, (uintptr_t)init_cb, (uintptr_t)update_cb,
                 0u, 0u);
}

void wm_800978FC(void) { record_event(EV_978FC, 0u, 0u, 0u, 0u); }
void wm_8008901C(void) { record_event(EV_8901C, 0u, 0u, 0u, 0u); }
void wm_800865A0(void) { record_event(EV_865A0, 0u, 0u, 0u, 0u); }

void func_80039FF8(void)
{
    record_event(EV_SOUND_SHUTDOWN, 0u, 0u, 0u, 0u);
}

void func_8003852C(void* seds)
{
    record_event(EV_SOUND_RELEASE, (uintptr_t)seds, 0u, 0u, 0u);
}

unsigned int HeapFree(void* pointer)
{
    record_event(EV_HEAP_FREE, (uintptr_t)pointer, 0u, 0u, 0u);
    return 0u;
}

void wm_80084818(void) { record_event(EV_84818, 0u, 0u, 0u, 0u); }
void wm_80086568(void) { record_event(EV_86568, 0u, 0u, 0u, 0u); }
void wm_800866C8(void) { record_event(EV_866C8, 0u, 0u, 0u, 0u); }
void wm_80074F04(void) { record_event(EV_74F04, 0u, 0u, 0u, 0u); }
void wm_800750DC(void) { record_event(EV_750DC, 0u, 0u, 0u, 0u); }
void wm_80088FF4(void) { record_event(EV_88FF4, 0u, 0u, 0u, 0u); }
void wm_80089128(void) { record_event(EV_89128, 0u, 0u, 0u, 0u); }
void wm_80097D64(void) { record_event(EV_97D64, 0u, 0u, 0u, 0u); }
void wm_800976A0(void) { record_event(EV_976A0, 0u, 0u, 0u, 0u); }

static void test_setup(void)
{
    static const int expected[] = {
        EV_FRAMEBUFFER, EV_MOVE_IMAGE, EV_DRAW_SYNC, EV_TRANSITION,
        EV_SECOND_FINISH, EV_OBJECT_POOL, EV_CROSS_PRODUCTS,
        EV_ARCHIVE_CD_SYNC, EV_DECODE_SIZE, EV_HEAP_ALLOC, EV_ARCHIVE_READ,
        EV_OBJECT_MATRIX, EV_GPU_A, EV_GPU_B, EV_BSS_CONSTANTS,
        EV_HEAP_TABLE, EV_UPLOAD_A, EV_UPLOAD_B, EV_DRAW_PACKETS, EV_TABLES,
        EV_ARCHIVE_CD_SYNC, EV_SOUND_ADD, EV_ARCHIVE_INDEX,
        EV_TERRAIN_POSITION, EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_REGISTER, EV_REGISTER, EV_REGISTER, EV_REGISTER,
        EV_978FC, EV_8901C, EV_865A0
    };
    uint8_t template_bytes[32];
    int index;

    reset_test();
    for (index = 0; index < 32; index++)
        template_bytes[index] = (uint8_t)(0x40 + index);
    memcpy(PSX_ADDR(0x8009A180u), template_bytes, sizeof(template_bytes));
    write32(0x8009D3C8u, 0x55u);
    distance_values[0] = 1u;
    distance_values[1] = 0u;
    distance_count = 2;

    check(wm_80077A64() == 0, "setup.return_zero");
    check_event_kinds(expected, (int)(sizeof(expected) / sizeof(expected[0])),
                      "setup.stage_order");
    check(events[1].a0 == 0u && events[1].a1 == 0x014000D8u &&
          events[1].a2 == 704u && events[1].a3 == 256u,
          "setup.move_image_args");
    check(events[3].a0 == 64u && events[3].a1 == 0u &&
          events[3].a2 == 4u && events[3].a3 == 1u,
          "setup.transition_args");
    check(events[8].a0 == 0x55u && events[9].a0 == 0x100u &&
          events[9].a1 == 0u && events[10].a0 == 0x55u &&
          events[10].a1 == (uintptr_t)PSX_ADDR(ALLOCATION) &&
          D_8006259C == PSX_ADDR(ALLOCATION),
          "setup.seds_load");
    check(events[21].a0 == (uintptr_t)PSX_ADDR(ALLOCATION),
          "setup.sound_link");
    check(memcmp(PSX_ADDR(0x8009BE4Cu), template_bytes,
                 sizeof(template_bytes)) == 0,
          "setup.template_copy");
    check(read32(0x8009CCA4u) == 1u && read32(0x8009D3CCu) == 4u &&
          read32(0x8009D804u) == 0u && read32(0x8009D144u) == 0u &&
          read32(0x8009CD40u) == 0x80086700u,
          "setup.state_values");
    check(read32(0x8009C5ACu) == 0x02000000u &&
          read32(0x8009C5B0u) == 0xFFE00000u &&
          read32(0x8009C5B4u) == 0x02000000u,
          "setup.position_vector");
    check(events[30].a0 == 0x800923A8u &&
          events[30].a1 == 0x800925A0u &&
          events[31].a0 == 0x80077DC8u &&
          events[31].a1 == 0x80077E68u &&
          events[32].a0 == 0x8007828Cu &&
          events[32].a1 == 0x800783E8u &&
          events[33].a0 == 0x80078948u &&
          events[33].a1 == 0x80078950u,
          "setup.registration_pairs");
    check(distance_index == 2, "setup.cd_drain_positive");
}

static void test_teardown(void)
{
    static const int expected[] = {
        EV_SOUND_SHUTDOWN, EV_SOUND_RELEASE, EV_HEAP_FREE,
        EV_84818, EV_86568, EV_866C8, EV_74F04, EV_750DC,
        EV_88FF4, EV_89128, EV_97D64,
        EV_HEAP_FREE, EV_HEAP_FREE, EV_HEAP_FREE, EV_HEAP_FREE,
        EV_HEAP_FREE, EV_976A0
    };
    static const u32 sources[] = {
        0x8009BC38u, 0x8009BCB0u, 0x8009BC3Cu,
        0x8009BCB4u, 0x8009C180u
    };
    int index;

    reset_test();
    D_8006259C = PSX_ADDR(ALLOCATION);
    for (index = 0; index < (int)(sizeof(sources) / sizeof(sources[0]));
         index++)
        write32(sources[index], 0x800A0000u + (u32)index * 0x100u);
    write16(0x8009BD3Au, 0xBEEFu);

    wm_80077CC0();
    check_event_kinds(expected, (int)(sizeof(expected) / sizeof(expected[0])),
                      "teardown.call_order");
    check(events[1].a0 == (uintptr_t)PSX_ADDR(ALLOCATION) &&
          events[2].kind == EV_HEAP_FREE &&
          events[2].a0 == (uintptr_t)PSX_ADDR(ALLOCATION),
          "teardown.seds_free");
    for (index = 0; index < (int)(sizeof(sources) / sizeof(sources[0]));
         index++)
        check(events[11 + index].a0 ==
              (uintptr_t)PSX_ADDR(0x800A0000u + (u32)index * 0x100u),
              "teardown.free_sequence");
    check(read16(0x8006F94Eu) == 270u &&
          read16(0x8006F954u) == 0u &&
          read32(0x8009BBC4u) == 1u &&
          read16(0x8006F950u) == 0xBEEFu,
          "teardown.state_values");
}

int main(void)
{
    test_setup();
    test_teardown();
    if (failures != 0) {
        fprintf(stderr, "W34N61 MODE9 LIFECYCLE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N61 MODE9 LIFECYCLE CERTIFICATE PASS");
    return 0;
}

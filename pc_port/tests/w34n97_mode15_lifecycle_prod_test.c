/* Focused production certificate for retail mode-15 lifecycle. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_mode15_lifecycle.h"

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
    EV_FIELD_WDS_CLEANUP,
    EV_OBJECT_MATRIX,
    EV_GPU_A,
    EV_GPU_B,
    EV_THIRD_WAVE,
    EV_BSS_CONSTANTS,
    EV_HEAP_TABLE,
    EV_UPLOAD_A,
    EV_UPLOAD_B,
    EV_DRAW_PACKETS,
    EV_TABLES,
    EV_FIRST_WDS,
    EV_ARCHIVE_INDEX,
    EV_TERRAIN_POSITION,
    EV_CD_WORK,
    EV_VSYNC,
    EV_DISTANCE,
    EV_HEAP_FREE,
    EV_SOUND_ADD,
    EV_ARCHIVE_SIZE,
    EV_SONG_CREATE,
    EV_SONG_LEVEL,
    EV_REGISTER,
    EV_978FC,
    EV_8901C,
    EV_865A0,
    EV_75228,
    EV_SONG_FADE,
    EV_SOUND_SHUTDOWN,
    EV_SOUND_RELEASE,
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

#define MAX_EVENTS 128
#define WDS_SOURCE UINT32_C(0x800B8000)
#define SONG_SOURCE UINT32_C(0x800B9000)
#define SEDS_SOURCE UINT32_C(0x800B7000)
#define MANAGER ((void *)(uintptr_t)UINT32_C(0x12345000))

static Event events[MAX_EVENTS];
static int event_count;
static int failures;
static u32 distance_values[4];
static int distance_count;
static int distance_index;
short g_SoundControlFlags;
unsigned char D_80062648[256];
void *D_80062528;
void *D_8006259C;

static void check(int condition, const char *name)
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

static int count_kind(int kind)
{
    int count = 0;
    int index;

    for (index = 0; index < event_count; index++)
        if (events[index].kind == kind)
            count++;
    return count;
}

static void reset_test(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    memset(D_80062648, 0, sizeof(D_80062648));
    memset(events, 0, sizeof(events));
    event_count = 0;
    distance_count = 0;
    distance_index = 0;
    g_SoundControlFlags = 0;
    D_80062528 = NULL;
    D_8006259C = NULL;
}

static void check_event_kinds(const int *expected, int count,
                              const char *name)
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

void wm_80072BB0(void) { record_event(EV_FRAMEBUFFER, 0u, 0u, 0u, 0u); }

int MoveImage(void *rect, int x, int y)
{
    s16 values[4];
    memcpy(values, rect, sizeof(values));
    record_event(EV_MOVE_IMAGE,
                 ((uintptr_t)(u16)values[0] << 16) |
                     (uintptr_t)(u16)values[1],
                 ((uintptr_t)(u16)values[2] << 16) |
                     (uintptr_t)(u16)values[3],
                 (uintptr_t)(unsigned int)x,
                 (uintptr_t)(unsigned int)y);
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

int wm_mode15_stage_second_wave_finish(void)
{
    record_event(EV_SECOND_FINISH, 0u, 0u, 0u, 0u);
    return 0;
}

#define STAGE(name, event_id) \
    int name(void) \
    { \
        record_event((event_id), 0u, 0u, 0u, 0u); \
        return 0; \
    }

STAGE(wm_72238_stage_object_pool, EV_OBJECT_POOL)
STAGE(wm_72238_stage_cross_products, EV_CROSS_PRODUCTS)
STAGE(wm_72238_stage_object_matrix, EV_OBJECT_MATRIX)
STAGE(wm_72238_stage_gpu_asset_a, EV_GPU_A)
STAGE(wm_72238_stage_gpu_asset_b, EV_GPU_B)
STAGE(wm_72238_stage_third_wave, EV_THIRD_WAVE)
STAGE(wm_72238_stage_bss_constants, EV_BSS_CONSTANTS)
STAGE(wm_72238_stage_heap_table, EV_HEAP_TABLE)
STAGE(wm_72238_stage_upload_a, EV_UPLOAD_A)
STAGE(wm_72238_stage_upload_b, EV_UPLOAD_B)
STAGE(wm_72238_stage_draw_packets, EV_DRAW_PACKETS)
STAGE(wm_72238_stage_88f64, EV_TABLES)

int wm_72238_stage_first_wds(void)
{
    record_event(EV_FIRST_WDS, 0u, 0u, 0u, 0u);
    return 0;
}

void ArchiveCdDataSync(int mode)
{
    record_event(EV_ARCHIVE_CD_SYNC, (uintptr_t)(unsigned int)mode,
                 0u, 0u, 0u);
}

void func_8001B66C(void)
{
    record_event(EV_FIELD_WDS_CLEANUP, 0u, 0u, 0u, 0u);
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

int ArchiveDecodeAlignedSize(unsigned int entry_index)
{
    record_event(EV_ARCHIVE_SIZE, (uintptr_t)entry_index, 0u, 0u, 0u);
    return 12;
}

void *func_80039850(void *song_file)
{
    record_event(EV_SONG_CREATE, (uintptr_t)song_file, 0u, 0u, 0u);
    return MANAGER;
}

void func_80039A80(void *manager, int level, int steps)
{
    record_event(EV_SONG_LEVEL, (uintptr_t)manager,
                 (uintptr_t)(u32)level, (uintptr_t)(u32)steps, 0u);
}

void SoundAddSedsEntry(void *seds)
{
    record_event(EV_SOUND_ADD, (uintptr_t)seds, 0u, 0u, 0u);
}

void wm_pool_register(u32 init_cb, u32 update_cb)
{
    record_event(EV_REGISTER, (uintptr_t)init_cb, (uintptr_t)update_cb,
                 0u, 0u);
}

#define VOID_EVENT(name, event_id) \
    void name(void) \
    { \
        record_event((event_id), 0u, 0u, 0u, 0u); \
    }

VOID_EVENT(wm_800978FC, EV_978FC)
VOID_EVENT(wm_8008901C, EV_8901C)
VOID_EVENT(wm_800865A0, EV_865A0)
VOID_EVENT(wm_80075228, EV_75228)
VOID_EVENT(func_80039FF8, EV_SOUND_SHUTDOWN)
VOID_EVENT(wm_80084818, EV_84818)
VOID_EVENT(wm_80086568, EV_86568)
VOID_EVENT(wm_800866C8, EV_866C8)
VOID_EVENT(wm_80074F04, EV_74F04)
VOID_EVENT(wm_800750DC, EV_750DC)
VOID_EVENT(wm_80088FF4, EV_88FF4)
VOID_EVENT(wm_80089128, EV_89128)
VOID_EVENT(wm_80097D64, EV_97D64)
VOID_EVENT(wm_800976A0, EV_976A0)

void func_8003A89C(void *manager, s32 level, s32 steps)
{
    record_event(EV_SONG_FADE, (uintptr_t)manager, (uintptr_t)(u32)level,
                 (uintptr_t)(u32)steps, 0u);
}

void func_8003852C(void *seds)
{
    record_event(EV_SOUND_RELEASE, (uintptr_t)seds, 0u, 0u, 0u);
}

unsigned int HeapFree(void *pointer)
{
    record_event(EV_HEAP_FREE, (uintptr_t)pointer, 0u, 0u, 0u);
    return 0u;
}

static void test_setup(void)
{
    static const int expected[] = {
        EV_FRAMEBUFFER, EV_MOVE_IMAGE, EV_DRAW_SYNC, EV_TRANSITION,
        EV_SECOND_FINISH, EV_OBJECT_POOL, EV_CROSS_PRODUCTS,
        EV_ARCHIVE_CD_SYNC, EV_FIELD_WDS_CLEANUP, EV_OBJECT_MATRIX,
        EV_GPU_A, EV_GPU_B, EV_THIRD_WAVE, EV_BSS_CONSTANTS, EV_HEAP_TABLE,
        EV_UPLOAD_A, EV_UPLOAD_B, EV_DRAW_PACKETS, EV_TABLES,
        EV_ARCHIVE_CD_SYNC, EV_FIRST_WDS, EV_ARCHIVE_INDEX,
        EV_TERRAIN_POSITION, EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_CD_WORK, EV_VSYNC, EV_DISTANCE, EV_HEAP_FREE, EV_SOUND_ADD,
        EV_ARCHIVE_SIZE, EV_SONG_CREATE, EV_SONG_LEVEL,
        EV_REGISTER, EV_REGISTER, EV_REGISTER, EV_REGISTER, EV_REGISTER,
        EV_REGISTER, EV_REGISTER, EV_REGISTER, EV_REGISTER, EV_REGISTER,
        EV_REGISTER, EV_978FC, EV_8901C, EV_865A0, EV_75228
    };
    static const u32 registrations[][2] = {
        {UINT32_C(0x800923A8), UINT32_C(0x800925A0)},
        {UINT32_C(0x8007DE14), UINT32_C(0x8007DE98)},
        {UINT32_C(0x8007E450), UINT32_C(0x8007E4E4)},
        {UINT32_C(0x8007ECA4), UINT32_C(0x8007EE34)},
        {UINT32_C(0x8007F8AC), UINT32_C(0x8007F968)},
        {UINT32_C(0x8007F8AC), UINT32_C(0x8007F968)},
        {UINT32_C(0x8007F8AC), UINT32_C(0x8007F968)},
        {UINT32_C(0x8007F8AC), UINT32_C(0x8007F968)},
        {UINT32_C(0x8007F8AC), UINT32_C(0x8007F968)},
        {UINT32_C(0x8007FC8C), UINT32_C(0x8007FD30)},
        {UINT32_C(0x80078948), UINT32_C(0x80078950)}
    };
    uint8_t template_bytes[32];
    uint8_t song_bytes[12];
    int index;

    reset_test();
    for (index = 0; index < 32; index++)
        template_bytes[index] = (uint8_t)(0x40 + index);
    for (index = 0; index < 12; index++)
        song_bytes[index] = (uint8_t)(0x90 + index);
    memcpy(PSX_ADDR(UINT32_C(0x8009A180)), template_bytes,
           sizeof(template_bytes));
    memcpy(PSX_ADDR(SONG_SOURCE), song_bytes, sizeof(song_bytes));
    write32(UINT32_C(0x8009D3D4), 1u);
    write16(UINT32_C(0x8009A5BC), UINT16_C(0x0123));
    write16(UINT32_C(0x8009A5BE), (u16)(s16)-32);
    write16(UINT32_C(0x8009A5C0), UINT16_C(0x0456));
    write32(UINT32_C(0x8009C88C), WDS_SOURCE);
    write32(UINT32_C(0x8009C884), SONG_SOURCE);
    write32(UINT32_C(0x8009D3D0), 7u);
    D_8006259C = PSX_ADDR(SEDS_SOURCE);
    distance_values[0] = 1u;
    distance_values[1] = 0u;
    distance_count = 2;

    check(wm_8007D918() == 0, "setup.return_zero");
    check_event_kinds(expected, (int)(sizeof(expected) / sizeof(expected[0])),
                      "setup.stage_order");
    check(events[1].a0 == 0u && events[1].a1 == UINT32_C(0x014000D8) &&
          events[1].a2 == 704u && events[1].a3 == 256u,
          "setup.move_image_args");
    check(events[3].a0 == 64u && events[3].a1 == 0u &&
          events[3].a2 == 4u && events[3].a3 == 2u,
          "setup.transition_args");
    check(count_kind(EV_FIRST_WDS) == 1, "setup.wds_load");
    check(memcmp(PSX_ADDR(UINT32_C(0x8009BE4C)), template_bytes,
                 sizeof(template_bytes)) == 0,
          "setup.template_copy");
    check(read32(UINT32_C(0x8009CCA4)) == 2u &&
          read32(UINT32_C(0x8009D3CC)) == 16u &&
          read32(UINT32_C(0x8009D804)) == 0u &&
          read32(UINT32_C(0x8009D144)) == 0u &&
          read32(UINT32_C(0x8009CD40)) == UINT32_C(0x80086700),
          "setup.state_values");
    check(read32(UINT32_C(0x8009C5AC)) == UINT32_C(0x00123000) &&
          read32(UINT32_C(0x8009C5B0)) == UINT32_C(0xFFFE0000) &&
          read32(UINT32_C(0x8009C5B4)) == UINT32_C(0x00456000),
          "setup.position_vector");
    check(events[29].a0 == (uintptr_t)PSX_ADDR(WDS_SOURCE) &&
          events[30].a0 == (uintptr_t)PSX_ADDR(SEDS_SOURCE),
          "setup.wds_release_and_seds_link");
    check(memcmp(D_80062648, song_bytes, sizeof(song_bytes)) == 0 &&
          D_80062528 == MANAGER && count_kind(EV_SONG_CREATE) == 1 &&
          count_kind(EV_SONG_LEVEL) == 1,
          "setup.song_start");
    check(events[31].a0 == 7u && events[32].a0 == (uintptr_t)D_80062648 &&
          events[33].a0 == (uintptr_t)MANAGER && events[33].a1 == 127u &&
          events[33].a2 == 0u,
          "setup.song_args");
    check(count_kind(EV_REGISTER) == 11, "setup.registration_count");
    for (index = 0; index < 11; index++)
        check(events[34 + index].a0 == registrations[index][0] &&
              events[34 + index].a1 == registrations[index][1],
              "setup.registration_pairs");
    check(distance_index == 2, "setup.cd_drain_positive");
}

static void test_teardown(void)
{
    static const int expected[] = {
        EV_SONG_FADE, EV_SOUND_SHUTDOWN, EV_SOUND_RELEASE, EV_HEAP_FREE,
        EV_84818, EV_86568, EV_866C8, EV_74F04, EV_750DC, EV_88FF4,
        EV_89128, EV_97D64, EV_HEAP_FREE, EV_HEAP_FREE, EV_HEAP_FREE,
        EV_HEAP_FREE, EV_HEAP_FREE, EV_976A0
    };
    static const u32 sources[] = {
        UINT32_C(0x8009BC38), UINT32_C(0x8009BCB0),
        UINT32_C(0x8009BC3C), UINT32_C(0x8009BCB4),
        UINT32_C(0x8009C180)
    };
    int index;

    reset_test();
    D_80062528 = MANAGER;
    D_8006259C = PSX_ADDR(SEDS_SOURCE);
    write32(UINT32_C(0x8009D3D4), 1u);
    write16(UINT32_C(0x8009A5CE), UINT16_C(0x1357));
    for (index = 0; index < (int)(sizeof(sources) / sizeof(sources[0]));
         index++)
        write32(sources[index], UINT32_C(0x800A0000) +
                                    (u32)index * UINT32_C(0x100));
    write16(UINT32_C(0x8009BD3A), UINT16_C(0xBEEF));

    wm_8007DCE0();
    check_event_kinds(expected, (int)(sizeof(expected) / sizeof(expected[0])),
                      "teardown.call_order");
    check(events[0].a0 == (uintptr_t)MANAGER && events[0].a1 == 0u &&
          events[0].a2 == 240u,
          "teardown.fade_args");
    check(events[2].a0 == (uintptr_t)PSX_ADDR(SEDS_SOURCE) &&
          events[3].a0 == (uintptr_t)PSX_ADDR(SEDS_SOURCE),
          "teardown.seds_free");
    for (index = 0; index < (int)(sizeof(sources) / sizeof(sources[0]));
         index++)
        check(events[12 + index].a0 ==
                  (uintptr_t)PSX_ADDR(UINT32_C(0x800A0000) +
                                      (u32)index * UINT32_C(0x100)),
              "teardown.free_sequence");
    check(read16(UINT32_C(0x8006F94E)) == 417u &&
          read16(UINT32_C(0x8006F950)) == UINT16_C(0xBEEF) &&
          read16(UINT32_C(0x8006F954)) == UINT16_C(0x1357) &&
          read32(UINT32_C(0x8009BBC4)) == 1u,
          "teardown.state_values");
}

int main(void)
{
    test_setup();
    test_teardown();
    if (failures != 0) {
        fprintf(stderr,
                "W34N97 MODE15 LIFECYCLE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N97 MODE15 LIFECYCLE CERTIFICATE PASS");
    return 0;
}

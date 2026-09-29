/* Focused production certificate for retail mode-17 lifecycle. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_mode17_lifecycle.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

enum EventKind {
    EV_FRAMEBUFFER = 1, EV_MOVE, EV_DRAW_SYNC, EV_TRANSITION,
    EV_SECOND_FINISH, EV_OBJECT_POOL, EV_CROSS, EV_CD_SYNC, EV_WDS_CLEAN,
    EV_OBJECT_MATRIX, EV_GPU_A, EV_GPU_B, EV_THIRD, EV_BSS, EV_CLUT,
    EV_HEAP_TABLE, EV_UPLOAD_A, EV_UPLOAD_B, EV_DRAW_PACKETS, EV_TABLES,
    EV_FIRST_WDS, EV_ARCHIVE_INDEX, EV_TERRAIN, EV_CD_WORK, EV_VSYNC,
    EV_DISTANCE, EV_HEAP_FREE, EV_SOUND_ADD, EV_ARCHIVE_SIZE, EV_SONG_CREATE,
    EV_SONG_LEVEL, EV_REGISTER, EV_978FC, EV_8901C, EV_865A0, EV_85FE0,
    EV_75228, EV_SOUND_SHUTDOWN, EV_SOUND_RELEASE, EV_84818, EV_86124,
    EV_86568, EV_866C8, EV_74F04, EV_750DC, EV_88FF4, EV_89128,
    EV_97D64, EV_976A0
};

typedef struct Event {
    int kind;
    uintptr_t a0;
    uintptr_t a1;
    uintptr_t a2;
    uintptr_t a3;
} Event;

#define MAX_EVENTS 96
#define WDS_SOURCE  UINT32_C(0x800B8000)
#define SONG_SOURCE UINT32_C(0x800B9000)
#define SEDS_SOURCE UINT32_C(0x800B7000)
#define MANAGER ((void *)(uintptr_t)UINT32_C(0x12345000))

static Event events[MAX_EVENTS];
static int event_count;
static int failures;
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

static void event(int kind, uintptr_t a0, uintptr_t a1,
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
    memset(D_80062648, 0, sizeof(D_80062648));
    event_count = 0;
    g_SoundControlFlags = 0;
    D_80062528 = NULL;
    D_8006259C = NULL;
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

static int first_kind(int kind)
{
    int index;

    for (index = 0; index < event_count; index++)
        if (events[index].kind == kind)
            return index;
    return -1;
}

void wm_80072BB0(void) { event(EV_FRAMEBUFFER, 0u, 0u, 0u, 0u); }

int MoveImage(void *rect, int x, int y)
{
    s16 values[4];
    memcpy(values, rect, sizeof(values));
    event(EV_MOVE,
          ((uintptr_t)(u16)values[0] << 16) | (uintptr_t)(u16)values[1],
          ((uintptr_t)(u16)values[2] << 16) | (uintptr_t)(u16)values[3],
          (uintptr_t)(u32)x, (uintptr_t)(u32)y);
    return 0;
}

int DrawSync(int mode)
{
    event(EV_DRAW_SYNC, (uintptr_t)(u32)mode, 0u, 0u, 0u);
    return 0;
}

int wm_80072DB4(s32 a0, s32 a1, s32 a2, s32 a3)
{
    event(EV_TRANSITION, (uintptr_t)(u32)a0, (uintptr_t)(u32)a1,
          (uintptr_t)(u32)a2, (uintptr_t)(u32)a3);
    return 0;
}

int wm_mode17_stage_second_wave_finish(void)
{
    event(EV_SECOND_FINISH, 0u, 0u, 0u, 0u);
    return 0;
}

#define STAGE(name, event_id) \
    int name(void) \
    { \
        event((event_id), 0u, 0u, 0u, 0u); \
        return 0; \
    }

STAGE(wm_72238_stage_object_pool, EV_OBJECT_POOL)
STAGE(wm_72238_stage_cross_products, EV_CROSS)
STAGE(wm_72238_stage_object_matrix, EV_OBJECT_MATRIX)
STAGE(wm_72238_stage_gpu_asset_a, EV_GPU_A)
STAGE(wm_72238_stage_gpu_asset_b, EV_GPU_B)
STAGE(wm_72238_stage_third_wave, EV_THIRD)
STAGE(wm_72238_stage_bss_constants, EV_BSS)
STAGE(wm_72238_stage_record_clut, EV_CLUT)
STAGE(wm_72238_stage_heap_table, EV_HEAP_TABLE)
STAGE(wm_72238_stage_upload_a, EV_UPLOAD_A)
STAGE(wm_72238_stage_upload_b, EV_UPLOAD_B)
STAGE(wm_72238_stage_draw_packets, EV_DRAW_PACKETS)
STAGE(wm_72238_stage_88f64, EV_TABLES)
STAGE(wm_72238_stage_first_wds, EV_FIRST_WDS)

void ArchiveCdDataSync(int mode)
{
    event(EV_CD_SYNC, (uintptr_t)(u32)mode, 0u, 0u, 0u);
}

void func_8001B66C(void) { event(EV_WDS_CLEAN, 0u, 0u, 0u, 0u); }

int ArchiveSetIndex(int directory_index, int entry_index)
{
    event(EV_ARCHIVE_INDEX, (uintptr_t)(u32)directory_index,
          (uintptr_t)(u32)entry_index, 0u, 0u);
    return 0;
}

void wm_80097BC0(u32 position)
{
    event(EV_TERRAIN, (uintptr_t)position, 0u, 0u, 0u);
}

u32 wm_800967E4(void)
{
    event(EV_CD_WORK, 0u, 0u, 0u, 0u);
    return 0u;
}

int Vsync(int mode)
{
    event(EV_VSYNC, (uintptr_t)(u32)mode, 0u, 0u, 0u);
    return 0;
}

u32 wm_80096668_circular_distance(void)
{
    event(EV_DISTANCE, 0u, 0u, 0u, 0u);
    return 0u;
}

int ArchiveDecodeAlignedSize(unsigned int entry_index)
{
    event(EV_ARCHIVE_SIZE, (uintptr_t)entry_index, 0u, 0u, 0u);
    return 12;
}

void *func_80039850(void *song_file)
{
    event(EV_SONG_CREATE, (uintptr_t)song_file, 0u, 0u, 0u);
    return MANAGER;
}

void func_80039A80(void *manager, int level, int steps)
{
    event(EV_SONG_LEVEL, (uintptr_t)manager, (uintptr_t)(u32)level,
          (uintptr_t)(u32)steps, 0u);
}

void SoundAddSedsEntry(void *seds)
{
    event(EV_SOUND_ADD, (uintptr_t)seds, 0u, 0u, 0u);
}

void wm_pool_register(u32 init_cb, u32 update_cb)
{
    event(EV_REGISTER, (uintptr_t)init_cb, (uintptr_t)update_cb, 0u, 0u);
}

#define VOID_EVENT(name, event_id) \
    void name(void) \
    { \
        event((event_id), 0u, 0u, 0u, 0u); \
    }

VOID_EVENT(wm_800978FC, EV_978FC)
VOID_EVENT(wm_8008901C, EV_8901C)
VOID_EVENT(wm_800865A0, EV_865A0)
VOID_EVENT(wm_80085FE0, EV_85FE0)
VOID_EVENT(wm_80075228, EV_75228)
VOID_EVENT(func_80039FF8, EV_SOUND_SHUTDOWN)
VOID_EVENT(wm_80084818, EV_84818)
VOID_EVENT(wm_80086124, EV_86124)
VOID_EVENT(wm_80086568, EV_86568)
VOID_EVENT(wm_800866C8, EV_866C8)
VOID_EVENT(wm_80074F04, EV_74F04)
VOID_EVENT(wm_800750DC, EV_750DC)
VOID_EVENT(wm_80088FF4, EV_88FF4)
VOID_EVENT(wm_80089128, EV_89128)
VOID_EVENT(wm_80097D64, EV_97D64)
VOID_EVENT(wm_800976A0, EV_976A0)

void func_8003852C(void *seds)
{
    event(EV_SOUND_RELEASE, (uintptr_t)seds, 0u, 0u, 0u);
}

unsigned int HeapFree(void *pointer)
{
    event(EV_HEAP_FREE, (uintptr_t)pointer, 0u, 0u, 0u);
    return 0u;
}

static void seed_setup(void)
{
    u8 template_bytes[32];
    u8 song_bytes[12];
    int index;

    reset_test();
    for (index = 0; index < 32; index++)
        template_bytes[index] = (u8)(0x40 + index);
    memcpy(PSX_ADDR(UINT32_C(0x8009A180)), template_bytes,
           sizeof(template_bytes));
    for (index = 0; index < 12; index++)
        song_bytes[index] = (u8)(0x90 + index);
    memcpy(PSX_ADDR(SONG_SOURCE), song_bytes, sizeof(song_bytes));
    write32(UINT32_C(0x8009C88C), WDS_SOURCE);
    write32(UINT32_C(0x8009C884), SONG_SOURCE);
    write32(UINT32_C(0x8009D3D0), 7u);
    D_8006259C = PSX_ADDR(SEDS_SOURCE);
}

static void test_setup(void)
{
    int register_index = 0;
    int index;
    static const u32 expected_init[] = {
        UINT32_C(0x800923A8), UINT32_C(0x800827C8),
        UINT32_C(0x800827EC), UINT32_C(0x80083214),
        UINT32_C(0x80083214), UINT32_C(0x80083214),
        UINT32_C(0x80083214), UINT32_C(0x80083214),
        UINT32_C(0x800834D0), UINT32_C(0x80076A14)
    };
    static const u32 expected_update[] = {
        UINT32_C(0x800925A0), UINT32_C(0x80076B34),
        UINT32_C(0x800828DC), UINT32_C(0x80083264),
        UINT32_C(0x80083264), UINT32_C(0x80083264),
        UINT32_C(0x80083264), UINT32_C(0x80083264),
        UINT32_C(0x800834D8), UINT32_C(0x80076A1C)
    };

    seed_setup();
    check(wm_80082324() == 0, "setup.return");
    check(events[3].kind == EV_TRANSITION && events[3].a0 == 64u &&
          events[3].a1 == 0u && events[3].a2 == 4u && events[3].a3 == 2u,
          "setup.transition_args");
    check(first_kind(EV_OBJECT_MATRIX) < first_kind(EV_GPU_A) &&
          first_kind(EV_GPU_A) < first_kind(EV_GPU_B) &&
          first_kind(EV_CLUT) < first_kind(EV_HEAP_TABLE),
          "setup.stage_order");
    check(memcmp(PSX_ADDR(UINT32_C(0x8009A180)),
                 PSX_ADDR(UINT32_C(0x8009BE4C)), 32u) == 0 &&
          read32(UINT32_C(0x8009CCA4)) == 2u &&
          read32(UINT32_C(0x8009D3CC)) == 16u &&
          read32(UINT32_C(0x8009D804)) == 0u &&
          read32(UINT32_C(0x8009D144)) == 0u &&
          read32(UINT32_C(0x8009CD40)) == UINT32_C(0x80086700),
          "setup.state_values");
    check(read32(UINT32_C(0x8009C5AC)) == UINT32_C(0x070A9000) &&
          read32(UINT32_C(0x8009C5B0)) == UINT32_C(0xFFEB8000) &&
          read32(UINT32_C(0x8009C5B4)) == UINT32_C(0x042AA000),
          "setup.position");
    check(count_kind(EV_FIRST_WDS) == 1, "setup.wds_load");
    check(D_80062528 == MANAGER && count_kind(EV_SONG_CREATE) == 1 &&
          count_kind(EV_SONG_LEVEL) == 1,
          "setup.song_start");
    check(count_kind(EV_REGISTER) == 10, "setup.registration_count");
    for (index = 0; index < event_count; index++) {
        if (events[index].kind != EV_REGISTER)
            continue;
        check(register_index < 10 &&
              events[index].a0 == (uintptr_t)expected_init[register_index] &&
              events[index].a1 == (uintptr_t)expected_update[register_index],
              "setup.registration_pairs");
        register_index++;
    }
    check(count_kind(EV_978FC) == 1 && count_kind(EV_8901C) == 1 &&
          count_kind(EV_865A0) == 1 && count_kind(EV_85FE0) == 1 &&
          count_kind(EV_75228) == 1,
          "setup.common_tail");
}

static void test_teardown(void)
{
    static const u32 globals[] = {
        UINT32_C(0x8009BC38), UINT32_C(0x8009BCB0),
        UINT32_C(0x8009BC3C), UINT32_C(0x8009BCB4),
        UINT32_C(0x8009C180)
    };
    u32 index;

    reset_test();
    D_8006259C = PSX_ADDR(SEDS_SOURCE);
    write16(UINT32_C(0x8009BD3A), UINT16_C(0x1357));
    for (index = 0u; index < 5u; index++)
        write32(globals[index], UINT32_C(0x800B0000) + index * 0x100u);
    wm_800826B4();
    check(count_kind(EV_SOUND_SHUTDOWN) == 1 &&
          count_kind(EV_SOUND_RELEASE) == 1 &&
          count_kind(EV_HEAP_FREE) == 6,
          "teardown.seds_free");
    check(first_kind(EV_84818) < first_kind(EV_86124) &&
          first_kind(EV_86124) < first_kind(EV_86568) &&
          first_kind(EV_97D64) < first_kind(EV_976A0),
          "teardown.order");
    check(read16(UINT32_C(0x8006F94E)) == 617u &&
          read16(UINT32_C(0x8006F954)) == 2u &&
          read32(UINT32_C(0x8009BBC4)) == 1u &&
          read16(UINT32_C(0x8006F950)) == UINT16_C(0x1357),
          "teardown.state_values");
}

int main(void)
{
    test_setup();
    test_teardown();
    if (failures != 0) {
        fprintf(stderr, "W34N109 MODE17 LIFECYCLE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N109 MODE17 LIFECYCLE CERTIFICATE PASS");
    return 0;
}

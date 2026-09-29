/* Focused production certificate for retail mode-8/11 lifecycle callbacks. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_mode811_lifecycle.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

enum TestEvent {
    EV_FRAMEBUFFER = 1,
    EV_MOVE_IMAGE,
    EV_DRAW_SYNC,
    EV_TRANSITION,
    EV_SECOND_FINISH,
    EV_OBJECT_POOL,
    EV_CROSS_PRODUCTS,
    EV_ARCHIVE_CD_SYNC,
    EV_OBJECT_MATRIX,
    EV_GPU_A,
    EV_GPU_B,
    EV_BSS_CONSTANTS,
    EV_RECORD_CLUT,
    EV_HEAP_TABLE,
    EV_UPLOAD_A,
    EV_UPLOAD_B,
    EV_DRAW_PACKETS,
    EV_TABLES,
    EV_ARCHIVE_INDEX,
    EV_TERRAIN_POSITION,
    EV_CD_WORK,
    EV_VSYNC,
    EV_DISTANCE,
    EV_REGISTER,
    EV_978FC,
    EV_8901C,
    EV_865A0,
    EV_85FE0,
    EV_75228,
    EV_89160,
    EV_84818,
    EV_86124,
    EV_HEAP_FREE,
    EV_866C8,
    EV_89128,
    EV_97D64
};

typedef struct TestEventRecord {
    int kind;
    u32 a0;
    u32 a1;
    u32 a2;
    u32 a3;
} TestEventRecord;

#define MAX_EVENTS 128

static TestEventRecord events[MAX_EVENTS];
static int event_count;
static int failures;
static u32 distance_values[4];
static int distance_count;
static int distance_index;
static uint8_t before_ram[PSX_RAM_SIZE];

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void record_event(int kind, u32 a0, u32 a1, u32 a2, u32 a3)
{
    if (event_count >= MAX_EVENTS) {
        fprintf(stderr, "ASSERTION event.capacity FAILED\n");
        failures++;
        return;
    }
    events[event_count].kind = kind;
    events[event_count].a0 = a0;
    events[event_count].a1 = a1;
    events[event_count].a2 = a2;
    events[event_count].a3 = a3;
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
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    memset(events, 0, sizeof(events));
    event_count = 0;
    failures = 0;
    distance_count = 0;
    distance_index = 0;
}

void wm_80072BB0(void)
{
    record_event(EV_FRAMEBUFFER, 0u, 0u, 0u, 0u);
}

int MoveImage(void* rect, int x, int y)
{
    const s16* values = (const s16*)rect;
    record_event(EV_MOVE_IMAGE,
                 ((u32)(u16)values[0] << 16) | (u32)(u16)values[1],
                 ((u32)(u16)values[2] << 16) | (u32)(u16)values[3],
                 (u32)x, (u32)y);
    return 0;
}

int DrawSync(int mode)
{
    record_event(EV_DRAW_SYNC, (u32)mode, 0u, 0u, 0u);
    return 0;
}

int Vsync(int mode)
{
    record_event(EV_VSYNC, (u32)mode, 0u, 0u, 0u);
    return 0;
}

int wm_80072DB4(s32 a0, s32 a1, s32 a2, s32 a3)
{
    record_event(EV_TRANSITION, (u32)a0, (u32)a1, (u32)a2, (u32)a3);
    return 0;
}

int wm_mode811_stage_second_wave_submit(void)
{
    return 0;
}

int wm_mode811_stage_second_wave_finish(void)
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
STAGE(wm_72238_stage_record_clut, EV_RECORD_CLUT)
STAGE(wm_72238_stage_heap_table, EV_HEAP_TABLE)
STAGE(wm_72238_stage_upload_a, EV_UPLOAD_A)
STAGE(wm_72238_stage_upload_b, EV_UPLOAD_B)
STAGE(wm_72238_stage_draw_packets, EV_DRAW_PACKETS)
STAGE(wm_72238_stage_88f64, EV_TABLES)

void ArchiveCdDataSync(int mode)
{
    record_event(EV_ARCHIVE_CD_SYNC, (u32)mode, 0u, 0u, 0u);
}

int ArchiveSetIndex(int directory_index, int entry_index)
{
    record_event(EV_ARCHIVE_INDEX, (u32)directory_index,
                 (u32)entry_index, 0u, 0u);
    return 0;
}

void wm_80097BC0(u32 position)
{
    record_event(EV_TERRAIN_POSITION, position, 0u, 0u, 0u);
}

u32 wm_800967E4(void)
{
    record_event(EV_CD_WORK, 0u, 0u, 0u, 0u);
    return 0u;
}

u32 wm_80096668_circular_distance(void)
{
    u32 value = 0u;
    if (distance_index < distance_count)
        value = distance_values[distance_index];
    distance_index++;
    record_event(EV_DISTANCE, value, 0u, 0u, 0u);
    return value;
}

void wm_pool_register(u32 a0, u32 a1)
{
    record_event(EV_REGISTER, a0, a1, 0u, 0u);
}

#define VOID_STAGE(name, event_id) \
    void name(void) { record_event((event_id), 0u, 0u, 0u, 0u); }

VOID_STAGE(wm_800978FC, EV_978FC)
VOID_STAGE(wm_8008901C, EV_8901C)
VOID_STAGE(wm_800865A0, EV_865A0)
VOID_STAGE(wm_80085FE0, EV_85FE0)
VOID_STAGE(wm_80075228, EV_75228)

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    record_event(EV_89160, a0, a1, a2, 0u);
}

VOID_STAGE(wm_80084818, EV_84818)
VOID_STAGE(wm_80086124, EV_86124)
VOID_STAGE(wm_800866C8, EV_866C8)
VOID_STAGE(wm_80089128, EV_89128)
VOID_STAGE(wm_80097D64, EV_97D64)

unsigned int HeapFree(void* ptr)
{
    uintptr_t raw = (uintptr_t)ptr;
    u32 encoded;
    uintptr_t base = (uintptr_t)&g_PsxRam[0];
    uintptr_t limit = base + (uintptr_t)PSX_RAM_SIZE;

    if (raw >= base && raw < limit)
        encoded = 0x80000000u + (u32)(raw - base);
    else
        encoded = (u32)raw;
    record_event(EV_HEAP_FREE, encoded, 0u, 0u, 0u);
    return 0u;
}

static int allowed_setup_write(u32 address)
{
    if (address >= 0x8009BE4Cu && address < 0x8009BE6Cu)
        return 1;
    if (address >= 0x8009C5ACu && address < 0x8009C5B8u)
        return 1;
    if (address >= 0x8009CCA4u && address < 0x8009CCA8u)
        return 1;
    if (address >= 0x8009D3CCu && address < 0x8009D3D0u)
        return 1;
    if (address >= 0x8009D804u && address < 0x8009D808u)
        return 1;
    if (address >= 0x8009D144u && address < 0x8009D148u)
        return 1;
    if (address >= 0x8009CD40u && address < 0x8009CD44u)
        return 1;
    return 0;
}

static int allowed_teardown_write(u32 address)
{
    if (address >= 0x8006F94Eu && address < 0x8006F956u)
        return 1;
    if (address >= 0x8009BBC4u && address < 0x8009BBC8u)
        return 1;
    return 0;
}

static void check_write_set(int teardown, const char* name)
{
    size_t i;
    for (i = 0u; i < sizeof(g_PsxRam); i++) {
        u32 address = 0x80000000u + (u32)i;
        int allowed = teardown ? allowed_teardown_write(address)
                               : allowed_setup_write(address);
        if (before_ram[i] != g_PsxRam[i] && !allowed) {
            fprintf(stderr, "ASSERTION %s FAILED at 0x%08x\n", name,
                    address);
            failures++;
            return;
        }
    }
}

static void check_event_kinds(const int* expected, int count,
                              const char* name)
{
    int i;
    if (event_count != count) {
        fprintf(stderr, "ASSERTION %s FAILED count=%d expected=%d\n",
                name, event_count, count);
        failures++;
        return;
    }
    for (i = 0; i < count; i++) {
        if (events[i].kind != expected[i]) {
            fprintf(stderr,
                    "ASSERTION %s FAILED ordinal=%d got=%d expected=%d\n",
                    name, i, events[i].kind, expected[i]);
            failures++;
            return;
        }
    }
}

static void test_setup(void)
{
    static const int expected[] = {
        EV_FRAMEBUFFER, EV_MOVE_IMAGE, EV_DRAW_SYNC, EV_TRANSITION,
        EV_SECOND_FINISH, EV_OBJECT_POOL, EV_CROSS_PRODUCTS,
        EV_ARCHIVE_CD_SYNC, EV_OBJECT_MATRIX, EV_GPU_A, EV_GPU_B,
        EV_BSS_CONSTANTS, EV_RECORD_CLUT, EV_HEAP_TABLE, EV_UPLOAD_A,
        EV_UPLOAD_B, EV_DRAW_PACKETS, EV_TABLES, EV_ARCHIVE_INDEX,
        EV_TERRAIN_POSITION, EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_CD_WORK, EV_VSYNC, EV_DISTANCE,
        EV_REGISTER, EV_REGISTER, EV_REGISTER, EV_REGISTER,
        EV_978FC, EV_8901C, EV_865A0, EV_85FE0, EV_75228, EV_89160
    };
    u8 template_bytes[32];
    int i;
    int result;
    int register_start = 26;

    reset_test();
    for (i = 0; i < 32; i++)
        template_bytes[i] = (u8)(0x30 + i);
    memcpy(PSX_ADDR(0x8009A180u), template_bytes, sizeof(template_bytes));
    distance_values[0] = 1u;
    distance_values[1] = 0u;
    distance_count = 2;
    memcpy(before_ram, g_PsxRam, sizeof(before_ram));

    result = wm_80077214();
    check(result == 0, "setup.return_zero");
    check_event_kinds(expected, (int)(sizeof(expected) / sizeof(expected[0])),
                      "setup.stage_order");
    check(events[1].a0 == 0u && events[1].a1 == 0x014000D8u &&
          events[1].a2 == 704u && events[1].a3 == 256u,
          "setup.move_image_args");
    check(events[3].a0 == 64u && events[3].a1 == 0u &&
          events[3].a2 == 4u && events[3].a3 == 2u,
          "setup.transition_args");
    check(distance_index == 2, "setup.cd_drain_positive");
    check(events[register_start + 0].a0 == 0x800923A8u &&
          events[register_start + 0].a1 == 0x800925A0u &&
          events[register_start + 1].a0 == 0x8007756Cu &&
          events[register_start + 1].a1 == 0x800776E0u &&
          events[register_start + 2].a0 == 0x80087710u &&
          events[register_start + 2].a1 == 0x80087734u &&
          events[register_start + 3].a0 == 0x80071A50u &&
          events[register_start + 3].a1 == 0x80071A58u,
          "setup.registration_pairs");
    check(memcmp(PSX_ADDR(0x8009BE4Cu), template_bytes,
                 sizeof(template_bytes)) == 0,
          "setup.template_copy");
    check(read32(0x8009CCA4u) == 2u && read32(0x8009D3CCu) == 4u &&
          read32(0x8009D804u) == 0u && read32(0x8009D144u) == 0u &&
          read32(0x8009CD40u) == 0x80086700u,
          "setup.state_values");
    check(read32(0x8009C5ACu) == 0x07702000u &&
          read32(0x8009C5B0u) == 0xFFD00000u &&
          read32(0x8009C5B4u) == 0x027C0000u,
          "setup.position_vector");
    check(events[18].a0 == 36u && events[18].a1 == 0u,
          "setup.archive_index_args");
    check(events[19].a0 == 0x8009C5ACu,
          "setup.terrain_position_arg");
    check(events[event_count - 1].a0 == 14u &&
          events[event_count - 1].a1 == 0u &&
          events[event_count - 1].a2 == 0u,
          "setup.final_initializer_args");
    check_write_set(0, "setup.write_set");
}

static void test_teardown(void)
{
    static const u32 pointer_sources[] = {
        0x8009CEB4u, 0x8009D150u, 0x8009D780u, 0x8009D7D0u,
        0x8009BDF4u, 0x8009BC38u, 0x8009BCB0u, 0x8009BC3Cu,
        0x8009BCB4u, 0x8009C180u, 0x8009BE24u
    };
    static const int expected[] = {
        EV_84818, EV_86124, EV_HEAP_FREE, EV_HEAP_FREE, EV_866C8,
        EV_HEAP_FREE, EV_HEAP_FREE, EV_HEAP_FREE, EV_89128, EV_97D64,
        EV_HEAP_FREE, EV_HEAP_FREE, EV_HEAP_FREE, EV_HEAP_FREE,
        EV_HEAP_FREE, EV_HEAP_FREE
    };
    u32 expected_free[sizeof(pointer_sources) / sizeof(pointer_sources[0])];
    int i;
    int free_index;

    reset_test();
    for (i = 0; i < (int)(sizeof(pointer_sources) / sizeof(pointer_sources[0]));
         i++) {
        expected_free[i] = 0x800A0000u + (u32)i * 0x100u;
        write32(pointer_sources[i], expected_free[i]);
    }
    write16(0x8009BD3Au, 0xBEEFu);
    memcpy(before_ram, g_PsxRam, sizeof(before_ram));

    wm_80077480();
    check_event_kinds(expected, (int)(sizeof(expected) / sizeof(expected[0])),
                      "teardown.call_order");
    free_index = 0;
    for (i = 0; i < event_count; i++) {
        if (events[i].kind == EV_HEAP_FREE) {
            if (free_index >=
                (int)(sizeof(expected_free) / sizeof(expected_free[0])) ||
                events[i].a0 != expected_free[free_index]) {
                failures++;
                fprintf(stderr,
                        "ASSERTION teardown.free_sequence FAILED ordinal=%d\n",
                        free_index);
                break;
            }
            free_index++;
        }
    }
    check(free_index ==
          (int)(sizeof(expected_free) / sizeof(expected_free[0])),
          "teardown.free_sequence");
    check(read16(0x8006F94Eu) == 17u &&
          read16(0x8006F954u) == 7u &&
          read32(0x8009BBC4u) == 1u &&
          read16(0x8006F950u) == 0xBEEFu,
          "teardown.state_values");
    check_write_set(1, "teardown.write_set");
}

int main(void)
{
    int setup_failures;

    test_setup();
    setup_failures = failures;
    test_teardown();
    failures += setup_failures;
    if (failures != 0)
        return 1;
    puts("W34N57 MODE8/11 LIFECYCLE PRODUCTION CERTIFICATE PASS");
    return 0;
}

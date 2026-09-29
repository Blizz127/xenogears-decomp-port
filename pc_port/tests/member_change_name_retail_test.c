/* Differential oracle for retail func_801C95A0 (801C95A0..801C969C).
 * The raw MIPS and installed native function share boundary mocks which record
 * exact pointer identities, call order, buffer state and GPU rectangle data. */
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"

#define g_GameState g_GameState_header_declaration_only
#include "common.h"
#include "system/menu.h"
#undef g_GameState

enum {
    RAM_BASE = 0x80000000u,
    RAM_SIZE = 0x00200000u,
    ENTRY = 0x801c95a0u,
    RETAIL_BYTES = 0xfcu,
    HALT = 0xfffffffcu,
    STACK = 0x801ff000u,
    GAME_ADDR = 0x8006d634u,
    GAME_BYTES = 0x4600u,
    BUFFER_ADDR = 0x800f0000u,
    BUFFER_BYTES = 0x3f6u,
    TABLE_U_ADDR = 0x801cb344u,
    TABLE_V_ADDR = 0x801cb390u,
    TABLE_BYTES = 0x4cu,
    MAX_EVENTS = 8u,
};

enum EventId {
    EVENT_ALLOC = 1,
    EVENT_ZERO,
    EVENT_RENDER,
    EVENT_LOAD_IMAGE,
    EVENT_DRAW_SYNC,
    EVENT_FREE,
};

typedef union NativeGameStorage {
    uint64_t alignment;
    unsigned char bytes[GAME_BYTES];
} NativeGameStorage;

typedef struct Event {
    uint32_t id;
    uint32_t args[6];
} Event;

typedef struct Trace {
    Event events[MAX_EVENTS];
    unsigned count;
    int failed;
} Trace;

typedef struct RawContext {
    unsigned char ram[RAM_SIZE];
    Trace trace;
    unsigned char game_before[GAME_BYTES];
} RawContext;

typedef struct SideResult {
    Trace trace;
    unsigned char buffer[BUFFER_BYTES];
} SideResult;

NativeGameStorage g_GameState;
SystemMenu *g_Menu;
extern int g_MemberChangeMenuCharTexcoordsU[];
extern int g_MemberChangeMenuCharTexcoordsV[];
extern void func_801C95A0(int32_t char_byte, int32_t slot);

static unsigned char native_buffer[BUFFER_BYTES];
static unsigned char native_game_before[GAME_BYTES];
static Trace *active_trace;
static unsigned checks;
static unsigned case_id;
static unsigned instruction_seen[RETAIL_BYTES / 4];
static PcPortMipsCpu *oracle_cpu;

static void require(int condition, const char *reason)
{
    checks++;
    if (!condition) {
        fprintf(stderr,
                "MEMBER_CHANGE_NAME_RETAIL_FAIL case=%u %s\n",
                case_id, reason);
        exit(1);
    }
}

static uint32_t read_le(const unsigned char *p, unsigned width)
{
    uint32_t value = 0;
    unsigned i;
    for (i = 0; i < width; i++)
        value |= (uint32_t)p[i] << (i * 8u);
    return value;
}

static void write_le(unsigned char *p, unsigned width, uint32_t value)
{
    unsigned i;
    for (i = 0; i < width; i++)
        p[i] = (unsigned char)(value >> (i * 8u));
}

static uint32_t scan32(const unsigned char *p, size_t size)
{
    uint32_t value = 2166136261u;
    size_t i;
    for (i = 0; i < size; i++) {
        value ^= p[i];
        value *= 16777619u;
    }
    return value;
}

static void add_event(Trace *trace, uint32_t id, uint32_t a0, uint32_t a1,
                      uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
    Event *event;
    if (trace == NULL || trace->count >= MAX_EVENTS) {
        if (trace != NULL)
            trace->failed = 1;
        return;
    }
    event = &trace->events[trace->count++];
    event->id = id;
    event->args[0] = a0;
    event->args[1] = a1;
    event->args[2] = a2;
    event->args[3] = a3;
    event->args[4] = a4;
    event->args[5] = a5;
}

static uint32_t native_pointer_id(const void *pointer)
{
    uintptr_t p = (uintptr_t)pointer;
    uintptr_t game = (uintptr_t)g_GameState.bytes;
    uintptr_t buffer = (uintptr_t)native_buffer;
    if (p >= buffer && p < buffer + BUFFER_BYTES)
        return BUFFER_ADDR + (uint32_t)(p - buffer);
    if (p >= game && p < game + GAME_BYTES)
        return GAME_ADDR + (uint32_t)(p - game);
    return (uint32_t)p;
}

static void render_effect(unsigned char *source, unsigned char *work,
                          uint32_t source_id, unsigned flag)
{
    uint32_t source_scan = scan32(source, 20);
    unsigned i;
    for (i = 0; i < 12; i++) {
        size_t at = (size_t)((source_id - GAME_ADDR) + flag * 29u + i * 71u)
                    % BUFFER_BYTES;
        work[at] = (unsigned char)(source_scan >> ((i & 3u) * 8u));
    }
}

void *HeapAlloc(u_int size, u_int flags)
{
    add_event(active_trace, EVENT_ALLOC, size, flags, BUFFER_ADDR,
              scan32(native_buffer, BUFFER_BYTES), 0, 0);
    return native_buffer;
}

void *bzero(unsigned char *pointer, int count)
{
    add_event(active_trace, EVENT_ZERO, native_pointer_id(pointer),
              (uint32_t)count, scan32(pointer, BUFFER_BYTES), 0, 0, 0);
    memset(pointer, 0, (size_t)count);
    return pointer;
}

s32 SystemRenderStringEntry(void *source, void *work, s32 height, s32 flag)
{
    uint32_t source_id = native_pointer_id(source);
    add_event(active_trace, EVENT_RENDER, source_id, native_pointer_id(work),
              (uint32_t)height, (uint32_t)flag,
              scan32(work, BUFFER_BYTES), scan32(source, 20));
    render_effect(source, work, source_id, (unsigned)flag);
    return 0;
}

int LoadImage(RECT *rect, u_long *pixels)
{
    add_event(active_trace, EVENT_LOAD_IMAGE,
              (uint16_t)rect->x | ((uint32_t)(uint16_t)rect->y << 16),
              (uint16_t)rect->w | ((uint32_t)(uint16_t)rect->h << 16),
              native_pointer_id(pixels), scan32((unsigned char *)pixels,
                                                BUFFER_BYTES), 0, 0);
    return 0;
}

int DrawSync(int mode)
{
    add_event(active_trace, EVENT_DRAW_SYNC, (uint32_t)mode, 0, 0, 0, 0, 0);
    return 0;
}

u_int HeapFree(void *pointer)
{
    add_event(active_trace, EVENT_FREE, native_pointer_id(pointer),
              scan32(pointer, BUFFER_BYTES), 0, 0, 0, 0);
    return 0;
}

static unsigned char *raw_pointer(RawContext *ctx, uint32_t address,
                                  unsigned width)
{
    uint32_t offset;
    if (address < RAM_BASE)
        return NULL;
    offset = address - RAM_BASE;
    if (offset > RAM_SIZE || width > RAM_SIZE - offset)
        return NULL;
    return ctx->ram + offset;
}

static int raw_read(void *opaque, uint32_t address, unsigned width,
                    uint32_t *value)
{
    RawContext *ctx = opaque;
    unsigned char *p = raw_pointer(ctx, address, width);
    if (p == NULL)
        return -1;
    *value = read_le(p, width);
    if (oracle_cpu != NULL && width == 4 && address == oracle_cpu->pc &&
        address >= ENTRY && address < ENTRY + RETAIL_BYTES)
        instruction_seen[(address - ENTRY) / 4u]++;
    return 0;
}

static int raw_write(void *opaque, uint32_t address, unsigned width,
                     uint32_t value)
{
    RawContext *ctx = opaque;
    unsigned char *p = raw_pointer(ctx, address, width);
    if (p == NULL)
        return -1;
    write_le(p, width, value);
    return 0;
}

static void raw_render(RawContext *ctx, uint32_t source, uint32_t work,
                       uint32_t height, uint32_t flag)
{
    unsigned char *source_p = raw_pointer(ctx, source, 20);
    unsigned char *work_p = raw_pointer(ctx, work, BUFFER_BYTES);
    if (source_p == NULL || work_p == NULL) {
        ctx->trace.failed = 1;
        return;
    }
    add_event(&ctx->trace, EVENT_RENDER, source, work, height, flag,
              scan32(work_p, BUFFER_BYTES), scan32(source_p, 20));
    render_effect(source_p, work_p, source, flag);
}

static int raw_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    RawContext *ctx = opaque;
    uint32_t *args = cpu->gpr + 4;
    unsigned char *p;
    if (target >= ENTRY && target < ENTRY + RETAIL_BYTES)
        return 0;
    switch (target) {
    case 0x80031bdcu:
        add_event(&ctx->trace, EVENT_ALLOC, args[0], args[1], BUFFER_ADDR,
                  scan32(raw_pointer(ctx, BUFFER_ADDR, BUFFER_BYTES),
                         BUFFER_BYTES), 0, 0);
        cpu->gpr[2] = BUFFER_ADDR;
        return 1;
    case 0x8003f8e8u:
        p = raw_pointer(ctx, args[0], args[1]);
        if (p == NULL || args[1] != BUFFER_BYTES) {
            ctx->trace.failed = 1;
            return -1;
        }
        add_event(&ctx->trace, EVENT_ZERO, args[0], args[1],
                  scan32(p, BUFFER_BYTES), 0, 0, 0);
        memset(p, 0, args[1]);
        cpu->gpr[2] = args[0];
        return 1;
    case 0x80034eacu:
        raw_render(ctx, args[0], args[1], args[2], args[3]);
        cpu->gpr[2] = 0;
        return ctx->trace.failed ? -1 : 1;
    case 0x80044894u: {
        unsigned char *rect = raw_pointer(ctx, args[0], 8);
        unsigned char *pixels = raw_pointer(ctx, args[1], BUFFER_BYTES);
        if (rect == NULL || pixels == NULL) {
            ctx->trace.failed = 1;
            return -1;
        }
        add_event(&ctx->trace, EVENT_LOAD_IMAGE,
                  read_le(rect, 2) | (read_le(rect + 2, 2) << 16),
                  read_le(rect + 4, 2) | (read_le(rect + 6, 2) << 16),
                  args[1], scan32(pixels, BUFFER_BYTES), 0, 0);
        cpu->gpr[2] = 0;
        return 1;
    }
    case 0x800445d0u:
        add_event(&ctx->trace, EVENT_DRAW_SYNC, args[0], 0, 0, 0, 0, 0);
        cpu->gpr[2] = 0;
        return 1;
    case 0x800320e8u:
        p = raw_pointer(ctx, args[0], BUFFER_BYTES);
        if (p == NULL) {
            ctx->trace.failed = 1;
            return -1;
        }
        add_event(&ctx->trace, EVENT_FREE, args[0],
                  scan32(p, BUFFER_BYTES), 0, 0, 0, 0);
        cpu->gpr[2] = 0;
        return 1;
    default:
        ctx->trace.failed = 1;
        return -1;
    }
}

static void seed_game(unsigned char *game, int32_t char_byte, int32_t slot)
{
    unsigned i;
    uint32_t seed = (uint32_t)char_byte ^ ((uint32_t)slot * 0x9e3779b9u);
    for (i = 0; i < GAME_BYTES; i++)
        game[i] = (unsigned char)(i * 37u + (i >> 3) + seed);
}

static void validate_trace(const Trace *trace, int32_t char_byte,
                           int32_t slot, const unsigned char *table_u,
                           const unsigned char *table_v)
{
    uint32_t game_offset = (((uint32_t)char_byte & 0xffu) >> 1) * 0x28u;
    uint32_t table_offset = ((uint32_t)slot << 1) & 0x1fcu;
    uint16_t expected_x = (uint16_t)read_le(table_u + table_offset, 2);
    uint16_t expected_y = (uint16_t)read_le(table_v + table_offset, 2);
    require(!trace->failed, "boundary mock failure");
    require(trace->count == 7, "boundary call count");
    require(trace->events[0].id == EVENT_ALLOC &&
            trace->events[0].args[0] == BUFFER_BYTES &&
            trace->events[0].args[1] == 0 &&
            trace->events[0].args[2] == BUFFER_ADDR,
            "HeapAlloc size/flags/result");
    require(trace->events[1].id == EVENT_ZERO &&
            trace->events[1].args[0] == BUFFER_ADDR &&
            trace->events[1].args[1] == BUFFER_BYTES,
            "bzero pointer/count/order");
    require(trace->events[2].id == EVENT_RENDER &&
            trace->events[2].args[0] == GAME_ADDR + game_offset &&
            trace->events[2].args[1] == BUFFER_ADDR &&
            trace->events[2].args[2] == 0x24 &&
            trace->events[2].args[3] == 0,
            "first string pointer/height/flag");
    require(trace->events[3].id == EVENT_RENDER &&
            trace->events[3].args[0] == GAME_ADDR + game_offset + 0x14 &&
            trace->events[3].args[1] == BUFFER_ADDR &&
            trace->events[3].args[2] == 0x24 &&
            trace->events[3].args[3] == 1,
            "second string pointer/height/flag");
    require(trace->events[4].id == EVENT_LOAD_IMAGE &&
            (trace->events[4].args[0] & 0xffffu) ==
                (uint16_t)(expected_x + 0x180u) &&
            (trace->events[4].args[0] >> 16) == expected_y &&
            trace->events[4].args[1] == (0x0du << 16 | 0x28u) &&
            trace->events[4].args[2] == BUFFER_ADDR,
            "LoadImage rectangle/table index/buffer");
    require(trace->events[5].id == EVENT_DRAW_SYNC &&
            trace->events[5].args[0] == 0,
            "DrawSync argument/order");
    require(trace->events[6].id == EVENT_FREE &&
            trace->events[6].args[0] == BUFFER_ADDR,
            "HeapFree pointer/order");
}

static void run_raw(const unsigned char *retail, const unsigned char *table_u,
                    const unsigned char *table_v, int32_t char_byte,
                    int32_t slot, SideResult *result)
{
    RawContext *ctx = calloc(1, sizeof(*ctx));
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    unsigned char *game;
    int rc;
    require(ctx != NULL, "raw context allocation");
    memcpy(raw_pointer(ctx, ENTRY, RETAIL_BYTES), retail, RETAIL_BYTES);
    memcpy(raw_pointer(ctx, TABLE_U_ADDR, TABLE_BYTES), table_u, TABLE_BYTES);
    memcpy(raw_pointer(ctx, TABLE_V_ADDR, TABLE_BYTES), table_v, TABLE_BYTES);
    game = raw_pointer(ctx, GAME_ADDR, GAME_BYTES);
    seed_game(game, char_byte, slot);
    memcpy(ctx->game_before, game, GAME_BYTES);
    memset(raw_pointer(ctx, BUFFER_ADDR, BUFFER_BYTES), 0xa5, BUFFER_BYTES);
    memset(&bus, 0, sizeof(bus));
    bus.opaque = ctx;
    bus.read = raw_read;
    bus.write = raw_write;
    bus.bridge = raw_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)char_byte;
    cpu.gpr[5] = (uint32_t)slot;
    cpu.gpr[29] = STACK;
    cpu.gpr[31] = HALT;
    oracle_cpu = &cpu;
    rc = PcPortMipsRun(&cpu, ENTRY, HALT, 1000);
    oracle_cpu = NULL;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr,
                "MEMBER_CHANGE_NAME_RETAIL_DIAG case=%u raw_rc=%d pc=%08x error=%s\n",
                case_id, rc, cpu.pc, cpu.error);
    }
    require(rc == PC_PORT_MIPS_HALTED, "raw retail execution");
    require(memcmp(game, ctx->game_before, GAME_BYTES) == 0,
            "raw game state read-only");
    validate_trace(&ctx->trace, char_byte, slot, table_u, table_v);
    result->trace = ctx->trace;
    memcpy(result->buffer, raw_pointer(ctx, BUFFER_ADDR, BUFFER_BYTES),
           BUFFER_BYTES);
    free(ctx);
}

static void run_native(int32_t char_byte, int32_t slot, SideResult *result)
{
    memset(native_buffer, 0xa5, sizeof(native_buffer));
    seed_game(g_GameState.bytes, char_byte, slot);
    memcpy(native_game_before, g_GameState.bytes, GAME_BYTES);
    memset(&result->trace, 0, sizeof(result->trace));
    active_trace = &result->trace;
    func_801C95A0(char_byte, slot);
    active_trace = NULL;
    require(memcmp(g_GameState.bytes, native_game_before, GAME_BYTES) == 0,
            "native game state read-only");
    memcpy(result->buffer, native_buffer, BUFFER_BYTES);
}

static void run_case(const unsigned char *retail, const unsigned char *table_u,
                     const unsigned char *table_v, int32_t char_byte,
                     int32_t slot)
{
    SideResult raw;
    SideResult native;
    memset(&raw, 0, sizeof(raw));
    memset(&native, 0, sizeof(native));
    run_raw(retail, table_u, table_v, char_byte, slot, &raw);
    run_native(char_byte, slot, &native);
    if (memcmp(&raw.trace, &native.trace, sizeof(raw.trace)) != 0) {
        unsigned i;
        fprintf(stderr,
                "MEMBER_CHANGE_NAME_RETAIL_DIAG case=%u char=%08x slot=%08x raw_n=%u native_n=%u\n",
                case_id, (uint32_t)char_byte, (uint32_t)slot,
                raw.trace.count, native.trace.count);
        for (i = 0; i < raw.trace.count || i < native.trace.count; i++) {
            const Event *a = &raw.trace.events[i];
            const Event *b = &native.trace.events[i];
            fprintf(stderr,
                    " event%u raw=%u:%08x,%08x,%08x,%08x,%08x,%08x native=%u:%08x,%08x,%08x,%08x,%08x,%08x\n",
                    i, a->id, a->args[0], a->args[1], a->args[2], a->args[3], a->args[4], a->args[5],
                    b->id, b->args[0], b->args[1], b->args[2], b->args[3], b->args[4], b->args[5]);
        }
    }
    require(memcmp(&raw.trace, &native.trace, sizeof(raw.trace)) == 0,
            "native/raw helper trace");
    require(memcmp(raw.buffer, native.buffer, BUFFER_BYTES) == 0,
            "native/raw complete work buffer");
    case_id++;
}

static int load_exact(const char *path, unsigned char *data, size_t size)
{
    FILE *stream = fopen(path, "rb");
    size_t got;
    if (stream == NULL)
        return -1;
    got = fread(data, 1, size, stream);
    if (got != size || fgetc(stream) != EOF || fclose(stream) != 0)
        return -1;
    return 0;
}

int main(int argc, char **argv)
{
    static const int32_t character_values[] = {
        INT_MIN, INT_MAX, -1, 0, 1, 2, 0xfe, 0xff,
        0x12345678, -0x1234567
    };
    unsigned char retail[RETAIL_BYTES];
    unsigned char table_u[TABLE_BYTES];
    unsigned char table_v[TABLE_BYTES];
    unsigned low;
    unsigned form;
    require(argc == 4, "usage: retail table-u table-v");
    require(load_exact(argv[1], retail, sizeof(retail)) == 0,
            "load retail function");
    require(load_exact(argv[2], table_u, sizeof(table_u)) == 0,
            "load retail U table");
    require(load_exact(argv[3], table_v, sizeof(table_v)) == 0,
            "load retail V table");
    for (low = 0; low <= 10; low++) {
        int32_t slots[4] = {
            (int32_t)low,
            (int32_t)(0x12340000u | low),
            (int32_t)low - 0x100,
            (int32_t)(0x80000000u | low),
        };
        for (form = 0; form < 4; form++) {
            int32_t character = character_values[
                (low * 4u + form) %
                (sizeof(character_values) / sizeof(character_values[0]))];
            run_case(retail, table_u, table_v, character, slots[form]);
        }
    }
    for (low = 0; low < RETAIL_BYTES / 4; low++)
        require(instruction_seen[low] != 0, "retail instruction coverage");
    printf("MEMBER_CHANGE_NAME_RETAIL_PASS cases=%u checks=%u "
           "retail_instructions=%u\n", case_id, checks, RETAIL_BYTES / 4);
    return 0;
}

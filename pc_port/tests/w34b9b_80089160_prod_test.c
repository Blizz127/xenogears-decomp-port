/*
 * W34B9-B -- retail-authority certificate for wm_80089160.
 *
 * This test links the production world_map_common_tail.c translation unit.
 * When that TU is compiled with WM_89160_TEST_TRACE, its compile-time-only
 * seam reports each semantic guest-memory access made by wm_80089160.  The
 * oracle below is deliberately declarative: addresses, retail PCs, access
 * kinds, widths, values, path bodies, and their order are transcribed from
 * the frozen retail listing, not derived from the production control flow.
 *
 * The paired SWL/SWR operations in Paths B/D ultimately touch the same
 * aligned host word for these record offsets.  A final-memory/address-only
 * oracle therefore cannot prove their order.  The trace compares the frozen
 * raw instruction PCs and semantic kinds (SWL before SWR at 0x...90/94 and
 * 0x...98/9C), in addition to checking their byte effects independently.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

/* Link-only stubs for other functions in world_map_common_tail.c. */
void *HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocSize;
    (void)allocFlags;
    return NULL;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
    (void)tp;
    (void)abr;
    (void)x;
    (void)y;
    return 0;
}

u_short GetClut(int x, int y)
{
    (void)x;
    (void)y;
    return 0;
}

void SystemTransferPaletteToVRAM(short xDest, short yDest)
{
    (void)xDest;
    (void)yDest;
}

enum {
    TRACE_LBU = 1,
    TRACE_LHU = 2,
    TRACE_LW  = 3,
    TRACE_SB  = 4,
    TRACE_SH  = 5,
    TRACE_SW  = 6,
    TRACE_LWL = 7,
    TRACE_LWR = 8,
    TRACE_SWL = 9,
    TRACE_SWR = 10
};

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 addr;
    u32 width;
    u32 value;
} TraceEvent;

#define TRACE_CAPACITY 512u

static TraceEvent g_actual_trace[TRACE_CAPACITY];
static size_t g_actual_count;
static int g_actual_overflow;

/* Called only by the WM_89160_TEST_TRACE production seam. */
void wm_89160_test_trace(u32 pc, u32 kind, u32 addr, u32 width, u32 value)
{
    if (g_actual_count < TRACE_CAPACITY) {
        TraceEvent *event = &g_actual_trace[g_actual_count];
        event->pc = pc;
        event->kind = kind;
        event->addr = addr;
        event->width = width;
        event->value = value;
    } else {
        g_actual_overflow = 1;
    }
    g_actual_count++;
}

static int g_total;
static int g_pass;
static int g_fail;

static void check(const char *name, int condition)
{
    g_total++;
    if (condition) {
        g_pass++;
        printf("  PASS: %s\n", name);
    } else {
        g_fail++;
        printf("  FAIL: %s\n", name);
    }
}

#define TABLE_PTR_ADDR  0x8009BCC0u
#define TABLE_BASE      0x80040000u
#define TARGET_INDEX    3u
#define RECORD_STRIDE   0x2A0u
#define SUB_STRIDE      0x54u
#define SUB_COUNT       8u
#define TARGET_RECORD   (TABLE_BASE + TARGET_INDEX * RECORD_STRIDE)
#define COPY_SRC_BASE   0x80010040u
#define HALF_SRC_BASE   0x80020040u
#define FLAG_OFFSET     0x4Fu
#define FLAG_BIT        0x80u
#define ADDRESS_MASK    0x1FFFFFu

typedef enum RetailPath {
    PATH_A,
    PATH_B,
    PATH_C,
    PATH_D
} RetailPath;

typedef struct TestCase {
    const char *name;
    RetailPath path;
    int dirty;
    u32 copy_alignment;
    u32 finite_fixture;
    size_t declared_trace_count;
    size_t declared_store_count;
} TestCase;

typedef struct Oracle {
    uint8_t *memory;
    uint8_t *write_mask;
    TraceEvent trace[TRACE_CAPACITY];
    size_t trace_count;
    int trace_overflow;
} Oracle;

static size_t ram_index(u32 addr)
{
    return (size_t)(addr & ADDRESS_MASK);
}

static u8 mem_read8(const uint8_t *memory, u32 addr)
{
    return memory[ram_index(addr)];
}

static u16 mem_read16(const uint8_t *memory, u32 addr)
{
    size_t index = ram_index(addr);
    return (u16)((u16)memory[index] |
                 (u16)((u16)memory[index + 1u] << 8));
}

static u32 mem_read32(const uint8_t *memory, u32 addr)
{
    size_t index = ram_index(addr);
    return (u32)memory[index] |
           ((u32)memory[index + 1u] << 8) |
           ((u32)memory[index + 2u] << 16) |
           ((u32)memory[index + 3u] << 24);
}

static void mem_write8(uint8_t *memory, u32 addr, u8 value)
{
    memory[ram_index(addr)] = value;
}

static void mem_write16(uint8_t *memory, u32 addr, u16 value)
{
    size_t index = ram_index(addr);
    memory[index] = (u8)value;
    memory[index + 1u] = (u8)(value >> 8);
}

static void mem_write32(uint8_t *memory, u32 addr, u32 value)
{
    size_t index = ram_index(addr);
    memory[index] = (u8)value;
    memory[index + 1u] = (u8)(value >> 8);
    memory[index + 2u] = (u8)(value >> 16);
    memory[index + 3u] = (u8)(value >> 24);
}

static const char *kind_name(u32 kind)
{
    switch (kind) {
    case TRACE_LBU: return "LBU";
    case TRACE_LHU: return "LHU";
    case TRACE_LW:  return "LW";
    case TRACE_SB:  return "SB";
    case TRACE_SH:  return "SH";
    case TRACE_SW:  return "SW";
    case TRACE_LWL: return "LWL";
    case TRACE_LWR: return "LWR";
    case TRACE_SWL: return "SWL";
    case TRACE_SWR: return "SWR";
    default:        return "UNKNOWN";
    }
}

static void oracle_event(Oracle *oracle, u32 pc, u32 kind, u32 addr,
                         u32 width, u32 value)
{
    if (oracle->trace_count < TRACE_CAPACITY) {
        TraceEvent *event = &oracle->trace[oracle->trace_count];
        event->pc = pc;
        event->kind = kind;
        event->addr = addr;
        event->width = width;
        event->value = value;
    } else {
        oracle->trace_overflow = 1;
    }
    oracle->trace_count++;
}

static u8 oracle_lbu(Oracle *oracle, u32 pc, u32 addr)
{
    u8 value = mem_read8(oracle->memory, addr);
    oracle_event(oracle, pc, TRACE_LBU, addr, 1u, (u32)value);
    return value;
}

static u16 oracle_lhu(Oracle *oracle, u32 pc, u32 addr)
{
    u16 value = mem_read16(oracle->memory, addr);
    oracle_event(oracle, pc, TRACE_LHU, addr, 2u, (u32)value);
    return value;
}

static u32 oracle_lw(Oracle *oracle, u32 pc, u32 addr)
{
    u32 value = mem_read32(oracle->memory, addr);
    oracle_event(oracle, pc, TRACE_LW, addr, 4u, value);
    return value;
}

static void mark_write(Oracle *oracle, u32 addr, u32 width)
{
    u32 i;
    for (i = 0; i < width; i++) {
        oracle->write_mask[ram_index(addr + i)] = 1u;
    }
}

static void oracle_sb(Oracle *oracle, u32 pc, u32 addr, u8 value)
{
    mem_write8(oracle->memory, addr, value);
    mark_write(oracle, addr, 1u);
    oracle_event(oracle, pc, TRACE_SB, addr, 1u, (u32)value);
}

static void oracle_sh(Oracle *oracle, u32 pc, u32 addr, u16 value)
{
    mem_write16(oracle->memory, addr, value);
    mark_write(oracle, addr, 2u);
    oracle_event(oracle, pc, TRACE_SH, addr, 2u, (u32)value);
}

static void oracle_sw(Oracle *oracle, u32 pc, u32 addr, u32 value)
{
    mem_write32(oracle->memory, addr, value);
    mark_write(oracle, addr, 4u);
    oracle_event(oracle, pc, TRACE_SW, addr, 4u, value);
}

/* Independent, switch-declared R3000A little-endian partial load values. */
static u32 oracle_lwl_value(const uint8_t *memory, u32 addr)
{
    u32 base = addr & ~3u;
    switch (addr & 3u) {
    case 0u:
        return (u32)mem_read8(memory, base) << 24;
    case 1u:
        return ((u32)mem_read8(memory, base) << 16) |
               ((u32)mem_read8(memory, base + 1u) << 24);
    case 2u:
        return ((u32)mem_read8(memory, base) << 8) |
               ((u32)mem_read8(memory, base + 1u) << 16) |
               ((u32)mem_read8(memory, base + 2u) << 24);
    default:
        return mem_read32(memory, base);
    }
}

static u32 oracle_lwr_value(const uint8_t *memory, u32 addr)
{
    u32 base = addr & ~3u;
    switch (addr & 3u) {
    case 0u:
        return mem_read32(memory, base);
    case 1u:
        return (u32)mem_read8(memory, base + 1u) |
               ((u32)mem_read8(memory, base + 2u) << 8) |
               ((u32)mem_read8(memory, base + 3u) << 16);
    case 2u:
        return (u32)mem_read8(memory, base + 2u) |
               ((u32)mem_read8(memory, base + 3u) << 8);
    default:
        return (u32)mem_read8(memory, base + 3u);
    }
}

static u32 oracle_lwl(Oracle *oracle, u32 pc, u32 addr)
{
    u32 value = oracle_lwl_value(oracle->memory, addr);
    u32 width = (addr & 3u) + 1u;
    oracle_event(oracle, pc, TRACE_LWL, addr, width, value);
    return value;
}

static u32 oracle_lwr(Oracle *oracle, u32 pc, u32 addr)
{
    u32 value = oracle_lwr_value(oracle->memory, addr);
    u32 width = 4u - (addr & 3u);
    oracle_event(oracle, pc, TRACE_LWR, addr, width, value);
    return value;
}

static void oracle_swl(Oracle *oracle, u32 pc, u32 addr, u32 value)
{
    u32 base = addr & ~3u;
    u32 offset = addr & 3u;
    u32 j;
    for (j = 0; j <= offset; j++) {
        u32 shift = (3u - offset + j) * 8u;
        mem_write8(oracle->memory, base + j, (u8)(value >> shift));
        mark_write(oracle, base + j, 1u);
    }
    oracle_event(oracle, pc, TRACE_SWL, addr, offset + 1u, value);
}

static void oracle_swr(Oracle *oracle, u32 pc, u32 addr, u32 value)
{
    u32 base = addr & ~3u;
    u32 offset = addr & 3u;
    u32 j;
    for (j = offset; j < 4u; j++) {
        u32 shift = (j - offset) * 8u;
        mem_write8(oracle->memory, base + j, (u8)(value >> shift));
        mark_write(oracle, base + j, 1u);
    }
    oracle_event(oracle, pc, TRACE_SWR, addr, 4u - offset, value);
}

static u16 negate_low16(u16 value)
{
    return (u16)(0u - (u32)value);
}

typedef struct PathPcs {
    u32 flag_lbu;
    u32 flag_sb;
    u32 clean_lhu;
    u32 clean_lw;
    u32 clean_sh_zero;
    u32 clean_sh_copy;
    u32 clean_sw_copy;
} PathPcs;

static const PathPcs g_path_pcs[4] = {
    { 0x800891ECu, 0x800891F8u, 0x800891FCu, 0x80089200u,
      0x80089204u, 0x80089208u, 0x8008920Cu },
    { 0x8008925Cu, 0x80089268u, 0x8008926Cu, 0x80089270u,
      0x80089274u, 0x80089278u, 0x8008927Cu },
    { 0x800892E0u, 0x800892ECu, 0x800892F0u, 0x800892F4u,
      0x800892F8u, 0x800892FCu, 0x80089300u },
    { 0x8008935Cu, 0x80089368u, 0x8008936Cu, 0x80089370u,
      0x80089374u, 0x80089378u, 0x8008937Cu }
};

static void oracle_clean_block(Oracle *oracle, RetailPath path, u32 sub)
{
    const PathPcs *pcs = &g_path_pcs[(unsigned)path];
    u8 flag = oracle_lbu(oracle, pcs->flag_lbu, sub + 0x4Fu);
    u16 heading;
    u32 root_word;

    oracle_sb(oracle, pcs->flag_sb, sub + 0x4Fu, (u8)(flag | FLAG_BIT));

    /* Retail completes both loads before any of these three stores. */
    heading = oracle_lhu(oracle, pcs->clean_lhu, sub + 0x10u);
    root_word = oracle_lw(oracle, pcs->clean_lw, sub + 0x00u);
    oracle_sh(oracle, pcs->clean_sh_zero, sub + 0x0Au, 0u);
    oracle_sh(oracle, pcs->clean_sh_copy, sub + 0x12u, heading);
    oracle_sw(oracle, pcs->clean_sw_copy, sub + 0x04u, root_word);
}

static void oracle_unaligned_copy(Oracle *oracle, RetailPath path, u32 sub,
                                  u32 source)
{
    u32 lwl0_pc = path == PATH_B ? 0x80089280u : 0x80089380u;
    u32 lwr0_pc = path == PATH_B ? 0x80089284u : 0x80089384u;
    u32 lwl1_pc = path == PATH_B ? 0x80089288u : 0x80089388u;
    u32 lwr1_pc = path == PATH_B ? 0x8008928Cu : 0x8008938Cu;
    u32 swl0_pc = path == PATH_B ? 0x80089290u : 0x80089390u;
    u32 swr0_pc = path == PATH_B ? 0x80089294u : 0x80089394u;
    u32 swl1_pc = path == PATH_B ? 0x80089298u : 0x80089398u;
    u32 swr1_pc = path == PATH_B ? 0x8008929Cu : 0x8008939Cu;
    u32 word0_l = oracle_lwl(oracle, lwl0_pc, source + 3u);
    u32 word0_r = oracle_lwr(oracle, lwr0_pc, source);
    u32 word1_l = oracle_lwl(oracle, lwl1_pc, source + 7u);
    u32 word1_r = oracle_lwr(oracle, lwr1_pc, source + 4u);
    u32 word0 = word0_l | word0_r;
    u32 word1 = word1_l | word1_r;

    oracle_swl(oracle, swl0_pc, sub + 0x17u, word0);
    oracle_swr(oracle, swr0_pc, sub + 0x14u, word0);
    oracle_swl(oracle, swl1_pc, sub + 0x1Bu, word1);
    oracle_swr(oracle, swr1_pc, sub + 0x18u, word1);
}

static void oracle_negated_halfwords(Oracle *oracle, RetailPath path, u32 sub,
                                     u32 fixed_source)
{
    u32 load0_pc = path == PATH_C ? 0x8008930Cu : 0x800893A0u;
    u32 store0_pc = path == PATH_C ? 0x80089318u : 0x800893ACu;
    u32 load1_pc = path == PATH_C ? 0x8008931Cu : 0x800893B0u;
    u32 store1_pc = path == PATH_C ? 0x80089328u : 0x800893BCu;
    u32 load2_pc = path == PATH_C ? 0x8008932Cu : 0x800893C0u;
    u32 store2_pc = path == PATH_C ? 0x80089338u : 0x800893CCu;
    u16 value0 = oracle_lhu(oracle, load0_pc, fixed_source);
    u16 value1;
    u16 value2;

    oracle_sh(oracle, store0_pc, sub + 0x1Cu, negate_low16(value0));
    value1 = oracle_lhu(oracle, load1_pc, fixed_source + 2u);
    oracle_sh(oracle, store1_pc, sub + 0x1Eu, negate_low16(value1));
    value2 = oracle_lhu(oracle, load2_pc, fixed_source + 4u);
    oracle_sh(oracle, store2_pc, sub + 0x20u, negate_low16(value2));
}

static void build_retail_oracle(Oracle *oracle, const TestCase *test,
                                u32 copy_source, u32 half_source)
{
    u32 table_base;
    u32 record;
    u8 initial_flag;
    u32 i;

    table_base = oracle_lw(oracle, 0x80089180u, TABLE_PTR_ADDR);
    record = table_base + TARGET_INDEX * RECORD_STRIDE;
    initial_flag = oracle_lbu(oracle, 0x8008918Cu, record + FLAG_OFFSET);
    (void)initial_flag;
    table_base = oracle_lw(oracle, 0x800891BCu, TABLE_PTR_ADDR);
    record = table_base + TARGET_INDEX * RECORD_STRIDE;

    for (i = 0; i < SUB_COUNT; i++) {
        u32 sub = record + i * SUB_STRIDE;

        if (!test->dirty) {
            oracle_clean_block(oracle, test->path, sub);
        }

        switch (test->path) {
        case PATH_A:
            oracle_sw(oracle, 0x80089210u, sub + 0x1Cu, 0u);
            oracle_sw(oracle, 0x80089214u, sub + 0x14u, 0u);
            oracle_sh(oracle, 0x80089218u, sub + 0x20u, 0u);
            oracle_sh(oracle, 0x8008921Cu, sub + 0x18u, 0u);
            break;
        case PATH_B:
            oracle_unaligned_copy(oracle, PATH_B, sub, copy_source);
            oracle_sw(oracle, 0x800892A0u, sub + 0x1Cu, 0u);
            oracle_sh(oracle, 0x800892A4u, sub + 0x20u, 0u);
            break;
        case PATH_C:
            oracle_sw(oracle, 0x80089304u, sub + 0x14u, 0u);
            oracle_sh(oracle, 0x80089308u, sub + 0x18u, 0u);
            oracle_negated_halfwords(oracle, PATH_C, sub, half_source);
            break;
        case PATH_D:
            oracle_unaligned_copy(oracle, PATH_D, sub, copy_source);
            oracle_negated_halfwords(oracle, PATH_D, sub, half_source);
            break;
        }
    }
}

static u8 seed_pattern(size_t index, u32 salt)
{
    u32 x = (u32)index;
    x ^= salt * 0x45D9F3Bu;
    x ^= x >> 11;
    x *= 0x27D4EB2Du;
    x ^= x >> 15;
    return (u8)(x | 1u);
}

static const u16 g_finite_values[4][3] = {
    { 0x0000u, 0x0001u, 0x7FFFu },
    { 0x8000u, 0xFFFFu, 0x1234u },
    { 0x8001u, 0xFFFEu, 0x4000u },
    { 0xC000u, 0x00FFu, 0xA55Au }
};

static void seed_case(const TestCase *test, u32 case_number,
                      u32 copy_source, u32 half_source)
{
    size_t i;
    u32 sub_index;

    for (i = 0; i < (size_t)PSX_RAM_SIZE; i++) {
        g_PsxRam[i] = seed_pattern(i, case_number + 1u);
    }

    mem_write32(g_PsxRam, TABLE_PTR_ADDR, TABLE_BASE);

    for (sub_index = 0; sub_index < SUB_COUNT; sub_index++) {
        u32 sub = TARGET_RECORD + sub_index * SUB_STRIDE;
        u32 root_word = 0x10203040u ^ (sub_index * 0x11121314u) ^ case_number;
        u16 heading = (u16)(0x1101u + sub_index * 0x101u + case_number);
        u8 flag = (u8)(0x01u + sub_index * 3u + case_number);
        u32 source_block = half_source + sub_index * SUB_STRIDE;
        u16 base0 = g_finite_values[test->finite_fixture & 3u][0];
        u16 base1 = g_finite_values[test->finite_fixture & 3u][1];
        u16 base2 = g_finite_values[test->finite_fixture & 3u][2];

        flag &= 0x7Fu;
        if (test->dirty && sub_index == 0u) {
            flag |= FLAG_BIT;
        }
        mem_write32(g_PsxRam, sub + 0x00u, root_word);
        mem_write16(g_PsxRam, sub + 0x10u, heading);
        mem_write8(g_PsxRam, sub + FLAG_OFFSET, flag);

        /* Block zero contains finite-width edge cases.  Blocks 1..7 are
         * intentionally different: advancing a2 by 0x54 is observable. */
        mem_write16(g_PsxRam, source_block + 0u,
                    (u16)(base0 + (u16)(sub_index * 0x1111u)));
        mem_write16(g_PsxRam, source_block + 2u,
                    (u16)(base1 + (u16)(sub_index * 0x0101u)));
        mem_write16(g_PsxRam, source_block + 4u,
                    (u16)(base2 ^ (u16)(sub_index * 0x2222u)));
    }

    for (i = 0; i < 16u; i++) {
        mem_write8(g_PsxRam, copy_source - 4u + (u32)i,
                   (u8)(0x31u + (u8)(i * 13u) + (u8)case_number));
    }
}

static int event_equal(const TraceEvent *left, const TraceEvent *right)
{
    return left->pc == right->pc &&
           left->kind == right->kind &&
           left->addr == right->addr &&
           left->width == right->width &&
           left->value == right->value;
}

static int is_store_kind(u32 kind)
{
    return kind == TRACE_SB || kind == TRACE_SH || kind == TRACE_SW ||
           kind == TRACE_SWL || kind == TRACE_SWR;
}

static size_t count_store_events(const TraceEvent *trace, size_t count)
{
    size_t result = 0u;
    size_t i;
    for (i = 0; i < count && i < TRACE_CAPACITY; i++) {
        if (is_store_kind(trace[i].kind)) {
            result++;
        }
    }
    return result;
}

static int fixture_is_asymmetric(const uint8_t *before, RetailPath path,
                                 u32 half_source)
{
    u32 i;
    u32 first_root = mem_read32(before, TARGET_RECORD);
    u16 first_heading = mem_read16(before, TARGET_RECORD + 0x10u);

    for (i = 1u; i < SUB_COUNT; i++) {
        u32 sub = TARGET_RECORD + i * SUB_STRIDE;
        if (mem_read32(before, sub) == first_root ||
            mem_read16(before, sub + 0x10u) == first_heading) {
            return 0;
        }
        if ((path == PATH_C || path == PATH_D) &&
            memcmp(&before[ram_index(half_source)],
                   &before[ram_index(half_source + i * SUB_STRIDE)], 6u) == 0) {
            return 0;
        }
    }
    return 1;
}

static int compare_trace(const char *case_name, const Oracle *oracle)
{
    size_t limit = oracle->trace_count < g_actual_count
                       ? oracle->trace_count : g_actual_count;
    size_t i;

    if (g_actual_overflow || oracle->trace_overflow ||
        g_actual_count != oracle->trace_count) {
        printf("    TRACE-DIAG %s: expected=%zu actual=%zu overflow=%d/%d\n",
               case_name, oracle->trace_count, g_actual_count,
               oracle->trace_overflow, g_actual_overflow);
        return 0;
    }

    for (i = 0; i < limit; i++) {
        if (!event_equal(&oracle->trace[i], &g_actual_trace[i])) {
            const TraceEvent *expected = &oracle->trace[i];
            const TraceEvent *actual = &g_actual_trace[i];
            printf("    TRACE-DIAG %s[%zu]: "
                   "expected pc=%08x %s addr=%08x w=%u val=%08x; "
                   "actual pc=%08x %s addr=%08x w=%u val=%08x\n",
                   case_name, i,
                   expected->pc, kind_name(expected->kind), expected->addr,
                   expected->width, expected->value,
                   actual->pc, kind_name(actual->kind), actual->addr,
                   actual->width, actual->value);
            return 0;
        }
    }
    return 1;
}

static int compare_outside_writes(const uint8_t *before,
                                  const uint8_t *write_mask)
{
    size_t i;
    for (i = 0; i < (size_t)PSX_RAM_SIZE; i++) {
        if (write_mask[i] == 0u && g_PsxRam[i] != before[i]) {
            printf("    RAM-DIAG unauthorized host offset=%08zx before=%02x after=%02x\n",
                   i, before[i], g_PsxRam[i]);
            return 0;
        }
    }
    return 1;
}

static int compare_range(const uint8_t *left, const uint8_t *right,
                         u32 addr, size_t length)
{
    return memcmp(&left[ram_index(addr)], &right[ram_index(addr)], length) == 0;
}

static int compare_internal_canaries(const uint8_t *before,
                                     const uint8_t *write_mask)
{
    u32 i;
    for (i = 0; i < SUB_COUNT; i++) {
        u32 sub = TARGET_RECORD + i * SUB_STRIDE;
        u32 byte;
        for (byte = 0; byte < SUB_STRIDE; byte++) {
            size_t index = ram_index(sub + byte);
            if (write_mask[index] == 0u && g_PsxRam[index] != before[index]) {
                return 0;
            }
        }
    }
    return 1;
}

static void run_case(const TestCase *test, u32 case_number,
                     uint8_t *before, uint8_t *expected, uint8_t *write_mask)
{
    char check_name[160];
    Oracle oracle;
    u32 copy_source = COPY_SRC_BASE + test->copy_alignment;
    u32 half_source = HALF_SRC_BASE;
    u32 a1 = (test->path == PATH_B || test->path == PATH_D) ? copy_source : 0u;
    u32 a2 = (test->path == PATH_C || test->path == PATH_D) ? half_source : 0u;
    u32 i;

    seed_case(test, case_number, copy_source, half_source);
    memcpy(before, g_PsxRam, (size_t)PSX_RAM_SIZE);
    memcpy(expected, g_PsxRam, (size_t)PSX_RAM_SIZE);
    memset(write_mask, 0, (size_t)PSX_RAM_SIZE);
    memset(&oracle, 0, sizeof(oracle));
    oracle.memory = expected;
    oracle.write_mask = write_mask;
    build_retail_oracle(&oracle, test, copy_source, half_source);

    memset(g_actual_trace, 0, sizeof(g_actual_trace));
    g_actual_count = 0u;
    g_actual_overflow = 0;
    wm_89160_reset();
    wm_80089160(TARGET_INDEX, a1, a2);

    (void)snprintf(check_name, sizeof(check_name), "%s: call counter == 1",
                   test->name);
    check(check_name, wm_89160_get_calls() == 1);
    (void)snprintf(check_name, sizeof(check_name), "%s: exactly 8 iterations",
                   test->name);
    check(check_name, wm_89160_get_iterations() == 8);
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: oracle trace count is declared retail %zu",
                   test->name, test->declared_trace_count);
    check(check_name, oracle.trace_count == test->declared_trace_count);
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: exact retail store count %zu", test->name,
                   test->declared_store_count);
    check(check_name,
          count_store_events(g_actual_trace, g_actual_count) ==
              test->declared_store_count &&
          count_store_events(oracle.trace, oracle.trace_count) ==
              test->declared_store_count);
    (void)snprintf(check_name, sizeof(check_name), "%s: exact trace count %zu",
                   test->name, oracle.trace_count);
    check(check_name, !g_actual_overflow && !oracle.trace_overflow &&
                      g_actual_count == oracle.trace_count);
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: ordered PC/kind/address/width/value trace", test->name);
    check(check_name, compare_trace(test->name, &oracle));
    (void)snprintf(check_name, sizeof(check_name), "%s: full 3 MiB RAM image",
                   test->name);
    check(check_name,
          memcmp(g_PsxRam, expected, (size_t)PSX_RAM_SIZE) == 0);
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: every byte outside retail write set unchanged",
                   test->name);
    check(check_name, compare_outside_writes(before, write_mask));
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: internal subrecord field canaries", test->name);
    check(check_name, compare_internal_canaries(before, write_mask));
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: preceding/following record canaries", test->name);
    check(check_name,
          compare_range(g_PsxRam, before, TARGET_RECORD - SUB_STRIDE,
                        SUB_STRIDE) &&
          compare_range(g_PsxRam, before, TARGET_RECORD + RECORD_STRIDE,
                        SUB_STRIDE));
    (void)snprintf(check_name, sizeof(check_name), "%s: source regions read-only",
                   test->name);
    check(check_name,
          compare_range(g_PsxRam, before, copy_source - 8u, 32u) &&
          compare_range(g_PsxRam, before, half_source - 8u,
                        (size_t)(SUB_COUNT * SUB_STRIDE + 32u)));
    (void)snprintf(check_name, sizeof(check_name),
                   "%s: all eight fixture records/sources asymmetric",
                   test->name);
    check(check_name, fixture_is_asymmetric(before, test->path, half_source));

    for (i = 0; i < SUB_COUNT; i++) {
        (void)snprintf(check_name, sizeof(check_name),
                       "%s: asymmetric record %u exact", test->name, i);
        check(check_name,
              compare_range(g_PsxRam, expected,
                            TARGET_RECORD + i * SUB_STRIDE, SUB_STRIDE));
    }
}

int main(void)
{
    static const TestCase cases[] = {
        { "A-clean", PATH_A, 0, 0u, 0u, 91u, 64u },
        { "A-dirty", PATH_A, 1, 0u, 1u, 35u, 32u },
        { "B-clean-align0", PATH_B, 0, 0u, 0u, 139u, 80u },
        { "B-dirty-align0", PATH_B, 1, 0u, 1u, 83u, 48u },
        { "B-clean-align1", PATH_B, 0, 1u, 2u, 139u, 80u },
        { "B-dirty-align1", PATH_B, 1, 1u, 3u, 83u, 48u },
        { "B-clean-align2", PATH_B, 0, 2u, 1u, 139u, 80u },
        { "B-dirty-align2", PATH_B, 1, 2u, 2u, 83u, 48u },
        { "B-clean-align3", PATH_B, 0, 3u, 3u, 139u, 80u },
        { "B-dirty-align3", PATH_B, 1, 3u, 0u, 83u, 48u },
        { "C-clean", PATH_C, 0, 0u, 0u, 123u, 72u },
        { "C-dirty", PATH_C, 1, 0u, 1u, 67u, 40u },
        { "D-clean-align0", PATH_D, 0, 0u, 0u, 171u, 88u },
        { "D-dirty-align0", PATH_D, 1, 0u, 1u, 115u, 56u },
        { "D-clean-align1", PATH_D, 0, 1u, 2u, 171u, 88u },
        { "D-dirty-align1", PATH_D, 1, 1u, 3u, 115u, 56u },
        { "D-clean-align2", PATH_D, 0, 2u, 1u, 171u, 88u },
        { "D-dirty-align2", PATH_D, 1, 2u, 2u, 115u, 56u },
        { "D-clean-align3", PATH_D, 0, 3u, 3u, 171u, 88u },
        { "D-dirty-align3", PATH_D, 1, 3u, 0u, 115u, 56u }
    };
    uint8_t *before;
    uint8_t *expected;
    uint8_t *write_mask;
    size_t i;

    printf("=== W34B9-B wm_80089160 retail trace certificate ===\n");
    printf("Oracle: frozen raw PCs and declarative A/B/C/D clean+dirty traces\n\n");

    check("metadata start == 0x80089160",
          WM_80089160_START == 0x80089160u);
    check("metadata end-exclusive == 0x800893E0",
          WM_80089160_END_EXCLUSIVE == 0x800893E0u);
    check("metadata size == 0x280",
          WM_80089160_END_EXCLUSIVE - WM_80089160_START == 0x280u);
    check("record stride authority == 0x2A0",
          WM_89160_RECORD_STRIDE == 0x2A0u);
    check("subrecord stride authority == 0x54",
          WM_89160_SUBRECORD_STRIDE == 0x54u);
    check("subrecord count authority == 8",
          WM_89160_SUBRECORD_COUNT == 8);

    before = (uint8_t *)malloc((size_t)PSX_RAM_SIZE);
    expected = (uint8_t *)malloc((size_t)PSX_RAM_SIZE);
    write_mask = (uint8_t *)malloc((size_t)PSX_RAM_SIZE);
    if (before == NULL || expected == NULL || write_mask == NULL) {
        fprintf(stderr, "allocation failure\n");
        free(before);
        free(expected);
        free(write_mask);
        return 2;
    }

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        printf("\n-- %s --\n", cases[i].name);
        run_case(&cases[i], (u32)i, before, expected, write_mask);
    }

    free(before);
    free(expected);
    free(write_mask);

    printf("\n=== Results: %d/%d PASS ===\n", g_pass, g_total);
    if (g_fail != 0) {
        printf("FAILED: %d checks\n", g_fail);
        return 1;
    }
    return 0;
}

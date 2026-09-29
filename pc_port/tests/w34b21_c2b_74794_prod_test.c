/* Focused production-linked certificate for world helper 0x80074794. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_74794.h"

#define RING_INDEX_ADDR 0x8009BE38u
#define RING_BASE_PTR   0x8009D30Cu
#define RING_ADDR       0x800A1000u
#define POSE_ADDR       0x800A2000u

typedef struct StoreEvent {
    u32 address;
    u32 width;
    u32 value;
} StoreEvent;

static StoreEvent stores[8];
static u32 store_count;
static u32 shift_inputs[2];
static u32 shift_results[2];
static u32 shift_count;
static u32 checks;

void wm_74794_test_trace(u32 address, u32 width, u32 value)
{
    if (store_count < 8u) {
        stores[store_count].address = address;
        stores[store_count].width = width;
        stores[store_count].value = value;
    }
    store_count++;
}

void wm_74794_test_shift(u32 input, u32 result)
{
    if (shift_count < 2u) {
        shift_inputs[shift_count] = input;
        shift_results[shift_count] = result;
    }
    shift_count++;
}

static void fail(const char* name, u32 got, u32 expected)
{
    fprintf(stderr, "ASSERTION %s got=0x%08x expected=0x%08x\n",
            name, got, expected);
    exit(1);
}

static void expect_u32(const char* name, u32 got, u32 expected)
{
    checks++;
    if (got != expected)
        fail(name, got, expected);
}

static void poke_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 peek_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 peek_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 oracle_sra12_low(u32 bits)
{
    u32 shifted = bits >> 12;
    if ((bits & 0x80000000u) != 0u)
        shifted |= 0xFFF00000u;
    return (u16)shifted;
}

static u32 oracle_sra12(u32 bits)
{
    u32 shifted = bits >> 12;
    if ((bits & 0x80000000u) != 0u)
        shifted |= 0xFFF00000u;
    return shifted;
}

static void reset_case(u32 index, u32 x, u32 y, u32 z)
{
    memset(PSX_ADDR(RING_ADDR - 16u), 0xA5, 160u);
    poke_u32(RING_INDEX_ADDR, index);
    poke_u32(RING_BASE_PTR, RING_ADDR);
    poke_u32(POSE_ADDR, x);
    poke_u32(POSE_ADDR + 4u, y);
    poke_u32(POSE_ADDR + 8u, z);
    memset(stores, 0, sizeof(stores));
    store_count = 0u;
    memset(shift_inputs, 0, sizeof(shift_inputs));
    memset(shift_results, 0, sizeof(shift_results));
    shift_count = 0u;
}

static void check_case(const char* prefix, u32 index, u32 x, u32 y, u32 z,
                       s32 tag)
{
    u32 entry = RING_ADDR + index * 8u;
    u32 next = (index + 1u) & 0x0Fu;
    (void)prefix;
    reset_case(index, x, y, z);
    wm_80074794(tag, POSE_ADDR);

    expect_u32("x halfword", peek_u16(entry), oracle_sra12_low(x));
    expect_u32("tag halfword", peek_u16(entry + 2u), (u16)tag);
    expect_u32("z halfword", peek_u16(entry + 4u), oracle_sra12_low(z));
    expect_u32("entry padding untouched", peek_u16(entry + 6u), 0xA5A5u);
    expect_u32("ring index", peek_u32(RING_INDEX_ADDR), next);
    expect_u32("store count", store_count, 4u);
    expect_u32("store0 address", stores[0].address, entry);
    expect_u32("store0 width", stores[0].width, 2u);
    expect_u32("store0 value", stores[0].value, oracle_sra12_low(x));
    expect_u32("store1 address", stores[1].address, entry + 2u);
    expect_u32("store1 width", stores[1].width, 2u);
    expect_u32("store1 value", stores[1].value, (u16)tag);
    expect_u32("store2 address", stores[2].address, RING_INDEX_ADDR);
    expect_u32("store2 width", stores[2].width, 4u);
    expect_u32("store2 value", stores[2].value, next);
    expect_u32("store3 address", stores[3].address, entry + 4u);
    expect_u32("store3 width", stores[3].width, 2u);
    expect_u32("store3 value", stores[3].value, oracle_sra12_low(z));
    expect_u32("shift count", shift_count, 2u);
    expect_u32("shift0 input", shift_inputs[0], x);
    expect_u32("shift0 full result", shift_results[0], oracle_sra12(x));
    expect_u32("shift1 input", shift_inputs[1], z);
    expect_u32("shift1 full result", shift_results[1], oracle_sra12(z));
    expect_u32("Y unread", peek_u32(POSE_ADDR + 4u), y);
}

int main(void)
{
    PsxMemory_Init();

    /* Ring index zero, tags 0/1/2, positive truncation, and distinct Y. */
    check_case("zero/tag0", 0u, 0x12345ABCu, 0xDEADBEEFu,
               0x07654FEDu, 0);
    check_case("tag1", 3u, 0x00001FFFu, 0x11111111u,
               0x00002ABCu, 1);
    check_case("tag2", 7u, 0x7FFFFFFFu, 0x22222222u,
               0x40000FFFu, 2);

    /* Negative arithmetic shifts and truncation after shifting. */
    check_case("negative", 5u, 0xFFFFEFFFu, 0x13572468u,
               0xFEDCBA98u, 2);
    check_case("negative min", 9u, 0x80000FFFu, 0x24681357u,
               0xFFF00FFFu, 1);

    /* Store uses entry 15; index wraps only after publication. */
    check_case("wrap", 15u, 0x34567FFFu, 0xAAAAAAAAu,
               0xCBA98001u, 0);

    /* Neighboring bytes prove exact-width writes at the final entry. */
    expect_u32("pre-ring canary", peek_u32(RING_ADDR - 4u), 0xA5A5A5A5u);
    expect_u32("post-ring canary", peek_u32(RING_ADDR + 128u), 0xA5A5A5A5u);

    printf("PASS: wm_80074794 %u focused checks\n", checks);
    return 0;
}

/*
 * W34B8-F production-linked certification for the retail descending clear
 * inside wm_80097BC0.
 *
 * The expected sequence is deliberately declared here, independently of the
 * production translation:
 *
 *     expected[i] = 0x8009C580 - 4*i, i = 0..255
 *
 * Build the production translation unit with WM_97BC0_CLEAR_TRACE so its
 * test-only observer reports each completed clear store. Ordinary production
 * builds do not contain the observer.
 *
 * Example strict build:
 *   cc -std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *      -DWM_97BC0_CLEAR_TRACE -Ipc_port/include -Iinclude -Ipc_port/src \
 *      -Wall -Wextra -Wconversion -Wsign-conversion -Werror -O2 \
 *      pc_port/tests/w34b8f_80097bc0_clear_prod_test.c \
 *      pc_port/src/world_map_terrain_init.c -o /tmp/w34b8f_clear_O2
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_terrain_init.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

enum {
    RETAIL_WORD_COUNT = 256,
    TRACE_CAPACITY = 257,
    POINTER_RING_WORD_COUNT = 16
};

#define RETAIL_FIRST_ADDR UINT32_C(0x8009C580)
#define RETAIL_LAST_ADDR  UINT32_C(0x8009C184)
#define RETAIL_STRIDE     UINT32_C(4)

#define MATRIX_SRC_ADDR   UINT32_C(0x8009A180)
#define MATRIX_DST_ADDR   UINT32_C(0x8009D534)
#define TEST_POS_ADDR     UINT32_C(0x8009C000)

#define C17C_ADDR         UINT32_C(0x8009C17C)
#define C180_ADDR         UINT32_C(0x8009C180)
#define C184_ADDR         UINT32_C(0x8009C184)
#define C580_ADDR         UINT32_C(0x8009C580)
#define C584_ADDR         UINT32_C(0x8009C584)
#define C588_ADDR         UINT32_C(0x8009C588)
#define C61C_ADDR         UINT32_C(0x8009C61C)
#define C620_ADDR         UINT32_C(0x8009C620)
#define C624_ADDR         UINT32_C(0x8009C624)
#define C660_ADDR         UINT32_C(0x8009C660)

#define BBB4_ADDR         UINT32_C(0x8009BBB4)
#define BBB8_ADDR         UINT32_C(0x8009BBB8)
#define BBBC_ADDR         UINT32_C(0x8009BBBC)
#define C5BC_ADDR         UINT32_C(0x8009C5BC)
#define C618_ADDR         UINT32_C(0x8009C618)
#define C838_ADDR         UINT32_C(0x8009C838)

#define RAM_OFFSET(addr_) ((size_t)((uint32_t)(addr_) & UINT32_C(0x001FFFFF)))
#define RAM_U32(addr_) (*(uint32_t *)(void *)(g_PsxRam + RAM_OFFSET(addr_)))

static uint8_t s_before[PSX_RAM_SIZE];
static uint32_t s_trace[TRACE_CAPACITY];
static size_t s_trace_count;
static size_t s_trace_overflow;
static unsigned int s_helper_981c8_calls;
static unsigned int s_helper_97dc0_calls;
static uint32_t s_helper_981c8_arg;

static unsigned int s_total;
static unsigned int s_pass;
static unsigned int s_fail;

void wm_97bc0_trace_store(u32 guest_addr)
{
    if (s_trace_count < (size_t)TRACE_CAPACITY) {
        s_trace[s_trace_count] = guest_addr;
    } else {
        s_trace_overflow++;
    }
    s_trace_count++;
}

void wm_800981C8(u32 pos_ptr)
{
    s_helper_981c8_calls++;
    s_helper_981c8_arg = pos_ptr;
}

void wm_80097DC0(void)
{
    s_helper_97dc0_calls++;
}

static void check(const char *name, int condition)
{
    s_total++;
    if (condition != 0) {
        s_pass++;
        (void)printf("PASS %s\n", name);
    } else {
        s_fail++;
        (void)printf("FAIL %s\n", name);
    }
}

static uint32_t oracle_addr(size_t index)
{
    return RETAIL_FIRST_ADDR -
           (RETAIL_STRIDE * (uint32_t)index);
}

static uint32_t target_seed(size_t index)
{
    return UINT32_C(0x6D000001) ^
           ((uint32_t)index * UINT32_C(0x00010101));
}

static uint32_t ring_seed(size_t index)
{
    return UINT32_C(0xA5001001) +
           ((uint32_t)index * UINT32_C(0x00100101));
}

static void reset_fixture(void)
{
    size_t index;

    for (index = 0; index < (size_t)PSX_RAM_SIZE; index++) {
        g_PsxRam[index] = (uint8_t)(UINT32_C(0x5A) ^
                                    ((uint32_t)index * UINT32_C(0x6D)));
    }

    memset(s_before, 0, sizeof(s_before));
    memset(s_trace, 0, sizeof(s_trace));
    s_trace_count = 0;
    s_trace_overflow = 0;
    s_helper_981c8_calls = 0;
    s_helper_97dc0_calls = 0;
    s_helper_981c8_arg = 0;
    wm_tpi_reset();
}

static void seed_matrix_and_position(void)
{
    static const uint32_t matrix_words[8] = {
        UINT32_C(0x00001000), UINT32_C(0x00000000),
        UINT32_C(0x00001000), UINT32_C(0x00000000),
        UINT32_C(0x00001000), UINT32_C(0x00000000),
        UINT32_C(0x00000000), UINT32_C(0x00000000)
    };

    memcpy(g_PsxRam + RAM_OFFSET(MATRIX_SRC_ADDR),
           matrix_words, sizeof(matrix_words));
    RAM_U32(TEST_POS_ADDR) = UINT32_C(0x00123456);
    RAM_U32(TEST_POS_ADDR + UINT32_C(4)) = UINT32_C(0x00765432);
    RAM_U32(TEST_POS_ADDR + UINT32_C(8)) = UINT32_C(0x000FEDCB);
}

static void seed_retail_words(void)
{
    size_t index;

    for (index = 0; index < (size_t)RETAIL_WORD_COUNT; index++) {
        RAM_U32(oracle_addr(index)) = target_seed(index);
    }
}

static void seed_canaries(uint32_t c620_value)
{
    size_t index;

    RAM_U32(C17C_ADDR) = UINT32_C(0x171C171C);
    RAM_U32(C180_ADDR) = UINT32_C(0xC180C180);
    RAM_U32(C584_ADDR) = UINT32_C(0xC584C584);
    RAM_U32(C588_ADDR) = UINT32_C(0xC588C588);
    RAM_U32(C61C_ADDR) = UINT32_C(0xC61CC61C);
    RAM_U32(C620_ADDR) = c620_value;

    for (index = 0; index < (size_t)POINTER_RING_WORD_COUNT; index++) {
        RAM_U32(C624_ADDR + (UINT32_C(4) * (uint32_t)index)) =
            ring_seed(index);
    }
}

static int all_retail_words_are_zero(size_t *zero_count)
{
    size_t index;
    size_t count = 0;

    for (index = 0; index < (size_t)RETAIL_WORD_COUNT; index++) {
        if (RAM_U32(oracle_addr(index)) == UINT32_C(0)) {
            count++;
        }
    }

    *zero_count = count;
    return count == (size_t)RETAIL_WORD_COUNT;
}

static int pointer_ring_is_intact(size_t *intact_count)
{
    size_t index;
    size_t count = 0;

    for (index = 0; index < (size_t)POINTER_RING_WORD_COUNT; index++) {
        uint32_t actual =
            RAM_U32(C624_ADDR + (UINT32_C(4) * (uint32_t)index));
        if (actual == ring_seed(index)) {
            count++;
        }
    }

    *intact_count = count;
    return count == (size_t)POINTER_RING_WORD_COUNT;
}

static int trace_matches_oracle(const uint32_t *trace, size_t count)
{
    size_t index;

    if (count != (size_t)RETAIL_WORD_COUNT) {
        return 0;
    }
    for (index = 0; index < (size_t)RETAIL_WORD_COUNT; index++) {
        if (trace[index] != oracle_addr(index)) {
            return 0;
        }
    }
    return 1;
}

static int oracle_is_unique(void)
{
    size_t first;
    size_t second;

    for (first = 0; first < (size_t)RETAIL_WORD_COUNT; first++) {
        for (second = first + 1; second < (size_t)RETAIL_WORD_COUNT;
             second++) {
            if (oracle_addr(first) == oracle_addr(second)) {
                return 0;
            }
        }
    }
    return 1;
}

static int byte_is_authorized(size_t offset)
{
    const size_t clear_low = RAM_OFFSET(C184_ADDR);
    const size_t clear_high_exclusive = RAM_OFFSET(C580_ADDR) + 4u;
    const size_t matrix_low = RAM_OFFSET(MATRIX_DST_ADDR);
    const size_t matrix_high_exclusive = matrix_low + 32u;

    if (offset >= clear_low && offset < clear_high_exclusive) {
        return 1;
    }
    if (offset >= matrix_low && offset < matrix_high_exclusive) {
        return 1;
    }
    if (offset >= RAM_OFFSET(BBB4_ADDR) &&
        offset < RAM_OFFSET(BBB4_ADDR) + 4u) {
        return 1;
    }
    if (offset >= RAM_OFFSET(BBB8_ADDR) &&
        offset < RAM_OFFSET(BBB8_ADDR) + 4u) {
        return 1;
    }
    if (offset >= RAM_OFFSET(BBBC_ADDR) &&
        offset < RAM_OFFSET(BBBC_ADDR) + 4u) {
        return 1;
    }
    if (offset >= RAM_OFFSET(C5BC_ADDR) &&
        offset < RAM_OFFSET(C5BC_ADDR) + 4u) {
        return 1;
    }
    if (offset >= RAM_OFFSET(C618_ADDR) &&
        offset < RAM_OFFSET(C618_ADDR) + 4u) {
        return 1;
    }
    if (offset >= RAM_OFFSET(C838_ADDR) &&
        offset < RAM_OFFSET(C838_ADDR) + 6u) {
        return 1;
    }
    return 0;
}

static int all_other_ram_is_unchanged(void)
{
    size_t offset;

    for (offset = 0; offset < (size_t)PSX_RAM_SIZE; offset++) {
        if (byte_is_authorized(offset) == 0 &&
            g_PsxRam[offset] != s_before[offset]) {
            (void)printf("INFO first unauthorized byte: +0x%zX "
                         "before=%02X after=%02X\n",
                         offset, (unsigned int)s_before[offset],
                         (unsigned int)g_PsxRam[offset]);
            return 0;
        }
    }
    return 1;
}

static size_t make_model_trace(uint32_t *trace, size_t capacity,
                               uint32_t start, size_t count,
                               uint32_t stride, int descending)
{
    size_t index;
    uint32_t address = start;

    for (index = 0; index < count; index++) {
        if (index < capacity) {
            trace[index] = address;
        }
        if (descending != 0) {
            address -= stride;
        } else {
            address += stride;
        }
    }
    return count;
}

static void run_primary_production_fixture(void)
{
    size_t zero_count = 0;
    size_t ring_count = 0;
    int trace_exact;

    (void)printf("SECTION production-footprint\n");
    reset_fixture();
    seed_matrix_and_position();
    seed_retail_words();
    seed_canaries(UINT32_C(0x81234567));
    memcpy(s_before, g_PsxRam, sizeof(s_before));

    wm_80097BC0(TEST_POS_ADDR);

    check("oracle count is 256", (size_t)RETAIL_WORD_COUNT == 256u);
    check("oracle first is C580", oracle_addr(0) == C580_ADDR);
    check("oracle second is C57C",
          oracle_addr(1) == UINT32_C(0x8009C57C));
    check("oracle last is C184",
          oracle_addr((size_t)RETAIL_WORD_COUNT - 1u) == C184_ADDR);
    check("oracle contains 256 unique addresses", oracle_is_unique());

    check("C580 is zero", RAM_U32(C580_ADDR) == UINT32_C(0));
    check("C184 is zero", RAM_U32(C184_ADDR) == UINT32_C(0));
    check("every retail word is zero (256/256)",
          all_retail_words_are_zero(&zero_count));
    (void)printf("INFO retail-zero-count=%zu/256\n", zero_count);

    check("C584 upper canary survives",
          RAM_U32(C584_ADDR) == UINT32_C(0xC584C584));
    check("C588 upper canary survives",
          RAM_U32(C588_ADDR) == UINT32_C(0xC588C588));
    check("C61C upper canary survives",
          RAM_U32(C61C_ADDR) == UINT32_C(0xC61CC61C));
    check("C620 pointer canary 0x81234567 survives bit-identical",
          RAM_U32(C620_ADDR) == UINT32_C(0x81234567));
    check("C17C lower canary survives",
          RAM_U32(C17C_ADDR) == UINT32_C(0x171C171C));
    check("C180 lower canary survives",
          RAM_U32(C180_ADDR) == UINT32_C(0xC180C180));
    check("C624..C660 pointer ring survives (16/16)",
          pointer_ring_is_intact(&ring_count));
    (void)printf("INFO pointer-ring-intact=%zu/16\n", ring_count);

    trace_exact = s_trace_overflow == 0u &&
                  trace_matches_oracle(s_trace, s_trace_count) != 0;
    check("production trace count is exactly 256",
          s_trace_count == (size_t)RETAIL_WORD_COUNT &&
          s_trace_overflow == 0u);
    check("production trace ordering is exactly C580,C57C,...,C184",
          trace_exact);
    check("production trace first is C580",
          s_trace_count > 0u && s_trace[0] == C580_ADDR);
    check("production trace last is C184",
          s_trace_count == (size_t)RETAIL_WORD_COUNT &&
          s_trace[(size_t)RETAIL_WORD_COUNT - 1u] == C184_ADDR);

    check("only authorized production RAM bytes changed",
          all_other_ram_is_unchanged());
    check("981C8 helper called exactly once",
          s_helper_981c8_calls == 1u &&
          s_helper_981c8_arg == TEST_POS_ADDR);
    check("97DC0 helper called exactly once", s_helper_97dc0_calls == 1u);
}

static void run_c620_lifetime_fixture(void)
{
    size_t zero_count = 0;

    (void)printf("SECTION c620-lifetime\n");
    reset_fixture();
    seed_matrix_and_position();
    seed_retail_words();
    seed_canaries(UINT32_C(0xDEADBEEF));

    wm_80097BC0(TEST_POS_ADDR);

    check("lifetime C620 0xDEADBEEF survives bit-identical",
          RAM_U32(C620_ADDR) == UINT32_C(0xDEADBEEF));
    check("lifetime clear still zeros all 256 retail words",
          all_retail_words_are_zero(&zero_count));
    check("lifetime production trace remains exact",
          s_trace_overflow == 0u &&
          trace_matches_oracle(s_trace, s_trace_count) != 0);
}

static void run_mutant_rejection_matrix(void)
{
    uint32_t mutant[TRACE_CAPACITY];
    size_t count;

    (void)printf("SECTION wrong-model-mutants\n");

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C580_ADDR,
                             256u, 4u, 0);
    check("MUTANT DETECTED existing upward C580 clear",
          trace_matches_oracle(mutant, count) == 0);
    check("MUTANT DETECTED memset(C580,1024)",
          trace_matches_oracle(mutant, count) == 0);

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C184_ADDR,
                             256u, 4u, 0);
    check("MUTANT DETECTED forward lower-base clear",
          trace_matches_oracle(mutant, count) == 0);
    check("MUTANT DETECTED memset(C184,1024) wrong order",
          trace_matches_oracle(mutant, count) == 0);

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C580_ADDR,
                             255u, 4u, 1);
    check("MUTANT DETECTED 255 words",
          trace_matches_oracle(mutant, count) == 0);

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C580_ADDR,
                             257u, 4u, 1);
    check("MUTANT DETECTED 257 words",
          trace_matches_oracle(mutant, count) == 0);
    check("MUTANT DETECTED wrong end C180",
          trace_matches_oracle(mutant, count) == 0 &&
          mutant[256] == C180_ADDR);

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C580_ADDR,
                             256u, 2u, 1);
    check("MUTANT DETECTED stride -2",
          trace_matches_oracle(mutant, count) == 0);

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C580_ADDR,
                             256u, 8u, 1);
    check("MUTANT DETECTED stride -8",
          trace_matches_oracle(mutant, count) == 0);

    count = make_model_trace(mutant, (size_t)TRACE_CAPACITY, C584_ADDR,
                             256u, 4u, 1);
    check("MUTANT DETECTED wrong start C584",
          trace_matches_oracle(mutant, count) == 0);
}

int main(void)
{
    (void)printf("W34B8-F 80097BC0 PRODUCTION-LINKED CLEAR TEST\n");
    (void)printf("ORACLE expected[i]=0x8009C580-4*i, i=0..255\n");

    run_primary_production_fixture();
    run_c620_lifetime_fixture();
    run_mutant_rejection_matrix();

    (void)printf("RESULT %u/%u passed; %u failed\n",
                 s_pass, s_total, s_fail);
    return s_fail == 0u ? 0 : 1;
}

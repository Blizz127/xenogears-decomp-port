#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_private_collision.h"

#define BASE_ADDR UINT32_C(0x800A1000)
#define DIR_ADDR  UINT32_C(0x800A1100)
#define WORK_ADDR UINT32_C(0x1F800000)

typedef s32 (*ProbeFn)(u32, u32, u32, s32);

typedef struct FamilySpec {
    const char *name;
    ProbeFn fn;
    int axis_z;
    int negative;
    s32 second_code;
} FamilySpec;

static const FamilySpec s_specs[] = {
    { "9443C-X+", wm_8009443C, 0, 0, 3 },
    { "945C8-X-", wm_800945C8, 0, 1, 3 },
    { "94750-Z+", wm_80094750, 1, 0, 2 },
    { "948D8-Z-", wm_800948D8, 1, 1, 2 },
};

static u32 s_events[16];
static u32 s_event_addr[16];
static u32 s_event_count;
static s32 s_codes[2];
static s32 s_blocked[2];
static u32 s_code_index;
static u32 s_block_index;
static s32 s_last_modes[2];
static s32 s_last_codes[2];
static int s_failures;

static void fail_u32(const char *case_name, const char *what,
                     u32 got, u32 expected)
{
    fprintf(stderr, "ASSERTION %s %s: got=0x%08x expected=0x%08x\n",
            case_name, what, got, expected);
    s_failures++;
}

static void check_u32(const char *case_name, const char *what,
                      u32 got, u32 expected)
{
    if (got != expected)
        fail_u32(case_name, what, got, expected);
}

static u32 load_u32(u32 addr)
{
    u32 value;
    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

static void store_u32(u32 addr, u32 value)
{
    memcpy(PSX_ADDR(addr), &value, sizeof(value));
}

static s32 as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 sra32(u32 bits, u32 amount)
{
    u32 value = bits >> amount;
    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

static u32 ref_mul(u32 a, u32 b)
{
    s64 product = (s64)as_s32(a) * (s64)as_s32(b);
    return sra32((u32)((u64)product & UINT64_C(0xFFFFFFFF)), 12u);
}

static u32 ref_ratio(u32 num, u32 den)
{
    s32 n = as_s32(num << 12);
    s32 d = as_s32(den);
    return as_u32((s32)((s64)n / (s64)d));
}

static void event(u32 kind, u32 addr)
{
    if (s_event_count < 16u) {
        s_events[s_event_count] = kind;
        s_event_addr[s_event_count] = addr;
    }
    s_event_count++;
}

void wm_private_test_93354(u32 addr)
{
    event(1u, addr);
}

s32 wm_private_test_93f18(u32 addr)
{
    event(2u, addr);
    return s_codes[s_code_index++];
}

s32 wm_private_test_94060(s32 mode, s32 code)
{
    event(3u, 0u);
    if (s_block_index < 2u) {
        s_last_modes[s_block_index] = mode;
        s_last_codes[s_block_index] = code;
    }
    return s_blocked[s_block_index++];
}

static void reset_case(void)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0x5A, 4096u);
    memset(s_events, 0, sizeof(s_events));
    memset(s_event_addr, 0, sizeof(s_event_addr));
    memset(s_last_modes, 0, sizeof(s_last_modes));
    memset(s_last_codes, 0, sizeof(s_last_codes));
    s_event_count = 0u;
    s_code_index = 0u;
    s_block_index = 0u;
    s_codes[0] = as_s32(UINT32_C(0x12348001));
    s_codes[1] = as_s32(UINT32_C(0x56787FFF));
}

static void run_case(const FamilySpec *spec, int outcome, u32 ordinal)
{
    char name[64];
    u32 x = (ordinal & 1u) != 0u ? UINT32_C(0xFFF7FFFF)
                                     : UINT32_C(0x00080001);
    u32 z = (ordinal & 2u) != 0u ? UINT32_C(0x0007FFFF)
                                     : UINT32_C(0xFFF80001);
    u32 dx = spec->axis_z ? UINT32_C(0xFFFFE000) : UINT32_C(0x00001000);
    u32 dz = spec->axis_z ? UINT32_C(0x00001000) : UINT32_C(0xFFFFD000);
    u32 axis = spec->axis_z ? z : x;
    u32 ortho = spec->axis_z ? x : z;
    u32 num = spec->axis_z ? dx : dz;
    u32 den = spec->axis_z ? dz : dx;
    u32 seed_off = spec->axis_z ? 0x18u : 0x10u;
    u32 seed = axis + UINT32_C(0x00123456);
    u32 edge_source = spec->negative ? axis : seed;
    u32 edge = (edge_source & UINT32_C(0xFFF80000)) - axis;
    u32 d0 = spec->negative ? edge : edge - 1u;
    u32 d1 = spec->negative ? edge - 1u : edge;
    u32 ratio = ref_ratio(num, den);
    u32 c0_axis = spec->axis_z ? 0x28u : 0x20u;
    u32 c0_ortho = spec->axis_z ? 0x20u : 0x28u;
    u32 c1_axis = spec->axis_z ? 0x38u : 0x30u;
    u32 c1_ortho = spec->axis_z ? 0x30u : 0x38u;
    s32 result;
    u32 i;

    snprintf(name, sizeof(name), "%s-outcome%d", spec->name, outcome);
    reset_case();
    store_u32(BASE_ADDR, x);
    store_u32(BASE_ADDR + 8u, z);
    store_u32(DIR_ADDR, dx);
    store_u32(DIR_ADDR + 8u, dz);
    store_u32(WORK_ADDR + seed_off, seed);
    store_u32(WORK_ADDR + 0x24u, UINT32_C(0x11112222));
    store_u32(WORK_ADDR + 0x2Cu, UINT32_C(0x33334444));
    store_u32(WORK_ADDR + 0x34u, UINT32_C(0x55556666));
    store_u32(WORK_ADDR + 0x3Cu, UINT32_C(0x77778888));

    if (outcome == 0) {
        s_blocked[0] = as_s32(UINT32_C(0x43210000));
        s_blocked[1] = 1;
    } else if (outcome == 1) {
        s_blocked[0] = 1;
        s_blocked[1] = as_s32(UINT32_C(0xABCD0000));
    } else {
        s_blocked[0] = 1;
        s_blocked[1] = as_s32(UINT32_C(0x10001));
    }

    result = spec->fn(BASE_ADDR, DIR_ADDR, WORK_ADDR,
                      as_s32(UINT32_C(0x13578000)));
    check_u32(name, "return", as_u32(result),
              outcome == 0 ? 1u : (outcome == 1 ? (u32)spec->second_code : 0u));
    check_u32(name, "c0-axis", load_u32(WORK_ADDR + c0_axis), axis + d0);
    check_u32(name, "c0-ortho", load_u32(WORK_ADDR + c0_ortho),
              ortho + ref_mul(ratio, d0));
    check_u32(name, "c1-axis", load_u32(WORK_ADDR + c1_axis), axis + d1);
    check_u32(name, "c1-ortho", load_u32(WORK_ADDR + c1_ortho),
              ortho + ref_mul(ratio, d1));
    check_u32(name, "event-count", s_event_count, outcome == 0 ? 3u : 6u);
    check_u32(name, "event0", s_events[0], 1u);
    check_u32(name, "event1", s_events[1], 2u);
    check_u32(name, "event2", s_events[2], 3u);
    check_u32(name, "candidate0-pointer", s_event_addr[0], WORK_ADDR + 0x20u);
    check_u32(name, "mode-sign16", as_u32(s_last_modes[0]), UINT32_C(0xFFFF8000));
    check_u32(name, "code0-sign16", as_u32(s_last_codes[0]), UINT32_C(0xFFFF8001));
    if (outcome != 0) {
        check_u32(name, "event3", s_events[3], 1u);
        check_u32(name, "event4", s_events[4], 2u);
        check_u32(name, "event5", s_events[5], 3u);
        check_u32(name, "candidate1-pointer", s_event_addr[3], WORK_ADDR + 0x30u);
        check_u32(name, "code1-sign16", as_u32(s_last_codes[1]), UINT32_C(0x00007FFF));
        check_u32(name, "failure-path edge residue",
                  load_u32(WORK_ADDR + (spec->axis_z ? 8u : 0u)), edge);
    } else {
        for (i = 0u; i < 4u; i++)
            check_u32(name, "copy-word", load_u32(WORK_ADDR + i * 4u),
                      load_u32(WORK_ADDR + 0x20u + i * 4u));
    }
    check_u32(name, "real-scratchpad-untouched", g_PsxScratchpad[0], 0x5Au);
}

static void expect_trap(const FamilySpec *spec, int overflow)
    __attribute__((unused));
static void expect_trap(const FamilySpec *spec, int overflow)
{
    pid_t pid;
    int status = 0;

    reset_case();
    store_u32(BASE_ADDR, 1u);
    store_u32(BASE_ADDR + 8u, 2u);
    store_u32(DIR_ADDR, overflow && spec->axis_z ? UINT32_C(0x00080000) : 1u);
    store_u32(DIR_ADDR + 8u, overflow && !spec->axis_z ? UINT32_C(0x00080000) : 1u);
    if (spec->axis_z)
        store_u32(DIR_ADDR + 8u, overflow ? UINT32_MAX : 0u);
    else
        store_u32(DIR_ADDR, overflow ? UINT32_MAX : 0u);

    pid = fork();
    if (pid == 0) {
        (void)spec->fn(BASE_ADDR, DIR_ADDR, WORK_ADDR, 0);
        _exit(0);
    }
    if (pid < 0 || waitpid(pid, &status, 0) != pid ||
        !WIFSIGNALED(status) || WTERMSIG(status) != SIGILL) {
        fprintf(stderr, "ASSERTION %s %s division trap missing status=0x%x\n",
                spec->name, overflow ? "overflow" : "zero", status);
        s_failures++;
    }
}

int main(void)
{
    u32 i;
    int outcome;
    struct rlimit no_core = { 0u, 0u };

    (void)setrlimit(RLIMIT_CORE, &no_core);
    PsxMemory_Init();
    check_u32("scratchpad", "PSX_ADDR base offset",
              (u32)((u8 *)PSX_ADDR(WORK_ADDR) - g_PsxRam), 0u);
    check_u32("scratchpad", "candidate0 alias",
              (u32)((u8 *)PSX_ADDR(WORK_ADDR + 0x20u) - g_PsxRam), 0x20u);
    check_u32("scratchpad", "candidate1 alias",
              (u32)((u8 *)PSX_ADDR(WORK_ADDR + 0x30u) - g_PsxRam), 0x30u);

    for (i = 0u; i < 4u; i++) {
        for (outcome = 0; outcome < 3; outcome++)
            run_case(&s_specs[i], outcome, i * 3u + (u32)outcome);
#if !defined(WM_PRIVATE_PROBE_MUTANT_BUILD) || \
    defined(WM_PRIVATE_MUTANT_MISSING_TRAP)
        expect_trap(&s_specs[i], 0);
        expect_trap(&s_specs[i], 1);
#endif
    }

    if (s_failures != 0) {
        fprintf(stderr, "ASSERTION failures=%d\n", s_failures);
        return 1;
    }
    printf("W34B22-I4B private collision family PASS cases=20 wrappers=4\n");
    return 0;
}

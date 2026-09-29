#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_94060.h"
#include "world_map_private_collision.h"

#define BASE_ADDR UINT32_C(0x800A1000)
#define DIR_ADDR  UINT32_C(0x800A1100)
#define WORK_ADDR UINT32_C(0x1F800000)

static u32 s_93f18_candidate_loads;
static int s_failures;

void wm_93354_test_load(u32 address, u32 value)
{
    (void)address;
    (void)value;
}

void wm_93354_test_store(u32 address, u32 value)
{
    (void)address;
    (void)value;
}

void wm_93e8c_test_load(u32 address, u32 width, u32 value)
{
    (void)address;
    (void)width;
    (void)value;
}

void wm_93f18_test_load(u32 address, u32 width, u32 value)
{
    (void)value;
    if (width == 4u &&
        (address == WORK_ADDR + 0x20u || address == WORK_ADDR + 0x28u))
        s_93f18_candidate_loads++;
}

void wm_94060_test_load(u32 address, u32 width, u32 value)
{
    (void)address;
    (void)width;
    (void)value;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

int main(void)
{
    s32 (*const wrappers[4])(u32, u32, u32, s32) = {
        wm_8009443C, wm_800945C8, wm_80094750, wm_800948D8
    };
    u32 i;

    PsxMemory_Init();
    store_u32(BASE_ADDR, 0u);
    store_u32(BASE_ADDR + 8u, 0u);
    store_u32(DIR_ADDR, 0x1000u);
    store_u32(DIR_ADDR + 8u, 0x1000u);
    store_u32(WORK_ADDR + 0x10u, 0x100000u);
    store_u32(WORK_ADDR + 0x18u, 0x100000u);
    wm_80093354_reset_exec_count();
    wm_80094060_reset_exec_count();

    check("PSX_ADDR scratch base aliases main backing",
          PSX_ADDR(WORK_ADDR) == (void *)g_PsxRam);
    check("candidate0 aliases +0x20",
          PSX_ADDR(WORK_ADDR + 0x20u) == (void *)(g_PsxRam + 0x20u));
    check("candidate1 aliases +0x30",
          PSX_ADDR(WORK_ADDR + 0x30u) == (void *)(g_PsxRam + 0x30u));

    for (i = 0u; i < 4u; i++) {
        s32 result = wrappers[i](BASE_ADDR, DIR_ADDR, WORK_ADDR, 0);
        check("actual canonical chain returns candidate0 success", result == 1);
    }
    check("93354 actual calls", wm_80093354_get_exec_count() == 4u);
    check("94060 actual calls", wm_80094060_get_exec_count() == 4u);
    check("93F18 saw both candidate X/Z words",
          s_93f18_candidate_loads == 8u);
    check("separate scratchpad buffer untouched", g_PsxScratchpad[0] == 0u);

    if (s_failures != 0)
        return 1;
    printf("W34B22-I4B scratchpad canonical-chain integration PASS\n");
    return 0;
}

/* Focused production-linked certificate for the world WDS ownership seam. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "system/sound.h"

#define C894 UINT32_C(0x8009C894)
#define C88C UINT32_C(0x8009C88C)
#define WDS_RESULT UINT32_C(0x8006258C)
#define SOURCE UINT32_C(0x80101000)
#define RESULT ((SoundWDSEntry *)(uintptr_t)UINT32_C(0x00123450))

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
SoundWDSEntry *g_GameCurLoadedWDS;

static int failures;
static int cleanup_calls;
static int load_calls;

extern int wm_fresh_session_wds_cleanup(void);
extern int wm_first_wds_consumer(void);

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

void func_8001B66C(void)
{
    cleanup_calls++;
}

SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *source, s32 mode)
{
    check(source == (SoundWDSEntry *)PSX_ADDR(SOURCE),
          "world_load_uses_C88C_guest_source");
    check(mode == 0, "world_load_mode_zero");
    load_calls++;
    return RESULT;
}

int main(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));

    write32(C894, 0u);
    check(wm_fresh_session_wds_cleanup() == 0,
          "cleanup_fresh_session_returns_zero");
    check(cleanup_calls == 1, "cleanup_fresh_session_C894_zero");

    write32(C894, 1u);
    check(wm_fresh_session_wds_cleanup() == 0,
          "cleanup_restore_session_returns_zero");
    check(cleanup_calls == 1, "cleanup_restore_session_skipped");

    write32(C88C, SOURCE);
    check(wm_first_wds_consumer() == 0, "world_consumer_returns_zero");
    check(load_calls == 1, "world_consumer_loads_once");
    check(read32(WDS_RESULT) == (u32)(uintptr_t)RESULT,
          "world_guest_authority_published");
    check(g_GameCurLoadedWDS == RESULT,
          "world_native_authority_published");

    check(wm_first_wds_consumer() == -1,
          "world_consumer_second_call_blocked");
    check(load_calls == 1, "world_consumer_second_call_does_not_load");

    if (failures != 0)
        return 1;
    puts("W34N5 WDS LIFECYCLE FOCUSED CERTIFICATE PASS");
    return 0;
}

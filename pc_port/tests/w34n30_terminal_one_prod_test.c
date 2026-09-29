/* Production-linked certificate for WorldMapMain's retail D7CC==1 lane. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_terminal_one_711b0.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u8 D_800594F8;
s32 D_8004F2FC;
void* D_80062528;
u8 D_80062648[0x4000];
u8 D_800591AE;

#define BCC8   0x8009BCC8u
#define C614   0x8009C614u
#define EE70   0x8006EE70u
#define F8E5   0x8006F8E5u
#define SOURCE 0x800B2000u

static int failures;
static int events[16];
static int event_count;
static int overlay_calls;
static unsigned int overlay_arg;
static int state_calls;
static unsigned int state_arg;
static int cleanup_calls;
static int size_calls;
static unsigned int size_arg;
static int create_calls;
static void* create_arg;
static int configure_calls;
static void* configure_manager;
static int configure_level;
static int configure_steps;
static int sync_calls;
static int main_calls;
static int main_arg;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

void wm_711b0_test_event(int event)
{
    if (event_count < (int)(sizeof(events) / sizeof(events[0])))
        events[event_count] = event;
    event_count++;
}

void* LoadGameStateOverlay(unsigned int overlay_index)
{
    overlay_calls++;
    overlay_arg = overlay_index;
    return PSX_ADDR(0x800C0000u);
}

void ChangeGameState(unsigned int state)
{
    state_calls++;
    state_arg = state;
}

void func_80039CC4(void) { cleanup_calls++; }

int ArchiveDecodeAlignedSize(unsigned int entry_index)
{
    size_calls++;
    size_arg = entry_index;
    return 16;
}

void* func_80039850(void* sound_file)
{
    create_calls++;
    create_arg = sound_file;
    return (void*)(uintptr_t)0x00693020u;
}

void func_80039A80(void* manager, int level, int steps)
{
    configure_calls++;
    configure_manager = manager;
    configure_level = level;
    configure_steps = steps;
}

void wm_800762FC(void) { sync_calls++; }

void MainLoop(int error_code)
{
    main_calls++;
    main_arg = error_code;
}

static void seed(void)
{
    u32 i;
    memset(g_PsxRam, 0xCD, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    memset(D_80062648, 0xA5, sizeof(D_80062648));
    memset(events, 0, sizeof(events));
    event_count = 0;
    overlay_calls = 0;
    state_calls = 0;
    cleanup_calls = 0;
    size_calls = 0;
    create_calls = 0;
    configure_calls = 0;
    sync_calls = 0;
    main_calls = 0;
    overlay_arg = 0u;
    state_arg = 0u;
    size_arg = 0u;
    create_arg = NULL;
    configure_manager = NULL;
    configure_level = -1;
    configure_steps = -1;
    main_arg = -1;

    sw(BCC8, 0x33u);
    sw(C614, SOURCE);
    for (i = 0u; i < 24u; i++)
        *(u8*)PSX_ADDR(SOURCE + i) = (u8)(0x40u + i);
    *(u8*)PSX_ADDR(F8E5 + 0u) = 1u;
    *(u8*)PSX_ADDR(F8E5 + 1u) = 2u;
    *(u8*)PSX_ADDR(F8E5 + 2u) = 3u;
    sh(EE70 + 0u, 0xAAAAu);
    sh(EE70 + 2u, 0xBBBBu);
    sh(EE70 + 4u, 0xCCCCu);
    D_800594F8 = 0x7Bu;
    *(u8*)PSX_ADDR(0x800594F8u) = 0x6Au;
    D_8004F2FC = 0;
    D_80062528 = (void*)(uintptr_t)0x00672010u;
    D_800591AE = 0x5Bu;
}

static void check_event_order(void)
{
    int i;
    check(event_count == 11, "order.event.count");
    if (event_count != 11)
        return;
    for (i = 0; i < 11; i++) {
        if (events[i] != i + 1) {
            check(0, "order.retail.sequence");
            return;
        }
    }
}

int main(void)
{
    u32 i;
    seed();
    wm_71034_run_terminal_one_lane();

    check(overlay_calls == 1 && overlay_arg == 2u &&
          state_calls == 1 && state_arg == 2u,
          "overlay.state.arguments");
    check(D_800594F8 == 0u && *(u8*)PSX_ADDR(0x800594F8u) == 0x6Au,
          "byte594f8.native.authority");
    check(lhu(EE70) == 1u && lhu(EE70 + 2u) == 2u &&
          lhu(EE70 + 4u) == 3u, "party.halfword.stride");
    check(cleanup_calls == 1, "sound.cleanup.once");
    check(size_calls == 1 && size_arg == 0x33u,
          "archive.id.from.bcc8");
    for (i = 0u; i < 16u; i++)
        check(D_80062648[i] == (u8)(0x40u + i), "copy.source.exact");
    check(D_8004F2FC == 0x00672010, "manager.old.saved");
    check(create_calls == 1 && create_arg == D_80062648,
          "manager.create.fixed.buffer");
    check(D_80062528 == (void*)(uintptr_t)0x00693020u,
          "manager.new.published");
    check(configure_calls == 1 &&
          configure_manager == (void*)(uintptr_t)0x00693020u &&
          configure_level == 127 && configure_steps == 0,
          "manager.configure.127.0");
    check(D_800591AE == 0u, "system.byte.cleared");
    check(sync_calls == 1 && main_calls == 1 && main_arg == 0,
          "epilogue.calls.once");
    check_event_order();

    if (failures != 0) {
        fprintf(stderr, "W34N30 TRANSITION CERTIFICATE: %d failure(s)\n",
                failures);
        return EXIT_FAILURE;
    }
    puts("W34N30 terminal one certificate PASS");
    return EXIT_SUCCESS;
}

/* Retail D7CC==1 transition/audio lane and shared terminal epilogue. */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_762fc.h"
#include "world_map_terminal_one_711b0.h"

extern void* LoadGameStateOverlay(unsigned int overlay_index);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(int error_code);
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern void func_80039CC4(void);
extern void* func_80039850(void* sound_file);
extern void func_80039A80(void* manager, int level, int steps);

extern u8 D_800594F8;
extern s32 D_8004F2FC;
extern void* D_80062528;
extern u8 D_80062648[];
extern u8 D_800591AE;

#define WM_711B0_BCC8 0x8009BCC8u
#define WM_711B0_C614 0x8009C614u
#define WM_711B0_EE70 0x8006EE70u
#define WM_711B0_F8E5 0x8006F8E5u

#if defined(WM_711B0_TEST_HOOKS)
extern void wm_711b0_test_event(int event);
#define WM_711B0_EVENT(event) wm_711b0_test_event((event))
#else
#define WM_711B0_EVENT(event) ((void)0)
#endif

static u32 wm_711b0_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u8 wm_711b0_lbu(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_711b0_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_71034_run_terminal_one_lane(void)
{
    u32 archive_id;
    u32 source;
    size_t size;
    void* manager;
    u32 i;

#if defined(W34N30_MUTANT_SWAP_OVERLAY_STATE)
    WM_711B0_EVENT(2);
    ChangeGameState(2u);
    WM_711B0_EVENT(1);
    (void)LoadGameStateOverlay(2u);
#else
    WM_711B0_EVENT(1);
    (void)LoadGameStateOverlay(2u);
    WM_711B0_EVENT(2);
    ChangeGameState(2u);
#endif

    WM_711B0_EVENT(3);
#if defined(W34N30_MUTANT_GUEST_594F8)
    *(u8*)PSX_ADDR(0x800594F8u) = 0u;
#else
    D_800594F8 = 0u;
#endif

    for (i = 0u; i < 3u; i++) {
#if defined(W34N30_MUTANT_WRONG_PARTY_STRIDE)
        wm_711b0_sh(WM_711B0_EE70 + i * 4u,
                    (u16)wm_711b0_lbu(WM_711B0_F8E5 + i));
#else
        wm_711b0_sh(WM_711B0_EE70 + i * 2u,
                    (u16)wm_711b0_lbu(WM_711B0_F8E5 + i));
#endif
    }

    WM_711B0_EVENT(4);
#if !defined(W34N30_MUTANT_SKIP_SOUND_CLEANUP)
    func_80039CC4();
#endif

#if defined(W34N30_MUTANT_WRONG_ARCHIVE_ID)
    archive_id = wm_711b0_lw(WM_711B0_C614);
#else
    archive_id = wm_711b0_lw(WM_711B0_BCC8);
#endif
    source = wm_711b0_lw(WM_711B0_C614);
    WM_711B0_EVENT(5);
    size = (size_t)(u32)ArchiveDecodeAlignedSize(archive_id);
    WM_711B0_EVENT(6);
#if defined(W34N30_MUTANT_WRONG_COPY_SOURCE)
    memcpy(D_80062648, PSX_ADDR(source + 4u), size);
#else
    memcpy(D_80062648, PSX_ADDR(source), size);
#endif

    WM_711B0_EVENT(7);
#if !defined(W34N30_MUTANT_SKIP_OLD_MANAGER_SAVE)
    D_8004F2FC = (s32)(intptr_t)D_80062528;
#endif
    manager = func_80039850(D_80062648);
#if !defined(W34N30_MUTANT_SKIP_NEW_MANAGER_PUBLISH)
    D_80062528 = manager;
#endif
    WM_711B0_EVENT(8);
#if defined(W34N30_MUTANT_WRONG_CONFIGURE_LEVEL)
    func_80039A80(manager, 0, 0);
#else
    func_80039A80(manager, 127, 0);
#endif

    WM_711B0_EVENT(9);
    D_800591AE = 0u;
#if defined(W34N30_MUTANT_SWAP_EPILOGUE)
    WM_711B0_EVENT(11);
    MainLoop(0);
    WM_711B0_EVENT(10);
    wm_800762FC();
#else
    WM_711B0_EVENT(10);
    wm_800762FC();
    WM_711B0_EVENT(11);
    MainLoop(0);
#endif
}

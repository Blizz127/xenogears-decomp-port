/* Retail base-world session setup callback 0x80072238..0x80072998. */

#include "world_map_session_setup_72238.h"

#include "psx_memory.h"
#include "world_map_common_tail.h"
#include "world_map_convergence.h"
#include "world_map_framebuffer_init.h"
#include "world_map_helper_72db4.h"
#include "world_map_helper_73398.h"
#include "world_map_helper_7565c.h"
#include "world_map_helper_75d4c.h"
#include "world_map_helper_96130.h"
#include "world_map_terrain_init.h"

#include <stdio.h>
#include <string.h>

#define WM_72238_C894 0x8009C894u
#define WM_72238_EE6A 0x8006EE6Au

typedef struct Wm72238Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Wm72238Rect;

extern int MoveImage(void* rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u32 wm_80096668_circular_distance(void);
extern void wm_ready_buffer_consume(void);
extern void wm_mode_audio_setup(void);
extern void func_80039CC4(void);
extern void func_800399D4(void* manager);
extern void* D_80062528;
extern s32 D_8004F2FC;

#if defined(WM_72238_TEST_HOOKS)
extern void wm_72238_test_audio_store(int destination, u32 value);
#endif

static u16 wm_72238_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 wm_72238_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static int wm_72238_require(int result, const char* stage)
{
    if (result == 0)
        return 0;
    fprintf(stderr, "[worldmap-slot1] stage=%s failed rc=%d\n", stage, result);
    return -1;
}

void PcPort_WorldMapResetStageGuards(void) __attribute__((weak));

int wm_80072238(void)
{
    Wm72238Rect source_rect;
    wm_conv_p1_next_t convergence_next;
    u32 initial_c894;

    /* A new session: the native setup stages may run again (retail re-runs
     * them every session). */
    if (PcPort_WorldMapResetStageGuards)
        PcPort_WorldMapResetStageGuards();
    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
#if !defined(W34N9_MUTANT_SKIP_TRANSITION)
    if (wm_80072DB4(64, 0, 4, 2) != 0)
        return -1;
#endif

    ArchiveCdDataSync(0);
    if (wm_72238_require(wm_72238_stage_second_wave(), "second_wave") != 0)
        return -1;
    if (wm_72238_require(wm_72238_stage_object_pool(), "object_pool") != 0)
        return -1;
#if defined(W34N9_MUTANT_SWAP_POOL_TEMPLATE)
    if (wm_72238_require(wm_72238_stage_mode_enter(), "mode_enter") != 0)
        return -1;
    if (wm_72238_require(wm_72238_stage_state_template(), "state_template") != 0)
        return -1;
#else
    if (wm_72238_require(wm_72238_stage_state_template(), "state_template") != 0)
        return -1;
    if (wm_72238_require(wm_72238_stage_mode_enter(), "mode_enter") != 0)
        return -1;
#endif
    if (wm_72238_require(wm_72238_stage_cross_products(), "cross_products") != 0)
        return -1;

    /* Retail reads C894 before its audio-ownership prelude, then reads it
     * again after the independent EE6A branch. */
    initial_c894 = wm_72238_lw(WM_72238_C894);
    if (initial_c894 == 0u) {
#if !defined(W34N9_MUTANT_SKIP_WDS_CLEANUP)
        if (wm_72238_require(wm_72238_stage_wds_cleanup(), "wds_cleanup") != 0)
            return -1;
#endif
    } else {
        /* Retail 0x800723A4..0x800723D0 transfers the parked field audio
         * manager back to the world owner before restoring the session. */
#if !defined(W34N22_MUTANT_SKIP_AUDIO_TRANSFER)
        u32 parked_manager;

        func_80039CC4();
        func_800399D4(D_80062528);
        parked_manager = (u32)D_8004F2FC;
#if defined(W34N22_MUTANT_SWAP_AUDIO_STORES)
        D_80062528 = (void*)(uintptr_t)parked_manager;
#if defined(WM_72238_TEST_HOOKS)
        wm_72238_test_audio_store(1, parked_manager);
#endif
#endif
        D_8004F2FC = 0;
#if defined(WM_72238_TEST_HOOKS)
        wm_72238_test_audio_store(0, 0u);
#endif
#if !defined(W34N22_MUTANT_SWAP_AUDIO_STORES)
        D_80062528 = (void*)(uintptr_t)parked_manager;
#if defined(WM_72238_TEST_HOOKS)
        wm_72238_test_audio_store(1, parked_manager);
#endif
#endif
#endif
    }

    /* Retail gives the restore-entry flag priority after the independent
     * audio/cleanup prelude.  The helper consumes EE6A and this arm rejoins
     * after placement without falling through to fresh/restore placement. */
    if (wm_72238_lhu(WM_72238_EE6A) != 0u) {
#if !defined(W34N25_MUTANT_SKIP_73398)
        wm_80073398();
#endif
#if !defined(W34N25_MUTANT_73398_FALLTHROUGH)
        goto entry_placement_done;
#endif
    }

#if defined(W34N22_MUTANT_CACHE_C894)
    if (initial_c894 == 0u) {
#else
    if (wm_72238_lw(WM_72238_C894) == 0u) {
#endif
        if (wm_72238_require(wm_72238_stage_entry_placement(), "entry_placement") != 0)
            return -1;
    } else {
#if defined(W34N22_MUTANT_SWAP_RESTORE_PAIR)
        wm_80075D4C();
        wm_8007565C();
#else
#if !defined(W34N22_MUTANT_SKIP_RESTORE_COPY)
        wm_8007565C();
#endif
        wm_80075D4C();
#endif
    }

#if !defined(W34N25_MUTANT_73398_FALLTHROUGH)
entry_placement_done:
#endif
    ArchiveCdDataSync(0);
    if (wm_72238_require(wm_72238_stage_gpu_asset_a(), "gpu_asset_a") != 0 ||
        wm_72238_require(wm_72238_stage_gpu_asset_b(), "gpu_asset_b") != 0 ||
        wm_72238_require(wm_72238_stage_object_matrix(), "object_matrix") != 0 ||
        wm_72238_require(wm_72238_stage_third_wave(), "third_wave") != 0 ||
        wm_72238_require(wm_72238_stage_bss_constants(), "bss_constants") != 0 ||
        wm_72238_require(wm_72238_stage_primitive_templates(), "primitive_templates") != 0 ||
        wm_72238_require(wm_72238_stage_record_clut(), "record_clut") != 0 ||
        wm_72238_require(wm_72238_stage_gfx_work_buffers(), "gfx_work_buffers") != 0 ||
        wm_72238_require(wm_72238_stage_ft4_pools(), "ft4_pools") != 0 ||
        wm_72238_require(wm_72238_stage_heap_table(), "heap_table") != 0 ||
        wm_72238_require(wm_72238_stage_upload_a(), "upload_a") != 0 ||
        wm_72238_require(wm_72238_stage_upload_b(), "upload_b") != 0 ||
        wm_72238_require(wm_72238_stage_draw_packets(), "draw_packets") != 0 ||
        wm_72238_require(wm_72238_stage_88f64(), "88f64") != 0)
        return -1;

    if (wm_72238_require(wm_72238_stage_archive_poll(), "archive_poll") != 0)
        return -1;
    if (wm_72238_lw(WM_72238_C894) == 0u &&
        wm_72238_require(wm_72238_stage_first_wds(), "first_wds") != 0)
        return -1;
    if (wm_72238_require(wm_72238_stage_archive_index(), "archive_index") != 0)
        return -1;

    /* Retail 0x800724F0..0x80072554 re-reads C894: a fresh session seeds the
     * terrain from the placed position and drains the CD queue to <2
     * outstanding; a resumed session (C894 != 0) keeps its restored terrain
     * origin (0x80097CB8 on 0x8009BE28) and drains the queue completely. */
#if !defined(WM_72238_MUTANT_FRESH_TERRAIN_ON_RESUME)
    if (wm_72238_lw(WM_72238_C894) != 0u) {
        wm_80097CB8(0x8009BE28u);
        wm_80096694();
    } else
#endif
    {
        wm_80097BC0(0x8009C5ACu);
        do {
            (void)wm_800967E4();
            (void)Vsync(0);
#if defined(W34N9_MUTANT_SINGLE_CD_DRAIN)
            break;
#endif
        } while (wm_80096668_circular_distance() >= 2u);
    }

    wm_ready_buffer_consume();
    wm_mode_audio_setup();
    convergence_next = wm_800726C0_convergence_p1();
#if !defined(WM_RESUME_MUTANT_SKIP_BRANCH)
    if (convergence_next == WM_CONV_P1_CUT_FLAG1_ARC)
        wm_80072784_convergence_resume();
#endif
#if !defined(W34N9_MUTANT_SKIP_CONVERGENCE_P2)
    if (convergence_next == WM_CONV_P1_CUT_SECOND_TABLE)
        (void)wm_8007272C_convergence_p2();
#endif

    (void)wm_8007290C_common_tail_p0();
    (void)wm_8007293C_common_tail_p1();
#if defined(W34N9_MUTANT_SWAP_COMMON_TAIL)
    (void)wm_8007294C_common_tail_p3();
    (void)wm_80072944_common_tail_p2();
#else
    (void)wm_80072944_common_tail_p2();
    (void)wm_8007294C_common_tail_p3();
#endif
    (void)wm_80072954_common_tail_p4();
    (void)wm_8007295C_common_tail_p5();
    return 0;
}

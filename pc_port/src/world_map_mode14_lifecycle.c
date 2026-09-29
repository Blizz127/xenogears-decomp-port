/* Retail mode-14 lifecycle [0x8007A5DC,0x8007A9B4). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_gamestate.h"
#include "world_map_common_tail.h"
#include "world_map_convergence.h"
#include "world_map_framebuffer_init.h"
#include "world_map_helper_72db4.h"
#include "world_map_helper_86124.h"
#include "world_map_helper_96130.h"
#include "world_map_mode811_lifecycle.h"
#include "world_map_mode9_lifecycle.h"
#include "world_map_mode14_lifecycle.h"
#include "world_map_session_setup_72238.h"
#include "world_map_teardown_7299c.h"
#include "world_map_terrain_init.h"

typedef struct WmMode14Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMode14Rect;

extern int MoveImage(void *rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern void ArchiveCdDataSync(int mode);
extern unsigned int HeapFree(void *pointer);
extern void SoundAddSedsEntry(void *seds);
extern void func_80039FF8(void);
extern void func_8003852C(void *seds);
extern u32 wm_80096668_circular_distance(void);
extern void *D_8006259C;

#define WM_M14L_TEMPLATE_SRC  UINT32_C(0x8009A180)
#define WM_M14L_TEMPLATE_DST  UINT32_C(0x8009BE4C)
#define WM_M14L_CCA4          UINT32_C(0x8009CCA4)
#define WM_M14L_D3CC          UINT32_C(0x8009D3CC)
#define WM_M14L_D804          UINT32_C(0x8009D804)
#define WM_M14L_D144          UINT32_C(0x8009D144)
#define WM_M14L_CD40          UINT32_C(0x8009CD40)
#define WM_M14L_POSITION      UINT32_C(0x8009C5AC)

#define WM_M14L_BC38          UINT32_C(0x8009BC38)
#define WM_M14L_BCB0          UINT32_C(0x8009BCB0)
#define WM_M14L_BC3C          UINT32_C(0x8009BC3C)
#define WM_M14L_BCB4          UINT32_C(0x8009BCB4)
#define WM_M14L_C180          UINT32_C(0x8009C180)

#define WM_M14L_BD3A          UINT32_C(0x8009BD3A)
#define WM_M14L_F94E          UINT32_C(0x8006F94E)
#define WM_M14L_F950          UINT32_C(0x8006F950)
#define WM_M14L_F954          UINT32_C(0x8006F954)
#define WM_M14L_BBC4          UINT32_C(0x8009BBC4)

static u32 m14l_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m14l_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m14l_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

__attribute__((unused)) static void m14l_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int m14l_require(int result)
{
    return result == 0 ? 0 : -1;
}

static void *m14l_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= UINT32_C(0x80000000) && value < UINT32_C(0x80200000))
        return PSX_ADDR(value);
    return (void *)(uintptr_t)value;
}

static void m14l_free_value(u32 address)
{
    (void)HeapFree(m14l_to_host(m14l_lw(address)));
}

int wm_8007A5DC(void)
{
    WmMode14Rect source_rect;

    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
#if defined(W34N76_MUTANT_WRONG_TRANSITION_ARG)
    if (m14l_require(wm_80072DB4(64, 0, 4, 1)) != 0)
#else
    if (m14l_require(wm_80072DB4(64, 0, 4, 2)) != 0)
#endif
        return -1;
    if (m14l_require(wm_mode14_stage_second_wave_finish()) != 0 ||
        m14l_require(wm_72238_stage_object_pool()) != 0)
        return -1;

    memcpy(PSX_ADDR(WM_M14L_TEMPLATE_DST),
           PSX_ADDR(WM_M14L_TEMPLATE_SRC), 32u);
    m14l_sw(WM_M14L_CCA4, 2u);
#if defined(W34N76_MUTANT_WRONG_MODE_CONSTANT)
    m14l_sw(WM_M14L_D3CC, 4u);
#else
    m14l_sw(WM_M14L_D3CC, 16u);
#endif
    m14l_sw(WM_M14L_D804, 0u);
    m14l_sw(WM_M14L_D144, 0u);
    m14l_sw(WM_M14L_CD40, UINT32_C(0x80086700));

    if (m14l_require(wm_72238_stage_cross_products()) != 0)
        return -1;
    ArchiveCdDataSync(0);
    if (m14l_require(wm_800721E4()) != 0)
        return -1;

    m14l_sw(WM_M14L_POSITION + 0u, UINT32_C(0x05BED000));
    m14l_sw(WM_M14L_POSITION + 4u, UINT32_C(0xFFF60000));
    m14l_sw(WM_M14L_POSITION + 8u, UINT32_C(0x062A8000));

#if defined(W34N76_MUTANT_SWAP_OBJECT_GPU_A)
    if (m14l_require(wm_72238_stage_gpu_asset_a()) != 0 ||
        m14l_require(wm_72238_stage_object_matrix()) != 0)
        return -1;
#else
    if (m14l_require(wm_72238_stage_object_matrix()) != 0 ||
        m14l_require(wm_72238_stage_gpu_asset_a()) != 0)
        return -1;
#endif
    if (m14l_require(wm_72238_stage_gpu_asset_b()) != 0 ||
        m14l_require(wm_72238_stage_bss_constants()) != 0 ||
        m14l_require(wm_72238_stage_heap_table()) != 0 ||
        m14l_require(wm_72238_stage_upload_a()) != 0 ||
        m14l_require(wm_72238_stage_upload_b()) != 0 ||
        m14l_require(wm_72238_stage_draw_packets()) != 0 ||
        m14l_require(wm_72238_stage_88f64()) != 0)
        return -1;

    ArchiveCdDataSync(0);
#if !defined(W34N76_MUTANT_SKIP_SOUND_LINK)
    SoundAddSedsEntry(D_8006259C);
#endif
    (void)ArchiveSetIndex(36, 0);
    wm_80097BC0(WM_M14L_POSITION);
    do {
        (void)wm_800967E4();
        (void)Vsync(0);
    } while (wm_80096668_circular_distance() > 0u);

    wm_pool_register(UINT32_C(0x800923A8), UINT32_C(0x800925A0));
    wm_pool_register(UINT32_C(0x8007A9B4), UINT32_C(0x8007A9F8));
    wm_pool_register(UINT32_C(0x8007AD34), UINT32_C(0x8007ADD4));
    wm_pool_register(UINT32_C(0x8007B200), UINT32_C(0x8007B394));
    wm_pool_register(UINT32_C(0x8007B604), UINT32_C(0x8007B798));
    wm_pool_register(UINT32_C(0x8007BA08), UINT32_C(0x8007BA10));
    wm_pool_register(UINT32_C(0x8007BB60), UINT32_C(0x8007BBEC));
#if defined(W34N76_MUTANT_WRONG_REGISTER_PAIR)
    wm_pool_register(UINT32_C(0x80078948), UINT32_C(0x8007ADD4));
#else
    wm_pool_register(UINT32_C(0x80078948), UINT32_C(0x80078950));
#endif

    wm_800978FC();
    wm_8008901C();
    wm_800865A0();
#if !defined(W34N76_MUTANT_SKIP_PALETTE_TAIL)
    wm_80075228();
#endif
    return 0;
}

void wm_8007A8AC(void)
{
    func_80039FF8();
    func_8003852C(D_8006259C);
#if !defined(W34N76_MUTANT_SKIP_SEDS_FREE)
    (void)HeapFree(D_8006259C);
#endif
    wm_80084818();
    wm_80086568();
    wm_800866C8();
    wm_80074F04();
    wm_800750DC();
    wm_80088FF4();
    wm_80089128();
    wm_80097D64();

    m14l_free_value(WM_M14L_BC38);
    m14l_free_value(WM_M14L_BCB0);
    m14l_free_value(WM_M14L_BC3C);
    m14l_free_value(WM_M14L_BCB4);
    m14l_free_value(WM_M14L_C180);
    wm_800976A0();

#if defined(W34N76_MUTANT_WRONG_TEARDOWN_STATE)
    wm_gs_sh(WM_M14L_F94E, 272u);
#else
    wm_gs_sh(WM_M14L_F94E, 282u);
#endif
    wm_gs_sh(WM_M14L_F954, 0u);
    m14l_sw(WM_M14L_BBC4, 1u);
    wm_gs_sh(WM_M14L_F950, m14l_lhu(WM_M14L_BD3A));
}

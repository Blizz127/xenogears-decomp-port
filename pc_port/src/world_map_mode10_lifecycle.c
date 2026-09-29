/* Retail mode-10 lifecycle [0x80078A60,0x80078E2C). */
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
#include "world_map_mode10_lifecycle.h"
#include "world_map_mode811_lifecycle.h"
#include "world_map_mode9_lifecycle.h"
#include "world_map_session_setup_72238.h"
#include "world_map_teardown_7299c.h"
#include "world_map_terrain_init.h"

typedef struct WmMode10Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMode10Rect;

extern int MoveImage(void* rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern void ArchiveCdDataSync(int mode);
extern unsigned int HeapFree(void* pointer);
extern void SoundAddSedsEntry(void* seds);
extern void func_80039FF8(void);
extern void func_8003852C(void* seds);
extern u32 wm_80096668_circular_distance(void);
extern void* D_8006259C;

#define WM_M10L_TEMPLATE_SRC 0x8009A180u
#define WM_M10L_TEMPLATE_DST 0x8009BE4Cu
#define WM_M10L_CCA4         0x8009CCA4u
#define WM_M10L_D3CC         0x8009D3CCu
#define WM_M10L_D804         0x8009D804u
#define WM_M10L_D144         0x8009D144u
#define WM_M10L_CD40         0x8009CD40u
#define WM_M10L_POSITION     0x8009C5ACu

#define WM_M10L_BC38         0x8009BC38u
#define WM_M10L_BCB0         0x8009BCB0u
#define WM_M10L_BC3C         0x8009BC3Cu
#define WM_M10L_BCB4         0x8009BCB4u
#define WM_M10L_C180         0x8009C180u
#define WM_M10L_BE24         0x8009BE24u

#define WM_M10L_BD3A         0x8009BD3Au
#define WM_M10L_F94E         0x8006F94Eu
#define WM_M10L_F950         0x8006F950u
#define WM_M10L_F954         0x8006F954u
#define WM_M10L_BBC4         0x8009BBC4u

static u32 m10l_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m10l_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m10l_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

__attribute__((unused)) static void m10l_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int m10l_require(int result)
{
    return result == 0 ? 0 : -1;
}

static void* m10l_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

static void m10l_free_value(u32 address)
{
    (void)HeapFree(m10l_to_host(m10l_lw(address)));
}

int wm_80078A60(void)
{
    WmMode10Rect source_rect;

    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
#if defined(W34N66_MUTANT_WRONG_TRANSITION_ARG)
    if (m10l_require(wm_80072DB4(64, 0, 4, 1)) != 0)
#else
    if (m10l_require(wm_80072DB4(64, 0, 4, 2)) != 0)
#endif
        return -1;
    if (m10l_require(wm_mode10_stage_second_wave_finish()) != 0 ||
        m10l_require(wm_72238_stage_object_pool()) != 0)
        return -1;

    memcpy(PSX_ADDR(WM_M10L_TEMPLATE_DST),
           PSX_ADDR(WM_M10L_TEMPLATE_SRC), 32u);
    m10l_sw(WM_M10L_CCA4, 2u);
#if defined(W34N66_MUTANT_WRONG_MODE_CONSTANT)
    m10l_sw(WM_M10L_D3CC, 4u);
#else
    m10l_sw(WM_M10L_D3CC, 16u);
#endif
    m10l_sw(WM_M10L_D804, 0u);
    m10l_sw(WM_M10L_D144, 0u);
    m10l_sw(WM_M10L_CD40, 0x80086700u);

    if (m10l_require(wm_72238_stage_cross_products()) != 0)
        return -1;
    ArchiveCdDataSync(0);
    if (m10l_require(wm_800721E4()) != 0)
        return -1;

    m10l_sw(WM_M10L_POSITION + 0u, 0x02000000u);
    m10l_sw(WM_M10L_POSITION + 4u, 0xFFD00000u);
    m10l_sw(WM_M10L_POSITION + 8u, 0x02000000u);

#if defined(W34N66_MUTANT_SWAP_OBJECT_GPU_A)
    if (m10l_require(wm_72238_stage_gpu_asset_a()) != 0 ||
        m10l_require(wm_72238_stage_object_matrix()) != 0)
        return -1;
#else
    if (m10l_require(wm_72238_stage_object_matrix()) != 0 ||
        m10l_require(wm_72238_stage_gpu_asset_a()) != 0)
        return -1;
#endif
    if (
        m10l_require(wm_72238_stage_gpu_asset_b()) != 0 ||
        m10l_require(wm_72238_stage_bss_constants()) != 0 ||
        m10l_require(wm_72238_stage_heap_table()) != 0 ||
        m10l_require(wm_72238_stage_upload_a()) != 0 ||
        m10l_require(wm_72238_stage_upload_b()) != 0 ||
        m10l_require(wm_72238_stage_draw_packets()) != 0 ||
        m10l_require(wm_72238_stage_88f64()) != 0)
        return -1;

    ArchiveCdDataSync(0);
#if !defined(W34N66_MUTANT_SKIP_SOUND_LINK)
    SoundAddSedsEntry(D_8006259C);
#endif
    (void)ArchiveSetIndex(36, 0);
    wm_80097BC0(WM_M10L_POSITION);
    do {
        (void)wm_800967E4();
        (void)Vsync(0);
    } while (wm_80096668_circular_distance() > 0u);

    wm_pool_register(0x800923A8u, 0x800925A0u);
    wm_pool_register(0x80078E2Cu, 0x80078EA4u);
    wm_pool_register(0x800795E4u, 0x80079778u);
    wm_pool_register(0x8007A144u, 0x8007A1B4u);
    wm_pool_register(0x8007A410u, 0x8007A430u);
    wm_pool_register(0x8007A568u, 0x8007A570u);
#if defined(W34N66_MUTANT_WRONG_REGISTER_PAIR)
    wm_pool_register(0x800794D8u, 0x800795E4u);
#else
    wm_pool_register(0x800794D8u, 0x80079538u);
#endif
    wm_pool_register(0x80078948u, 0x80078950u);

    wm_800978FC();
    wm_8008901C();
    wm_800865A0();
#if !defined(W34N66_MUTANT_SKIP_PALETTE_TAIL)
    wm_80075228();
#endif
    return 0;
}

void wm_80078D24(void)
{
    func_80039FF8();
    func_8003852C(D_8006259C);
#if !defined(W34N66_MUTANT_SKIP_SEDS_FREE)
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

    m10l_free_value(WM_M10L_BC38);
    m10l_free_value(WM_M10L_BCB0);
    m10l_free_value(WM_M10L_BC3C);
    m10l_free_value(WM_M10L_BCB4);
    m10l_free_value(WM_M10L_C180);
    wm_800976A0();

#if defined(W34N66_MUTANT_WRONG_TEARDOWN_STATE)
    wm_gs_sh(WM_M10L_F94E, 270u);
#else
    wm_gs_sh(WM_M10L_F94E, 272u);
#endif
    wm_gs_sh(WM_M10L_F954, 0u);
    m10l_sw(WM_M10L_BBC4, 1u);
    wm_gs_sh(WM_M10L_F950, m10l_lhu(WM_M10L_BD3A));
}

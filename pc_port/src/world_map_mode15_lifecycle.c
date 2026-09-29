/* Retail mode-15 lifecycle [0x8007D918,0x8007DE14). */
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
#include "world_map_mode15_lifecycle.h"
#include "world_map_session_setup_72238.h"
#include "world_map_teardown_7299c.h"
#include "world_map_terrain_init.h"

typedef struct WmMode15Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMode15Rect;

extern int MoveImage(void *rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern void ArchiveCdDataSync(int mode);
extern unsigned int HeapFree(void *pointer);
extern void SoundAddSedsEntry(void *seds);
extern void func_8001B66C(void);
extern void *func_80039850(void *song_file);
extern void func_80039A80(void *manager, int level, int steps);
extern void func_8003A89C(void *manager, s32 level, s32 steps);
extern void func_80039FF8(void);
extern void func_8003852C(void *seds);
extern u32 wm_80096668_circular_distance(void);
extern short g_SoundControlFlags;
extern unsigned char D_80062648[];
extern void *D_80062528;
extern void *D_8006259C;

#define WM_M15L_TEMPLATE_SRC   UINT32_C(0x8009A180)
#define WM_M15L_TEMPLATE_DST   UINT32_C(0x8009BE4C)
#define WM_M15L_POSITION_TABLE UINT32_C(0x8009A5B4)
#define WM_M15L_EXIT_TABLE     UINT32_C(0x8009A5CC)
#define WM_M15L_CCA4           UINT32_C(0x8009CCA4)
#define WM_M15L_D3CC           UINT32_C(0x8009D3CC)
#define WM_M15L_D3D0           UINT32_C(0x8009D3D0)
#define WM_M15L_D3D4           UINT32_C(0x8009D3D4)
#define WM_M15L_D804           UINT32_C(0x8009D804)
#define WM_M15L_D144           UINT32_C(0x8009D144)
#define WM_M15L_CD40           UINT32_C(0x8009CD40)
#define WM_M15L_POSITION       UINT32_C(0x8009C5AC)
#define WM_M15L_WDS_BUFFER     UINT32_C(0x8009C88C)
#define WM_M15L_SONG_BUFFER    UINT32_C(0x8009C884)

#define WM_M15L_BC38 UINT32_C(0x8009BC38)
#define WM_M15L_BCB0 UINT32_C(0x8009BCB0)
#define WM_M15L_BC3C UINT32_C(0x8009BC3C)
#define WM_M15L_BCB4 UINT32_C(0x8009BCB4)
#define WM_M15L_C180 UINT32_C(0x8009C180)

#define WM_M15L_BD3A UINT32_C(0x8009BD3A)
#define WM_M15L_F94E UINT32_C(0x8006F94E)
#define WM_M15L_F950 UINT32_C(0x8006F950)
#define WM_M15L_F954 UINT32_C(0x8006F954)
#define WM_M15L_BBC4 UINT32_C(0x8009BBC4)

static u32 m15l_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m15l_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 m15l_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m15l_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

__attribute__((unused)) static void m15l_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int m15l_require(int result)
{
    return result == 0 ? 0 : -1;
}

static void *m15l_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= UINT32_C(0x80000000) && value < UINT32_C(0x80200000))
        return PSX_ADDR(value);
    return (void *)(uintptr_t)value;
}

static void m15l_free_value(u32 address)
{
    (void)HeapFree(m15l_to_host(m15l_lw(address)));
}

static u32 m15l_fixed12(s16 value)
{
    return (u32)((s32)value * 4096);
}

int wm_8007D918(void)
{
    WmMode15Rect source_rect;
    u32 selector;
    u32 position_source;
    void *song_source;
    int song_size;
    void *manager;
    int repeat;

    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
#if defined(W34N97_MUTANT_WRONG_TRANSITION_ARG)
    if (m15l_require(wm_80072DB4(64, 0, 4, 1)) != 0)
#else
    if (m15l_require(wm_80072DB4(64, 0, 4, 2)) != 0)
#endif
        return -1;
    if (m15l_require(wm_mode15_stage_second_wave_finish()) != 0 ||
        m15l_require(wm_72238_stage_object_pool()) != 0)
        return -1;

    memcpy(PSX_ADDR(WM_M15L_TEMPLATE_DST),
           PSX_ADDR(WM_M15L_TEMPLATE_SRC), 32u);
    m15l_sw(WM_M15L_CCA4, 2u);
    m15l_sw(WM_M15L_D3CC, 16u);
    m15l_sw(WM_M15L_D804, 0u);
    m15l_sw(WM_M15L_D144, 0u);
    m15l_sw(WM_M15L_CD40, UINT32_C(0x80086700));

    if (m15l_require(wm_72238_stage_cross_products()) != 0)
        return -1;
    ArchiveCdDataSync(0);
    func_8001B66C();

    selector = m15l_lw(WM_M15L_D3D4);
#if defined(W34N97_MUTANT_WRONG_POSITION_STRIDE)
    position_source = WM_M15L_POSITION_TABLE + selector * 6u;
#else
    position_source = WM_M15L_POSITION_TABLE + selector * 8u;
#endif
    m15l_sw(WM_M15L_POSITION + 0u,
            m15l_fixed12(m15l_lh(position_source + 0u)));
    m15l_sw(WM_M15L_POSITION + 4u,
            m15l_fixed12(m15l_lh(position_source + 2u)));
    m15l_sw(WM_M15L_POSITION + 8u,
            m15l_fixed12(m15l_lh(position_source + 4u)));

#if defined(W34N97_MUTANT_SWAP_OBJECT_GPU_A)
    if (m15l_require(wm_72238_stage_gpu_asset_a()) != 0 ||
        m15l_require(wm_72238_stage_object_matrix()) != 0)
        return -1;
#else
    if (m15l_require(wm_72238_stage_object_matrix()) != 0 ||
        m15l_require(wm_72238_stage_gpu_asset_a()) != 0)
        return -1;
#endif
    if (m15l_require(wm_72238_stage_gpu_asset_b()) != 0 ||
        m15l_require(wm_72238_stage_third_wave()) != 0 ||
        m15l_require(wm_72238_stage_bss_constants()) != 0 ||
        m15l_require(wm_72238_stage_heap_table()) != 0 ||
        m15l_require(wm_72238_stage_upload_a()) != 0 ||
        m15l_require(wm_72238_stage_upload_b()) != 0 ||
        m15l_require(wm_72238_stage_draw_packets()) != 0 ||
        m15l_require(wm_72238_stage_88f64()) != 0)
        return -1;

    ArchiveCdDataSync(0);
#if !defined(W34N97_MUTANT_SKIP_WDS_LOAD)
    if (m15l_require(wm_72238_stage_first_wds()) != 0)
        return -1;
#endif
    (void)ArchiveSetIndex(36, 0);
    wm_80097BC0(WM_M15L_POSITION);
    do {
        (void)wm_800967E4();
        (void)Vsync(0);
    } while (wm_80096668_circular_distance() > 0u);

    while ((g_SoundControlFlags & 0x10) != 0) {
    }
    m15l_free_value(WM_M15L_WDS_BUFFER);
    SoundAddSedsEntry(D_8006259C);

    song_size = ArchiveDecodeAlignedSize(m15l_lw(WM_M15L_D3D0));
    song_source = m15l_to_host(m15l_lw(WM_M15L_SONG_BUFFER));
    memcpy(D_80062648, song_source, (size_t)song_size);
#if defined(W34N97_MUTANT_SKIP_SONG_START)
    manager = NULL;
#else
    manager = func_80039850(D_80062648);
    D_80062528 = manager;
    func_80039A80(manager, 127, 0);
#endif
    (void)manager;

    wm_pool_register(UINT32_C(0x800923A8), UINT32_C(0x800925A0));
    wm_pool_register(UINT32_C(0x8007DE14), UINT32_C(0x8007DE98));
    wm_pool_register(UINT32_C(0x8007E450), UINT32_C(0x8007E4E4));
    wm_pool_register(UINT32_C(0x8007ECA4), UINT32_C(0x8007EE34));
#if defined(W34N97_MUTANT_WRONG_REGISTER_COUNT)
    for (repeat = 0; repeat < 4; repeat++)
#else
    for (repeat = 0; repeat < 5; repeat++)
#endif
        wm_pool_register(UINT32_C(0x8007F8AC), UINT32_C(0x8007F968));
    wm_pool_register(UINT32_C(0x8007FC8C), UINT32_C(0x8007FD30));
    wm_pool_register(UINT32_C(0x80078948), UINT32_C(0x80078950));

    wm_800978FC();
    wm_8008901C();
    wm_800865A0();
    wm_80075228();
    return 0;
}

void wm_8007DCE0(void)
{
    u32 selector;

    func_8003A89C(D_80062528, 0, 240);
    func_80039FF8();
    func_8003852C(D_8006259C);
#if !defined(W34N97_MUTANT_SKIP_SEDS_FREE)
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

    m15l_free_value(WM_M15L_BC38);
    m15l_free_value(WM_M15L_BCB0);
    m15l_free_value(WM_M15L_BC3C);
    m15l_free_value(WM_M15L_BCB4);
    m15l_free_value(WM_M15L_C180);
    wm_800976A0();

    selector = m15l_lw(WM_M15L_D3D4);
#if defined(W34N97_MUTANT_WRONG_TEARDOWN_STATE)
    wm_gs_sh(WM_M15L_F94E, 416u);
#else
    wm_gs_sh(WM_M15L_F94E, 417u);
#endif
    wm_gs_sh(WM_M15L_F950, m15l_lhu(WM_M15L_BD3A));
    wm_gs_sh(WM_M15L_F954,
            m15l_lhu(WM_M15L_EXIT_TABLE + selector * 2u));
    m15l_sw(WM_M15L_BBC4, 1u);
}

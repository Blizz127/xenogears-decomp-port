/* Retail mode-17 lifecycle [0x80082324,0x800827C8). */
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
#include "world_map_mode17_lifecycle.h"
#include "world_map_session_setup_72238.h"
#include "world_map_teardown_7299c.h"
#include "world_map_terrain_init.h"

typedef struct WmMode17Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMode17Rect;

extern int MoveImage(void *rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern void ArchiveCdDataSync(int mode);
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern unsigned int HeapFree(void *pointer);
extern void SoundAddSedsEntry(void *seds);
extern void func_8001B66C(void);
extern void *func_80039850(void *song_file);
extern void func_80039A80(void *manager, int level, int steps);
extern void func_80039FF8(void);
extern void func_8003852C(void *seds);
extern u32 wm_80096668_circular_distance(void);
extern short g_SoundControlFlags;
extern unsigned char D_80062648[];
extern void *D_80062528;
extern void *D_8006259C;

#define WM_M17L_TEMPLATE_SRC UINT32_C(0x8009A180)
#define WM_M17L_TEMPLATE_DST UINT32_C(0x8009BE4C)
#define WM_M17L_POSITION     UINT32_C(0x8009C5AC)
#define WM_M17L_CCA4         UINT32_C(0x8009CCA4)
#define WM_M17L_D3CC         UINT32_C(0x8009D3CC)
#define WM_M17L_D3D0         UINT32_C(0x8009D3D0)
#define WM_M17L_D804         UINT32_C(0x8009D804)
#define WM_M17L_D144         UINT32_C(0x8009D144)
#define WM_M17L_CD40         UINT32_C(0x8009CD40)
#define WM_M17L_WDS_BUFFER   UINT32_C(0x8009C88C)
#define WM_M17L_SONG_BUFFER  UINT32_C(0x8009C884)

#define WM_M17L_BC38 UINT32_C(0x8009BC38)
#define WM_M17L_BCB0 UINT32_C(0x8009BCB0)
#define WM_M17L_BC3C UINT32_C(0x8009BC3C)
#define WM_M17L_BCB4 UINT32_C(0x8009BCB4)
#define WM_M17L_C180 UINT32_C(0x8009C180)

#define WM_M17L_BD3A UINT32_C(0x8009BD3A)
#define WM_M17L_F94E UINT32_C(0x8006F94E)
#define WM_M17L_F950 UINT32_C(0x8006F950)
#define WM_M17L_F954 UINT32_C(0x8006F954)
#define WM_M17L_BBC4 UINT32_C(0x8009BBC4)

static u32 m17l_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m17l_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m17l_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

__attribute__((unused)) static void m17l_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int m17l_require(int result)
{
    return result == 0 ? 0 : -1;
}

static void *m17l_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= UINT32_C(0x80000000) && value < UINT32_C(0x80200000))
        return PSX_ADDR(value);
    return (void *)(uintptr_t)value;
}

static void m17l_free_value(u32 address)
{
    (void)HeapFree(m17l_to_host(m17l_lw(address)));
}

int wm_80082324(void)
{
    WmMode17Rect source_rect;
    void *song_source;
    void *manager;
    int song_size;
    unsigned registration;

    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
#if defined(W34N109_MUTANT_WRONG_TRANSITION_ARG)
    if (m17l_require(wm_80072DB4(64, 0, 4, 1)) != 0)
#else
    if (m17l_require(wm_80072DB4(64, 0, 4, 2)) != 0)
#endif
        return -1;
    if (m17l_require(wm_mode17_stage_second_wave_finish()) != 0 ||
        m17l_require(wm_72238_stage_object_pool()) != 0)
        return -1;

    memcpy(PSX_ADDR(WM_M17L_TEMPLATE_DST),
           PSX_ADDR(WM_M17L_TEMPLATE_SRC), 32u);
    m17l_sw(WM_M17L_CCA4, 2u);
#if defined(W34N109_MUTANT_WRONG_MODE_CONSTANT)
    m17l_sw(WM_M17L_D3CC, 17u);
#else
    m17l_sw(WM_M17L_D3CC, 16u);
#endif
    m17l_sw(WM_M17L_D804, 0u);
    m17l_sw(WM_M17L_D144, 0u);
    m17l_sw(WM_M17L_CD40, UINT32_C(0x80086700));
    if (m17l_require(wm_72238_stage_cross_products()) != 0)
        return -1;
    ArchiveCdDataSync(0);
    func_8001B66C();

    m17l_sw(WM_M17L_POSITION + 0u, UINT32_C(0x070A9000));
    m17l_sw(WM_M17L_POSITION + 4u, UINT32_C(0xFFEB8000));
    m17l_sw(WM_M17L_POSITION + 8u, UINT32_C(0x042AA000));

#if defined(W34N109_MUTANT_SWAP_OBJECT_GPU_A)
    if (m17l_require(wm_72238_stage_gpu_asset_a()) != 0 ||
        m17l_require(wm_72238_stage_object_matrix()) != 0)
        return -1;
#else
    if (m17l_require(wm_72238_stage_object_matrix()) != 0 ||
        m17l_require(wm_72238_stage_gpu_asset_a()) != 0)
        return -1;
#endif
    if (m17l_require(wm_72238_stage_gpu_asset_b()) != 0 ||
        m17l_require(wm_72238_stage_third_wave()) != 0 ||
        m17l_require(wm_72238_stage_bss_constants()) != 0 ||
        m17l_require(wm_72238_stage_record_clut()) != 0 ||
        m17l_require(wm_72238_stage_heap_table()) != 0 ||
        m17l_require(wm_72238_stage_upload_a()) != 0 ||
        m17l_require(wm_72238_stage_upload_b()) != 0 ||
        m17l_require(wm_72238_stage_draw_packets()) != 0 ||
        m17l_require(wm_72238_stage_88f64()) != 0)
        return -1;

    ArchiveCdDataSync(0);
#if !defined(W34N109_MUTANT_SKIP_WDS_LOAD)
    if (m17l_require(wm_72238_stage_first_wds()) != 0)
        return -1;
#endif
    (void)ArchiveSetIndex(36, 0);
    wm_80097BC0(WM_M17L_POSITION);
    do {
        (void)wm_800967E4();
        (void)Vsync(0);
    } while (wm_80096668_circular_distance() > 0u);

    while ((g_SoundControlFlags & 0x10) != 0) {
    }
    m17l_free_value(WM_M17L_WDS_BUFFER);
    SoundAddSedsEntry(D_8006259C);
    song_size = ArchiveDecodeAlignedSize(m17l_lw(WM_M17L_D3D0));
    song_source = m17l_to_host(m17l_lw(WM_M17L_SONG_BUFFER));
    memcpy(D_80062648, song_source, (size_t)song_size);
#if defined(W34N109_MUTANT_SKIP_SONG_START)
    manager = NULL;
#else
    manager = func_80039850(D_80062648);
    D_80062528 = manager;
    func_80039A80(manager, 127, 0);
#endif
    (void)manager;

    wm_pool_register(UINT32_C(0x800923A8), UINT32_C(0x800925A0));
    wm_pool_register(UINT32_C(0x800827C8), UINT32_C(0x80076B34));
    wm_pool_register(UINT32_C(0x800827EC), UINT32_C(0x800828DC));
    for (registration = 0u; registration < 5u; registration++)
        wm_pool_register(UINT32_C(0x80083214), UINT32_C(0x80083264));
#if defined(W34N109_MUTANT_WRONG_REGISTER_PAIR)
    wm_pool_register(UINT32_C(0x800834D0), UINT32_C(0x80083264));
#else
    wm_pool_register(UINT32_C(0x800834D0), UINT32_C(0x800834D8));
#endif
    wm_pool_register(UINT32_C(0x80076A14), UINT32_C(0x80076A1C));

    wm_800978FC();
    wm_8008901C();
    wm_800865A0();
#if !defined(W34N109_MUTANT_SKIP_QUATERNARY)
    wm_80085FE0();
#endif
#if !defined(W34N109_MUTANT_SKIP_PALETTE_TAIL)
    wm_80075228();
#endif
    return 0;
}

void wm_800826B4(void)
{
    func_80039FF8();
    func_8003852C(D_8006259C);
#if !defined(W34N109_MUTANT_SKIP_SEDS_FREE)
    (void)HeapFree(D_8006259C);
#endif
    wm_80084818();
    wm_80086124();
    wm_80086568();
    wm_800866C8();
    wm_80074F04();
    wm_800750DC();
    wm_80088FF4();
    wm_80089128();
    wm_80097D64();
    m17l_free_value(WM_M17L_BC38);
    m17l_free_value(WM_M17L_BCB0);
    m17l_free_value(WM_M17L_BC3C);
    m17l_free_value(WM_M17L_BCB4);
    m17l_free_value(WM_M17L_C180);
    wm_800976A0();
#if defined(W34N109_MUTANT_WRONG_TEARDOWN_STATE)
    wm_gs_sh(WM_M17L_F94E, 616u);
#else
    wm_gs_sh(WM_M17L_F94E, 617u);
#endif
    wm_gs_sh(WM_M17L_F954, 2u);
    m17l_sw(WM_M17L_BBC4, 1u);
    wm_gs_sh(WM_M17L_F950, m17l_lhu(WM_M17L_BD3A));
}

/* Retail mode-8/mode-11 lifecycle [0x80077214,0x8007756C). */
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
#include "world_map_session_setup_72238.h"
#include "world_map_teardown_7299c.h"
#include "world_map_terrain_init.h"

typedef struct WmMode811Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMode811Rect;

extern int MoveImage(void* rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern void ArchiveCdDataSync(int mode);
extern u32 wm_80096668_circular_distance(void);
extern unsigned int HeapFree(void* ptr);

#define WM_M811_TEMPLATE_SRC 0x8009A180u
#define WM_M811_TEMPLATE_DST 0x8009BE4Cu
#define WM_M811_CCA4         0x8009CCA4u
#define WM_M811_D3CC         0x8009D3CCu
#define WM_M811_D804         0x8009D804u
#define WM_M811_D144         0x8009D144u
#define WM_M811_CD40         0x8009CD40u
#define WM_M811_POSITION     0x8009C5ACu

#define WM_M811_BC38         0x8009BC38u
#define WM_M811_BCB0         0x8009BCB0u
#define WM_M811_BC3C         0x8009BC3Cu
#define WM_M811_BCB4         0x8009BCB4u
#define WM_M811_C180         0x8009C180u
#define WM_M811_CEB4         0x8009CEB4u
#define WM_M811_D150         0x8009D150u
#define WM_M811_D780         0x8009D780u
#define WM_M811_D7D0         0x8009D7D0u
#define WM_M811_BDF4         0x8009BDF4u
#define WM_M811_BE24         0x8009BE24u

#define WM_M811_BD3A         0x8009BD3Au
#define WM_M811_F94E         0x8006F94Eu
#define WM_M811_F950         0x8006F950u
#define WM_M811_F954         0x8006F954u
#define WM_M811_BBC4         0x8009BBC4u

static u32 m811_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m811_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m811_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

__attribute__((unused)) static void m811_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int m811_require(int result)
{
    return result == 0 ? 0 : -1;
}

static void* m811_pointer_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

static void m811_free_value(u32 address)
{
    HeapFree(m811_pointer_to_host(m811_lw(address)));
}

int wm_80077214(void)
{
    WmMode811Rect source_rect;

    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
    if (m811_require(wm_80072DB4(64, 0, 4, 2)) != 0)
        return -1;
    if (m811_require(wm_mode811_stage_second_wave_finish()) != 0)
        return -1;
    if (m811_require(wm_72238_stage_object_pool()) != 0)
        return -1;

    memcpy(PSX_ADDR(WM_M811_TEMPLATE_DST),
           PSX_ADDR(WM_M811_TEMPLATE_SRC), 32u);
    m811_sw(WM_M811_CCA4, 2u);
    m811_sw(WM_M811_D3CC, 4u);
    m811_sw(WM_M811_D804, 0u);
    m811_sw(WM_M811_D144, 0u);
    m811_sw(WM_M811_CD40, 0x80086700u);

    if (m811_require(wm_72238_stage_cross_products()) != 0)
        return -1;
    ArchiveCdDataSync(0);

    m811_sw(WM_M811_POSITION + 0u, 0x07702000u);
    m811_sw(WM_M811_POSITION + 4u, 0xFFD00000u);
    m811_sw(WM_M811_POSITION + 8u, 0x027C0000u);

#if defined(W34N57_MUTANT_SWAP_OBJECT_GPU_A)
    if (m811_require(wm_72238_stage_gpu_asset_a()) != 0 ||
        m811_require(wm_72238_stage_object_matrix()) != 0)
        return -1;
#else
    if (m811_require(wm_72238_stage_object_matrix()) != 0 ||
        m811_require(wm_72238_stage_gpu_asset_a()) != 0)
        return -1;
#endif
    if (m811_require(wm_72238_stage_gpu_asset_b()) != 0 ||
        m811_require(wm_72238_stage_bss_constants()) != 0 ||
        m811_require(wm_72238_stage_record_clut()) != 0 ||
        m811_require(wm_72238_stage_heap_table()) != 0 ||
        m811_require(wm_72238_stage_upload_a()) != 0 ||
        m811_require(wm_72238_stage_upload_b()) != 0 ||
        m811_require(wm_72238_stage_draw_packets()) != 0 ||
        m811_require(wm_72238_stage_88f64()) != 0)
        return -1;

    (void)ArchiveSetIndex(36, 0);
    wm_80097BC0(WM_M811_POSITION);
    do {
        (void)wm_800967E4();
        (void)Vsync(0);
#if defined(W34N57_MUTANT_DRAIN_GE_TWO)
    } while (wm_80096668_circular_distance() >= 2u);
#else
    } while (wm_80096668_circular_distance() > 0u);
#endif

    wm_pool_register(0x800923A8u, 0x800925A0u);
#if defined(W34N57_MUTANT_WRONG_REGISTER_PAIR)
    wm_pool_register(0x8007756Cu, 0x80087734u);
#else
    wm_pool_register(0x8007756Cu, 0x800776E0u);
#endif
    wm_pool_register(0x80087710u, 0x80087734u);
    wm_pool_register(0x80071A50u, 0x80071A58u);

    wm_800978FC();
    wm_8008901C();
    wm_800865A0();
    wm_80085FE0();
    wm_80075228();
    wm_80089160(14u, 0u, 0u);
    return 0;
}

void wm_80086568(void)
{
    m811_free_value(WM_M811_CEB4);
    m811_free_value(WM_M811_D150);
}

void wm_80074F04(void)
{
    m811_free_value(WM_M811_D780);
}

void wm_800750DC(void)
{
    m811_free_value(WM_M811_D7D0);
}

void wm_80088FF4(void)
{
    m811_free_value(WM_M811_BDF4);
}

void wm_800976A0(void)
{
    m811_free_value(WM_M811_BE24);
}

void wm_80077480(void)
{
    wm_80084818();
    wm_80086124();
    wm_80086568();
    wm_800866C8();
    wm_80074F04();
    wm_800750DC();
    wm_80088FF4();
    wm_80089128();
    wm_80097D64();

    m811_free_value(WM_M811_BC38);
    m811_free_value(WM_M811_BCB0);
    m811_free_value(WM_M811_BC3C);
    m811_free_value(WM_M811_BCB4);
#if !defined(W34N57_MUTANT_SKIP_C180_FREE)
    m811_free_value(WM_M811_C180);
#endif
    wm_800976A0();

#if defined(W34N57_MUTANT_WRONG_TEARDOWN_STATE)
    wm_gs_sh(WM_M811_F94E, 16u);
#else
    wm_gs_sh(WM_M811_F94E, 17u);
#endif
    wm_gs_sh(WM_M811_F954, 7u);
    m811_sw(WM_M811_BBC4, 1u);
    wm_gs_sh(WM_M811_F950, m811_lhu(WM_M811_BD3A));
}

/* Retail mode-9 lifecycle [0x80077A64,0x80077DC8). */
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
#include "world_map_session_setup_72238.h"
#include "world_map_teardown_7299c.h"
#include "world_map_terrain_init.h"

typedef struct WmMode9Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMode9Rect;

extern int MoveImage(void* rect, int x, int y);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern void ArchiveCdDataSync(int mode);
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern int ArchiveReadFileToBuffer(int entry_index, void* destination,
                                   unsigned int offset, unsigned int flags);
extern void* HeapAlloc(unsigned int size, unsigned int flags);
extern unsigned int HeapFree(void* pointer);
extern void SoundAddSedsEntry(void* seds);
extern void func_80039FF8(void);
extern void func_8003852C(void* seds);
extern u32 wm_80096668_circular_distance(void);
extern void* D_8006259C;

#define WM_M9L_TEMPLATE_SRC 0x8009A180u
#define WM_M9L_TEMPLATE_DST 0x8009BE4Cu
#define WM_M9L_CCA4         0x8009CCA4u
#define WM_M9L_D3CC         0x8009D3CCu
#define WM_M9L_D804         0x8009D804u
#define WM_M9L_D144         0x8009D144u
#define WM_M9L_CD40         0x8009CD40u
#define WM_M9L_POSITION     0x8009C5ACu
#define WM_M9L_SEDS_ID      0x8009D3C8u

#define WM_M9L_BC38         0x8009BC38u
#define WM_M9L_BCB0         0x8009BCB0u
#define WM_M9L_BC3C         0x8009BC3Cu
#define WM_M9L_BCB4         0x8009BCB4u
#define WM_M9L_C180         0x8009C180u
#define WM_M9L_CEB4         0x8009CEB4u
#define WM_M9L_D150         0x8009D150u
#define WM_M9L_D780         0x8009D780u
#define WM_M9L_D7D0         0x8009D7D0u
#define WM_M9L_BDF4         0x8009BDF4u
#define WM_M9L_BE24         0x8009BE24u

#define WM_M9L_BD3A         0x8009BD3Au
#define WM_M9L_F94E         0x8006F94Eu
#define WM_M9L_F950         0x8006F950u
#define WM_M9L_F954         0x8006F954u
#define WM_M9L_BBC4         0x8009BBC4u

static u32 m9l_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m9l_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m9l_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

__attribute__((unused)) static void m9l_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static int m9l_require(int result)
{
    return result == 0 ? 0 : -1;
}

static void* m9l_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

static void m9l_free_value(u32 address)
{
    (void)HeapFree(m9l_to_host(m9l_lw(address)));
}

int wm_800721E4(void)
{
    u32 archive_id = m9l_lw(WM_M9L_SEDS_ID);
    int decoded_size = ArchiveDecodeAlignedSize(archive_id);
    void* destination;

    if (decoded_size <= 0)
        return -1;
    destination = HeapAlloc((unsigned int)decoded_size, 0u);
    D_8006259C = destination;
    if (destination == NULL)
        return -1;
    (void)ArchiveReadFileToBuffer((int)archive_id, destination, 0u, 0u);
    return 0;
}

int wm_80077A64(void)
{
    WmMode9Rect source_rect;

    wm_80072BB0();
    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.w = 320;
    source_rect.h = 216;
    (void)MoveImage(&source_rect, 704, 256);
    (void)DrawSync(0);
#if defined(W34N61_MUTANT_WRONG_TRANSITION_ARG)
    if (m9l_require(wm_80072DB4(64, 0, 4, 2)) != 0)
#else
    if (m9l_require(wm_80072DB4(64, 0, 4, 1)) != 0)
#endif
        return -1;
    if (m9l_require(wm_mode9_stage_second_wave_finish()) != 0 ||
        m9l_require(wm_72238_stage_object_pool()) != 0)
        return -1;

    memcpy(PSX_ADDR(WM_M9L_TEMPLATE_DST),
           PSX_ADDR(WM_M9L_TEMPLATE_SRC), 32u);
#if defined(W34N61_MUTANT_WRONG_MODE_CONSTANT)
    m9l_sw(WM_M9L_CCA4, 2u);
#else
    m9l_sw(WM_M9L_CCA4, 1u);
#endif
    m9l_sw(WM_M9L_D3CC, 4u);
    m9l_sw(WM_M9L_D804, 0u);
    m9l_sw(WM_M9L_D144, 0u);
    m9l_sw(WM_M9L_CD40, 0x80086700u);

    if (m9l_require(wm_72238_stage_cross_products()) != 0)
        return -1;
    ArchiveCdDataSync(0);
    if (m9l_require(wm_800721E4()) != 0)
        return -1;

    m9l_sw(WM_M9L_POSITION + 0u, 0x02000000u);
    m9l_sw(WM_M9L_POSITION + 4u, 0xFFE00000u);
    m9l_sw(WM_M9L_POSITION + 8u, 0x02000000u);

    if (m9l_require(wm_72238_stage_object_matrix()) != 0 ||
        m9l_require(wm_72238_stage_gpu_asset_a()) != 0 ||
        m9l_require(wm_72238_stage_gpu_asset_b()) != 0 ||
        m9l_require(wm_72238_stage_bss_constants()) != 0 ||
        m9l_require(wm_72238_stage_heap_table()) != 0 ||
        m9l_require(wm_72238_stage_upload_a()) != 0 ||
        m9l_require(wm_72238_stage_upload_b()) != 0 ||
        m9l_require(wm_72238_stage_draw_packets()) != 0 ||
        m9l_require(wm_72238_stage_88f64()) != 0)
        return -1;

    ArchiveCdDataSync(0);
#if !defined(W34N61_MUTANT_SKIP_SOUND_LINK)
    SoundAddSedsEntry(D_8006259C);
#endif
    (void)ArchiveSetIndex(36, 0);
    wm_80097BC0(WM_M9L_POSITION);
    do {
        (void)wm_800967E4();
        (void)Vsync(0);
    } while (wm_80096668_circular_distance() > 0u);

    wm_pool_register(0x800923A8u, 0x800925A0u);
    wm_pool_register(0x80077DC8u, 0x80077E68u);
    wm_pool_register(0x8007828Cu, 0x800783E8u);
#if defined(W34N61_MUTANT_WRONG_REGISTER_PAIR)
    wm_pool_register(0x8007756Cu, 0x800776E0u);
#else
    wm_pool_register(0x80078948u, 0x80078950u);
#endif

    wm_800978FC();
    wm_8008901C();
    wm_800865A0();
    return 0;
}

void wm_80077CC0(void)
{
    func_80039FF8();
    func_8003852C(D_8006259C);
#if !defined(W34N61_MUTANT_SKIP_SEDS_FREE)
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

    m9l_free_value(WM_M9L_BC38);
    m9l_free_value(WM_M9L_BCB0);
    m9l_free_value(WM_M9L_BC3C);
    m9l_free_value(WM_M9L_BCB4);
    m9l_free_value(WM_M9L_C180);
    wm_800976A0();

#if defined(W34N61_MUTANT_WRONG_TEARDOWN_STATE)
    wm_gs_sh(WM_M9L_F94E, 17u);
#else
    wm_gs_sh(WM_M9L_F94E, 270u);
#endif
    wm_gs_sh(WM_M9L_F954, 0u);
    m9l_sw(WM_M9L_BBC4, 1u);
    wm_gs_sh(WM_M9L_F950, m9l_lhu(WM_M9L_BD3A));
}

/* Shared retail mode-13 draw callbacks [0x80076A14,0x80076B34). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_76a14.h"
#include "world_map_helper_737ec.h"
#include "world_map_helper_73b04.h"
#include "world_map_helper_848f4.h"
#include "world_map_helper_8615c.h"
#include "world_map_helper_86798.h"
#include "world_map_helper_89748.h"
#include "world_map_helper_89c78.h"
#include "world_map_helper_96130.h"
#include "world_map_helper_97244.h"
#include "world_map_helper_980d4.h"
#include "world_map_helper_981c8.h"
#include "world_map_helper_983a0.h"
#include "world_map_helper_98cc0.h"
#include "world_map_helper_9932c.h"

#define WM_76A1C_RENDER_POSITION  0x8009BBB4u
#define WM_76A1C_CAMERA_INPUT     0x8009BD40u
#define WM_76A1C_TERRAIN_POSITION 0x8009BE28u
#define WM_76A1C_CONTEXT_PTR      0x8009BE3Cu
#define WM_76A1C_PALETTE_INDEX    0x8009C5BCu
#define WM_76A1C_CAMERA_KIND      0x8009D144u
#define WM_76A1C_PAGING_FLAGS     0x8009D558u

static u16 m76a1c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m76a1c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m76a1c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

s32 wm_80076A1C(s32 slot_index)
{
    u32 context;

    (void)slot_index;

#if defined(W34N94_MUTANT_SWAP_CAMERA_BRANCH)
    if (m76a1c_lw(WM_76A1C_CAMERA_KIND) == 0u)
        wm_80097244(WM_76A1C_CAMERA_INPUT);
    else
        wm_80097440(WM_76A1C_CAMERA_INPUT);
#else
    if (m76a1c_lw(WM_76A1C_CAMERA_KIND) == 0u)
        wm_80097440(WM_76A1C_CAMERA_INPUT);
    else
        wm_80097244(WM_76A1C_CAMERA_INPUT);
#endif

    wm_80089748();
    wm_80089C78(WM_76A1C_RENDER_POSITION);
#if !defined(W34N94_MUTANT_SKIP_VERTEX_SETUP)
    wm_8008615C();
#endif
    wm_800848F4();
#if !defined(W34N94_MUTANT_SKIP_POSITION_WRAP)
    wm_800980D4(WM_76A1C_RENDER_POSITION);
#endif

    if (m76a1c_lhu(WM_76A1C_PAGING_FLAGS) != 0u) {
#if !defined(W34N94_MUTANT_SKIP_PAGING_CHAIN)
        wm_800981C8(WM_76A1C_TERRAIN_POSITION);
        wm_80096130();
        wm_80098CC0();
#endif
    }

#if defined(W34N94_MUTANT_WRONG_TERRAIN_POSITION)
    wm_800983A0(WM_76A1C_RENDER_POSITION);
#else
    wm_800983A0(WM_76A1C_TERRAIN_POSITION);
#endif

    context = m76a1c_lw(WM_76A1C_CONTEXT_PTR);
    wm_8009932C(m76a1c_lw(context + 0x70u),
                m76a1c_lw(context + 0x74u),
                WM_76A1C_TERRAIN_POSITION);

#if defined(W34N94_MUTANT_WRONG_PALETTE_STEP)
    m76a1c_sw(WM_76A1C_PALETTE_INDEX,
              m76a1c_lw(WM_76A1C_PALETTE_INDEX) + 0x20u);
#else
    m76a1c_sw(WM_76A1C_PALETTE_INDEX,
              m76a1c_lw(WM_76A1C_PALETTE_INDEX) + 0x40u);
#endif

#if !defined(W34N94_MUTANT_SKIP_TERRAIN_PACKETS)
    wm_80073B04();
#endif
    wm_800737EC();
#if !defined(W34N94_MUTANT_SKIP_FINAL_DRAW)
    wm_80086798();
#endif
    return 1;
}

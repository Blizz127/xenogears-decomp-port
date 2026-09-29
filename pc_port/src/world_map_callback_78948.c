/* Shared retail mode scheduler callbacks [0x80078948,0x80078A60). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_78948.h"
#include "world_map_helper_737ec.h"
#include "world_map_helper_73b04.h"
#include "world_map_helper_848f4.h"
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

#define WM_78950_RENDER_POSITION 0x8009BBB4u
#define WM_78950_CAMERA_INPUT    0x8009BD40u
#define WM_78950_TERRAIN_POS     0x8009BE28u
#define WM_78950_CONTEXT_PTR     0x8009BE3Cu
#define WM_78950_PALETTE_INDEX   0x8009C5BCu
#define WM_78950_CAMERA_KIND     0x8009D144u
#define WM_78950_PAGING_FLAGS    0x8009D558u

static u16 m78950_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m78950_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m78950_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

s32 wm_80078950(s32 slot_index)
{
    u32 context;

    (void)slot_index;

#if defined(W34N58_MUTANT_SWAP_CAMERA_BRANCH)
    if (m78950_lw(WM_78950_CAMERA_KIND) == 0u)
        wm_80097244(WM_78950_CAMERA_INPUT);
    else
        wm_80097440(WM_78950_CAMERA_INPUT);
#else
    if (m78950_lw(WM_78950_CAMERA_KIND) == 0u)
        wm_80097440(WM_78950_CAMERA_INPUT);
    else
        wm_80097244(WM_78950_CAMERA_INPUT);
#endif

    wm_80089748();
    wm_80089C78(WM_78950_RENDER_POSITION);
    wm_800848F4();
#if !defined(W34N58_MUTANT_SKIP_POSITION_WRAP)
    wm_800980D4(WM_78950_RENDER_POSITION);
#endif

    if (m78950_lhu(WM_78950_PAGING_FLAGS) != 0u) {
#if !defined(W34N58_MUTANT_SKIP_PAGING_CHAIN)
        wm_800981C8(WM_78950_TERRAIN_POS);
        wm_80096130();
        wm_80098CC0();
#endif
    }

#if defined(W34N58_MUTANT_WRONG_TERRAIN_POSITION)
    wm_800983A0(WM_78950_RENDER_POSITION);
#else
    wm_800983A0(WM_78950_TERRAIN_POS);
#endif

    context = m78950_lw(WM_78950_CONTEXT_PTR);
    wm_8009932C(m78950_lw(context + 0x70u),
                m78950_lw(context + 0x74u),
                WM_78950_TERRAIN_POS);

#if defined(W34N58_MUTANT_WRONG_PALETTE_STEP)
    m78950_sw(WM_78950_PALETTE_INDEX,
              m78950_lw(WM_78950_PALETTE_INDEX) + 0x20u);
#else
    m78950_sw(WM_78950_PALETTE_INDEX,
              m78950_lw(WM_78950_PALETTE_INDEX) + 0x40u);
#endif

    wm_80073B04();
    wm_800737EC();
    wm_80086798();
    return 1;
}

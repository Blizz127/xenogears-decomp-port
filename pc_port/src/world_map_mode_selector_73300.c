/* Retail world submode selector [0x80073300, 0x80073398). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_mode_selector_73300.h"

#define WM_73300_CONTROL UINT32_C(0x8006EE68)
#define WM_73300_FLAGS   UINT32_C(0x8006F8E5)
#define WM_73300_MODE    UINT32_C(0x8009BE10)

static u8 wm_73300_lbu(u32 address)
{
    return *(u8*)PSX_ADDR(address);
}

static u16 wm_73300_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_73300_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_80073300(void)
{
    u16 control = wm_73300_lhu(WM_73300_CONTROL);
    u32 mode;

    if ((control & UINT16_C(0x4000)) != 0u) {
#if defined(W34N119_MUTANT_UNMASKED_INDEX)
        u16 index = control;
#else
        u16 index = (u16)(control & UINT16_C(0x1FFF));
#endif
        switch (index) {
        case 0u:
#if defined(W34N119_MUTANT_INDEX0_WRITES)
            wm_73300_sw(WM_73300_MODE, UINT32_C(1));
#endif
            return;
        case 1u:
            mode = UINT32_C(4);
            break;
        case 2u:
            mode = UINT32_C(5);
            break;
        case 3u:
#if defined(W34N119_MUTANT_INDEX3_MODE1)
            mode = UINT32_C(1);
#else
            mode = UINT32_C(7);
#endif
            break;
        case 4u:
            mode = UINT32_C(7);
            break;
        default:
#if defined(W34N119_MUTANT_OOB_WRITES)
            wm_73300_sw(WM_73300_MODE, UINT32_C(7));
#endif
            return;
        }
    } else {
        u8 combined = (u8)(wm_73300_lbu(WM_73300_FLAGS) |
                           wm_73300_lbu(WM_73300_FLAGS + 1u) |
                           wm_73300_lbu(WM_73300_FLAGS + 2u));
#if defined(W34N119_MUTANT_INVERT_FLAG_MODE)
        mode = combined != 0u ? UINT32_C(1) : UINT32_C(2);
#else
        mode = combined != 0u ? UINT32_C(2) : UINT32_C(1);
#endif
    }

    wm_73300_sw(WM_73300_MODE, mode);
}

/* Exact native transcription of retail [0x800834D0, 0x8008355C). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_834d0.h"

extern int DrawSync(int mode);
extern int Vsync(int mode);
extern void wm_80097D64(void);
extern void wm_80097BC0(u32 position);
extern u32 wm_800967E4(void);
extern u32 wm_80096668_circular_distance(void);

#define WM_M17T_POOL_PTR UINT32_C(0x8009BE24)
#define WM_M17T_POSITION UINT32_C(0x8009C5AC)

static s16 m17t_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m17t_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

#if !defined(W34N110_MUTANT_KEEP_LATCH)
static void m17t_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}
#endif

static u32 m17t_slot(s32 slot_index)
{
    return m17t_lw(WM_M17T_POOL_PTR) + ((u32)slot_index << 7u);
}

/* 0x800834D0 (init) is the matched C func_800834D0 (src/world_map/main.c). */

s32 wm_800834D8(s32 slot_index)
{
    u32 slot = m17t_slot(slot_index);

#if defined(W34N110_MUTANT_WRONG_LATCH_VALUE)
    if (m17t_lh(slot + 0x04u) == 2) {
#else
    if (m17t_lh(slot + 0x04u) == 1) {
#endif
#if !defined(W34N110_MUTANT_KEEP_LATCH)
        m17t_sh(slot + 0x04u, 0u);
#endif
        (void)DrawSync(0);
        (void)Vsync(0);
#if !defined(W34N110_MUTANT_SKIP_RESET)
        wm_80097D64();
#endif
#if defined(W34N110_MUTANT_WRONG_POSITION)
        wm_80097BC0(WM_M17T_POSITION + 4u);
#else
        wm_80097BC0(WM_M17T_POSITION);
#endif
        do {
            (void)wm_800967E4();
            (void)Vsync(0);
#if defined(W34N110_MUTANT_SINGLE_DRAIN)
            break;
#endif
        } while (wm_80096668_circular_distance() > 0u);
    }
    return 1;
}

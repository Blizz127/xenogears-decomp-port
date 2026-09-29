/* Retail world-map helper [0x8007565C, 0x800758C0). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_7565c.h"

#define WM_7565C_POOL_PTR 0x8009BE24u

/* This buffer is a compiled native authority in the PC port.  Field code
 * consumes the same symbol directly; it is not the guest-RAM byte range with
 * the matching retail address. */
extern u8 D_8005A4E4[];

#if defined(WM_7565C_TEST_HOOKS)
extern void wm_7565c_test_write(u32 destination, u32 source_offset,
                               u32 size);
#endif

static u32 wm56_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm56_restore_h(u32 destination, u32 source_offset)
{
    u16 value;
#if defined(W34N22_MUTANT_GUEST_SOURCE)
    memcpy(&value, PSX_ADDR(0x8005A4E4u + source_offset), sizeof(value));
#else
    memcpy(&value, D_8005A4E4 + source_offset, sizeof(value));
#endif
#if defined(WM_7565C_TEST_HOOKS)
    wm_7565c_test_write(destination, source_offset, 2u);
#endif
    memcpy(PSX_ADDR(destination), &value, sizeof(value));
}

static void wm56_restore_w(u32 destination, u32 source_offset)
{
    u32 value;
#if defined(W34N22_MUTANT_GUEST_SOURCE)
    memcpy(&value, PSX_ADDR(0x8005A4E4u + source_offset), sizeof(value));
#else
    memcpy(&value, D_8005A4E4 + source_offset, sizeof(value));
#endif
#if defined(WM_7565C_TEST_HOOKS)
    wm_7565c_test_write(destination, source_offset, 4u);
#endif
    memcpy(PSX_ADDR(destination), &value, sizeof(value));
}

static void wm56_copy(u32 destination, u32 source_offset, u32 size)
{
#if defined(WM_7565C_TEST_HOOKS)
    wm_7565c_test_write(destination, source_offset, size);
#endif
#if defined(W34N22_MUTANT_GUEST_SOURCE)
    memcpy(PSX_ADDR(destination), PSX_ADDR(0x8005A4E4u + source_offset),
           (size_t)size);
#else
    memcpy(PSX_ADDR(destination), D_8005A4E4 + source_offset, (size_t)size);
#endif
}

void wm_8007565C(void)
{
    u32 pool = wm56_lw(WM_7565C_POOL_PTR);

#if defined(W34N22_MUTANT_SHORT_POOL_COPY)
    wm56_copy(pool, 0u, 0x1F80u);
#else
    wm56_copy(pool, 0u, 0x2000u);
#endif

    /* Retail publishes the same four-word position record to both the shared
     * terrain position and the active world pose. */
    wm56_copy(0x8009C5ACu, 0x2000u, 0x10u);
#if !defined(W34N22_MUTANT_SKIP_POSE_COPY)
    wm56_copy(0x8009D55Cu, 0x2000u, 0x10u);
#endif

#if !defined(W34N22_MUTANT_SKIP_TIMERS)
    wm56_copy(0x8009C854u, 0x2020u, 0x20u);
#endif
    wm56_restore_h(0x8009D52Cu, 0x2010u);
    wm56_restore_w(0x8009BE40u, 0x2014u);
    wm56_restore_w(0x8009BCC4u, 0x2018u);
    wm56_restore_w(0x8009D64Cu, 0x201Cu);

#if defined(W34N22_MUTANT_SHORT_RING_COPY)
    wm56_copy(0x8009CEC4u, 0x2040u, 0x270u);
#else
    wm56_copy(0x8009CEC4u, 0x2040u, 0x280u);
#endif

    wm56_copy(0x8009BD38u, 0x22C4u, 0x08u);
    wm56_copy(0x8009BBB4u, 0x22D4u, 0x10u);
    wm56_copy(0x8009C838u, 0x22E4u, 0x08u);
#if !defined(W34N22_MUTANT_SKIP_CAMERA_POSITION)
    wm56_copy(0x8009BE28u, 0x22ECu, 0x10u);
#endif
#if defined(W34N22_MUTANT_SWAP_FINAL_STORES)
    wm56_restore_w(0x8009D3F0u, 0x22CCu);
    wm56_restore_h(0x8009D154u, 0x22C0u);
#else
    wm56_restore_h(0x8009D154u, 0x22C0u);
    wm56_restore_w(0x8009D3F0u, 0x22CCu);
#endif
    wm56_restore_w(0x8009BE0Cu, 0x22D0u);
}

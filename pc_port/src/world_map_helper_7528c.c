/*
 * World-map randomized timer-bank helper 0x8007528C.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x8007528C, 0x80075460).  This is a shared forward helper, not a
 * scheduler callback, and intentionally has no callback registration.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_7528c.h"

#define WM_7528C_TIMERS       0x8009C854u
#define WM_7528C_TIMER_COUNT  0x8009BCC4u
#define WM_7528C_TIMER_RANGE  0x8009BE40u
#define WM_7528C_COUNTDOWN    0x8009D64Cu
#define WM_7528C_EXPIRED      0x8009D80Cu

extern int rand(void);

static s32 wm_7528c_load_s32(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 wm_7528c_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

#if !defined(WM_7528C_MUTANT_M6)
static s16 wm_7528c_load_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}
#endif

static void wm_7528c_store_s32(u32 address, s32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_7528c_store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_8007528C(void)
{
    s32 countdown = wm_7528c_load_s32(WM_7528C_COUNTDOWN);
    s32 count;
    s32 index;

#if !defined(WM_7528C_MUTANT_M1)
    countdown = (s32)((u32)countdown - 1u);
#endif
    wm_7528c_store_s32(WM_7528C_COUNTDOWN, countdown);

#if defined(WM_7528C_MUTANT_M2)
    if (countdown == 1) {
#else
    if (countdown == 0) {
#endif
        count = wm_7528c_load_s32(WM_7528C_TIMER_COUNT);
#if defined(WM_7528C_MUTANT_M13)
        if (count > 0)
            count--;
#endif
        for (index = 0; index < count; index++) {
            s32 candidate;
#if !defined(WM_7528C_MUTANT_M6)
            s32 prior;
#endif
            int duplicate;

            do {
                s32 range = wm_7528c_load_s32(WM_7528C_TIMER_RANGE);
                s32 random_value = (s32)rand();
#if defined(WM_7528C_MUTANT_M14)
                (void)rand();
#endif
#if defined(WM_7528C_MUTANT_M4)
                candidate = random_value % range;
#elif defined(WM_7528C_MUTANT_M5)
                candidate = random_value % (range - 1) + 1;
#else
                candidate = random_value % range + 1;
#endif
                duplicate = 0;
#if !defined(WM_7528C_MUTANT_M6)
                for (prior = 0; prior < index; prior++) {
#if defined(WM_7528C_MUTANT_M7)
                    if (prior + 1 == index)
                        continue;
#endif
                    if ((s32)wm_7528c_load_s16(
                            WM_7528C_TIMERS + (u32)prior * 2u) == candidate) {
                        duplicate = 1;
                        break;
                    }
                }
#endif
            } while (duplicate != 0);

#if defined(WM_7528C_MUTANT_M8)
            wm_7528c_store_u16(WM_7528C_TIMERS + (u32)index * 4u,
                               (u16)candidate);
#else
            wm_7528c_store_u16(WM_7528C_TIMERS + (u32)index * 2u,
                               (u16)candidate);
#endif
        }

#if defined(WM_7528C_MUTANT_M3)
        wm_7528c_store_s32(WM_7528C_COUNTDOWN,
                           wm_7528c_load_s32(WM_7528C_TIMER_COUNT));
#else
        wm_7528c_store_s32(WM_7528C_COUNTDOWN,
                           wm_7528c_load_s32(WM_7528C_TIMER_RANGE));
#endif
    }

    count = wm_7528c_load_s32(WM_7528C_TIMER_COUNT);
#if !defined(WM_7528C_MUTANT_M12)
    wm_7528c_store_s32(WM_7528C_EXPIRED, 0);
#endif
    for (index = 0; index < count; index++) {
        u32 address = WM_7528C_TIMERS + (u32)index * 2u;
        u16 timer = wm_7528c_load_u16(address);

#if defined(WM_7528C_MUTANT_M10)
        if (timer != 0u)
            timer = (u16)(timer - 1u);
#elif !defined(WM_7528C_MUTANT_M9)
        timer = (u16)(timer - 1u);
#endif
        wm_7528c_store_u16(address, timer);
        if (timer == 0u) {
#if !defined(WM_7528C_MUTANT_M11)
            s32 expired = wm_7528c_load_s32(WM_7528C_EXPIRED);
            wm_7528c_store_s32(WM_7528C_EXPIRED,
                               (s32)((u32)expired + 1u));
#endif
        }
    }
}

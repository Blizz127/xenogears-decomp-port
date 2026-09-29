/* Exact native transcriptions of three retail paired-free leaves. */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_86124.h"

extern unsigned int HeapFree(void* ptr);

#define WM_D7E8 0x8009D7E8u
#define WM_D7EC 0x8009D7ECu
#define WM_D7F8 0x8009D7F8u
#define WM_D7FC 0x8009D7FCu
#define WM_BE1C 0x8009BE1Cu
#define WM_BE20 0x8009BE20u

static u32 wm_86124_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

/* Allocations owned by these globals are stored as guest KSEG values by the
 * port's world allocators.  Keep the established low-native compatibility
 * case for allocations originating in older compiled system translation
 * units. */
static void* wm_86124_pointer_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

static void wm_86124_free_global(u32 address)
{
    (void)HeapFree(wm_86124_pointer_to_host(wm_86124_lw(address)));
}

/* Retail [0x80086124,0x8008615C): 14 instructions. */
void wm_80086124(void)
{
#if defined(W34N15_MUTANT_SWAP_86124)
    wm_86124_free_global(WM_D7E8);
    wm_86124_free_global(WM_D7EC);
#elif defined(W34N15_MUTANT_WRONG_86124_SECOND)
    wm_86124_free_global(WM_D7EC);
    wm_86124_free_global(WM_D7EC);
#else
    wm_86124_free_global(WM_D7EC);
    wm_86124_free_global(WM_D7E8);
#endif
}

/* Retail [0x800866C8,0x80086700): 14 instructions. */
void wm_800866C8(void)
{
#if defined(W34N15_MUTANT_SWAP_866C8)
    wm_86124_free_global(WM_D7F8);
    wm_86124_free_global(WM_D7FC);
#elif defined(W34N15_MUTANT_WRONG_866C8_SECOND)
    wm_86124_free_global(WM_D7FC);
    wm_86124_free_global(WM_D7FC);
#else
    wm_86124_free_global(WM_D7FC);
    wm_86124_free_global(WM_D7F8);
#endif
}

/* Retail [0x80089128,0x80089160): 14 instructions. */
void wm_80089128(void)
{
#if defined(W34N15_MUTANT_SWAP_89128)
    wm_86124_free_global(WM_BE20);
    wm_86124_free_global(WM_BE1C);
#elif defined(W34N15_MUTANT_WRONG_89128_SECOND)
    wm_86124_free_global(WM_BE1C);
    wm_86124_free_global(WM_BE1C);
#else
    wm_86124_free_global(WM_BE1C);
    wm_86124_free_global(WM_BE20);
#endif
}

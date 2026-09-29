/* Exact native transcription of retail 0x80071FEC..0x80072090. */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_71fec.h"

extern void* D_8005945C;
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern void* HeapAlloc(u_int size, u_int flags);
extern int func_80029AFC(void* entries, int arg1, int arg2);

#define WM_REQUEST_BASE 0x8009D3F8u
#define WM_SECOND_MIRROR 0x8009D528u

static void wm_71fec_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_71fec_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_80071FEC(void)
{
    void* first;
    void* second;
    u32 first_guest;
    u32 second_guest;
    u32 first_size;
    u32 second_size;

#if defined(W34N16_MUTANT_SWAP_DECODE_ORDER)
    first_size = (u32)ArchiveDecodeAlignedSize(0x25u);
#else
    first_size = (u32)ArchiveDecodeAlignedSize(0x26u);
#endif
    first = HeapAlloc(first_size, 1u);

    /* D_8005945C is a native pointer authority used by the compiled menu TUs. */
#if defined(W34N16_MUTANT_GUEST_MENU_AUTHORITY)
    D_8005945C = (void*)(uintptr_t)PsxMemory_GuestAddr(first);
#else
    D_8005945C = first;
#endif

#if defined(W34N16_MUTANT_SWAP_DECODE_ORDER)
    second_size = (u32)ArchiveDecodeAlignedSize(0x26u);
#else
    second_size = (u32)ArchiveDecodeAlignedSize(0x25u);
#endif
#if defined(W34N16_MUTANT_WRONG_ALLOC_FLAG)
    second = HeapAlloc(second_size, 0u);
#else
    second = HeapAlloc(second_size, 1u);
#endif

    first_guest = PsxMemory_GuestAddr(first);
#if defined(W34N16_MUTANT_RAW_SECOND_POINTER)
    second_guest = (u32)(uintptr_t)second;
#else
    second_guest = PsxMemory_GuestAddr(second);
#endif

    wm_71fec_sw(WM_SECOND_MIRROR, second_guest);
    wm_71fec_sw(WM_REQUEST_BASE + 4u, second_guest);
    wm_71fec_sh(WM_REQUEST_BASE + 0u, 0x25u);
    wm_71fec_sh(WM_REQUEST_BASE + 8u, 0x26u);
#if defined(W34N16_MUTANT_SWAP_QUEUE_PAYLOADS)
    (void)first_guest;
    wm_71fec_sw(WM_REQUEST_BASE + 12u, second_guest);
#else
    wm_71fec_sw(WM_REQUEST_BASE + 12u, first_guest);
#endif
#if defined(W34N16_MUTANT_MISSING_TERMINATOR)
    wm_71fec_sh(WM_REQUEST_BASE + 16u, 1u);
#else
    wm_71fec_sh(WM_REQUEST_BASE + 16u, 0u);
#endif
    wm_71fec_sw(WM_REQUEST_BASE + 20u, 0u);

#if defined(W34N16_MUTANT_WRONG_QUEUE_BASE)
    (void)func_80029AFC(PSX_ADDR(WM_REQUEST_BASE + 8u), 0, 0);
#else
    (void)func_80029AFC(PSX_ADDR(WM_REQUEST_BASE), 0, 0);
#endif
}

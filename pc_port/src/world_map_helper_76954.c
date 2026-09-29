/* Retail mode-9 second-wave relocation helper [0x80076954,0x80076A14). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_76954.h"

extern void* LZSSHeapDecompress(void* source, int flags);
extern unsigned int HeapFree(void* pointer);

#define WM_76954_C180 0x8009C180u
#define WM_76954_CD48 0x8009CD48u
#define WM_76954_BD30 0x8009BD30u
#define WM_76954_D308 0x8009D308u
#define WM_76954_BCC0 0x8009BCC0u
#define WM_76954_C7EC 0x8009C7ECu
#define WM_76954_D77C 0x8009D77Cu
#define WM_76954_D7C8 0x8009D7C8u

static u32 h76954_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void h76954_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 h76954_host_lw(const uint8_t* base, u32 offset)
{
    u32 value;
    memcpy(&value, base + offset, sizeof(value));
    return value;
}

static void* h76954_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

int wm_80076954(void)
{
    u32 compressed_guest = h76954_lw(WM_76954_C180);
    void* compressed = h76954_to_host(compressed_guest);
    void* decoded;
    u32 base;
    const uint8_t* bytes;

    if (compressed == NULL)
        return -1;
    decoded = LZSSHeapDecompress(compressed, 0);
#if defined(W34N61_MUTANT_MISSING_KSEG_PUBLICATION)
    base = PsxMemory_GuestAddr(decoded) & 0x001FFFFFu;
#else
    base = PsxMemory_GuestAddr(decoded);
#endif
    h76954_sw(WM_76954_C180, base);
#if !defined(W34N61_MUTANT_SKIP_COMPRESSED_FREE)
    (void)HeapFree(compressed);
#endif
    if (decoded == NULL || base == 0u)
        return -1;

    bytes = (const uint8_t*)decoded;
    h76954_sw(WM_76954_CD48, base + h76954_host_lw(bytes, 0x08u));
#if defined(W34N61_MUTANT_SWAP_HEADER_OFFSETS)
    h76954_sw(WM_76954_BD30, base + h76954_host_lw(bytes, 0x0Cu));
    h76954_sw(WM_76954_D308, base + h76954_host_lw(bytes, 0x10u));
#else
    h76954_sw(WM_76954_BD30, base + h76954_host_lw(bytes, 0x10u));
    h76954_sw(WM_76954_D308, base + h76954_host_lw(bytes, 0x0Cu));
#endif
    h76954_sw(WM_76954_BCC0, base + h76954_host_lw(bytes, 0x18u));
    h76954_sw(WM_76954_C7EC, base + h76954_host_lw(bytes, 0x14u));
    h76954_sw(WM_76954_D77C, base + h76954_host_lw(bytes, 0x20u));
    h76954_sw(WM_76954_D7C8, base + h76954_host_lw(bytes, 0x24u));
    return 0;
}

/*
 * psx_memory.h - PSX main-RAM emulation for the Xenogears PC port.
 *
 * Modeled on the Silent Hill PC port (pc_port/src/psx_memory.c there). The PS1
 * has 2 MB of RAM at 0x80000000-0x801FFFFF, and the decompiled game hardcodes
 * absolute addresses into it (e.g. the mem/heap fields of g_MainGameStates).
 * Rather than emulate the CPU, we allocate one real buffer and translate a PSX
 * address to a host pointer by masking to the 2 MB range and indexing the buffer.
 */
#ifndef XENO_PSX_MEMORY_H
#define XENO_PSX_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* 2 MB main RAM + 1 MB guard (some buffers near the top of RAM can overrun;
 * the original wraps, on PC the guard prevents corrupting adjacent globals). */
#define PSX_RAM_SIZE (2 * 1024 * 1024 + 1024 * 1024)
extern uint8_t g_PsxRam[];

/* Scratchpad (real PSX is 1 KB; allow headroom). */
extern uint8_t g_PsxScratchpad[];

/* Convert a PSX address (0x80XXXXXX / 0x00XXXXXX) to a host pointer into g_PsxRam. */
#define PSX_ADDR(addr) ((void*)(g_PsxRam + ((uintptr_t)(addr) & 0x1FFFFF)))

void PsxMemory_Init(void);

/* Guest (KUSEG) address of a host pointer that lies inside g_PsxRam; 0 for
 * NULL. HeapAlloc returns host pointers into g_PsxRam, while retail tables
 * store guest addresses that other port code rebases through PSX_ADDR. */
static inline uint32_t PsxMemory_GuestAddr(const void* p)
{
    uintptr_t host = (uintptr_t)p;
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (p == NULL)
        return 0u;
    if (host >= base && host < base + (uintptr_t)PSX_RAM_SIZE)
        return 0x80000000u | (uint32_t)(host - base);
    return (uint32_t)host;
}

/* W34C2 — PS-X static data load path.
 *
 * SLUS_006.64 is a PS-X EXE: 0x800-byte header, then t_size bytes that the
 * PS1 loader copies to t_addr (0x80010000). The port runs natively-translated
 * C and never loaded that image, so every port site that rebases a main-exe
 * data address through PSX_ADDR read zeros. The initialized sections are
 * (config/slus_006.64.yaml subsegments):
 *   rodata [0x80010000,0x80019524)   copied
 *   .text  [0x80019524,0x8004EA90)   NOT copied (no PSX_ADDR consumer reads it)
 *   sdata  [0x8004EA90,0x800592BC)   copied
 *   .data  [0x800592BC,0x800592C0)   copied (4 bytes, all zero in retail)
 *   .sbss  starts at 0x800592C0      NOT copied (zero-filled on the PS1)
 *   ._49AC0 island (ELF 0x80097704)  NOT copied (inside the world overlay's
 *                                    text range; order-dependent corruption)
 *
 * The loader copies the single range [0x8004EA90,0x800592C0), i.e. .sdata plus
 * the 4-byte .data word, and stops exactly at the .sbss boundary. The last
 * non-zero byte of the retail initialized image is 0x800592BB; retail
 * [0x800592C0,0x80059800) is .sbss and is zero. (An earlier revision ended the
 * copy at 0x800576E4, silently dropping 0x1BD8 bytes of real .sdata.)
 */
#define PSX_EXE_HEADER_SIZE   0x800u
#define PSX_EXE_LOAD_BASE     0x80010000u
#define PSX_EXE_RODATA_START  0x80010000u
#define PSX_EXE_RODATA_END    0x80019524u
#define PSX_EXE_TEXT_START    0x80019524u
#define PSX_EXE_TEXT_END      0x8004EA90u
#define PSX_EXE_SDATA_START   0x8004EA90u
/* .sdata end == .data end == .sbss start: config/slus_006.64.yaml places the
 * 4-byte .data subsegment at [0x800592BC,0x800592C0) and .sbss at 0x800592C0. */
#define PSX_EXE_SDATA_END     0x800592C0u
#define PSX_EXE_SBSS_START    0x800592C0u
#define PSX_EXE_IMAGE_END     0x80059800u
#define PSX_EXE_ISLAND_FILE   0x49AC0u
#define PSX_EXE_ISLAND_START  0x80097704u
#define PSX_EXE_ISLAND_END    0x80097C44u

/* Copy the initialized data sections of a PS-X EXE image (whole file, header
 * included) into g_PsxRam. Returns 0 on success, -1 if the image is not a
 * PS-X EXE for load base 0x80010000 or is too short. */
int PsxMemory_LoadStaticDataFromImage(const uint8_t* image, size_t size);

/* Locate SLUS_006.64 (XENO_SLUS, else beside the disc image defaults) and
 * load it. Returns 0 on success; on failure g_PsxRam is left untouched and a
 * diagnostic is printed (the pre-W34C2 zero-filled behaviour). */
int PsxMemory_LoadStaticData(void);

#endif /* XENO_PSX_MEMORY_H */

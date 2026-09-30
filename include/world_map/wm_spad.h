#ifndef WORLD_MAP_WM_SPAD_H
#define WORLD_MAP_WM_SPAD_H

/* PSX scratchpad (0x1F800000, 1 KiB) addresses as the world-map code writes
 * them. The retail build keeps the literal address. The port maps them the
 * way its hand-written world-map bodies do (PSX_ADDR, from psx_memory.h via
 * world_map_port_data.h), so matched and hand-written bodies share one
 * scratchpad image while they call each other; a port-wide scratchpad
 * (g_PsxScratchpad for every 0x1F80xxxx access) is a separate change.
 * Byte-neutral for the matching build. */
#ifdef XENO_PC_PORT
#define WM_SPAD(a) ((void*)PSX_ADDR((unsigned int)(a)))
#else
#define WM_SPAD(a) (a)
#endif

/* Guest-pointer domain seams for world-map bodies the port compiles. Guest
 * RAM stores 32-bit PSX addresses: WM_GPTR turns a loaded guest word into a
 * pointer, WM_GADDR turns a pointer back into the guest address to store.
 * The port gets PSX_ADDR / PsxMemory_GuestAddr from psx_memory.h (pulled in
 * by world_map_port_data.h before any use); retail is a plain cast, so the
 * matching build emits the same lw/sw. */
#ifdef XENO_PC_PORT
#define WM_GPTR(g)  ((void*)PSX_ADDR((u32)(g)))
#define WM_GADDR(p) (PsxMemory_GuestAddr((const void*)(p)))
#else
#define WM_GPTR(g)  ((void*)(g))
#define WM_GADDR(p) ((u32)(p))
#endif

#endif /* WORLD_MAP_WM_SPAD_H */

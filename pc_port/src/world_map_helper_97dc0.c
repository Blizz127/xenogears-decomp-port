/*
 * World-map helper 0x80097DC0 (initial tile slot population).
 * Retail boundary: [0x80097DC0, 0x800980D4), 197 instructions.
 *
 * W34C5 re-derivation from disc/world_map.bin; retail PCs cited inline.
 * For every tile of the 9x9 index window D_8009D570 whose slot in the
 * 256-entry table D_8009C184 is empty: HeapAlloc(0x710) and queue a copy.
 * Ready path (archive sector known): 3x3 centre first, flush, then all 81,
 * flush. Not-ready path: file-path variant for all 81, flush.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_97dc0.h"
#include "world_map_helper_9623c.h"

extern u32 func_8002C3D8(void);
extern void* HeapAlloc(u32 size, u32 flags);
extern int ArchiveDecodeSector(int entryIndex);
extern char* ArchiveGetFilePath(int entryIndex);
extern s32 wm_80096328(void);
extern s32 wm_800965A4(void);

#define D_8009BCD8  0x8009BCD8u  /* archive entry index (0x80097E14 lw)   */
#define D_8009D570  0x8009D570u  /* 9x9 tile index window (0x80097E1C)    */
#define D_8009C184  0x8009C184u  /* 256-entry tile slot table (0x80097E24) */
#define TILE_BYTES  0x710u

static u32 a97_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void a97_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static s16 a97_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

static u32 a97_window_index(s32 row, s32 col)
{
#if defined(WM_97DC0_MUTANT_M3)             /* old shape: (i + j) */
    return (u32)(row + col);
#else
    return (u32)(row * 9 + col);              /* s3 steps by 9: 0x80097EA0/F38 */
#endif
}

static u32 a97_publish(void* buf)
{
#if defined(WM_97DC0_MUTANT_M5)             /* host pointer leak */
    return (u32)(uintptr_t)buf;
#else
    return PsxMemory_GuestAddr(buf);
#endif
}

/* One window tile on the sector path: 0x80097E3C-0x80097E80 / 0x80097ED4-F18. */
static void a97_fill_sector(u32 sector, s32 row, s32 col)
{
    s32 tile = (s32)a97_lh(D_8009D570 + a97_window_index(row, col) * 2u);
    u32 tile_u = (u32)tile;
    u32 slot = D_8009C184 + tile_u * 4u;
    if (a97_lw(slot) == 0u) {
        void* buf = HeapAlloc(TILE_BYTES, 0u);        /* 0x80097E68 */
        u32 guest = a97_publish(buf);
        a97_sw(slot, guest);                          /* 0x80097E70 */
        wm_8009623C(sector + tile_u, TILE_BYTES, guest); /* 0x80097E7C */
    }
}

/* One window tile on the file-path path: 0x80097F88-0x80097FD0. */
static void a97_fill_path(u32 path, s32 row, s32 col)
{
    s32 tile = (s32)a97_lh(D_8009D570 + a97_window_index(row, col) * 2u);
    u32 tile_u = (u32)tile;
    u32 slot = D_8009C184 + tile_u * 4u;
    if (a97_lw(slot) == 0u) {
        void* buf = HeapAlloc(TILE_BYTES, 0u);        /* 0x80097FB4 */
        u32 guest = a97_publish(buf);
        a97_sw(slot, guest);                          /* 0x80097FBC */
        wm_800962B0(path, tile_u << 11, TILE_BYTES, guest); /* 0x80097FCC */
    }
}

void wm_80097DC0(void)
{
    u32 r1 = func_8002C3D8();                 /* 0x80097DE8 */
    u32 r2 = func_8002C3D8();                 /* 0x80097DF0 */
    /* 0x80097DF8-0x80097E04: (r1 == 0) | (~r2 == 0) */
    int ready = (r1 == 0u) || (~r2 == 0u);
    s32 row, col;

    if (ready) {
        u32 sector = (u32)ArchiveDecodeSector((int)a97_lw(D_8009BCD8)); /* 0x80097E28 */

#if !defined(WM_97DC0_MUTANT_M6)            /* 3x3 centre: s3 = 0x1B, rows 3..5 */
        for (row = 3; row < 6; row++)
            for (col = 3; col < 6; col++)
                a97_fill_sector(sector, row, col);
        wm_8009623C(0u, 0u, 0u);              /* 0x80097EAC */
        (void)wm_80096328();                  /* 0x80097EB4 */
#endif
        for (row = 0; row < 9; row++)         /* 0x80097ECC-0x80097F38 */
            for (col = 0; col < 9; col++)
                a97_fill_sector(sector, row, col);
        wm_8009623C(0u, 0u, 0u);              /* 0x80097F44 */
        (void)wm_80096328();                  /* 0x80097F4C */
    } else {
        char* path = ArchiveGetFilePath((int)a97_lw(D_8009BCD8)); /* 0x80097F78 */
        u32 path_guest = PsxMemory_GuestAddr(path);
        for (row = 0; row < 9; row++)         /* 0x80097F84-0x80097FF0 */
            for (col = 0; col < 9; col++)
                a97_fill_path(path_guest, row, col);
        wm_800962B0(0u, 0u, 0u, 0u);          /* 0x80098000 */
        (void)wm_800965A4();                  /* 0x80098008 */
    }
}

/*
 * World-map helper 0x80098CC0 (paged terrain-tile refill).
 * Retail boundary: [0x80098CC0, 0x8009932C), 411 instructions.
 *
 * W34N48: full transcription from disc/world_map.bin. The previous body
 * was a sketch: it scanned the new window as both old and new state and did
 * not reproduce either retail archive path or the four-entry closure pass.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_98cc0.h"
#include "world_map_helper_9623c.h"
#include "world_map_helper_96328.h"
#include "world_map_helper_965a4.h"

extern u32 func_8002C3D8(void);
extern void *HeapAlloc(u32 size, u32 flags);
extern void HeapFree(void *ptr);
extern int ArchiveDecodeSector(int entry_index);
extern char *ArchiveGetFilePath(int entry_index);

#define WM_98CC0_SLOT_TABLE       UINT32_C(0x8009C184)
#define WM_98CC0_SELECT_INDICES   UINT32_C(0x8009BBAC)
#define WM_98CC0_ARCHIVE_PRIMARY  UINT32_C(0x8009BCD8)
#define WM_98CC0_ARCHIVE_SECOND   UINT32_C(0x8009BD08)
#define WM_98CC0_WORLD_WIDTH      UINT32_C(0x8009D160)
#define WM_98CC0_WORLD_HEIGHT     UINT32_C(0x8009D2B4)
#define WM_98CC0_OLD_WINDOW       UINT32_C(0x8009D318)
#define WM_98CC0_NEW_WINDOW       UINT32_C(0x8009D570)

#define WM_98CC0_WINDOW_ENTRIES   UINT32_C(81)
#define WM_98CC0_TILE_BYTES       UINT32_C(0x710)
#define WM_98CC0_PRIMARY_LOADED   UINT32_C(2)
#define WM_98CC0_SECOND_LOADED    UINT32_C(1)

static u32 wm_98cc0_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_98cc0_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_98cc0_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 wm_98cc0_tile_u32(s16 tile)
{
    s32 signed_tile = (s32)tile;
    u32 value;

    memcpy(&value, &signed_tile, sizeof(value));
    return value;
}

static u32 wm_98cc0_slot_address(s16 tile)
{
    return WM_98CC0_SLOT_TABLE + (u32)((s32)tile * 4);
}

static s16 wm_98cc0_window_tile(u32 index)
{
    return wm_98cc0_lh(WM_98CC0_NEW_WINDOW + index * 2u);
}

static u32 wm_98cc0_publish(void *buffer)
{
#if defined(WM_98CC0_MUTANT_M9)
    return (u32)(uintptr_t)buffer;
#else
    return PsxMemory_GuestAddr(buffer);
#endif
}

static u32 wm_98cc0_allocate(s16 tile)
{
    u32 slot = wm_98cc0_slot_address(tile);
    void *buffer = HeapAlloc(WM_98CC0_TILE_BYTES, 0u);
    u32 guest = wm_98cc0_publish(buffer);

    wm_98cc0_sw(slot, guest);
    return guest;
}

/* The secondary archive is transposed relative to the primary one. */
static u32 wm_98cc0_transposed_index(s16 tile)
{
    s32 value = (s32)tile;
#if defined(WM_98CC0_MUTANT_M5)
    return (u32)value;
#else
    s32 width = (s32)wm_98cc0_lw(WM_98CC0_WORLD_WIDTH);
    s32 height = (s32)wm_98cc0_lw(WM_98CC0_WORLD_HEIGHT);
    s32 quotient = value / width;
    s32 remainder = value % width;

    return (u32)(remainder * height + quotient);
#endif
}

static int wm_98cc0_slot_is_empty(s16 tile)
{
    return wm_98cc0_lw(wm_98cc0_slot_address(tile)) == 0u;
}

static void wm_98cc0_evict_old_window(void)
{
    u32 old_index;

    for (old_index = 0u; old_index < WM_98CC0_WINDOW_ENTRIES; old_index++) {
#if defined(WM_98CC0_MUTANT_M1)
        s16 tile = wm_98cc0_window_tile(old_index);
#else
        s16 tile = wm_98cc0_lh(WM_98CC0_OLD_WINDOW + old_index * 2u);
#endif
        u32 slot = wm_98cc0_slot_address(tile);
        u32 value = wm_98cc0_lw(slot);
        u32 new_index;

        if (value == 0u)
            continue;
        for (new_index = 0u; new_index < WM_98CC0_WINDOW_ENTRIES;
             new_index++) {
            if (wm_98cc0_window_tile(new_index) == tile)
                break;
        }
        if (new_index == WM_98CC0_WINDOW_ENTRIES) {
            HeapFree(PSX_ADDR(value));
            wm_98cc0_sw(slot, 0u);
        }
    }
}

static u32 wm_98cc0_primary_sector_edges(u32 sector)
{
    u32 loaded = 0u;
    u32 pass;

    for (pass = 0u; pass < 2u; pass++) {
#if defined(WM_98CC0_MUTANT_M3)
        u32 index = pass == 0u ? 1u : 72u;
#else
        u32 index = pass == 0u ? 1u : 73u;
#endif
        u32 count;
        for (count = 0u; count < 7u; count++, index++) {
            s16 tile = wm_98cc0_window_tile(index);
            if (wm_98cc0_slot_is_empty(tile)) {
                u32 buffer = wm_98cc0_allocate(tile);
                u32 source = sector + wm_98cc0_tile_u32(tile);
                wm_8009623C(source, WM_98CC0_TILE_BYTES, buffer);
                loaded |= WM_98CC0_PRIMARY_LOADED;
            }
        }
    }
    return loaded;
}

static u32 wm_98cc0_second_sector_edges(u32 sector)
{
    u32 loaded = 0u;
    u32 pass;

    for (pass = 0u; pass < 2u; pass++) {
        u32 index = pass == 0u ? 9u : 17u;
        u32 count;
        for (count = 0u; count < 7u; count++, index +=
#if defined(WM_98CC0_MUTANT_M4)
             1u
#else
             9u
#endif
        ) {
            s16 tile = wm_98cc0_window_tile(index);
            if (wm_98cc0_slot_is_empty(tile)) {
                u32 buffer = wm_98cc0_allocate(tile);
                wm_8009623C(sector + wm_98cc0_transposed_index(tile),
                            WM_98CC0_TILE_BYTES, buffer);
                loaded |= WM_98CC0_SECOND_LOADED;
            }
        }
    }
    return loaded;
}

static u32 wm_98cc0_primary_path_edges(u32 path)
{
    u32 loaded = 0u;
    u32 pass;

    for (pass = 0u; pass < 2u; pass++) {
        u32 index = pass == 0u ? 1u : 73u;
        u32 count;
        for (count = 0u; count < 7u; count++, index++) {
            s16 tile = wm_98cc0_window_tile(index);
            if (wm_98cc0_slot_is_empty(tile)) {
                u32 buffer = wm_98cc0_allocate(tile);
                wm_800962B0(path, wm_98cc0_tile_u32(tile) << 11u,
                            WM_98CC0_TILE_BYTES, buffer);
                loaded |= WM_98CC0_PRIMARY_LOADED;
            }
        }
    }
    return loaded;
}

static u32 wm_98cc0_second_path_edges(u32 path)
{
    u32 loaded = 0u;
    u32 pass;

    for (pass = 0u; pass < 2u; pass++) {
        u32 index = pass == 0u ? 9u : 17u;
        u32 count;
        for (count = 0u; count < 7u; count++, index += 9u) {
            s16 tile = wm_98cc0_window_tile(index);
            if (wm_98cc0_slot_is_empty(tile)) {
                u32 buffer = wm_98cc0_allocate(tile);
                u32 source_index = wm_98cc0_transposed_index(tile);
#if defined(WM_98CC0_MUTANT_M8)
                u32 source_offset = source_index;
#else
                u32 source_offset = source_index << 11u;
#endif
                wm_800962B0(path, source_offset, WM_98CC0_TILE_BYTES,
                            buffer);
                loaded |= WM_98CC0_SECOND_LOADED;
            }
        }
    }
    return loaded;
}

static void wm_98cc0_sector_closure(u32 loaded)
{
    u32 select;

#if defined(WM_98CC0_MUTANT_M6)
    (void)loaded;
    return;
#endif
    for (select = 0u; select < 4u; select++) {
        s16 window_index = wm_98cc0_lh(WM_98CC0_SELECT_INDICES + select * 2u);
        s16 tile = wm_98cc0_window_tile((u32)(s32)window_index);
        if (wm_98cc0_slot_is_empty(tile)) {
            u32 buffer = wm_98cc0_allocate(tile);
#if defined(WM_98CC0_MUTANT_M7)
            if ((loaded & WM_98CC0_SECOND_LOADED) != 0u) {
                u32 sector = (u32)ArchiveDecodeSector(
                    (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_SECOND));
                wm_8009623C(sector + wm_98cc0_transposed_index(tile),
                            WM_98CC0_TILE_BYTES, buffer);
            } else if ((loaded & WM_98CC0_PRIMARY_LOADED) != 0u) {
#else
            if ((loaded & WM_98CC0_PRIMARY_LOADED) != 0u) {
#endif
                u32 sector = (u32)ArchiveDecodeSector(
                    (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_PRIMARY));
                u32 source = sector + wm_98cc0_tile_u32(tile);
                wm_8009623C(source, WM_98CC0_TILE_BYTES, buffer);
#if !defined(WM_98CC0_MUTANT_M7)
            } else if ((loaded & WM_98CC0_SECOND_LOADED) != 0u) {
                u32 sector = (u32)ArchiveDecodeSector(
                    (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_SECOND));
                wm_8009623C(sector + wm_98cc0_transposed_index(tile),
                            WM_98CC0_TILE_BYTES, buffer);
#endif
            }
        }
    }
}

static void wm_98cc0_path_closure(u32 loaded)
{
    u32 select;

#if defined(WM_98CC0_MUTANT_M6)
    (void)loaded;
    return;
#endif
    for (select = 0u; select < 4u; select++) {
        s16 window_index = wm_98cc0_lh(WM_98CC0_SELECT_INDICES + select * 2u);
        s16 tile = wm_98cc0_window_tile((u32)(s32)window_index);
        if (wm_98cc0_slot_is_empty(tile)) {
            u32 buffer = wm_98cc0_allocate(tile);
            if ((loaded & WM_98CC0_PRIMARY_LOADED) != 0u) {
                char *host_path = ArchiveGetFilePath(
                    (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_PRIMARY));
                wm_800962B0(PsxMemory_GuestAddr(host_path),
                            wm_98cc0_tile_u32(tile) << 11u,
                            WM_98CC0_TILE_BYTES, buffer);
            } else if ((loaded & WM_98CC0_SECOND_LOADED) != 0u) {
                char *host_path = ArchiveGetFilePath(
                    (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_SECOND));
                wm_800962B0(PsxMemory_GuestAddr(host_path),
                            wm_98cc0_transposed_index(tile) << 11u,
                            WM_98CC0_TILE_BYTES, buffer);
            }
        }
    }
}

void wm_80098CC0(void)
{
    u32 first;
    u32 second;
    int ready;
    u32 loaded;

    wm_98cc0_evict_old_window();

    first = func_8002C3D8();
    second = func_8002C3D8();
#if defined(WM_98CC0_MUTANT_M2)
    ready = first == 0u && second == UINT32_MAX;
#else
    ready = first == 0u || second == UINT32_MAX;
#endif

    if (ready) {
        u32 primary_sector = (u32)ArchiveDecodeSector(
            (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_PRIMARY));
        u32 second_sector;

        loaded = wm_98cc0_primary_sector_edges(primary_sector);
        second_sector = (u32)ArchiveDecodeSector(
            (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_SECOND));
        loaded |= wm_98cc0_second_sector_edges(second_sector);
        wm_98cc0_sector_closure(loaded);
        wm_8009623C(0u, 0u, 0u);
#if defined(WM_98CC0_MUTANT_M10)
        (void)wm_800965A4();
#else
        (void)wm_80096328();
#endif
    } else {
        char *primary_host = ArchiveGetFilePath(
            (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_PRIMARY));
        char *second_host;

        loaded = wm_98cc0_primary_path_edges(PsxMemory_GuestAddr(primary_host));
        second_host = ArchiveGetFilePath(
            (int)wm_98cc0_lw(WM_98CC0_ARCHIVE_SECOND));
        loaded |= wm_98cc0_second_path_edges(PsxMemory_GuestAddr(second_host));
        wm_98cc0_path_closure(loaded);
        wm_800962B0(0u, 0u, 0u, 0u);
#if defined(WM_98CC0_MUTANT_M10)
        (void)wm_80096328();
#else
        (void)wm_800965A4();
#endif
    }
}

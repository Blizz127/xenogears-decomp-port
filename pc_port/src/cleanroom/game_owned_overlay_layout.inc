/* SPDX-License-Identifier: MIT */
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include "lzss_bounds.h"

int xg_plat_disc_read_sectors(uint32_t lba, uint32_t count, void *out);

enum {
    XenoOverlaySectorBytes = 2048,
    XenoOverlayFatStart = 0x18,
    XenoOverlayFatSectors = 16,
    XenoOverlayDirectorySector = 0x28,
    XenoOverlayEntryBytes = 7,
    /* The image occupies original PlayStation main RAM. A stream consisting
     * solely of literals also needs a flag byte per eight output bytes. */
    XenoOverlayRamBytes = 2 * 1024 * 1024,
    XenoOverlayMaxPacked = 4 + XenoOverlayRamBytes + XenoOverlayRamBytes / 8,
    XenoOverlayMaxSectors = (XenoOverlayMaxPacked + XenoOverlaySectorBytes - 1) / XenoOverlaySectorBytes
};

static uint32_t PcPort_OverlayReadLE32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int PcPort_ReadOwnedOverlayLayout(unsigned int file, PcPortOverlayLayout *layout)
{
    unsigned char table[XenoOverlaySectorBytes * 2];
    if (xg_plat_disc_read_sectors(XenoOverlayDirectorySector, 1, table) != 0)
        return 0;

    unsigned int directory = table[2] | (unsigned int)table[3] << 8;
    if (directory == 0)
        return 0;
    unsigned int offset = (directory + file - 2) * XenoOverlayEntryBytes;
    if (offset > XenoOverlaySectorBytes * XenoOverlayFatSectors - XenoOverlayEntryBytes)
        return 0;
    unsigned int within = offset % XenoOverlaySectorBytes;
    unsigned int sectors = within + XenoOverlayEntryBytes > XenoOverlaySectorBytes ? 2 : 1;
    if (xg_plat_disc_read_sectors(XenoOverlayFatStart + offset / XenoOverlaySectorBytes,
                                  sectors, table) != 0)
        return 0;

    const unsigned char *entry = table + within;
    uint32_t lba = entry[0] | (uint32_t)entry[1] << 8 | (uint32_t)entry[2] << 16;
    uint32_t raw_size = PcPort_OverlayReadLE32(entry + 3);
    if (raw_size == 0 || raw_size > INT_MAX - 3)
        return 0;
    uint32_t logical = (raw_size + 3u) & ~3u;
    uint32_t payload_sectors = (logical + XenoOverlaySectorBytes - 1u) / XenoOverlaySectorBytes;
    if (payload_sectors > XenoOverlayMaxSectors)
        return 0;

    size_t capacity = (size_t)payload_sectors * XenoOverlaySectorBytes;
    unsigned char *payload = malloc(capacity);
    if (payload == NULL)
        return 0;
    int ok = 0;
    if (xg_plat_disc_read_sectors(lba, payload_sectors, payload) == 0) {
        uint32_t image = PcPort_OverlayReadLE32(payload);
        size_t used;
        /* A complete final token group can extend into the last sector's
         * padding. Logical archive length and consumed length are distinct. */
        if (image > 0 && image <= XenoOverlayRamBytes &&
            PcPort_LzssStreamExtent(payload, capacity, image, &used)) {
            layout->file = (int)file;
            layout->logicalSize = (int)logical;
            layout->sourceSize = (int)used;
            layout->imageSize = (int)image;
            ok = 1;
        }
    }
    free(payload);
    return ok;
}

const PcPortOverlayLayout *PcPort_GetOverlayLayout(unsigned int state)
{
    static const unsigned char files[] = { [1] = 14, [4] = 13, [6] = 18 };
    static PcPortOverlayLayout layouts[sizeof files];
    static _Atomic unsigned int ready;
    static _Atomic int loading;

    if (state >= sizeof files || files[state] == 0)
        return NULL;
    unsigned int bit = 1u << state;
    /* Only cold reads need serialization. This also protects disc backends
     * that resolve their mounted image on the first read. */
    for (;;) {
        if (atomic_load_explicit(&ready, memory_order_acquire) & bit)
            return &layouts[state];
        int idle = 0;
        if (atomic_compare_exchange_weak_explicit(&loading, &idle, 1,
                memory_order_acquire, memory_order_relaxed))
            break;
        while (atomic_load_explicit(&loading, memory_order_relaxed)) {
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
            __builtin_ia32_pause();
#endif
        }
    }

    int ok = (atomic_load_explicit(&ready, memory_order_relaxed) & bit) != 0;
    if (!ok && PcPort_ReadOwnedOverlayLayout(files[state], &layouts[state])) {
        atomic_fetch_or_explicit(&ready, bit, memory_order_release);
        ok = 1;
    }
    atomic_store_explicit(&loading, 0, memory_order_release);
    return ok ? &layouts[state] : NULL;
}

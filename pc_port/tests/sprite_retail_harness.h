/*
 * Shared harness for the d2 sprite-engine retail-differential tests.
 *
 * Guest view for the MIPS oracle (pc_port/src/battle_mips_adapter.c):
 *   - the whole retail image is read at runtime from disc/SLUS_006.64 into a
 *     2 MiB guest RAM at 0x8000F800 (never copied into source);
 *   - h_map() aliases a guest address range onto a host variable (retail
 *     globals such as D_800591AD live in host memory so the native C and the
 *     oracle observe the same object);
 *   - h_region() identity-maps a host buffer (statics, below 4 GiB because the
 *     tests link -no-pie) so host pointers stored in sprite fields are valid
 *     guest addresses for the oracle.
 */
#ifndef SPRITE_RETAIL_HARNESS_H
#define SPRITE_RETAIL_HARNESS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"

typedef uint8_t hu8;
typedef uint32_t hu32;

static hu8 h_ram[0x200000];

struct HMap { hu32 guest; hu8* host; hu32 len; };
static struct HMap h_maps[24];
static unsigned h_nmaps;

static void h_map(hu32 guest, void* host, hu32 len)
{
    h_maps[h_nmaps].guest = guest;
    h_maps[h_nmaps].host = (hu8*)host;
    h_maps[h_nmaps].len = len;
    ++h_nmaps;
}

static void h_region(void* host, hu32 len)
{
    if ((uintptr_t)host + len > 0xFFFFFFFFu || (uintptr_t)host < 0x1000u) {
        fprintf(stderr, "HARNESS FAIL host region not below 4GiB\n");
        exit(2);
    }
    h_map((hu32)(uintptr_t)host, host, len);
}

static void h_load_retail(void)
{
    FILE* f = fopen("disc/SLUS_006.64", "rb");
    size_t n;

    if (f == NULL) {
        fprintf(stderr, "HARNESS FAIL cannot open disc/SLUS_006.64\n");
        exit(2);
    }
    n = fread(h_ram + 0xF800, 1, sizeof(h_ram) - 0xF800, f);
    fclose(f);
    if (n < 0x40000) {
        fprintf(stderr, "HARNESS FAIL short retail image\n");
        exit(2);
    }
}

static hu8* h_addr(hu32 a, unsigned w)
{
    unsigned i;

    for (i = 0; i < h_nmaps; ++i) {
        if (a >= h_maps[i].guest && (uint64_t)a + w <= (uint64_t)h_maps[i].guest + h_maps[i].len) {
            return h_maps[i].host + (a - h_maps[i].guest);
        }
    }
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) {
        return h_ram + (a & 0x1FFFFFu);
    }
    return NULL;
}

static int h_rd(void* u, hu32 a, unsigned w, hu32* v)
{
    hu8* p = h_addr(a, w);
    unsigned i;

    (void)u;
    if (p == NULL) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (hu32)p[i] << (i * 8);
    }
    return 0;
}

static int h_wr(void* u, hu32 a, unsigned w, hu32 v)
{
    hu8* p = h_addr(a, w);
    unsigned i;

    (void)u;
    if (p == NULL) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        p[i] = (hu8)(v >> (i * 8));
    }
    return 0;
}

static hu32 h_lcg(hu32* s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s >> 8 | *s << 24;
}

#endif

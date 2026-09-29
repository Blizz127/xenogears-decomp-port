/*
 * Guest-safe world-map binding for retail 0x80025044.
 *
 * The compiled generic body treats the u32 image links as native pointers.
 * World-map producers store PSX/KSEG guest addresses, so map each known link
 * before passing its rectangle and data to the host GPU implementation.
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_image_transfer_25044.h"

typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Wm25044Rect;
typedef unsigned long Wm25044ULong;

extern s32 g_GfxCurContext;
extern u32 g_GfxImageList[];
extern int LoadImage(Wm25044Rect *rect, Wm25044ULong *data);
extern int ClearImage(Wm25044Rect *rect, unsigned char r,
                      unsigned char g, unsigned char b);

static int s_wm25044_unknowns;

void wm_25044_reset(void)
{
    s_wm25044_unknowns = 0;
}

int wm_25044_get_unknowns(void)
{
    return s_wm25044_unknowns;
}

static void *wm25044_resolve(u32 value)
{
    uintptr_t host = (uintptr_t)value;
    uintptr_t base = (uintptr_t)g_PsxRam;

    if (((value & 0xFFE00000u) == 0x80000000u) ||
        ((value & 0xFFE00000u) == 0xA0000000u))
        return PSX_ADDR(value);
    if (host >= base && host < base + (uintptr_t)PSX_RAM_SIZE)
        return (void *)host;
    return NULL;
}

static void wm25044_unknown(const char *kind, u32 value)
{
    s_wm25044_unknowns++;
    fprintf(stderr,
            "[worldmap-image-transfer] unknown %s=0x%08x count=%d\n",
            kind, value, s_wm25044_unknowns);
}

void wm_80025044_guest_safe(void)
{
    s32 context = g_GfxCurContext;
    u32 image;

    if (context < 0 || context >= 2) {
        wm25044_unknown("context", (u32)context);
        return;
    }

    image = g_GfxImageList[context];
    while (image != 0u) {
        u8 *guest_image;
        u32 data;
        u32 next;

        guest_image = (u8 *)wm25044_resolve(image);
        if (guest_image == NULL) {
            wm25044_unknown("image", image);
            break;
        }
        data = *(u32 *)(guest_image + 0x08u);
        next = *(u32 *)(guest_image + 0x0Cu);
        if (data != 0u && wm25044_resolve(data) == NULL) {
            wm25044_unknown("data", data);
            break;
        }
        if (data != 0u)
            (void)LoadImage((Wm25044Rect *)guest_image,
                            (Wm25044ULong *)wm25044_resolve(data));
        else
            (void)ClearImage((Wm25044Rect *)guest_image, 0u, 0u, 0u);
        image = next;
    }
    g_GfxImageList[context] = 0u;
}

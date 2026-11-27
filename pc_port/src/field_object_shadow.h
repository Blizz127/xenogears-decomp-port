#ifndef XENO_FIELD_OBJECT_SHADOW_H
#define XENO_FIELD_OBJECT_SHADOW_H

#include <stdint.h>
#include <string.h>
#include "psyq/libgpu.h"

_Static_assert(sizeof(POLY_FT4) == 0x28, "Field object shadows use compact GPU packets");

/* US 801E7888..801E7924 initializes one packet per draw buffer. Keep each
 * packet's DMA link, screen coordinates and padding until the draw pass. */
static inline void PcPort_InitFieldObjectShadows(uint8_t *actor)
{
    for (unsigned context = 0; context < 2; ++context) {
        POLY_FT4 polygon;
        uint8_t *packet = actor + 0xB8 + context * sizeof(polygon);
        memcpy(&polygon, packet, sizeof(polygon));
        SetPolyFT4(&polygon);
        SetSemiTrans(&polygon, 1);
        setRGB0(&polygon, 0x40, 0x40, 0x40);
        polygon.clut = GetClut(0x100, 0xF3);
        polygon.tpage = GetTPage(0, 2, 0x280, 0x100);
        setUV4(&polygon, 0, 0xE0, 0x0F, 0xE0, 0, 0xEF, 0x0F, 0xEF);
        memcpy(packet, &polygon, sizeof(polygon));
    }
}

#endif

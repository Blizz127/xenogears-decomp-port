/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "model_polygon_lighting.h"
#include "model_prim_link.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"

extern u8 *D_80059424;
extern u32 D_80059498, D_8005952C, D_8005953C, D_80059568;
extern s32 D_800500F8, D_800500FC, D_80050100, D_80059578;

static void PcPort_ShadeModelFace(int mode, const u8 *record, u8 *packet,
                                  const unsigned indices[4], const SVECTOR *first_vertex)
{
    CVECTOR light;
    SVECTOR normal;
    if (mode == PCPORT_MODEL_LIT_FT3 || mode == PCPORT_MODEL_LIT_FT4) {
        memcpy(&normal, record, sizeof normal);
        /* Textured triangles use the first vertex X/Y with the stored normal Z. */
        if (mode == PCPORT_MODEL_LIT_FT3) {
            normal.vx = first_vertex->vx;
            normal.vy = first_vertex->vy;
        }
        NormalColor(&normal, &light);
        memcpy(packet + 4, &light, 3);
        return;
    }

    CVECTOR base;
    memcpy(&base, record, sizeof base);
    if (mode == PCPORT_MODEL_LIT_G3) {
        const u8 *normals = (const u8 *)(uintptr_t)D_8005952C;
        SVECTOR corners[3];
        CVECTOR colors[3];
        for (unsigned i = 0; i < 3; ++i)
            memcpy(&corners[i], normals + indices[i] * 8u, sizeof corners[i]);
        NormalColorCol3(&corners[0], &corners[1], &corners[2], &base,
                        &colors[0], &colors[1], &colors[2]);
        for (unsigned i = 0; i < 3; ++i)
            memcpy(packet + 4 + 8 * i, &colors[i], sizeof colors[i]);
    } else {
        memcpy(&normal, record + 4, sizeof normal);
        NormalColorCol(&normal, &base, &light);
        memcpy(packet + 4, &light, sizeof light);
    }
}

int PcPort_DrawLitModelPolygons(const unsigned char *commands, int count, int lighting_mode)
{
    static const struct {
        unsigned char corners, packet_bytes, stride, record_bytes;
    } layout[] = {
        [PCPORT_MODEL_LIT_F3]  = {3, 20, 4, 12},
        [PCPORT_MODEL_LIT_FT3] = {3, 32, 8, 8},
        [PCPORT_MODEL_LIT_FT4] = {4, 40, 8, 8},
        [PCPORT_MODEL_LIT_F4]  = {4, 24, 4, 12},
        [PCPORT_MODEL_LIT_G3]  = {3, 28, 8, 4}
    };
    if ((unsigned)lighting_mode >= sizeof layout / sizeof layout[0] ||
        !commands || !D_8005953C || !D_80059424 || !D_80059568 || !D_80059498 ||
        (lighting_mode == PCPORT_MODEL_LIT_G3 && !D_8005952C))
        return D_800500FC;

    const unsigned corners = layout[lighting_mode].corners;
    const unsigned stride = layout[lighting_mode].stride;
    const unsigned packet_bytes = layout[lighting_mode].packet_bytes;
    const u8 *vertices = (const u8 *)(uintptr_t)D_8005953C;
    for (int face = 0; face < count; ++face, commands += 8) {
        const u8 *record = (const u8 *)(uintptr_t)D_80059498;
        u8 *packet = D_80059424;
        D_80059498 += layout[lighting_mode].record_bytes;
        D_80059424 += packet_bytes;

        int xy[4];
        unsigned indices[4];
        SVECTOR points[4];
        int visible_x = 0, visible_y = 0;
        for (unsigned i = 0; i < corners; ++i) {
            unsigned index = commands[2 * i] | ((unsigned)commands[2 * i + 1] << 8);
            if (i == 1 || (i == 0 && face && corners == 4))
                index &= 0x1fffu;
            indices[i] = index;
            memcpy(&points[i], vertices + index * 8u, sizeof points[i]);
            long cue = 0, flags = 0;
            RotTransPers(&points[i], &xy[i], &cue, &flags);
            const int x = (int16_t)xy[i];
            const int y = (int16_t)((u32)xy[i] >> 16);
            visible_x |= x >= 0 && x < D_800500F8;
            visible_y |= y >= 0 && y < (D_800500FC >> 16);
        }
        if (!visible_y || !visible_x) continue;

        /* Triangle packets retain these coordinates even when back-facing. */
        if (corners == 3) {
            unsigned early = lighting_mode == PCPORT_MODEL_LIT_F3 ? 3u : 2u;
            for (unsigned i = 0; i < early; ++i)
                memcpy(packet + 8 + i * stride, &xy[i], 4);
        }
        if (NormalClip(xy[0], xy[1], xy[2]) <= 0) continue;
        if (corners == 3) {
            memcpy(packet + 8 + 2 * stride, &xy[2], 4);
            gte_avsz3();
        } else {
            gte_avsz4();
        }
        unsigned depth;
        gte_stotz(&depth);
        if (lighting_mode != PCPORT_MODEL_LIT_G3 || depth) ++D_80059578;
        if (!depth && lighting_mode != PCPORT_MODEL_LIT_F3) continue;
        if (corners == 4) {
            for (unsigned i = 0; i < corners; ++i)
                memcpy(packet + 8 + i * stride, &xy[i], 4);
        }

        PcPort_ShadeModelFace(lighting_mode, record, packet, indices, &points[0]);
        unsigned bucket = depth >> ((unsigned)D_80050100 & 31u);
        PcPort_LinkModelPrim((u32 *)(uintptr_t)D_80059568, (s32)bucket, packet,
                             (packet_bytes / 4u - 1u) << 24);
    }
    return D_800500FC;
}

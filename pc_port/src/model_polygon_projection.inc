/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "model_polygon_projection.h"
#include "model_prim_link.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"

extern u8 *D_80059424;
extern u32 D_8005953C, D_80059568;
extern s32 D_800500F8, D_800500FC, D_80050100, D_80059578;

int PcPort_DrawModelPolygons(const unsigned char *commands, int count, int nvertices,
                             int packet_bytes, int xy_stride, int depth_mode)
{
    if ((nvertices != 3 && nvertices != 4) || xy_stride < 4 ||
        packet_bytes < 12 || xy_stride > (packet_bytes - 12) / (nvertices - 1) ||
        (packet_bytes & 3) || packet_bytes > 1024 || depth_mode < PCPORT_MODEL_DEPTH_AVERAGE ||
        depth_mode > PCPORT_MODEL_DEPTH_MAXIMUM || !commands || !D_8005953C || !D_80059424 ||
        !D_80059568) {
        return D_800500FC;
    }

    const u8 *vertices = (const u8 *)(uintptr_t)D_8005953C;
    u32 *ordering_table = (u32 *)(uintptr_t)D_80059568;
    const u32 tag_length = (u32)(packet_bytes / 4 - 1) << 24;
    for (int face = 0; face < count; ++face, commands += 8) {
        int xy[4];
        unsigned projected_depth[4];
        u8 *packet = D_80059424;
        D_80059424 += packet_bytes;
        for (int corner = 0; corner < nvertices; ++corner) {
            unsigned index = commands[corner * 2] | ((unsigned)commands[corner * 2 + 1] << 8);
            SVECTOR vertex;
            long cue = 0, flags = 0;
            memcpy(&vertex, vertices + index * 8u, sizeof(vertex));
            projected_depth[corner] = (unsigned)RotTransPers(&vertex, &xy[corner], &cue, &flags);
        }
        int visible_x = 0, visible_y = 0;
        for (int corner = 0; corner < nvertices; ++corner) {
            int x = (int16_t)xy[corner];
            int y = (int16_t)((u32)xy[corner] >> 16);
            visible_x |= x >= 0 && x < D_800500F8;
            visible_y |= y >= 0 && y < (D_800500FC >> 16);
        }
        if (!visible_y) continue;

        /* Culled min/max packets can still receive their first projected corner. */
        int front_facing = NormalClip(xy[0], xy[1], xy[2]) > 0;
        if (depth_mode != PCPORT_MODEL_DEPTH_AVERAGE &&
            (nvertices == 3 ? visible_x : front_facing)) {
            memcpy(packet + 8, xy, 4);
        }
        if (!visible_x || !front_facing) continue;
        /* The counter includes visible faces whose average depth is zero. */
        ++D_80059578;

        unsigned depth;
        if (depth_mode == PCPORT_MODEL_DEPTH_AVERAGE) {
            if (nvertices == 3) gte_avsz3();
            else gte_avsz4();
            gte_stotz(&depth);
        } else {
            depth = projected_depth[0];
            for (int corner = 1; corner < nvertices; ++corner) {
                if ((depth_mode == PCPORT_MODEL_DEPTH_MINIMUM && projected_depth[corner] < depth) ||
                    (depth_mode == PCPORT_MODEL_DEPTH_MAXIMUM && projected_depth[corner] > depth))
                    depth = projected_depth[corner];
            }
        }
        if (depth_mode != PCPORT_MODEL_DEPTH_AVERAGE || depth || nvertices == 3) {
            for (int corner = 0; corner < nvertices; ++corner)
                memcpy(packet + 8 + corner * xy_stride, &xy[corner], 4);
        }
        if (depth_mode != PCPORT_MODEL_DEPTH_AVERAGE || depth) {
            unsigned bucket = depth >> ((unsigned)D_80050100 & 31u);
            PcPort_LinkModelPrim(ordering_table, (s32)bucket, packet, tag_length);
        }
    }
    return D_800500FC;
}

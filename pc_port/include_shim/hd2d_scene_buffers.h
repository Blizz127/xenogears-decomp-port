#ifndef PC_PORT_HD2D_SCENE_BUFFERS_H
#define PC_PORT_HD2D_SCENE_BUFFERS_H

#include <stdint.h>
#include "xg_plat/hd2d.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Borrowed full-drawable GL_R8 coverage texture, never a depth attachment.
 * Coverage bytes use XG_HD2D_PROTECTION_COVERAGE_BITS: only exact byte 2 is
 * certified map coverage. The producer owns texture allocation and lifetime. */
#define PC_PORT_HD2D_SCENE_COVERAGE_VALID UINT32_C(1)
typedef struct PcPortHd2dSceneBuffers {
    unsigned coverage_tex;
    int drawable_width, drawable_height;
    int viewport_x, viewport_y, viewport_width, viewport_height;
    int field_id;
    uint64_t epoch;
    uint32_t flags;
} PcPortHd2dSceneBuffers;

/* CPU-only nonzero generation. Changes with master, configured maps, observed
 * field scope or map transitions. No GL calls; do not cache across a frame. */
uint64_t PcPort_Hd2dSceneEpoch(void);

/* Copies metadata only. A new ON offer replaces any previous offer, including
 * NULL (which clears it). OFF returns before inspecting the input pointer. */
void PcPort_Hd2dSceneBuffersOffer(const PcPortHd2dSceneBuffers *buffers);

/* Call exactly once in every ON PostProcess before any scope/viewport/path
 * early return. Pending metadata expires first, even for NULL or invalid
 * outputs. Only matching epoch/map/drawable/viewport path-0 coverage is
 * accepted. Preserve field/viewport/projected items; clear all attachment
 * claims before validation, then fill coverage only. No GL calls.
 * OFF returns before inspecting pending metadata or the output pointer. */
int PcPort_Hd2dSceneBuffersConsume(int path, int drawable_width,
                                 int drawable_height, XgHd2dFrame *frame);

/* CPU-only invalidation; invoke on both master transitions. */
void PcPort_Hd2dSceneBuffersReset(void);

#ifdef __cplusplus
}
#endif
#endif

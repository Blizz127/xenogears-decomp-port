/* Model submission can discard a box when none of the requested samples
 * project inside the draw area. Project in triples to preserve the renderer's
 * depth FIFO and the point at which an accepted sample ends the search. */
#include <string.h>
#include <psx/gtereg.h>

#include "common.h"
#include "psyq/libgte.h"

extern s32 D_800500F8;
extern s32 D_800500FC;

enum BoxAxis { BOX_X, BOX_Y, BOX_Z, BOX_AXES };
typedef struct {
    s16 endpoint[2][BOX_AXES];
} ModelBounds;

static SVECTOR box_sample(const ModelBounds *bounds, unsigned corner,
                          unsigned halfway_axes)
{
    s16 coordinates[BOX_AXES];
    for (unsigned axis = BOX_X; axis < BOX_AXES; ++axis) {
        unsigned bit = 1u << axis;
        unsigned side = (corner & bit) != 0u;
        int value = bounds->endpoint[side][axis];
        if ((halfway_axes & bit) != 0u) {
            int distance = bounds->endpoint[1u - side][axis] - value;
            value += distance / 2;
        }
        coordinates[axis] = (s16)value;
    }
    SVECTOR sample = {coordinates[BOX_X], coordinates[BOX_Y],
                      coordinates[BOX_Z], 0};
    return sample;
}

static int any_sample_visible(SVECTOR points[3])
{
    long xy[3] = {0}, cue = 0, flags = 0;
    (void)RotTransPers3(&points[0], &points[1], &points[2],
                        &xy[0], &xy[1], &xy[2], &cue, &flags);
    const u16 depth[3] = {(u16)C2_SZ1, (u16)C2_SZ2, (u16)C2_SZ3};
    for (unsigned i = 0; i < 3; ++i) {
        u32 screen = (u32)xy[i];
        if (depth[i] != 0 && depth[i] != UINT16_MAX &&
            (u32)(u16)screen < (u32)D_800500F8 && screen < (u32)D_800500FC)
            return 1;
    }
    return 0;
}

s32 func_8003101C(const u8 *model, s32 mode)
{
    if ((mode & 3) == 0)
        return 1;

    ModelBounds bounds;
    memcpy(bounds.endpoint[0], model + 0x20, sizeof(bounds.endpoint[0]));
    memcpy(bounds.endpoint[1], model + 0x28, sizeof(bounds.endpoint[1]));
    SVECTOR samples[3];

    if ((mode & 1) != 0) {
        /* Start with the model diagonal and its center, then visit the six
         * corners between its ends in ascending corner order. */
        /* These are complete vertex records: their fourth halfwords remain
         * observable in the GTE input registers on an early return. */
        memcpy(samples, model + 0x20, 2 * sizeof(SVECTOR));
        samples[2] = box_sample(&bounds, 0, 7);
        if (any_sample_visible(samples))
            return 0;
        for (unsigned first = 1; first < 7; first += 3) {
            for (unsigned i = 0; i < 3; ++i)
                samples[i] = box_sample(&bounds, first + i, 0);
            if (any_sample_visible(samples))
                return 0;
        }
    }
    if ((mode & 2) != 0) {
        /* The four even-parity corners touch all twelve edges exactly once.
         * The order within each group determines the GTE FIFO on return. */
        static const enum BoxAxis axes[4][3] = {
            {BOX_X, BOX_Z, BOX_Y}, {BOX_Y, BOX_X, BOX_Z},
            {BOX_Z, BOX_Y, BOX_X}, {BOX_Z, BOX_X, BOX_Y}
        };
        for (unsigned group = 0; group < 4; ++group) {
            unsigned x = group & 1u;
            unsigned z = group >> 1;
            unsigned corner = x | ((x ^ z) << BOX_Y) | (z << BOX_Z);
            for (unsigned i = 0; i < 3; ++i)
                samples[i] = box_sample(&bounds, corner, 1u << axes[group][i]);
            if (any_sample_visible(samples))
                return 0;
        }
    }
    return 1;
}

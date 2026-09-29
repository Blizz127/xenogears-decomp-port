#ifndef W34B5P_TRACKED_TEST_LIBGTE_H
#define W34B5P_TRACKED_TEST_LIBGTE_H

/* Minimal tracked PsyQ contract for focused production tests.
 * The real PC-port build uses PsyCross; this header provides only the
 * fixed-width VECTOR ABI needed to compile world_map_terrain_normal.c in a
 * clean tracked-only test export. */
#include <stdint.h>

typedef struct {
    int32_t vx;
    int32_t vy;
    int32_t vz;
    int32_t pad;
} VECTOR;

#endif

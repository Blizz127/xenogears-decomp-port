/* W34N54 — production-linked certificate for retail func_8003101C. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psx/gtereg.h>

#include "common.h"
#include "psyq/libgte.h"
#include "world_map_helper_3101c.h"

GTERegisters gteRegs;
s32 D_800500F8 = 320;
s32 D_800500FC = 239 << 16;

typedef struct ProjectionCall {
    SVECTOR vertex[3];
} ProjectionCall;

static ProjectionCall s_calls[8];
static int s_call_count;
static int s_visible_call;
static int s_visible_vertex;
static int s_invalid_depth;
static int s_failures;

static u32 packed_xy(s16 x, s16 y)
{
    return (u32)(u16)x | ((u32)(u16)y << 16);
}

static void check(int condition, const char* name)
{
    if (condition == 0) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static void reset_projection(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    memset(s_calls, 0, sizeof(s_calls));
    s_call_count = 0;
    s_visible_call = -1;
    s_visible_vertex = 0;
    s_invalid_depth = 0;
}

int RotTransPers3(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2,
                  long* xy0, long* xy1, long* xy2,
                  long* p, long* flag)
{
    long* outputs[3] = {xy0, xy1, xy2};
    SVECTOR* inputs[3] = {v0, v1, v2};
    int vertex;

    if (s_call_count < 8) {
        for (vertex = 0; vertex < 3; vertex++)
            s_calls[s_call_count].vertex[vertex] = *inputs[vertex];
    }
    for (vertex = 0; vertex < 3; vertex++) {
        u32 xy = packed_xy(400, 100);
        if (s_call_count == s_visible_call && vertex == s_visible_vertex)
            xy = packed_xy(100, 100);
        *outputs[vertex] = (long)xy;
    }
    C2_SZ1 = (s_invalid_depth != 0 && s_call_count == 0) ? 0xFFFFu : 100u;
    C2_SZ2 = 100u;
    C2_SZ3 = 100u;
    *p = 0;
    *flag = 0;
    s_call_count++;
    return 0;
}

static void write_s16(u8* bytes, size_t offset, s16 value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}

static void initialize_bounds(u8* bytes, size_t size)
{
    memset(bytes, 0xA5, size);
    write_s16(bytes, 0x20u, -10);
    write_s16(bytes, 0x22u, 5);
    write_s16(bytes, 0x24u, -3);
    write_s16(bytes, 0x28u, -5);
    write_s16(bytes, 0x2Au, 0);
    write_s16(bytes, 0x2Cu, 4);
}

static int vector_equals(SVECTOR value, s16 x, s16 y, s16 z)
{
    return value.vx == x && value.vy == y && value.vz == z &&
           value.pad == 0;
}

static int triplet_equals(int call,
                          s16 x0, s16 y0, s16 z0,
                          s16 x1, s16 y1, s16 z1,
                          s16 x2, s16 y2, s16 z2)
{
    return vector_equals(s_calls[call].vertex[0], x0, y0, z0) &&
           vector_equals(s_calls[call].vertex[1], x1, y1, z1) &&
           vector_equals(s_calls[call].vertex[2], x2, y2, z2);
}

static void test_modes_and_samples(u8* bounds)
{
    s32 result;

    reset_projection();
    result = func_8003101C(NULL, 0);
    check(result == 1, "mode0.culls_without_samples");
    check(s_call_count == 0, "mode0.no_projection");

    reset_projection();
    result = func_8003101C(bounds, 1);
    check(result == 1, "mode1.all_outside_culled");
    check(s_call_count == 3, "mode1.call_count");
    check(triplet_equals(0, -10, 5, -3, -5, 0, 4, -8, 3, 0),
          "retail.midpoint_direction");
    check(triplet_equals(1, -5, 5, -3, -10, 0, -3, -5, 0, -3),
          "mode1.triplet1");
    check(triplet_equals(2, -10, 5, 4, -5, 5, 4, -10, 0, 4),
          "mode1.triplet2");

    reset_projection();
    s_visible_call = 1;
    s_visible_vertex = 2;
    result = func_8003101C(bounds, 1);
    check(result == 0, "mode1.visible_early_out");
    check(s_call_count == 2, "mode1.early_out_count");

    reset_projection();
    result = func_8003101C(bounds, 2);
    check(result == 1, "mode2.all_outside_culled");
    check(s_call_count == 4, "mode2.call_count");
    check(triplet_equals(0, -8, 5, -3, -10, 5, 0, -10, 3, -3),
          "mode2.triplet0");
    check(triplet_equals(1, -5, 2, -3, -7, 0, -3, -5, 0, 0),
          "mode2.triplet1");
    check(triplet_equals(2, -10, 0, 1, -10, 2, 4, -8, 0, 4),
          "mode2.triplet2");
    check(triplet_equals(3, -5, 5, 1, -7, 5, 4, -5, 3, 4),
          "mode2.triplet3");

    reset_projection();
    result = func_8003101C(bounds, 3);
    check(result == 1, "mode3.all_outside_culled");
    check(s_call_count == 7, "mode3.seven_triplets");
}

static void test_depth_rule(u8* bounds)
{
    s32 result;

    reset_projection();
    s_visible_call = 0;
    s_visible_vertex = 0;
    s_invalid_depth = 1;
    result = func_8003101C(bounds, 1);
    check(result == 1 && s_call_count == 3, "depth-rejects-ffff");
}

int main(void)
{
    u8 guarded[0x70];
    u8 before[sizeof(guarded)];

    initialize_bounds(guarded, sizeof(guarded));
    memcpy(before, guarded, sizeof(guarded));
    test_modes_and_samples(guarded);
    test_depth_rule(guarded);
    check(memcmp(before, guarded, sizeof(guarded)) == 0,
          "bounds.input_read_only");

    if (s_failures != 0)
        return EXIT_FAILURE;
    puts("W34N54 0x8003101C model-bounds culler certificate PASS");
    return EXIT_SUCCESS;
}

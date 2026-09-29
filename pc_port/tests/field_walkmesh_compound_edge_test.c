/*
 * Retail certificate for func_8007BEF4's compound triangle-side cases.
 *
 * The field collision walker can classify a point beyond two triangle edges
 * at once (masks 3, 5, or 6).  Retail chooses one neighbor from the old/new
 * path direction and, critically, rewrites the compound mask to the chosen
 * single edge before returning a blocking edge to func_8007BAC0.  Leaving the
 * mask compound returns stale edge data and can strand a scripted actor.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgte.h"

extern s32 func_8007BEF4(s32* move, s32* base, u8* actorData,
                         s16* outEdge, s16* outPoint, s32 mode,
                         s32* outFlags);

s32 D_800AFB24[3];
s32 D_800AFB34[3];
u32 D_800AFB20[1];
u8 D_800B21CC;

static s16 s_triangles[7];
static s16 s_vertices[3][4];
static u32 s_materials[1];
static int s_clip_values[4];
static int s_clip_count;
static int s_clip_index;
static unsigned int s_checks;

int NormalClip(int a, int b, int c)
{
    (void)a;
    (void)b;
    (void)c;
    if (s_clip_index >= s_clip_count) {
        fprintf(stderr, "ASSERTION compound.side.clip.count overflow=%d\n",
                s_clip_index);
        exit(1);
    }
    return s_clip_values[s_clip_index++];
}

long VectorNormal(VECTOR* in, VECTOR* out)
{
    (void)in;
    memset(out, 0, sizeof(*out));
    return 0;
}

void OuterProduct12(VECTOR* a, VECTOR* b, VECTOR* out)
{
    (void)a;
    (void)b;
    memset(out, 0, sizeof(*out));
}

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_s16(const char* case_name, const char* field,
                          s16 actual, s16 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail),
             "case=%s field=%s actual=%d expected=%d",
             case_name, field, (int)actual, (int)expected);
    fail("compound.side.normalized", detail);
}

/* All three retail arms reach the b.z store at 8007C628..8007C634.
 * Arms 1/2 jump there; counting only their local stores misses that tail. */
static void expect_edge(const char* case_name, const s16* edge,
                        int first_vertex, int second_vertex, s16 expected_bz)
{
    expect_eq_s16(case_name, "a.x", edge[0], s_vertices[first_vertex][0]);
    expect_eq_s16(case_name, "a.y", edge[1], s_vertices[first_vertex][1]);
    expect_eq_s16(case_name, "a.z", edge[2], s_vertices[first_vertex][2]);
    expect_eq_s16(case_name, "b.x", edge[4], s_vertices[second_vertex][0]);
    expect_eq_s16(case_name, "b.y", edge[5], s_vertices[second_vertex][1]);
    expect_eq_s16(case_name, "b.z", edge[6], expected_bz);
}

static void run_case(const char* name,
                     int clip0, int clip1, int clip2, int chooser,
                     int expected_first, int expected_second, s16 expected_bz)
{
    u8 actor[0x140];
    s32 move[3] = { 0x10000, 0, 0x20000 };
    s32 base[3] = { 0, 0, 0 };
    s16 edge[8];
    s16 point[16];
    s32 flags = 0;
    s32 result;

    memset(actor, 0, sizeof(actor));
    memset(edge, 0x6B, sizeof(edge));
    memset(point, 0, sizeof(point));
    *(s16*)(actor + 0x08) = 0;
    *(s16*)(actor + 0x10) = 0;
    *(u32*)(actor + 0x04) = 0x08;

    s_clip_values[0] = clip0;
    s_clip_values[1] = clip1;
    s_clip_values[2] = clip2;
    s_clip_values[3] = chooser;
    s_clip_count = (clip0 < 0) + (clip1 < 0) + (clip2 < 0) == 2 ? 4 : 3;
    s_clip_index = 0;

    result = func_8007BEF4(move, base, actor, edge, point, 0, &flags);
    if (result != -1) {
        fail("compound.side.block.result", name);
    }
    if (s_clip_index != s_clip_count) {
        fail("compound.side.clip.count", name);
    }
    expect_edge(name, edge, expected_first, expected_second, expected_bz);
}

int main(void)
{
    uintptr_t tri_addr;
    uintptr_t vert_addr;
    uintptr_t material_addr;

    memset(s_triangles, 0, sizeof(s_triangles));
    s_triangles[0] = 0;
    s_triangles[1] = 1;
    s_triangles[2] = 2;
    s_triangles[3] = -1;
    s_triangles[4] = -1;
    s_triangles[5] = -1;
    s_triangles[6] = 0;

    s_vertices[0][0] = 10;
    s_vertices[0][1] = 11;
    s_vertices[0][2] = 12;
    s_vertices[1][0] = 20;
    s_vertices[1][1] = 21;
    s_vertices[1][2] = 22;
    s_vertices[2][0] = 30;
    s_vertices[2][1] = 31;
    s_vertices[2][2] = 32;
    s_materials[0] = 0;

    tri_addr = (uintptr_t)s_triangles;
    vert_addr = (uintptr_t)s_vertices;
    material_addr = (uintptr_t)s_materials;
    if (tri_addr > UINT32_MAX || vert_addr > UINT32_MAX ||
        material_addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }
    D_800AFB24[0] = (s32)(u32)tri_addr;
    D_800AFB34[0] = (s32)(u32)vert_addr;
    D_800AFB20[0] = (u32)material_addr;
    D_800B21CC = 0;

    /* Direct edge exits, including the field-12 failing edge-2 path. */
    run_case("mask1.edge1", -1, 1, 1, 0, 0, 1, 22);
    run_case("mask2.edge2", 1, -1, 1, 0, 1, 2, 32);
    run_case("mask4.edge4", 1, 1, -1, 0, 2, 0, 12);

    /* Compound masks must select a single edge AND write both endpoints. */
    run_case("mask3.edge1", -1, -1, 1, -1, 0, 1, 22);
    run_case("mask3.edge2", -1, -1, 1, 1, 1, 2, 32);
    run_case("mask5.edge4", -1, 1, -1, -1, 2, 0, 12);
    run_case("mask5.edge1", -1, 1, -1, 1, 0, 1, 22);
    run_case("mask6.edge4", 1, -1, -1, 1, 2, 0, 12);
    run_case("mask6.edge2", 1, -1, -1, -1, 1, 2, 32);

    printf("FIELD WALKMESH COMPOUND EDGE certificate PASS checks=%u\n",
           s_checks);
    return 0;
}

/*
 * Retail certificate for func_8007CD80 case 7 (all three NCLIP signs
 * negative — a back-facing triangle).
 *
 * Retail dispatch is `sltiu $v0, $a1, 0x8` (eight cases) and jtbl_8006FBCC
 * index 7 is `addiu $a3, $zero, -0x1` (nextTri = -1). A mutant that walks
 * the neighbor at tri[3] instead continues into a front-facing triangle and
 * returns 0.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    u8 pad[0x140];
} FieldScene;

extern s32 func_8007CD80(void* arg0, void* arg1, void* arg2);

s32 D_800AFB24[4];
s32 D_800AFB34[4];
s32 D_800AFB44[4];
u32 D_800AFB20[8];
s16 D_800AFB54;
u8 D_800B21CC;
FieldScene g_Scene;

static s16 s_triangles[14];
static s16 s_vertices[3][4];
static u32 s_materials[1];
static int s_clip_values[8];
static int s_clip_count;
static int s_clip_index;
static unsigned int s_checks;

int NormalClip(int a, int b, int c)
{
    (void)a;
    (void)b;
    (void)c;
    if (s_clip_index >= s_clip_count) {
        fprintf(stderr, "ASSERTION cd80.case7.clip.count overflow=%d\n",
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

static void expect_eq_s32(const char* case_name, const char* field,
                          s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail),
             "case=%s field=%s actual=%d expected=%d",
             case_name, field, (int)actual, (int)expected);
    fail("cd80.case7.result", detail);
}

static s32 run_lookup(int clip0, int clip1, int clip2, int extra_count,
                      const int* extra_clips)
{
    u8 pos[16];
    u8 edge[16];
    u8 out[32];
    int i;

    memset(pos, 0, sizeof(pos));
    memset(edge, 0x6B, sizeof(edge));
    memset(out, 0, sizeof(out));
    *(s16*)(pos + 0x02) = 0;
    *(s16*)(pos + 0x0A) = 0;

    s_clip_values[0] = clip0;
    s_clip_values[1] = clip1;
    s_clip_values[2] = clip2;
    s_clip_count = 3;
    for (i = 0; i < extra_count; i++) {
        s_clip_values[s_clip_count++] = extra_clips[i];
    }
    s_clip_index = 0;

    return func_8007CD80(pos, edge, out);
}

int main(void)
{
    uintptr_t tri_addr;
    uintptr_t vert_addr;
    uintptr_t material_addr;
    const int inside_clips[3] = { 1, 1, 1 };
    u8 edge_before[16];
    u8 pos[16];
    u8 edge[16];
    u8 out[32];
    s32 result;

    memset(&g_Scene, 0, sizeof(g_Scene));
    memset(s_triangles, 0, sizeof(s_triangles));
    /* Triangle 0: back-facing seed. Neighbor 0 (tri[3]) is triangle 1. */
    s_triangles[0] = 0;
    s_triangles[1] = 1;
    s_triangles[2] = 2;
    s_triangles[3] = 1;
    s_triangles[4] = -1;
    s_triangles[5] = -1;
    s_triangles[6] = 0;
    /* Triangle 1: front-facing containing triangle. */
    s_triangles[7] = 0;
    s_triangles[8] = 1;
    s_triangles[9] = 2;
    s_triangles[10] = -1;
    s_triangles[11] = -1;
    s_triangles[12] = -1;
    s_triangles[13] = 0;

    s_vertices[0][0] = 10;
    s_vertices[0][1] = 11;
    s_vertices[0][2] = 12;
    s_vertices[1][0] = 20;
    s_vertices[1][1] = 21;
    s_vertices[1][2] = 22;
    s_vertices[2][0] = 30;
    s_vertices[2][1] = 31;
    s_vertices[2][2] = 32;
    s_materials[0] = 0x800000;

    tri_addr = (uintptr_t)s_triangles;
    vert_addr = (uintptr_t)s_vertices;
    material_addr = (uintptr_t)s_materials;
    if (tri_addr > UINT32_MAX || vert_addr > UINT32_MAX ||
        material_addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }

    D_800AFB54 = 1;
    D_800AFB44[0] = 0; /* seed locator returns 0 without walking */
    D_800AFB20[0] = (u32)material_addr;
    D_800AFB20[1] = (s32)(u32)tri_addr;
    D_800AFB20[5] = (s32)(u32)vert_addr;
    D_800B21CC = 0;

    /* Zero-size scene bounds at the query point: no clamp. */
    *(s16*)((u8*)&g_Scene + 0x4C) = 0;
    *(s16*)((u8*)&g_Scene + 0x4E) = 0;
    *(s16*)((u8*)&g_Scene + 0x50) = 0;
    *(s16*)((u8*)&g_Scene + 0x52) = 0;

    /* mask 0: inside the seed triangle, accept. */
    expect_eq_s32("mask0.inside", "ret",
                  run_lookup(1, 1, 1, 0, NULL), 0);

    /* mask 1: walk neighbor 1, which contains the point. */
    expect_eq_s32("mask1.neighbor", "ret",
                  run_lookup(-1, 1, 1, 3, inside_clips), 0);

    /* mask 7: retail nextTri = -1; do not walk the neighbor. */
    memset(pos, 0, sizeof(pos));
    memset(edge, 0x6B, sizeof(edge));
    memcpy(edge_before, edge, sizeof(edge));
    memset(out, 0xA5, sizeof(out));
    *(s16*)(pos + 0x02) = 0;
    *(s16*)(pos + 0x0A) = 0;
    s_clip_values[0] = -1;
    s_clip_values[1] = -1;
    s_clip_values[2] = -1;
    /* Extra clips let a case-7-walks-neighbor mutant finish; retail must
     * not consume them. */
    s_clip_values[3] = 1;
    s_clip_values[4] = 1;
    s_clip_values[5] = 1;
    s_clip_count = 6;
    s_clip_index = 0;
    result = func_8007CD80(pos, edge, out);
    expect_eq_s32("mask7.backface", "ret", result, -1);
    s_checks++;
    if (s_clip_index != 3) {
        fail("cd80.case7.clip.count", "mask7 must not walk a neighbor");
    }
    s_checks++;
    if (memcmp(edge, edge_before, sizeof(edge)) != 0) {
        fail("cd80.case7.edge.untouched", "mask 7 is not a copyable edge");
    }
    s_checks++;
    if (*(s16*)(out + 0x00) != 0 || *(s16*)(out + 0x02) != 0 ||
        *(s16*)(out + 0x04) != 0 || *(s16*)(out + 0x06) != 0) {
        fail("cd80.case7.out.coords", "arg2 must record unclamped pos");
    }

    printf("FIELD WALKMESH CD80 CASE7 certificate PASS checks=%u\n",
           s_checks);
    return 0;
}

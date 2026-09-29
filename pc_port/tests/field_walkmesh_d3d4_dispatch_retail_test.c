/*
 * Retail certificate for func_8007D3D4's eight-way sideMask dispatch.
 *
 * Retail dispatch is `sltiu $v0, $s0, 0x8` and jtbl_8006FBEC has eight
 * entries whose targets map index -> body identically:
 *   0 -> 0x8007D5A8 steps = 0xFF
 *   1 -> 0x8007D5E8 nextTri = tri[3]
 *   2 -> 0x8007D61C nextTri = tri[4]
 *   3 -> 0x8007D5B0 nclip(p1,new,old) < 0 ? tri[3] : tri[4]
 *   4 -> 0x8007D638 nextTri = tri[5]
 *   5 -> 0x8007D5D0 nclip(p0,new,old) < 0 ? tri[5] : tri[3]
 *   6 -> 0x8007D604 nclip(p2,new,old) >= 0 ? tri[5] : tri[4]
 *   7 -> 0x8007D654 nextTri = -1
 *
 * The C source now writes the case labels in retail's emission order
 * 0,3,5,1,6,2,4,7 and recomputes `triBase + triIndex * 14 + offset` inside
 * each body (retail 0x8007D5E8/0x8007D61C/0x8007D638 share that address
 * computation with the conditional arms). This test drives the shipped
 * func_8007D3D4 with controlled NormalClip signs for every mask and checks
 * the accepted triangle and the number of consumed NCLIPs, so a wrong
 * case -> body assignment or a re-inlined address shape is caught.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgte.h"

extern s32 func_8007D3D4(u8* actorData, s32 idx, s32* outHeight0,
                         VECTOR* outNormal, s16* outTriangle, s32* outHeight1);

s32 D_800AFB24[4];
s32 D_800AFB34[4];
s32 D_800AFB44[4];
u32 D_800AFB20[8];
u8 D_800B21CC;

static s16 s_triangles[7 * 4];
static s16 s_vertices[3][4];
static u32 s_materials[1];
static u8 s_actor[0x140];
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
        fprintf(stderr, "ASSERTION d3d4.clip.count overflow=%d\n",
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

static void expect_s32(const char* case_name, const char* field,
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
    fail("d3d4.dispatch.result", detail);
}

/*
 * c0/c1/c2 select the seed mask; have4 (and c3) cover the conditional
 * masks 3/5/6 where retail issues a fourth NCLIP against the old point.
 * Three trailing +1 clips let the chosen neighbour be accepted (mask 0);
 * retail returns -1 for mask 7 without consuming them.
 */
static s32 run_dispatch(int c0, int c1, int c2, int have4, int c3,
                        s32* out_tri, int* out_clips)
{
    s32 height0 = 0;
    s32 height1 = 0;
    s16 triangle = -999;
    VECTOR normal;
    s32 ret;

    memset(&normal, 0, sizeof(normal));
    s_clip_values[0] = c0;
    s_clip_values[1] = c1;
    s_clip_values[2] = c2;
    s_clip_count = 3;
    if (have4) {
        s_clip_values[s_clip_count++] = c3;
    }
    s_clip_values[s_clip_count++] = 1;
    s_clip_values[s_clip_count++] = 1;
    s_clip_values[s_clip_count++] = 1;
    s_clip_index = 0;

    ret = func_8007D3D4(s_actor, 0, &height0, &normal, &triangle, &height1);
    *out_tri = triangle;
    *out_clips = s_clip_index;
    return ret;
}

static void check_case(const char* name, int c0, int c1, int c2,
                       int have4, int c3, s32 exp_ret, s32 exp_tri,
                       int exp_clips)
{
    s32 tri = 0;
    int clips = 0;
    s32 ret = run_dispatch(c0, c1, c2, have4, c3, &tri, &clips);

    expect_s32(name, "ret", ret, exp_ret);
    expect_s32(name, "outTriangle", tri, exp_tri);
    expect_s32(name, "clips", clips, exp_clips);
}

int main(void)
{
    uintptr_t tri_addr;
    uintptr_t vert_addr;
    uintptr_t material_addr;
    int i;

    memset(s_triangles, 0, sizeof(s_triangles));
    memset(s_vertices, 0, sizeof(s_vertices));
    memset(s_materials, 0, sizeof(s_materials));
    memset(s_actor, 0, sizeof(s_actor));

    /* T0 (seed) at s16 offset 0: neighbours 1, 2, 3 for tri[3], tri[4], tri[5]. */
    s_triangles[0] = 0;
    s_triangles[1] = 1;
    s_triangles[2] = 2;
    s_triangles[3] = 1;
    s_triangles[4] = 2;
    s_triangles[5] = 3;
    s_triangles[6] = 0;
    /* T1/T2/T3: distinct, front-facing acceptors. */
    for (i = 1; i < 4; i++) {
        s_triangles[i * 7 + 0] = 0;
        s_triangles[i * 7 + 1] = 1;
        s_triangles[i * 7 + 2] = 2;
        s_triangles[i * 7 + 3] = -1;
        s_triangles[i * 7 + 4] = -1;
        s_triangles[i * 7 + 5] = -1;
        s_triangles[i * 7 + 6] = 0;
    }

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

    *(s16*)(s_actor + 0x08) = 0;   /* seed triIndex */
    *(s16*)(s_actor + 0x10) = 0;   /* == idx: same-actor height path */
    *(u32*)(s_actor + 0x04) = 0;   /* collisionMask enable bit clear */
    *(s32*)(s_actor + 0x20) = 0;
    *(s32*)(s_actor + 0x28) = 0;
    *(s32*)(s_actor + 0x30) = 0;
    *(s32*)(s_actor + 0x34) = 0;
    *(s32*)(s_actor + 0x38) = 0;
    *(s16*)(s_actor + 0x72) = 0;

    /* mask 0: inside the seed, accept it (3 clips). */
    check_case("mask0", 1, 1, 1, 0, 0, 0, 0, 3);
    /* mask 1/2/4: single neighbour (3 + 3 clips). */
    check_case("mask1", -1, 1, 1, 0, 0, 0, 1, 6);
    check_case("mask2", 1, -1, 1, 0, 0, 0, 2, 6);
    check_case("mask4", 1, 1, -1, 0, 0, 0, 3, 6);
    /* mask 3: nclip(p1,new,old) < 0 picks tri[3], else tri[4] (3 + 1 + 3). */
    check_case("mask3.tri3", -1, -1, 1, 1, -1, 0, 1, 7);
    check_case("mask3.tri4", -1, -1, 1, 1, 1, 0, 2, 7);
    /* mask 5: nclip(p0,new,old) < 0 picks tri[5], else tri[3]. */
    check_case("mask5.tri5", -1, 1, -1, 1, -1, 0, 3, 7);
    check_case("mask5.tri3", -1, 1, -1, 1, 1, 0, 1, 7);
    /* mask 6: nclip(p2,new,old) >= 0 picks tri[5], else tri[4]. */
    check_case("mask6.tri5", 1, -1, -1, 1, 1, 0, 3, 7);
    check_case("mask6.tri4", 1, -1, -1, 1, -1, 0, 2, 7);
    /* mask 7: nextTri = -1, return -1, no neighbour walk. */
    check_case("mask7", -1, -1, -1, 0, 0, -1, -999, 3);

    printf("FIELD WALKMESH D3D4 DISPATCH certificate PASS checks=%u\n",
           s_checks);
    return 0;
}

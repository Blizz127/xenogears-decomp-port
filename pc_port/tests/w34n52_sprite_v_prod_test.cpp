#include "PsyX/PsyX_render.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int g_cfg_bilinearFiltering = 0;

void MakeTexcoordRect(GrVertex* vertex, unsigned char* uv, short page,
                      short clut, short w, short h);

static int s_failures;

static void check(const char* name, bool condition)
{
    if (!condition) {
        fprintf(stderr, "W34N52_ASSERT_FAIL:%s\n", name);
        s_failures++;
    }
}

static void test_world_label_sprite(void)
{
    struct GuardedVertices {
        uint32_t before;
        GrVertex vertices[4];
        uint32_t after;
    } fixture;
    unsigned char uv[2] = { 0u, 128u };

    memset(&fixture, 0, sizeof(fixture));
    fixture.before = 0x13579bdfu;
    fixture.after = 0x2468ace0u;
    for (int i = 0; i < 4; i++)
        fixture.vertices[i]._p1 = 0x5a;
    g_cfg_bilinearFiltering = 0;
    MakeTexcoordRect(fixture.vertices, uv, 0x001f, 0x7813, 104, 13);

    check("label.vertical_direction",
          fixture.vertices[0].v < fixture.vertices[1].v);
    check("label.top_v_is_base",
          fixture.vertices[0].v == 128u && fixture.vertices[3].v == 128u);
    check("label.bottom_v_uses_exclusive_height",
          fixture.vertices[1].v == 141u && fixture.vertices[2].v == 141u);
    check("label.left_u_is_base",
          fixture.vertices[0].u == 0u && fixture.vertices[1].u == 0u);
    check("label.right_u_uses_width",
          fixture.vertices[2].u == 104u && fixture.vertices[3].u == 104u);
    check("label.input_read_only", uv[0] == 0u && uv[1] == 128u);
    check("label.write_bounds",
          fixture.before == 0x13579bdfu && fixture.after == 0x2468ace0u);

    for (int i = 0; i < 4; i++) {
        check("label.page", fixture.vertices[i].page == 0x001f);
        check("label.clut", fixture.vertices[i].clut == 0x7813);
        check("label.brightness", fixture.vertices[i].bright == 2u);
        check("label.no_dither", fixture.vertices[i].dither == 0u);
        check("label.not_fixed_uv_polygon", fixture.vertices[i]._p0 == 0);
        check("label.raw_identity_clear", fixture.vertices[i]._p1 == 0);
    }
}

static void test_uv_page_clamp(void)
{
    GrVertex vertices[4];
    unsigned char uv[2] = { 250u, 252u };

    memset(vertices, 0, sizeof(vertices));
    g_cfg_bilinearFiltering = 0;
    MakeTexcoordRect(vertices, uv, 3, 5, 16, 16);

    check("clamp.left_u", vertices[0].u == 250u && vertices[1].u == 250u);
    check("clamp.right_u", vertices[2].u == 255u && vertices[3].u == 255u);
    check("clamp.top_v", vertices[0].v == 252u && vertices[3].v == 252u);
    check("clamp.bottom_v", vertices[1].v == 255u && vertices[2].v == 255u);
}

static void test_bilinear_metadata(void)
{
    GrVertex vertices[4];
    unsigned char uv[2] = { 7u, 11u };

    memset(vertices, 0, sizeof(vertices));
    g_cfg_bilinearFiltering = 1;
    MakeTexcoordRect(vertices, uv, 3, 5, 9, 13);

    for (int i = 0; i < 4; i++) {
        check("bilinear.tcx", vertices[i].tcx == -1);
        check("bilinear.tcy", vertices[i].tcy == -1);
    }
    check("bilinear.orientation",
          vertices[0].v == 11u && vertices[1].v == 24u
          && vertices[2].v == 24u && vertices[3].v == 11u);
}

int main(void)
{
    test_world_label_sprite();
    test_uv_page_clamp();
    test_bilinear_metadata();

    if (s_failures != 0) {
        fprintf(stderr, "W34N52 SPRITE V CERTIFICATE FAIL count=%d\n",
                s_failures);
        return 1;
    }

    puts("W34N52 SPRITE V CERTIFICATE PASS");
    return 0;
}

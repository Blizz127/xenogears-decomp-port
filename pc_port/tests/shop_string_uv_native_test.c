/* Native semantic differential for shop func_801C5A7C.
 *
 * The actual production C (src/shop_menu/main/misc.c) is called directly;
 * an independently written reference model recomputes every observable
 * store from the documented semantics (docs/evidence/shop-string-uv-
 * 20260906/README.md). Compared per poly: rgb, prim code, tpage, all four
 * UV corners, clut, plus unk7C/unk7F and preservation of untouched screen
 * coordinates.
 *
 * NOT an instruction differential: value-identical shapes the retail
 * encoding distinguishes (u16 vs s32 blend; mid-block u copy) are covered
 * by static source-shape pins in the runner script. Retail byte parity
 * itself (576 bytes, d71c52a1...) was proven by the GCC 2.6 build.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"
#include "system/menu.h"

extern void func_801C5A7C(MenuString *pString, s32 index, s32 offset, u8 flags);
u16 g_SystemPalette1;
u16 g_SystemPalette2;

/* Retail helper semantics from the PsyQ header formulas; the shared spec.
 * Logic under test (branches, UV math, palette selection) is re-derived
 * independently in reference() below. */
u_short GetTPage(int tp, int abr, int x, int y)
{
    return (u_short)((((tp) & 0x3) << 7) | (((abr) & 0x3) << 5) |
                     (((y) & 0x100) >> 4) | (((x) & 0x3ff) >> 6) |
                     (((y) & 0x200) << 2));
}

void SetPolyFT4(POLY_FT4 *p)
{
    p->code = 0x2C;
}

void SetSemiTrans(void *p, int abe)
{
    POLY_FT4 *q = (POLY_FT4 *)p;
    if (abe)
        q->code = (u_char)(q->code | 0x02);
    else
        q->code = (u_char)(q->code & (u_char)~0x02);
}

void SetShadeTex(void *p, int tge)
{
    POLY_FT4 *q = (POLY_FT4 *)p;
    if (tge)
        q->code = (u_char)(q->code | 0x01);
    else
        q->code = (u_char)(q->code & (u_char)~0x01);
}

static unsigned cases;
static unsigned checks;

static void require(int ok, const char *why)
{
    ++checks;
    if (!ok) {
        fprintf(stderr, "SHOP UV FAIL case=%u %s\n", cases, why);
        exit(1);
    }
}

typedef struct {
    u8 r, g, b, code;
    u16 tpage, clut;
    u8 u0, v0, u1, v1, u2, v2, u3, v3;
    short x0, y0, x1, y1, x2, y2, x3, y3;
} ExpectPoly;

typedef struct {
    ExpectPoly poly[2];
    u8 unk7C, unk7F;
} Expect;

/* Independent expectation, structured around outcomes rather than the
 * production source's statement order. All narrowing replicates u8/u16
 * store truncation. */
static void reference(Expect *e, s32 index, s32 offset, u8 flags, u8 width,
                      u16 pal1, u16 pal2)
{
    int odd = index & 1;
    int half = index / 2;
    int i;

    for (i = 0; i < 2; i++) {
        ExpectPoly *p = &e->poly[i];
        p->r = 128;
        p->g = 128;
        p->b = 128;
        p->code = 0x2C;
        p->x0 = 11;
        p->y0 = 12;
        p->x1 = 13;
        p->y1 = 14;
        p->x2 = 15;
        p->y2 = 16;
        p->x3 = 17;
        p->y3 = 18;
        if (flags == 0) {
            int row = (index + offset) / 4;
            int side = (half & 1) * 128;
            e->unk7C = (u8)odd;
            p->tpage = GetTPage(0, 0, 320, 0);
            p->u0 = (u8)side;
            p->v0 = (u8)(row * 13);
            p->u1 = (u8)(side + width);
            p->v1 = (u8)(row * 13);
            p->u2 = (u8)side;
            p->v2 = (u8)(row * 13 + 13);
            p->u3 = (u8)(side + width);
            p->v3 = (u8)(row * 13 + 13);
        } else {
            int base = half * 13 + offset;
            u16 blend = 0;
            if (!(flags & 0x80)) {
                blend = 0x20;
                p->code = (u8)(p->code | 0x02);
                p->r = 0x20;
                p->g = 0x20;
                p->b = 0x20;
            }
            e->unk7C = (u8)((flags & 0x7F) + 0xFF);
            p->tpage = (u16)(blend | GetTPage(0, 0, 384, 128));
            p->u0 = (u8)(odd * 96);
            p->v0 = (u8)base;
            p->u1 = (u8)(odd * 96 + width);
            p->v1 = (u8)base;
            p->u2 = (u8)(odd * 96);
            p->v2 = (u8)(base + 13);
            p->u3 = (u8)(odd * 96 + width);
            p->v3 = (u8)(base + 13);
        }
        p->clut = e->unk7C ? pal2 : pal1;
    }
    e->unk7F = 0;
}

/* Single-fault variants. Each must observably differ from reference() on at
 * least one suite case, proving the suite is sensitive to that fault class. */
static void mutant_staging_page(Expect *e, s32 index, s32 offset, u8 flags,
                                u8 width, u16 pal1, u16 pal2)
{
    int i;
    reference(e, index, offset, flags, width, pal1, pal2);
    if (flags == 0)
        for (i = 0; i < 2; i++)
            e->poly[i].tpage = GetTPage(0, 0, 192, 0);
}

static void mutant_u2_plus_width(Expect *e, s32 index, s32 offset, u8 flags,
                                 u8 width, u16 pal1, u16 pal2)
{
    int i;
    reference(e, index, offset, flags, width, pal1, pal2);
    if (flags == 0)
        for (i = 0; i < 2; i++)
            e->poly[i].u2 = (u8)(e->poly[i].u2 + width);
}

static void mutant_clut_swapped(Expect *e, s32 index, s32 offset, u8 flags,
                                u8 width, u16 pal1, u16 pal2)
{
    int i;
    reference(e, index, offset, flags, width, pal1, pal2);
    for (i = 0; i < 2; i++)
        e->poly[i].clut = e->unk7C ? pal1 : pal2;
}

static void mutant_unk7c_no_bias(Expect *e, s32 index, s32 offset, u8 flags,
                                 u8 width, u16 pal1, u16 pal2)
{
    int i;
    reference(e, index, offset, flags, width, pal1, pal2);
    if (flags != 0) {
        e->unk7C = (u8)(flags & 0x7F);
        for (i = 0; i < 2; i++)
            e->poly[i].clut = e->unk7C ? pal2 : pal1;
    }
}

static void mutant_semiforce(Expect *e, s32 index, s32 offset, u8 flags,
                             u8 width, u16 pal1, u16 pal2)
{
    int i;
    reference(e, index, offset, flags, width, pal1, pal2);
    if (flags == 0)
        for (i = 0; i < 2; i++)
            e->poly[i].code = (u8)(e->poly[i].code | 0x02);
}

static void mutant_v_ignores_offset(Expect *e, s32 index, s32 offset, u8 flags,
                                    u8 width, u16 pal1, u16 pal2)
{
    int row;
    reference(e, index, offset, flags, width, pal1, pal2);
    if (flags == 0) {
        int i;
        row = index / 4;
        for (i = 0; i < 2; i++) {
            e->poly[i].v0 = (u8)(row * 13);
            e->poly[i].v1 = (u8)(row * 13);
            e->poly[i].v2 = (u8)(row * 13 + 13);
            e->poly[i].v3 = (u8)(row * 13 + 13);
        }
    }
}

static int expect_equal(const Expect *a, const Expect *b)
{
    int i;
    if (a->unk7C != b->unk7C || a->unk7F != b->unk7F)
        return 0;
    for (i = 0; i < 2; i++) {
        const ExpectPoly *x = &a->poly[i];
        const ExpectPoly *y = &b->poly[i];
        if (x->r != y->r || x->g != y->g || x->b != y->b || x->code != y->code ||
            x->tpage != y->tpage || x->clut != y->clut || x->u0 != y->u0 ||
            x->v0 != y->v0 || x->u1 != y->u1 || x->v1 != y->v1 ||
            x->u2 != y->u2 || x->v2 != y->v2 || x->u3 != y->u3 ||
            x->v3 != y->v3 || x->x0 != y->x0 || x->y0 != y->y0 ||
            x->x1 != y->x1 || x->y1 != y->y1 || x->x2 != y->x2 ||
            x->y2 != y->y2 || x->x3 != y->x3 || x->y3 != y->y3)
            return 0;
    }
    return 1;
}

static void snapshot(const MenuString *s, Expect *e)
{
    int i;
    for (i = 0; i < 2; i++) {
        const POLY_FT4 *p = &s->polys[i];
        ExpectPoly *q = &e->poly[i];
        q->r = p->r0;
        q->g = p->g0;
        q->b = p->b0;
        q->code = p->code;
        q->tpage = p->tpage;
        q->clut = p->clut;
        q->u0 = p->u0;
        q->v0 = p->v0;
        q->u1 = p->u1;
        q->v1 = p->v1;
        q->u2 = p->u2;
        q->v2 = p->v2;
        q->u3 = p->u3;
        q->v3 = p->v3;
        q->x0 = p->x0;
        q->y0 = p->y0;
        q->x1 = p->x1;
        q->y1 = p->y1;
        q->x2 = p->x2;
        q->y2 = p->y2;
        q->x3 = p->x3;
        q->y3 = p->y3;
    }
    e->unk7C = s->unk7C;
    e->unk7F = s->unk7F;
}

static void check_field(int same, const char *why)
{
    require(same, why);
}

static void run_case(s32 index, s32 offset, int flags_in, u8 width, u16 pal1,
                     u16 pal2, int *sens)
{
    static void (*mutants[])(Expect *, s32, s32, u8, u8, u16, u16) = {
        mutant_staging_page, mutant_u2_plus_width, mutant_clut_swapped,
        mutant_unk7c_no_bias, mutant_semiforce, mutant_v_ignores_offset,
    };
    MenuString s;
    Expect want, got, alt;
    unsigned m;
    u8 flags = (u8)flags_in;
    int i;

    ++cases;
    memset(&s, 0xA5, sizeof(s));
    for (i = 0; i < 2; i++) {
        s.polys[i].x0 = 11;
        s.polys[i].y0 = 12;
        s.polys[i].x1 = 13;
        s.polys[i].y1 = 14;
        s.polys[i].x2 = 15;
        s.polys[i].y2 = 16;
        s.polys[i].x3 = 17;
        s.polys[i].y3 = 18;
    }
    s.width = width;
    s.unk7C = 0x5A;
    s.unk7F = 0x5A;
    g_SystemPalette1 = pal1;
    g_SystemPalette2 = pal2;
    reference(&want, index, offset, flags, width, pal1, pal2);
    func_801C5A7C(&s, index, offset, flags);
    snapshot(&s, &got);
    check_field(got.poly[0].r == want.poly[0].r &&
                got.poly[1].r == want.poly[1].r, "r0");
    check_field(got.poly[0].g == want.poly[0].g &&
                got.poly[1].g == want.poly[1].g, "g0");
    check_field(got.poly[0].b == want.poly[0].b &&
                got.poly[1].b == want.poly[1].b, "b0");
    check_field(got.poly[0].code == want.poly[0].code &&
                got.poly[1].code == want.poly[1].code, "prim code");
    check_field(got.poly[0].tpage == want.poly[0].tpage &&
                got.poly[1].tpage == want.poly[1].tpage, "tpage");
    check_field(got.poly[0].u0 == want.poly[0].u0 &&
                got.poly[1].u0 == want.poly[1].u0, "u0");
    check_field(got.poly[0].v0 == want.poly[0].v0 &&
                got.poly[1].v0 == want.poly[1].v0, "v0");
    check_field(got.poly[0].u1 == want.poly[0].u1 &&
                got.poly[1].u1 == want.poly[1].u1, "u1");
    check_field(got.poly[0].v1 == want.poly[0].v1 &&
                got.poly[1].v1 == want.poly[1].v1, "v1");
    check_field(got.poly[0].u2 == want.poly[0].u2 &&
                got.poly[1].u2 == want.poly[1].u2, "u2");
    check_field(got.poly[0].v2 == want.poly[0].v2 &&
                got.poly[1].v2 == want.poly[1].v2, "v2");
    check_field(got.poly[0].u3 == want.poly[0].u3 &&
                got.poly[1].u3 == want.poly[1].u3, "u3");
    check_field(got.poly[0].v3 == want.poly[0].v3 &&
                got.poly[1].v3 == want.poly[1].v3, "v3");
    check_field(got.poly[0].clut == want.poly[0].clut &&
                got.poly[1].clut == want.poly[1].clut, "clut");
    check_field(got.unk7C == want.unk7C, "unk7C");
    check_field(got.unk7F == want.unk7F, "unk7F");
    check_field(got.poly[0].x0 == 11 && got.poly[0].y0 == 12 &&
                got.poly[1].x3 == 17 && got.poly[1].y3 == 18,
                "screen coords preserved");
    for (m = 0; m < 6; m++) {
        mutants[m](&alt, index, offset, flags, width, pal1, pal2);
        if (!expect_equal(&alt, &want))
            sens[m] = 1;
    }
}

int main(void)
{
    static const s32 indexes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 31,
                                  -1, -2, -3, -4, -5, -8, 100, 1000, 32767};
    static const s32 offsets[] = {0, 1, -1, 3, 4, -4, 13, -13, 100, -100};
    static const int flags[] = {0, 1, 2, 0x1F, 0x7F, 0x80, 0x81, 0xFF, 0x20,
                                0xA5};
    static const u8 widths[] = {0, 1, 8, 12, 96, 255};
    static const u16 pals[][2] = {{0x1111, 0x2222}, {0x0000, 0xFFFF},
                                  {0x1234, 0x1234}};
    static const char *mutant_names[] = {"staging-page", "u2-plus-width",
        "clut-swapped", "unk7c-no-bias", "semiforce", "v-ignores-offset"};
    int sens[6] = {0, 0, 0, 0, 0, 0};
    size_t ai, bi, ci, di, ei;
    unsigned m;

    for (ai = 0; ai < sizeof(indexes) / sizeof(indexes[0]); ai++)
        for (bi = 0; bi < sizeof(offsets) / sizeof(offsets[0]); bi++)
            for (ci = 0; ci < sizeof(flags) / sizeof(flags[0]); ci++)
                for (di = 0; di < sizeof(widths) / sizeof(widths[0]); di++) {
                    ei = (ai + bi + ci + di) % 3;
                    run_case(indexes[ai], offsets[bi], flags[ci],
                             widths[di], pals[ei][0], pals[ei][1], sens);
                }
    for (m = 0; m < 6; m++) {
        char msg[96];
        snprintf(msg, sizeof(msg), "mutant never distinguished: %s",
                 mutant_names[m]);
        require(sens[m], msg);
    }
    printf("SHOP UV NATIVE cases=%u checks=%u mutants=6/6 distinguished\n",
           cases, checks);
    return 0;
}

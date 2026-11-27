#ifndef XENO_RENDERER_GTE_COMPOSITES_H
#define XENO_RENDERER_GTE_COMPOSITES_H

#include "libgte.h"
#include "inline_c.h"

/* Renderer entry points in LIBGTE.C use these compound GTE operations.
 * Register assignments and command encodings are documented in PSX-SPX:
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/ */
static inline void gte_RotTrans(const void *point, void *transformed, void *flags)
{
    gte_ldv0(point);
    gte_rt();
    gte_stlvnl(transformed);
    gte_stflg(flags);
}

/* Both projection commands leave the final vertex's cue, flags and depth in
 * the same registers. The caller uses ordering-table depth (SZ3 / 4). */
static inline void xeno_gte_projection_depth(void *cue, void *flags, void *depth)
{
    gte_stdp(cue);
    gte_stflg(flags);
    gte_stszotz(depth);
}

static inline void gte_RotTransPers(const void *point, void *xy, void *cue,
                                    void *flags, void *depth)
{
    gte_ldv0(point);
    gte_rtps();
    gte_stsxy(xy);
    xeno_gte_projection_depth(cue, flags, depth);
}

static inline void gte_RotTransPers3(const void *a, const void *b, const void *c,
                                     void *xy_a, void *xy_b, void *xy_c,
                                     void *cue, void *flags, void *depth)
{
    gte_ldv3(a, b, c);
    gte_rtpt();
    gte_stsxy3(xy_a, xy_b, xy_c);
    xeno_gte_projection_depth(cue, flags, depth);
}

static inline void gte_NormalClip(const void *a, const void *b, const void *c, void *area)
{
    MTC2(xeno_gte_load_word(a), 12);
    MTC2(xeno_gte_load_word(c), 14);
    MTC2(xeno_gte_load_word(b), 13);
    gte_nclip();
    gte_stopz(area);
}

static inline void gte_LocalLight(const void *normal, VECTOR *light)
{
    gte_ldv0(normal);
    gte_mvmva(1, 1, 0, 3, 1);  /* light matrix times V0, positive IR saturation */
    gte_stlvl(light);
}

static inline void xeno_gte_color_fifo(void *oldest, void *middle, void *newest)
{
    xeno_gte_store_word(oldest, MFC2(20));
    xeno_gte_store_word(middle, MFC2(21));
    gte_strgb(newest);
}

static inline void gte_NormalColor(const void *normal, void *out)
{
    gte_ldv0(normal);
    gte_ncs();
    gte_strgb(out);
}

static inline void gte_NormalColor3(const void *a, const void *b, const void *c,
                                    void *color_a, void *color_b, void *color_c)
{
    gte_ldv3(a, b, c);
    gte_nct();
    xeno_gte_color_fifo(color_a, color_b, color_c);
}

static inline void gte_NormalColorCol(const void *normal, const void *base, void *out)
{
    gte_ldv0(normal);
    gte_ldrgb(base);
    gte_nccs();
    gte_strgb(out);
}

static inline void gte_NormalColorCol3(const void *a, const void *b, const void *c,
                                       const void *base,
                                       void *color_a, void *color_b, void *color_c)
{
    gte_ldv3(a, b, c);
    gte_ldrgb(base);
    gte_ncct();
    xeno_gte_color_fifo(color_a, color_b, color_c);
}

static inline void gte_NormalColorDpq(const void *normal, const void *base, int cue, void *out)
{
    gte_ldv0(normal);
    gte_ldrgb(base);
    MTC2((unsigned int)cue, 8);
    doCOP2(0xe80413);  /* NCDS */
    gte_strgb(out);
}

static inline void gte_ColorCol(const VECTOR *light, const void *base, void *out)
{
    gte_ldlvl(light);
    gte_ldrgb(base);
    doCOP2(0x138041c);  /* CC */
    gte_strgb(out);
}

static inline void gte_ColorDpq(const VECTOR *light, const void *base, int cue, void *out)
{
    gte_ldlvl(light);
    gte_ldrgb(base);
    MTC2((unsigned int)cue, 8);
    doCOP2(0x1280414);  /* CDP */
    gte_strgb(out);
}

static inline void gte_DpqColor(const void *base, const int *cue, void *out)
{
    gte_ldrgb(base);
    gte_lddp(cue);
    gte_dpcs();
    gte_strgb(out);
}

static inline void gte_MulMatrix0(const MATRIX *left, const MATRIX *right, MATRIX *out)
{
    gte_SetRotMatrix(left);
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row)
            MTC2((unsigned short)right->m[row][col], 9 + row);
        gte_rtir();
        for (int row = 0; row < 3; ++row)
            out->m[row][col] = (short)MFC2(9 + row);
    }
}

#endif

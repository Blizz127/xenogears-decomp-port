#include "../../include/xg_plat/hd2d.h"
#include <math.h>
#include <string.h>
#define BIT(e) (UINT32_C(1) << (e))
void XgHd2d_DefaultConfig(XgHd2dConfig *c)
{
    unsigned i;
    memset(c, 0, sizeof(*c));
    for (i = 0; i < XG_HD2D_EFFECT_COUNT; ++i) c->effect[i].intensity = 0.25f;
    c->focus_depth = 5.0f; c->focus_range = 2.0f;
    c->fog_start = 12.0f; c->fog_end = 40.0f;
    c->order_focus=0.5f; c->order_range=0.2f;
    c->order_fog_start=0.3f; c->order_fog_end=0.9f;
}
XgHd2dPlan XgHd2d_Plan(const XgHd2dConfig *c, const XgHd2dFrame *f)
{
    XgHd2dPlan p = {0};
    unsigned i;
    const uint32_t depth = BIT(XG_HD2D_DOF) | BIT(XG_HD2D_FOG) | BIT(XG_HD2D_PARTICLES);
    const uint32_t protection = (BIT(XG_HD2D_EFFECT_COUNT)-1);
    if (!c || !c->enabled || !f || !f->field_active || f->width <= 0 ||
        f->height <= 0 || f->x < 0 || f->y < 0 ||
        c->map_count == 0 || c->map_count > XG_HD2D_MAX_MAPS) return p;
    for (i = 0; i < c->map_count && c->maps[i] != f->field_id; ++i) {}
    if (i == c->map_count) return p;
    for (i = 0; i < XG_HD2D_EFFECT_COUNT; ++i) {
        float v = c->effect[i].intensity;
        if (c->effect[i].enabled && isfinite(v) && v > 0.0f) {
            p.active |= BIT(i); p.intensity[i] = v > 1.0f ? 1.0f : v;
        }
    }
    if (!f->linear_depth_texture || !f->linear_depth_verified ||
        (f->depth_mode!=XG_HD2D_DEPTH_LINEAR && f->depth_mode!=XG_HD2D_DEPTH_OT_ORDER) ||
        (f->depth_mode==XG_HD2D_DEPTH_OT_ORDER && (!isfinite(f->order_scale) || f->order_scale<=0))) {
        p.missing_depth = p.active & depth; p.active &= ~depth;
    }
    if (!f->protection_texture || !f->protection_verified ||
        (f->protection_mode!=XG_HD2D_PROTECTION_BINARY && f->protection_mode!=XG_HD2D_PROTECTION_COVERAGE_BITS)) {
        p.missing_protection = p.active & protection; p.active &= ~protection;
    }
    if ((f->auxiliary_width || f->auxiliary_height) &&
        (f->auxiliary_width < f->width || f->auxiliary_height < f->height ||
         f->x > f->auxiliary_width-f->width || f->y > f->auxiliary_height-f->height)) p.active=0;
    if (f->depth_mode==XG_HD2D_DEPTH_OT_ORDER) {
        if (!isfinite(c->order_focus) || !isfinite(c->order_range) || !isfinite(c->order_tilt) ||
            c->order_focus<0 || c->order_focus>1 || c->order_range<=0 || c->order_range>1)
            p.active &= ~BIT(XG_HD2D_DOF);
        if (!isfinite(c->order_fog_start) || !isfinite(c->order_fog_end) ||
            c->order_fog_start<0 || c->order_fog_end>1 || c->order_fog_end<=c->order_fog_start)
            p.active &= ~BIT(XG_HD2D_FOG);
        /* Projected particles currently carry physical positive view depth. */
        p.active &= ~BIT(XG_HD2D_PARTICLES);
    } else {
        if (!isfinite(c->focus_depth) || !isfinite(c->focus_range) || !isfinite(c->focus_tilt) ||
            c->focus_depth<=0 || c->focus_range<=0) p.active &= ~BIT(XG_HD2D_DOF);
        if (!isfinite(c->fog_start) || !isfinite(c->fog_end) ||
            c->fog_start<0 || c->fog_end<=c->fog_start) p.active &= ~BIT(XG_HD2D_FOG);
    }
    if (f->shadow_count == 0 || f->shadow_count > XG_HD2D_MAX_ITEMS) p.active &= ~BIT(XG_HD2D_SHADOW);
    if (f->particle_count == 0 || f->particle_count > XG_HD2D_MAX_ITEMS) p.active &= ~BIT(XG_HD2D_PARTICLES);
    for (i = 0; i < XG_HD2D_EFFECT_COUNT; ++i)
        if (!(p.active & BIT(i))) p.intensity[i] = 0.0f;
    return p;
}

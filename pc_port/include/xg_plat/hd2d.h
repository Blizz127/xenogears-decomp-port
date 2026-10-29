#ifndef XG_PLAT_HD2D_H
#define XG_PLAT_HD2D_H
/* Optional presentation-only effects. No retail state or environment access. */
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define XG_HD2D_MAX_MAPS 32
#define XG_HD2D_MAX_ITEMS 16
typedef enum XgHd2dEffect {
    XG_HD2D_DOF, XG_HD2D_BLOOM, XG_HD2D_LIGHT, XG_HD2D_SHADOW,
    XG_HD2D_VIGNETTE, XG_HD2D_FOG, XG_HD2D_PARTICLES, XG_HD2D_GRADE,
    XG_HD2D_EFFECT_COUNT
} XgHd2dEffect;
typedef enum XgHd2dDepthMode {
    XG_HD2D_DEPTH_LINEAR = 0, /* GL_R32F, positive linear view distance */
    XG_HD2D_DEPTH_OT_ORDER = 1 /* GL_R16, normalized order, larger is nearer */
} XgHd2dDepthMode;
typedef enum XgHd2dProtectionMode {
    XG_HD2D_PROTECTION_BINARY = 0,
    /* R8 byte: sprite1/map2/UI4/override8. Only exact map2 is editable;
     * zero, unknown bits and mixed coverage are protected. */
    XG_HD2D_PROTECTION_COVERAGE_BITS = 1
} XgHd2dProtectionMode;
typedef struct XgHd2dControl { int enabled; float intensity; } XgHd2dControl;
typedef struct XgHd2dConfig {
    int enabled; /* zero means no GL calls, queries, or allocations */
    XgHd2dControl effect[XG_HD2D_EFFECT_COUNT];
    unsigned map_count;
    int maps[XG_HD2D_MAX_MAPS]; /* exact IDs; empty is fail closed */
    float focus_depth, focus_range, fog_start, fog_end;
    /* DOF focal-plane slope in depth units, NOT a substitute for depth. */
    float focus_tilt;
    /* OT proxy distance = 1-normalized_order; no physical units implied.
     * Independent parameters never reinterpret the positive-linear defaults. */
    float order_focus, order_range, order_tilt, order_fog_start, order_fog_end;
} XgHd2dConfig;
/* All positions: normalized game viewport coordinates, origin lower left.
 * Point light: x,y,radius,strength. Shadow: foot x,y,radius x,radius y.
 * Particle: x,y,positive linear view depth,radius in normalized viewport.
 * All items must be projected from the current scene; no guessed actor feet. */
typedef struct XgHd2dItem { float x, y, z, w; } XgHd2dItem;
typedef struct XgHd2dFrame {
    int field_active, field_id, x, y, width, height;
    /* Exact current GL_TEXTURE_2D attachments (not multisampled), legacy
     * viewport-sized or explicit full-drawable auxiliary dimensions below.
     * Legacy depth is GL_R32F positive view distance (0 = invalid); explicit
     * OT mode is GL_R16 ordinal order (0 invalid/background, larger is nearer).
     * Protection must be GL_R8. Binary mode: nonzero protects ALL visible
     * sprites/UI, zero is environment. Coverage mode: only exact byte2 is
     * certified map-only coverage; every other byte is protected.
     * Caller certifies same frame/camera/viewport; never reuse across maps. */
    unsigned linear_depth_texture, protection_texture;
    int linear_depth_verified, protection_verified;
    unsigned light_count, shadow_count, particle_count;
    XgHd2dItem lights[XG_HD2D_MAX_ITEMS];
    XgHd2dItem shadows[XG_HD2D_MAX_ITEMS];
    XgHd2dItem particles[XG_HD2D_MAX_ITEMS];
    XgHd2dDepthMode depth_mode;
    XgHd2dProtectionMode protection_mode;
    /* Both zero: legacy viewport-size attachments. Both positive: shared
     * full drawable attachment size, sampled at viewport x/y plus local pixel. */
    int auxiliary_width, auxiliary_height;
    /* OT raw normalized texel multiplied by this positive scale then clamped
     * to [0,1]. Use 1 for full-range encoding, 65535/ot_max for raw slot codes. */
    float order_scale;
} XgHd2dFrame;
typedef struct XgHd2dPlan {
    uint32_t active, missing_depth, missing_protection;
    float intensity[XG_HD2D_EFFECT_COUNT];
} XgHd2dPlan;
void XgHd2d_DefaultConfig(XgHd2dConfig *config);
XgHd2dPlan XgHd2d_Plan(const XgHd2dConfig *config, const XgHd2dFrame *frame);
/* Desktop GL 3.3, current context, default framebuffer backbuffer.
 * Must run after retail framebuffer storage and before capture/host toolbar.
 * Returns 1 drawn, 0 bypassed, -1 unsupported/input/shader failure.
 * Renderer belongs to a single GL context; shutdown before destroying it.
 * Shutdown is explicit; turning master off itself never deletes GL resources. */
int XgHd2d_Present(const XgHd2dConfig *config, const XgHd2dFrame *frame);
void XgHd2d_Shutdown(void);
const char *XgHd2d_LastError(void);
#ifdef __cplusplus
}
#endif
#endif

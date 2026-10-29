#ifndef PC_PORT_HD2D_SPRITE_PARTS_H
#define PC_PORT_HD2D_SPRITE_PARTS_H
#include <stddef.h>
#include <stdint.h>
#include "hd2d_sprite.h"
/* Exact traced identities and retail UVs; screen geometry belongs to retail. */
typedef struct {
    uint32_t identity;
    int map, actor, frame, part;
    uint16_t tpage, clut;
    uint8_t u[4], v[4];
    uint16_t x, y, width, height;
} PcPortHd2dPartMapping;
typedef struct {
    uint16_t width, height;
    const PcPortHd2dPartMapping *parts;
    size_t count;
    const char *rgba_path;
} PcPortHd2dPartProfile;
#ifdef __cplusplus
extern "C" {
#endif
int PcPort_Hd2dPartProfileValidate(const PcPortHd2dPartProfile *profile);
const PcPortHd2dPartMapping *PcPort_Hd2dPartLookup(
    const PcPortHd2dPartProfile *profile, int map,
    const PcPortHd2dSpriteInfo *info, int part, const PcPortHd2dRetailQuad *quad);
/* Renderer transport must copy the complete retail packet, link at quad->ot,
 * preserve colour/code/tpage/ABR and identify by packet pointer until GPU flush.
 * RGBA upload must restore GL state, use nearest and alpha < .5 discard.
 * Return zero without mutation when transport/assets are unavailable. */
int PcPort_Hd2dSpriteSubmitPart(const PcPortHd2dPartProfile *profile,
    const PcPortHd2dPartMapping *mapping, const PcPortHd2dRetailQuad *quad);
int PcPort_Hd2dSpriteTextureOverride(const void *packet, int tpage, int clut,
    int u0, int v0, int u1, int v1, unsigned *texture, float uv[4]);
/* Presentation retires consumed entries only; preserves next-frame packets
 * built before the previous frame was presented. GPU OT-walk end retires seen
 * entries; hd2d_sprite_diagnostics.h exposes explicit enable-transition reset. */
void PcPort_Hd2dSpritePresentDone(void);
#ifdef __cplusplus
}
#endif
#endif

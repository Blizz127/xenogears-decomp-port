#include "hd2d_sprite_parts.h"
#include "hd2d_sprite_profile.h"
#include "hd2d_integration.h"
#include <string.h>
#pragma weak PcPort_Hd2dSpriteSubmitPart

static int SameKey(const PcPortHd2dPartMapping *a, const PcPortHd2dPartMapping *b)
{
    return a->identity == b->identity && a->map == b->map &&
        a->actor == b->actor && a->frame == b->frame && a->part == b->part;
}
int PcPort_Hd2dPartProfileValidate(const PcPortHd2dPartProfile *p)
{
    size_t i, j;
    if (!p || !p->count || p->count > 16384 || !p->parts ||
        !p->rgba_path || strncmp(p->rgba_path, "pc_port/assets/hd2d/", 20) ||
        strstr(p->rgba_path, "..") || !p->width || !p->height ||
        p->width > 4096 || p->height > 4096) return 0;
    for (i = 0; i < p->count; ++i) {
        const PcPortHd2dPartMapping *m = p->parts + i;
        if (m->map < 0 || m->map > 65535 || m->actor < 0 || m->actor > 65535 ||
            m->frame < 0 || m->frame > 32767 || m->part < 0 || m->part >= 64 ||
            !m->width || !m->height || m->x + m->width > p->width ||
            m->y + m->height > p->height) return 0;
        for (j = 0; j < i; ++j) if (SameKey(m, p->parts + j)) return 0;
    }
    return 1;
}
static const PcPortHd2dPartMapping *LookupValidated(const PcPortHd2dPartProfile *p,
    int map, const PcPortHd2dSpriteInfo *info, int part, const PcPortHd2dRetailQuad *q)
{
    size_t i;
    if (!info || !q) return NULL;
    for (i = 0; i < p->count; ++i) {
        const PcPortHd2dPartMapping *m = p->parts + i;
        if (m->identity == info->identity && m->map == map && m->actor == info->actor &&
            m->frame == info->frame && m->part == part && m->tpage == q->tpage &&
            m->clut == q->clut && !memcmp(m->u, q->u, 4) && !memcmp(m->v, q->v, 4)) return m;
    }
    return NULL;
}
const PcPortHd2dPartMapping *PcPort_Hd2dPartLookup(const PcPortHd2dPartProfile *p,
    int map, const PcPortHd2dSpriteInfo *info, int part, const PcPortHd2dRetailQuad *q)
{
    return PcPort_Hd2dPartProfileValidate(p) ? LookupValidated(p,map,info,part,q) : NULL;
}
static int ProductionProfileValid(void)
{
    static int valid = -1;
    if (valid < 0) valid = PcPort_Hd2dPartProfileValidate(&hd2d_sprite_profile);
    return valid;
}
int PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo *info)
{
    if (!PcPort_Hd2dEnabled()) return 0;
    return info && PcPort_Hd2dOpeningActive() && PcPort_Hd2dSpriteSubmitPart &&
        ProductionProfileValid();
}
int PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo *info, int part,
    const PcPortHd2dRetailQuad *quad)
{
    const PcPortHd2dPartMapping *m;
    if (!PcPort_Hd2dEnabled()) return 0;
    if (!PcPort_Hd2dOpeningActive() || !PcPort_Hd2dSpriteSubmitPart || !ProductionProfileValid()) return 0;
    m = LookupValidated(&hd2d_sprite_profile, PcPort_Hd2dCurrentMap(), info, part, quad);
    return m ? PcPort_Hd2dSpriteSubmitPart(&hd2d_sprite_profile, m, quad) : 0;
}
void PcPort_Hd2dSpriteEnd(const PcPortHd2dSpriteInfo *info) { (void)info; }

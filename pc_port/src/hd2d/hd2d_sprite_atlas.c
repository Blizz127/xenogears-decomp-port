#include "hd2d_sprite_atlas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int XgHd2dAtlasValidate(const XgHd2dSpriteAtlas *a)
{
    unsigned i, j;
    if (!a || !a->width || !a->height || a->width > XG_HD2D_ATLAS_MAX_DIM ||
        a->height > XG_HD2D_ATLAS_MAX_DIM || !a->frame_count ||
        a->frame_count > XG_HD2D_ATLAS_MAX_FRAMES || !a->frames ||
        !a->map_count || a->map_count > XG_HD2D_ATLAS_MAX_MAPS || !a->maps) return 0;
    for (i = 0; i < a->map_count; ++i)
        for (j = 0; j < i; ++j) if (a->maps[i] == a->maps[j]) return 0;
    for (i = 0; i < a->frame_count; ++i) {
        const XgHd2dSpriteFrame *f = &a->frames[i];
        if (f->facing >= 16 || !f->width || !f->height ||
            (unsigned)f->x + f->width > a->width ||
            (unsigned)f->y + f->height > a->height ||
            f->foot_x > f->width || f->foot_y > f->height) return 0;
        for (j = 0; j < i; ++j) {
            const XgHd2dSpriteFrame *p = &a->frames[j];
            if (f->animation == p->animation && f->cursor == p->cursor &&
                f->facing == p->facing) return 0;
        }
    }
    return 1;
}
const XgHd2dSpriteFrame *XgHd2dAtlasLookup(const XgHd2dSpriteAtlas *a,
    int enabled, unsigned map, unsigned animation, unsigned facing,
    unsigned cursor, unsigned parts)
{
    unsigned i;
    if (!enabled || !parts || parts > XG_HD2D_ATLAS_MAX_PARTS ||
        !XgHd2dAtlasValidate(a)) return NULL;
    for (i = 0; i < a->map_count && a->maps[i] != map; ++i) {}
    if (i == a->map_count) return NULL;
    for (i = 0; i < a->frame_count; ++i) {
        const XgHd2dSpriteFrame *f = &a->frames[i];
        if (f->animation == animation && f->facing == facing && f->cursor == cursor)
            return f;
    }
    return NULL;
}
int XgHd2dAtlasReadRgba(const XgHd2dSpriteAtlas *a, const char *path,
    unsigned char **pixels, size_t *bytes)
{
    FILE *file;
    unsigned char *data;
    size_t size;
    if (!pixels || !bytes) return 0;
    *pixels = NULL; *bytes = 0;
    if (!path || !XgHd2dAtlasValidate(a)) return 0;
    size = (size_t)a->width * a->height * 4;
    file = fopen(path, "rb");
    if (!file) return 0;
    data = malloc(size);
    if (!data) { fclose(file); return 0; }
    if (fread(data, 1, size, file) != size || fgetc(file) != EOF || ferror(file)) {
        free(data); fclose(file); return 0;
    }
    if (fclose(file)) { free(data); return 0; }
    *pixels = data; *bytes = size;
    return 1;
}

XgHd2dFacing XgHd2dAtlasCardinal(unsigned angle)
{
    angle &= 4095;
    return angle < 512 || angle >= 3584 ? XG_HD2D_RIGHT :
           angle < 1536 ? XG_HD2D_FRONT :
           angle < 2560 ? XG_HD2D_BACK : XG_HD2D_LEFT;
}
static int CanonicalHash(const char *hash, size_t length)
{
    size_t i;
    if (!hash || length != 64) return 0;
    for (i = 0; i < 64; ++i)
        if (!((hash[i] >= '0' && hash[i] <= '9') ||
              (hash[i] >= 'a' && hash[i] <= 'f'))) return 0;
    return 1;
}
const XgHd2dSpriteAtlas *XgHd2dAtlasFindSource(
    const XgHd2dSourceAssociation *sources, size_t count,
    int enabled, const char *hash, size_t hash_length, size_t bytes)
{
    size_t i, j;
    const XgHd2dSpriteAtlas *found = NULL;
    if (!enabled || !sources || !count || count > 256 || !bytes ||
        !CanonicalHash(hash, hash_length)) return NULL;
    /* Validate the complete association set; ambiguity fails closed. */
    for (i = 0; i < count; ++i) {
        const XgHd2dSourceAssociation *s = &sources[i];
        if (s->source_sha256[64] || !CanonicalHash(s->source_sha256, 64) ||
            !s->source_bytes || !XgHd2dAtlasValidate(s->atlas)) return NULL;
        for (j = 0; j < i; ++j)
            if (!memcmp(s->source_sha256, sources[j].source_sha256, 64)) return NULL;
        if (s->source_bytes == bytes && !memcmp(s->source_sha256, hash, 64))
            found = s->atlas;
    }
    return found;
}

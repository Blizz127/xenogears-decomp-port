#ifndef XG_HD2D_SPRITE_ATLAS_H
#define XG_HD2D_SPRITE_ATLAS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define XG_HD2D_ATLAS_MAX_DIM 4096
#define XG_HD2D_ATLAS_MAX_FRAMES 256
#define XG_HD2D_ATLAS_MAX_MAPS 32
#define XG_HD2D_ATLAS_MAX_PARTS 64
/* Pixel rectangles and foot anchors, never normalized/rounded UVs. */
typedef struct XgHd2dSpriteFrame {
    uint16_t animation, cursor, facing;
    uint16_t x, y, width, height, foot_x, foot_y;
} XgHd2dSpriteFrame;
typedef struct XgHd2dSpriteAtlas {
    uint16_t width, height, frame_count, map_count;
    const XgHd2dSpriteFrame *frames;
    const uint16_t *maps;
} XgHd2dSpriteAtlas;
/* Authored atlas row convention; cardinal quantization matches legacy presentation. */
typedef enum XgHd2dFacing {
    XG_HD2D_FRONT = 0, XG_HD2D_BACK = 1,
    XG_HD2D_LEFT = 2, XG_HD2D_RIGHT = 3
} XgHd2dFacing;
XgHd2dFacing XgHd2dAtlasCardinal(unsigned camera_relative_angle);
/* Optional association only: caller hashes ORIGINAL upload bytes before mods.
 * No hashes, upload hooks, or asset identity are inferred by this module. */
typedef struct XgHd2dSourceAssociation {
    char source_sha256[65]; /* canonical lowercase hexadecimal, exact terminator */
    size_t source_bytes;
    const XgHd2dSpriteAtlas *atlas;
} XgHd2dSourceAssociation;
const XgHd2dSpriteAtlas *XgHd2dAtlasFindSource(
    const XgHd2dSourceAssociation *sources, size_t count,
    int enabled, const char *sha256, size_t hash_length, size_t source_bytes);
int XgHd2dAtlasValidate(const XgHd2dSpriteAtlas *atlas);
/* Off returns immediately without inspecting atlas. No modulo/cursor invention. */
const XgHd2dSpriteFrame *XgHd2dAtlasLookup(const XgHd2dSpriteAtlas *atlas,
    int enabled, unsigned map, unsigned animation, unsigned facing,
    unsigned cursor, unsigned parts);
/* Strict raw RGBA reader. Caller uploads with NEAREST min/mag filters, no mipmaps.
 * Output is unpublished on failure; free successful output with free(). */
int XgHd2dAtlasReadRgba(const XgHd2dSpriteAtlas *atlas, const char *path,
    unsigned char **pixels, size_t *bytes);
#ifdef __cplusplus
}
#endif
#endif

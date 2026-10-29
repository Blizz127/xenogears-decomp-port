#include "hd2d_sprite_atlas.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(void)
{
    XgHd2dSpriteFrame frames[] = {{0, 0, 0, 0, 0, 2, 2, 1, 2},
                                {1, 6, 3, 2, 0, 2, 2, 1, 2}};
    uint16_t maps[] = {2, 4};
    XgHd2dSpriteAtlas a = {4, 2, 2, 2, frames, maps};
    unsigned char *pixels = NULL;
    size_t bytes = 0;
    char path[] = "/tmp/xg-atlas-XXXXXX";
    int fd = mkstemp(path);
    FILE *f;
    unsigned char raw[33] = {0};
    assert(fd >= 0); close(fd);
    XgHd2dSourceAssociation sources[2] = {{"", 32, &a}, {"", 32, &a}};
    memset(sources[0].source_sha256, 'a', 64);
    sources[0].source_sha256[64] = 0;
    sources[1] = sources[0];
    assert(!XgHd2dAtlasFindSource(NULL, 0, 0, NULL, 0, 0));
    assert(XgHd2dAtlasFindSource(sources, 1, 1, sources[0].source_sha256, 64, 32) == &a);
    assert(!XgHd2dAtlasFindSource(sources, 1, 1, sources[0].source_sha256, 63, 32));
    assert(!XgHd2dAtlasFindSource(sources, 1, 1, sources[0].source_sha256, 64, 33));
    assert(!XgHd2dAtlasFindSource(sources, 2, 1, sources[0].source_sha256, 64, 32));
    sources[0].source_sha256[0] = 'G';
    assert(!XgHd2dAtlasFindSource(sources, 1, 1, sources[0].source_sha256, 64, 32));
    assert(XgHd2dAtlasCardinal(0) == XG_HD2D_RIGHT);
    assert(XgHd2dAtlasCardinal(511) == XG_HD2D_RIGHT);
    assert(XgHd2dAtlasCardinal(512) == XG_HD2D_FRONT);
    assert(XgHd2dAtlasCardinal(1536) == XG_HD2D_BACK);
    assert(XgHd2dAtlasCardinal(2560) == XG_HD2D_LEFT);
    assert(XgHd2dAtlasCardinal(3584) == XG_HD2D_RIGHT);
    assert(XgHd2dAtlasValidate(&a));
    assert(!XgHd2dAtlasLookup(NULL, 0, 2, 0, 0, 0, 1));
    assert(XgHd2dAtlasLookup(&a, 1, 2, 1, 3, 6, 64) == &frames[1]);
    assert(!XgHd2dAtlasLookup(&a, 1, 5, 1, 3, 6, 1));
    assert(!XgHd2dAtlasLookup(&a, 1, 2, 1, 3, 7, 1));
    assert(!XgHd2dAtlasLookup(&a, 1, 2, 1, 3, 6, 65));
    assert(!XgHd2dAtlasLookup(&a, 1, 2, 1, 3, 6, 0));
    frames[1].x = 3; assert(!XgHd2dAtlasValidate(&a)); frames[1].x = 2;
    frames[1].facing = 16; assert(!XgHd2dAtlasValidate(&a)); frames[1].facing = 3;
    frames[1].foot_y = 3; assert(!XgHd2dAtlasValidate(&a)); frames[1].foot_y = 2;
    frames[1] = frames[0]; assert(!XgHd2dAtlasValidate(&a));
    a.frame_count = 1; assert(XgHd2dAtlasValidate(&a));
    maps[1] = 2; assert(!XgHd2dAtlasValidate(&a)); maps[1] = 4;
    a.width = 4097; assert(!XgHd2dAtlasValidate(&a)); a.width = 4;
    assert(!XgHd2dAtlasReadRgba(&a, "/absent/xg-atlas", &pixels, &bytes));
    for (size_t n = 31; n <= 33; ++n) {
        f = fopen(path, "wb"); assert(f); assert(fwrite(raw, 1, n, f) == n); fclose(f);
        assert(XgHd2dAtlasReadRgba(&a, path, &pixels, &bytes) == (n == 32));
        if (n == 32) { assert(bytes == 32 && pixels); free(pixels); }
        else assert(!pixels && !bytes);
    }
    unlink(path);
    puts("HD2D atlas: bounds, exact cursor, Off, maps, parts, malformed/missing RGBA PASS");
}

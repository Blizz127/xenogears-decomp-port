#include <stdint.h>
#include <string.h>
#include <psx/libgte.h>
#include <psx/inline_c.h>

static void load_projection_vertex(const SVECTOR *vertex, int slot)
{
    uint32_t words[2];
    memcpy(words, vertex, sizeof(words));
    MTC2(words[0], slot * 2);
    MTC2(words[1], slot * 2 + 1);
}

long RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                 long *sxy0, long *sxy1, long *sxy2, long *sxy3,
                 long *p, long *flag)
{
    load_projection_vertex(v0, 0);
    load_projection_vertex(v1, 1);
    load_projection_vertex(v2, 2);
    gte_rtpt();
    uint32_t projection_flags = CFC2(31);
    uint32_t screen[3] = {MFC2(12), MFC2(13), MFC2(14)};

    /* GPU packets contain packed XY pairs even when host longs are wider. */
    memcpy((unsigned char *)sxy0, &screen[0], sizeof(screen[0]));
    memcpy((unsigned char *)sxy1, &screen[1], sizeof(screen[1]));
    memcpy((unsigned char *)sxy2, &screen[2], sizeof(screen[2]));

    load_projection_vertex(v3, 0);
    gte_rtps();
    projection_flags |= CFC2(31);
    uint32_t last_screen = MFC2(14);
    memcpy((unsigned char *)sxy3, &last_screen, sizeof(last_screen));
    *p = (int32_t)MFC2(8);
    *flag = (int32_t)projection_flags;

    gte_avsz4();
    return MFC2(7);
}

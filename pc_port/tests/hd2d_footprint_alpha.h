/* Runtime-only retail alpha bounds, using b84 OpaqueTexels/QuadPoint/AddSilhouette math.
 * Reads the user disc VRAM mirror; writes geometry only, never retail pixels. */
#include <limits.h>
extern unsigned short vram[];
static int OpaqueTexels(const POLY_FT4 *p, int *ua, int *va, int *ub, int *vb)
{
    int u0 = p->u0 < p->u1 ? p->u0 : p->u1, u1 = p->u0 < p->u1 ? p->u1 : p->u0;
    int v0 = p->v0 < p->v2 ? p->v0 : p->v2, v1 = p->v0 < p->v2 ? p->v2 : p->v0;
    int xb = (p->tpage & 0xF) * 64, yb = ((p->tpage >> 4) & 1) * 256, mode = (p->tpage >> 7) & 3;
    int cx = (p->clut & 0x3F) * 16, cy = (p->clut >> 6) & 0x1FF;
    int u, v, found = 0;
    *ua = *va = INT_MAX; *ub = *vb = INT_MIN;
    for (v = v0; v <= v1; ++v) {
        const unsigned short *row = &vram[((yb + v) & 511) * 1024];
        for (u = u0; u <= u1; ++u) {
            unsigned short c;
            if (mode == 0) c = vram[cy * 1024 + ((cx + ((row[(xb + (u >> 2)) & 1023] >> ((u & 3) * 4)) & 0xF)) & 1023)];
            else if (mode == 1) c = vram[cy * 1024 + ((cx + ((row[(xb + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 0xFF)) & 1023)];
            else c = row[(xb + u) & 1023];
            if (!c) continue;
            found = 1;
            if (u < *ua) *ua = u;
            if (u > *ub) *ub = u;
            if (v < *va) *va = v;
            if (v > *vb) *vb = v;
        }
    }
    return found;
}

/* Screen point of UV-space fraction (s, t) on the part's quad: the bilinear
 * map through the four vertices, with vertex 0/1/2/3 at (u0,v0)/(u1,v1)/
 * (u2,v2)/(u3,v3) as func_8001E3D8 built them. */
static void QuadPoint(const POLY_FT4 *p, float s, float t, float *x, float *y)
{
    *x = (1 - s) * (1 - t) * p->x0 + s * (1 - t) * p->x1 + (1 - s) * t * p->x2 + s * t * p->x3;
    *y = (1 - s) * (1 - t) * p->y0 + s * (1 - t) * p->y1 + (1 - s) * t * p->y2 + s * t * p->y3;
}

/* Extend box with the screen rect of the part's opaque texels. */
static void AddSilhouette(const POLY_FT4 *p, float box[4])
{
    int ua, va, ub, vb, i;
    float su = (float)(p->u1 - p->u0), sv = (float)(p->v2 - p->v0), s[2], t[2];
    if (!OpaqueTexels(p, &ua, &va, &ub, &vb)) return;
    /* texel u spans [u, u+1) of the part's width (|u1-u0| + 1 texels) */
    if (su >= 0) { s[0] = (ua - p->u0) / (su + 1); s[1] = (ub + 1 - p->u0) / (su + 1); }
    else { s[0] = (p->u0 - ub) / (1 - su); s[1] = (p->u0 + 1 - ua) / (1 - su); }
    if (sv >= 0) { t[0] = (va - p->v0) / (sv + 1); t[1] = (vb + 1 - p->v0) / (sv + 1); }
    else { t[0] = (p->v0 - vb) / (1 - sv); t[1] = (p->v0 + 1 - va) / (1 - sv); }
    for (i = 0; i < 4; ++i) {
        float x, y;
        QuadPoint(p, s[i & 1], t[i >> 1], &x, &y);
        if (x < box[0]) box[0] = x;
        if (y < box[1]) box[1] = y;
        if (x > box[2]) box[2] = x;
        if (y > box[3]) box[3] = y;
    }
}

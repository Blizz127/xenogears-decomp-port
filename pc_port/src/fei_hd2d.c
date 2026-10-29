/* Optional field Fei art. All simulation and retail sprite generation still run.
 * Only the linked presentation primitives change. Missing art fails to retail.
 *
 * Footprint rule: every retail part (POLY_FT4 built by func_8001E3D8) is
 * replaced IN PLACE -- same four screen vertices, same OT slot, same link
 * order -- and only its texture changes. More detail comes from texel density
 * inside the retail quad, never from a bigger or moved quad.
 *
 * Mapping (see pc_port/assets/fei_hd2d/README.md): the retail frame's
 * silhouette box is the union, in screen space, of every part's opaque texels
 * (read from the CPU VRAM mirror through the part's tpage/CLUT). The atlas
 * cell's art box is its alpha >= 128 bounding box. The art box is mapped
 * affinely onto the silhouette box (so the cell takes the retail frame's
 * aspect, head/feet rows and side edges align), and each part vertex gets the
 * atlas texel under its own screen position. Each cell is uploaded as its own
 * texture with transparent padding (half the art box on every side) so part
 * margins outside the silhouette sample transparency, not a neighbour cell. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "common.h"
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "guest_prim_link.h"
#include "fei_hd2d.h"
#include "quick_checkpoint.h"
#include "psx_memory.h"

/* HD-2D lane sprite hooks (hd2d_sprite.h): weak, so this file links and
 * behaves as before when pc_port/src/hd2d defines none of them. */
#pragma weak PcPort_Hd2dSpriteBegin
#pragma weak PcPort_Hd2dSpriteDraw
#pragma weak PcPort_Hd2dSpriteEnd

extern unsigned int GR_CreateRGBATexture(int, int, unsigned char *);
extern void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
extern s32 g_PlayerActorIndex, g_FieldNumActors;
extern u8 D_800ADB04;
extern unsigned short vram[];          /* PsyCross CPU VRAM mirror, 1024 x 512 */
extern unsigned g_PcPortPresentedFrames; /* game_overrides.c present counter */
#pragma weak g_PcPortPresentedFrames

#define ATLAS_SIZE 1254
#define CELLS 8
#define EXTRA_BYTES_PER_PART (2 * sizeof(DR_PSYX_TEX))
static int enabled = -1, loadAttempted;
static unsigned int texture; /* nonzero once every cell texture is up */
static struct { int x0, y0, x1, y1; } cells[CELLS]; /* art box, atlas pixels, inclusive */
static struct { unsigned int tex; int w, h, padX, padY; } cellTex[CELLS];

int PcPort_FeiHd2dEnabled(void)
{
    if (enabled < 0) {
        const char *e = getenv("XENO_FEI_HD2D");
        enabled = !(e && strcmp(e, "0") == 0);
    }
    return enabled;
}

void PcPort_FeiHd2dToggle(void)
{
    enabled = !PcPort_FeiHd2dEnabled();
    fprintf(stderr, "[fei-hd2d] %s (field idle/walk experiment)\n",
            enabled ? "enabled" : "disabled");
}

/* The dev menu offers the HD-2D toggle only when the atlas file exists. */
int PcPort_FeiHd2dAssetPresent(void)
{
    const char *path = getenv("XENO_FEI_HD2D_ASSET");
    FILE *f;
    if (!path) path = "pc_port/assets/fei_hd2d/fei.rgba";
    f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* Cut cell c's art box out of the atlas into a padded texture. Alpha is made
 * binary at 128 (retail sprites have 1-bit transparency), so the drawn
 * silhouette is exactly the art box the mapping aligns. */
static int UploadCell(const unsigned char *atlas, int c)
{
    int bw = cells[c].x1 - cells[c].x0 + 1, bh = cells[c].y1 - cells[c].y0 + 1;
    int padX = (bw + 1) / 2, padY = (bh + 1) / 2, w = bw + 2 * padX, h = bh + 2 * padY;
    int x, y;
    unsigned char *px = calloc((size_t)w * h, 4);
    if (!px) return 0;
    for (y = 0; y < bh; ++y) for (x = 0; x < bw; ++x) {
        const unsigned char *s = atlas + ((size_t)(cells[c].y0 + y) * ATLAS_SIZE + cells[c].x0 + x) * 4;
        unsigned char *d = px + ((size_t)(padY + y) * w + padX + x) * 4;
        if (s[3] < 128) continue;
        d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
    }
    cellTex[c].tex = GR_CreateRGBATexture(w, h, px);
    cellTex[c].w = w; cellTex[c].h = h; cellTex[c].padX = padX; cellTex[c].padY = padY;
    free(px);
    return cellTex[c].tex != 0;
}

static int LoadTexture(void)
{
    const size_t bytes = (size_t)ATLAS_SIZE * ATLAS_SIZE * 4;
    const char *path = getenv("XENO_FEI_HD2D_ASSET");
    unsigned char *pixels;
    FILE *f;
    int x, y, c;
    if (loadAttempted) return texture != 0;
    loadAttempted = 1;
    if (!path) path = "pc_port/assets/fei_hd2d/fei.rgba";
    f = fopen(path, "rb");
    if (!f) goto unavailable;
    pixels = malloc(bytes);
    if (!pixels) { fclose(f); goto unavailable; }
    if (fread(pixels, 1, bytes, f) != bytes || fgetc(f) != EOF) {
        free(pixels); fclose(f); goto unavailable;
    }
    fclose(f);
    for (c = 0; c < CELLS; ++c) {
        cells[c].x0 = cells[c].y0 = ATLAS_SIZE;
        cells[c].x1 = cells[c].y1 = -1;
    }
    /* Read bounds from alpha, without modifying the authored image. */
    for (y = 0; y < ATLAS_SIZE; ++y) for (x = 0; x < ATLAS_SIZE; ++x) {
        if (pixels[((size_t)y * ATLAS_SIZE + x) * 4 + 3] < 128) continue;
        c = (y * 2 / ATLAS_SIZE) * 4 + x * 4 / ATLAS_SIZE;
        if (x < cells[c].x0) cells[c].x0 = x;
        if (x > cells[c].x1) cells[c].x1 = x;
        if (y < cells[c].y0) cells[c].y0 = y;
        if (y > cells[c].y1) cells[c].y1 = y;
    }
    for (c = 0; c < CELLS; ++c) if (cells[c].x1 <= cells[c].x0 || cells[c].y1 <= cells[c].y0) {
        free(pixels); goto unavailable;
    }
    for (c = 0; c < CELLS; ++c) if (!UploadCell(pixels, c)) { free(pixels); goto unavailable; }
    free(pixels);
    texture = cellTex[0].tex;
    fprintf(stderr, "[fei-hd2d] loaded %s (%d padded cell textures)\n", path, CELLS);
    return 1;
unavailable:
    texture = 0;
    fprintf(stderr, "[fei-hd2d] unavailable asset %s; retaining retail sprites\n", path);
    return 0;
}

/* ---- HD-2D lane sprite hooks ------------------------------------------ */

/* A guest pointer (u32 in retail structs) that lies inside guest RAM, else
 * NULL, so a stale or foreign pointer is never dereferenced. */
static const u8 *GuestRamPtr(u32 addr, size_t bytes)
{
    uintptr_t p = (uintptr_t)addr, base = (uintptr_t)g_PsxRam;
    if (!addr || p < base || p + bytes > base + (uintptr_t)PSX_RAM_SIZE) return NULL;
    return (const u8 *)p;
}

static u32 Fnv1a(u32 h, const u8 *p, size_t n)
{
    while (n--) { h ^= *p++; h *= 0x01000193u; }
    return h;
}

/* See hd2d_sprite.h: FNV-1a 32 over the package frame table (the u16 offset
 * table, content not address) then the TIM and CLUT VRAM origins. */
unsigned int PcPort_Hd2dSpriteIdentity(const void *sprite)
{
    const u8 *pkg, *frames;
    u32 h = 0x811C9DC5u;
    u8 xy[8];
    int n, i;
    if (!sprite) return h;
    pkg = GuestRamPtr(*(const u32 *)((const u8 *)sprite + 0x24), 0x14);
    if (!pkg) return h;
    frames = GuestRamPtr(*(const u32 *)pkg, 2);
    if (frames) {
        n = ((frames[0] | frames[1] << 8) & 0x1FF) + 1;
        if (n > 512) n = 512;
        if (GuestRamPtr(*(const u32 *)pkg, (size_t)n * 2)) h = Fnv1a(h, frames, (size_t)n * 2);
    }
    for (i = 0; i < 8; ++i) xy[i] = pkg[4 + i]; /* tex x, tex y, clut x, clut y */
    return Fnv1a(h, xy, sizeof xy);
}

static void Hd2dSpriteBegin(PcPortFeiHd2dBatch *b, void *sprite)
{
    int i;
    b->hookState = 0;
    b->part = 0;
    if (!PcPort_Hd2dSpriteBegin && !PcPort_Hd2dSpriteDraw && !PcPort_Hd2dSpriteEnd) return;
    b->info.identity = PcPort_Hd2dSpriteIdentity(sprite);
    b->info.frame = *(s16 *)((u8 *)sprite + 0x34);
    b->info.actor = -1;
    b->info.sprite = sprite;
    if (g_FieldActors)
        for (i = 0; i < g_FieldNumActors; ++i)
            if ((uintptr_t)sprite == g_FieldActors[i].pSpriteData) { b->info.actor = i; break; }
    b->hookState = 1;
    if (PcPort_Hd2dSpriteDraw && (!PcPort_Hd2dSpriteBegin || PcPort_Hd2dSpriteBegin(&b->info)))
        b->hookState = 2;
}

/* Offer one retail part to the lane first; 1 = it drew the part. */
static int Hd2dSpriteDraw(PcPortFeiHd2dBatch *b, void *poly, void *ot)
{
    const POLY_FT4 *p = poly;
    PcPortHd2dRetailQuad q;
    int part = b->part++;
    if (b->hookState != 2) return 0;
    q.x[0] = p->x0; q.y[0] = p->y0; q.u[0] = p->u0; q.v[0] = p->v0;
    q.x[1] = p->x1; q.y[1] = p->y1; q.u[1] = p->u1; q.v[1] = p->v1;
    q.x[2] = p->x2; q.y[2] = p->y2; q.u[2] = p->u2; q.v[2] = p->v2;
    q.x[3] = p->x3; q.y[3] = p->y3; q.u[3] = p->u3; q.v[3] = p->v3;
    q.tpage = p->tpage;
    q.clut = p->clut;
    q.ot = ot;
    q.poly = poly;
    return PcPort_Hd2dSpriteDraw(&b->info, part, &q) != 0;
}

/* ---- test tooling (pc_port/tests/run_fei_hd2d_footprint_test.sh) ------ */
/* XENO_FEI_HD2D_TEST_CYCLE=1: presentation mode follows the present index in
 * blocks of TEST_BLOCK presents, (present / TEST_BLOCK) % 3 = 0 retail,
 * 1 HD-2D, 2 Fei hidden, so one run renders the same camera three ways a few
 * frames apart. Blocks absorb the OT-build-to-present latency (<= 3
 * presents): a present at offset >= 4 in its block shows a frame built in
 * that block. XENO_FEI_HD2D_PART_LOG=file: one line per Fei part with the
 * build-time present index, mode, OT slot and screen quad, and the HD
 * silhouette box. XENO_FEI_HD2D_TEST_CAPTURES=N asks the field capture
 * (game_overrides.c, XENO_FIELD_CAPTURE_DIR) for N presents at offset 6 of
 * consecutive blocks once XENO_FEI_HD2D_TEST_SETTLE (120) presents have
 * passed with Fei drawn. */
#define TEST_BLOCK 8
enum { MODE_RETAIL, MODE_HD, MODE_HIDDEN };
static int s_testCycle = -1;
static FILE *s_partLog;
static unsigned s_firstFei, s_wantFrom, s_captured, s_captureMax, s_settle = 120;

static unsigned NextPresent(void)
{
    return &g_PcPortPresentedFrames ? g_PcPortPresentedFrames + 1 : 0;
}

static int TestMode(void)
{
    if (s_testCycle < 0) {
        const char *e = getenv("XENO_FEI_HD2D_TEST_CYCLE"), *log = getenv("XENO_FEI_HD2D_PART_LOG");
        const char *n = getenv("XENO_FEI_HD2D_TEST_CAPTURES"), *st = getenv("XENO_FEI_HD2D_TEST_SETTLE");
        s_testCycle = e && *e == '1';
        if (log && *log) s_partLog = fopen(log, "w");
        s_captureMax = s_testCycle && n ? (unsigned)atoi(n) : 0;
        if (st && *st) s_settle = (unsigned)atoi(st);
    }
    return s_testCycle ? (int)((NextPresent() / TEST_BLOCK) % 3) : -1;
}

/* game_overrides.c PcPort_FieldCaptureOnVsync: capture this present? */
int PcPort_FeiHd2dTestCaptureWanted(unsigned present)
{
    if (!s_captureMax || !s_wantFrom || present < s_wantFrom || present % TEST_BLOCK != 6 ||
        s_captured >= s_captureMax) return 0;
    s_captured++;
    return 1;
}

static void LogParts(PcPortFeiHd2dBatch *b, int mode, int cell)
{
    int i;
    unsigned present = NextPresent();
    if (s_testCycle > 0) {
        if (!s_firstFei) s_firstFei = present;
        /* start at a retail block so captures come in retail/HD/hidden order */
        if (!s_wantFrom && present - s_firstFei >= s_settle)
            s_wantFrom = (present / (3 * TEST_BLOCK) + 1) * 3 * TEST_BLOCK;
    }
    if (!s_partLog) return;
    for (i = 0; i < b->count; ++i) {
        const POLY_FT4 *p = b->prims[i];
        fprintf(s_partLog, "present=%u mode=%d cell=%d part=%d ot=%lx quad=%d,%d,%d,%d,%d,%d,%d,%d\n",
                NextPresent(), mode, cell, i, (unsigned long)(uintptr_t)b->ots[i],
                p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3);
    }
    fflush(s_partLog);
}

/* ---- Fei path ------------------------------------------------------------ */

void PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch *b, void *sprite, int parts)
{
    ActorData *actor;
    b->active = b->count = 0;
    b->sprite = sprite;
    Hd2dSpriteBegin(b, sprite);
    if ((!PcPort_FeiHd2dEnabled() && TestMode() < 0) || !PcPort_QuickCheckpointFieldIsActive() ||
        !D_800ADB04 || !g_FieldActors || g_PlayerActorIndex < 0 ||
        g_PlayerActorIndex >= g_FieldNumActors) return;
    if ((uintptr_t)sprite != g_FieldActors[g_PlayerActorIndex].pSpriteData) return;
    actor = (ActorData *)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    if (!actor || actor->characterId != 0) return;
    /* Animations 0/1/2 are normal idle/walk/run; preserve all other authored poses. */
    if (actor->curAnimationId < 0 || actor->curAnimationId > 2) return;
    b->walking = actor->curAnimationId != 0;
    if (parts > 64 || (size_t)((u8 *)g_GfxCurWorkBufferEnd - (u8 *)g_GfxCurWorkBuffer) <=
        (size_t)parts * (sizeof(POLY_FT4) + EXTRA_BYTES_PER_PART)) return;
    b->active = LoadTexture();
}

int PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch *b, void *poly, void *ot)
{
    if (b->hookState && Hd2dSpriteDraw(b, poly, ot)) return 1;
    if (!b->active || b->count >= 64) return 0;
    b->prims[b->count] = poly;
    b->ots[b->count++] = ot;
    return 1;
}

/* Opaque texel box of a retail part in its own UV space (inclusive), read from
 * the VRAM mirror through its tpage/CLUT. 0 when nothing is opaque. */
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

static int ClampUv(float v)
{
    int i = (int)(v + 0.5f);
    return i < 0 ? 0 : i > 255 ? 255 : i;
}

static void FeiEnd(PcPortFeiHd2dBatch *b);

void PcPort_FeiHd2dEnd(PcPortFeiHd2dBatch *b)
{
    FeiEnd(b);
    if (b->hookState && PcPort_Hd2dSpriteEnd) PcPort_Hd2dSpriteEnd(&b->info);
}

static void FeiEnd(PcPortFeiHd2dBatch *b)
{
    int i, j, cell, angle, mode = TestMode();
    float box[4] = { 1e9f, 1e9f, -1e9f, -1e9f }, sx, sy;
    u8 *sprite = b->sprite;
    DR_PSYX_TEX *packets;
    if (!b->count) return;
    if (mode < 0) mode = MODE_HD;
    /* Retail's camera-relative facing used by SpriteSetDirection. */
    angle = *(u16 *)(sprite + 0x80) & 4095;
    cell = angle < 512 || angle >= 3584 ? 6 :
           angle < 1536 ? 0 : angle < 2560 ? 4 : 2;
    /* The animation interpreter owns this frame cursor, so speed and pauses
     * follow the game rather than wall time. */
    if (b->walking && ((*(u16 *)(sprite + 0x34) / 2) & 1)) ++cell;
    LogParts(b, mode, cell);
    if (mode == MODE_HIDDEN) return;
    if (mode == MODE_RETAIL || !PcPort_FeiHd2dEnabled() && s_testCycle <= 0) goto retail;
    for (i = 0; i < b->count; ++i) {
        const POLY_FT4 *p = b->prims[i];
        for (j = 0; j < 4; ++j) {
            const short *xy = (const short *)((const u8 *)p + 8 + j * 8);
            if (xy[0] < -1024 || xy[0] > 1024 || xy[1] < -1024 || xy[1] > 1024) goto retail;
        }
        AddSilhouette(p, box);
    }
    if (box[2] - box[0] < 2 || box[3] - box[1] < 2 || box[2] - box[0] > 512 || box[3] - box[1] > 512)
        goto retail;
    if ((size_t)((u8 *)g_GfxCurWorkBufferEnd - (u8 *)g_GfxCurWorkBuffer) <=
        (size_t)b->count * EXTRA_BYTES_PER_PART) goto retail;
    /* screen -> cell texture: the silhouette box lands on the art box */
    if (s_partLog) {
        fprintf(s_partLog, "present=%u box=%.3f,%.3f,%.3f,%.3f\n", NextPresent(), box[0], box[1], box[2], box[3]);
        fflush(s_partLog);
    }
    sx = (cells[cell].x1 - cells[cell].x0 + 1) / (box[2] - box[0]);
    sy = (cells[cell].y1 - cells[cell].y0 + 1) / (box[3] - box[1]);
    packets = g_GfxCurWorkBuffer;
    g_GfxCurWorkBuffer = packets + 2 * b->count;
    memset(packets, 0, 2 * b->count * sizeof(DR_PSYX_TEX));
    for (i = 0; i < b->count; ++i) {
        POLY_FT4 *p = b->prims[i];
        DR_PSYX_TEX *bind = &packets[2 * i], *reset = bind + 1;
        const float kx = 256.0f / cellTex[cell].w, ky = 256.0f / cellTex[cell].h;
        const float ox = cellTex[cell].padX, oy = cellTex[cell].padY;
        /* The retail packet itself is reused: screen vertices untouched. */
        p->u0 = ClampUv((ox + (p->x0 - box[0]) * sx) * kx); p->v0 = ClampUv((oy + (p->y0 - box[1]) * sy) * ky);
        p->u1 = ClampUv((ox + (p->x1 - box[0]) * sx) * kx); p->v1 = ClampUv((oy + (p->y1 - box[1]) * sy) * ky);
        p->u2 = ClampUv((ox + (p->x2 - box[0]) * sx) * kx); p->v2 = ClampUv((oy + (p->y2 - box[1]) * sy) * ky);
        p->u3 = ClampUv((ox + (p->x3 - box[0]) * sx) * kx); p->v3 = ClampUv((oy + (p->y3 - box[1]) * sy) * ky);
        /* colour as before: modulation doubled, raw sprites full white */
        p->r0 = (p->code & 1) || p->r0 > 127 ? 255 : p->r0 * 2;
        p->g0 = (p->code & 1) || p->g0 > 127 ? 255 : p->g0 * 2;
        p->b0 = (p->code & 1) || p->b0 > 127 ? 255 : p->b0 * 2;
        p->code = (p->code & ~1) | 2; /* modulated, semi-transparent: RGBA alpha */
        p->tpage = 0;
        p->clut = 0;
        SetPsyXTexture(bind, cellTex[cell].tex, 256, 256);
        SetPsyXTexture(reset, 0, 0, 0);
        /* OT insertion prepends: bind -> part -> reset, at the part's own slot. */
        PcPort_AddPrimDomainAware(b->ots[i], reset);
        PcPort_AddPrimDomainAware(b->ots[i], p);
        PcPort_AddPrimDomainAware(b->ots[i], bind);
    }
    return;
retail:
    for (i = 0; i < b->count; ++i) PcPort_AddPrimDomainAware(b->ots[i], b->prims[i]);
}

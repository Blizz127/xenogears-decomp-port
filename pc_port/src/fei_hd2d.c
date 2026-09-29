/* Optional field Fei art. All simulation and retail sprite generation still run.
 * Only the linked presentation primitives change. Missing art fails to retail. */
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

extern unsigned int GR_CreateRGBATexture(int, int, unsigned char *);
extern void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
extern s32 g_PlayerActorIndex, g_FieldNumActors;
extern u8 D_800ADB04;

#define ATLAS_SIZE 1254
#define EXTRA_BYTES (sizeof(POLY_FT4) + 2 * sizeof(DR_PSYX_TEX))
static int enabled = -1, loadAttempted;
static unsigned int texture;
static struct { int x0, y0, x1, y1; } cells[8];

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
    for (c = 0; c < 8; ++c) {
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
    for (c = 0; c < 8; ++c) if (cells[c].x1 <= cells[c].x0 || cells[c].y1 <= cells[c].y0) {
        free(pixels); goto unavailable;
    }
    texture = GR_CreateRGBATexture(ATLAS_SIZE, ATLAS_SIZE, pixels);
    free(pixels);
    fprintf(stderr, "[fei-hd2d] loaded %s texture=%u\n", path, texture);
    return texture != 0;
unavailable:
    fprintf(stderr, "[fei-hd2d] unavailable asset %s; retaining retail sprites\n", path);
    return 0;
}

void PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch *b, void *sprite, int parts)
{
    ActorData *actor;
    b->active = b->count = 0;
    b->sprite = sprite;
    if (!PcPort_FeiHd2dEnabled() || !PcPort_QuickCheckpointFieldIsActive() ||
        !D_800ADB04 || !g_FieldActors || g_PlayerActorIndex < 0 ||
        g_PlayerActorIndex >= g_FieldNumActors) return;
    if ((uintptr_t)sprite != g_FieldActors[g_PlayerActorIndex].pSpriteData) return;
    actor = (ActorData *)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    if (!actor || actor->characterId != 0) return;
    /* Animations 0/1/2 are normal idle/walk/run; preserve all other authored poses. */
    if (actor->curAnimationId < 0 || actor->curAnimationId > 2) return;
    b->walking = actor->curAnimationId != 0;
    if ((size_t)((u8 *)g_GfxCurWorkBufferEnd - (u8 *)g_GfxCurWorkBuffer) <=
        (size_t)parts * sizeof(POLY_FT4) + EXTRA_BYTES) return;
    b->active = LoadTexture();
}

int PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch *b, void *poly, void *ot)
{
    if (!b->active) return 0;
    b->prims[b->count] = poly;
    b->ots[b->count++] = ot;
    return 1;
}

void PcPort_FeiHd2dEnd(PcPortFeiHd2dBatch *b)
{
    int i, j, minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
    int cell, angle, height, width, cx, walking;
    u8 *sprite = b->sprite;
    POLY_FT4 *p, *original;
    DR_PSYX_TEX *bind, *reset;
    if (!b->count) return;
    for (i = 0; i < b->count; ++i) {
        if (b->ots[i] != b->ots[0]) goto retail;
        original = b->prims[i];
        for (j = 0; j < 4; ++j) {
            short *xy = (short *)((u8 *)original + 8 + j * 8);
            if (xy[0] < minX) minX = xy[0];
            if (xy[0] > maxX) maxX = xy[0];
            if (xy[1] < minY) minY = xy[1];
            if (xy[1] > maxY) maxY = xy[1];
        }
    }
    height = maxY - minY;
    if (height < 2 || height > 512 || maxX - minX > 512) goto retail;
    /* Retail's camera-relative facing used by SpriteSetDirection. */
    angle = *(u16 *)(sprite + 0x80) & 4095;
    cell = angle < 512 || angle >= 3584 ? 6 :
           angle < 1536 ? 0 : angle < 2560 ? 4 : 2;
    walking = b->walking;
    /* The animation interpreter owns this frame cursor, so speed and pauses
     * follow the game rather than wall time. */
    if (walking && ((*(u16 *)(sprite + 0x34) / 2) & 1)) ++cell;
    width = height * (cells[cell].x1 - cells[cell].x0 + 1) /
                     (cells[cell].y1 - cells[cell].y0 + 1);
    cx = (minX + maxX) / 2;
    p = g_GfxCurWorkBuffer;
    bind = (DR_PSYX_TEX *)(p + 1);
    reset = bind + 1;
    g_GfxCurWorkBuffer = reset + 1;
    memset(p, 0, EXTRA_BYTES);
    setPolyFT4(p);
    setSemiTrans(p, 1); /* RGBA shader supplies source alpha, including holes. */
    original = b->prims[0];
    p->r0 = (original->code & 1) || original->r0 > 127 ? 255 : original->r0 * 2;
    p->g0 = (original->code & 1) || original->g0 > 127 ? 255 : original->g0 * 2;
    p->b0 = (original->code & 1) || original->b0 > 127 ? 255 : original->b0 * 2;
    setXY4(p, cx-width/2, minY, cx+(width+1)/2, minY,
              cx-width/2, maxY, cx+(width+1)/2, maxY);
    setUV4(p, cells[cell].x0*256/ATLAS_SIZE, cells[cell].y0*256/ATLAS_SIZE,
              cells[cell].x1*256/ATLAS_SIZE, cells[cell].y0*256/ATLAS_SIZE,
              cells[cell].x0*256/ATLAS_SIZE, cells[cell].y1*256/ATLAS_SIZE,
              cells[cell].x1*256/ATLAS_SIZE, cells[cell].y1*256/ATLAS_SIZE);
    SetPsyXTexture(bind, texture, 256, 256);
    SetPsyXTexture(reset, 0, 0, 0);
    /* OT insertion prepends: bind -> sprite -> reset -> prior content. */
    PcPort_AddPrimDomainAware(b->ots[0], reset);
    PcPort_AddPrimDomainAware(b->ots[0], p);
    PcPort_AddPrimDomainAware(b->ots[0], bind);
    return;
retail:
    for (i = 0; i < b->count; ++i) PcPort_AddPrimDomainAware(b->ots[i], b->prims[i]);
}

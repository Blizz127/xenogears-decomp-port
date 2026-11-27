/* The including field module supplies its GPU packet types, helpers and globals. */
#include <stdint.h>
#include <string.h>

static void fieldPrepareMarker(DR_MODE modes[2], SPRT sprites[2], int index,
                               int firstPageY, RECT *window)
{
    int frame;
    SPRT *sprite = &sprites[0];

    for (frame = 0; frame < 2; ++frame) {
        int pageY = frame == 0 ? firstPageY : 320;
        u16 page = (u16)GetTPage(0, 0, 960, pageY);
        SetDrawMode(&modes[frame], 0, 0, page, window);
    }
    SetSprt(sprite);
    sprite->r0 = 128;
    sprite->g0 = 128;
    sprite->b0 = 128;
    sprite->x0 = 160;
    sprite->y0 = 112;
    sprite->u0 = 224;
    sprite->v0 = index == 0 ? 112 : 96;
    sprite->w = index == 0 ? 16 : 8;
    sprite->h = sprite->w;
    sprite->clut = (u16)GetClut(256, 247);
    memcpy(&sprites[1], sprite, sizeof(*sprite));
}

void func_800AAC08(void)
{
    RECT window = {0, 0, 255, 255};
    SpriteList *list = HeapAlloc(sizeof(*list), 0);
    int i;

    D_800AFC68 = (u32)(uintptr_t)list;
    for (i = 0; i < 33; ++i) {
        fieldPrepareMarker(list->drModes[i], list->sprites[i], i, 256, &window);
    }
}

void func_800AAF80(void)
{
    RECT window = {0, 0, 255, 255};
    /* The backing allocation includes 160 bytes after the five packet pairs. */
    PolyList2 *bands = HeapAlloc(760, 0);
    SpriteList2 *markers;
    int i;

    D_800C3A3C = (u32)(uintptr_t)bands;
    markers = HeapAlloc(sizeof(*markers), 0);
    D_800B1DF0 = (u32)(uintptr_t)markers;
    for (i = 0; i < 4; ++i) {
        fieldPrepareMarker(markers->drModes[i], markers->sprites[i], i, 320, &window);
    }
    for (i = 0; i < 3; ++i) {
        POLY_FT4 *quad = &bands->polys[i][0];
        int pageX = 768 + 64 * i;
        int frame;

        SetPolyFT4(quad);
        bands->rects[i][0] = window;
        bands->rects[i][1] = window;
        quad->x0 = quad->x2 = (s16)(i * 128);
        quad->x1 = quad->x3 = (s16)((i + 1) * 128);
        quad->y0 = quad->y1 = 0;
        quad->y2 = quad->y3 = 223;
        for (frame = 0; frame < 2; ++frame) {
            u16 page = (u16)GetTPage(1, 0, pageX, 256);
            SetDrawMode(&bands->drModes[i][frame], 0, 0, page, &bands->rects[i][frame]);
        }
        quad->r0 = 128;
        quad->g0 = 128;
        quad->b0 = 128;
        SetSemiTrans(quad, 1);
        quad->u0 = quad->u2 = 0;
        quad->u1 = quad->u3 = 128;
        quad->v0 = quad->v1 = 0;
        quad->v2 = quad->v3 = 223;
        quad->tpage = (u16)GetTPage(1, 0, pageX, 256);
        quad->clut = (u16)GetClut(0, 246);
        memcpy(&bands->polys[i][1], quad, sizeof(*quad));
    }
}

/* Sign-extend the selected halfword before exposing it as a host coordinate. */
static int fieldSignedHalfword(uint32_t value)
{
    return (int)(value & UINT32_C(0x7fff)) - (int)(value & UINT32_C(0x8000));
}

void func_800AB378(s8 color)
{
    int slot = g_FieldCurRenderContextIndex;
    ActorData *actor = (ActorData *)(uintptr_t)g_FieldActors[g_PlayerActorIndex].pActorData;
    SpriteList2 *markers = (SpriteList2 *)(uintptr_t)D_800B1DF0;
    PolyList2 *bands = (PolyList2 *)(uintptr_t)D_800C3A3C;
    SPRT *sprite = &markers->sprites[0][slot];
    uint32_t actorX = (uint32_t)fieldSignedHalfword((uint32_t)actor->position.vx >> 16);
    uint32_t actorZ = (uint32_t)fieldSignedHalfword((uint32_t)actor->position.vz >> 16);
    uint32_t scaledX = actorX * (uint32_t)D_800C3914;
    uint32_t scaledY = 0u - actorZ * (uint32_t)D_800C3A18;
    u_long *ot = FIELD_CTX_OT3(g_FieldCurRenderContext);
    int i;

    sprite->x0 = (s16)fieldSignedHalfword((scaledX >> 16) + (uint32_t)D_800AFE78 - 4u);
    sprite->y0 = (s16)fieldSignedHalfword((scaledY >> 16) + (uint32_t)D_800AFE7C - 12u);
    sprite->r0 = sprite->g0 = sprite->b0 = (u8)color;
    addPrim(ot, sprite);
    addPrim(ot, &markers->drModes[0][slot]);
    for (i = 0; i < 3; ++i) {
        POLY_FT4 *quad = &bands->polys[i][slot];
        quad->r0 = quad->g0 = quad->b0 = (u8)color;
        addPrim(ot, quad);
        addPrim(ot, &bands->drModes[i][slot]);
    }
}

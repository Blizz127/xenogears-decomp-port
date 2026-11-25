enum { FIELD_FADE_WIDTH = 320, FIELD_FADE_HEIGHT = 224 };

void FieldFadeInitializePrimitives(int index)
{
    FieldFade *fade = &g_FieldEffects.fades[index];
    TILE *tile = &fade->tiles[0];

    SetTile(tile);
    SetSemiTrans(tile, 1);
    tile->x0 = 0;
    tile->y0 = 0;
    tile->w = FIELD_FADE_WIDTH;
    tile->h = FIELD_FADE_HEIGHT;
    fade->tiles[1] = *tile;

    fade->r0 = 0;
    fade->g0 = 0;
    fade->b0 = 0;
    fade->semitransparency = 2;
    fade->isVisible = 0;
    fade->duration = 0;
}

void FieldFadeDraw(u_long *ot, int renderContext)
{
    int index;

    for (index = 0; index < 2; ++index) {
        FieldFade *fade = &g_FieldEffects.fades[index];
        RECT *rect;
        DR_MODE *mode;
        TILE *tile;
        void *entry;
        unsigned short page;

        if (!fade->isVisible) {
            continue;
        }

        rect = &D_800AFE3C[index];
        rect->x = 0;
        rect->y = 0;
        rect->w = FIELD_FADE_WIDTH;
        rect->h = FIELD_FADE_HEIGHT;
        page = (unsigned short)GetTPage(0, fade->semitransparency, 0, 0);
        mode = &fade->drawModes[renderContext];
        SetDrawMode(mode, 0, 0, page, rect);

        tile = &fade->tiles[renderContext];
        tile->r0 = (unsigned char)((u32)fade->r0 >> 8);
        tile->g0 = (unsigned char)((u32)fade->g0 >> 8);
        tile->b0 = (unsigned char)((u32)fade->b0 >> 8);

        /* The field stores one four-byte OT tag per fade. */
        entry = (unsigned char *)ot + 4 * index;
        addPrim(entry, tile);
        addPrim(entry, mode);

        /* Any bit above the fractional byte keeps the fade visible. */
        if (((u32)fade->r0 | (u32)fade->g0 | (u32)fade->b0) < 0x100u) {
            fade->isVisible = 0;
            fade->duration = 0;
        }
    }
}

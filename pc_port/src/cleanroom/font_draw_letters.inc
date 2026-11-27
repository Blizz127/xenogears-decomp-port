void FontDrawLetters(void *ot)
{
    Font *font = g_Font;
    if (font == NULL)
        return;

    unsigned int bank = 0;
    if (font->flags & 1) {
        font->letterFlags &= ~FONT_LETTER_RENDER_CONTEXT;
    } else {
        bank = (unsigned int)font->letterFlags & FONT_LETTER_RENDER_CONTEXT;
        font->letterFlags ^= FONT_LETTER_RENDER_CONTEXT;
    }

    int internal = ot == NULL || (uintptr_t)ot == UINTPTR_MAX;
    if (internal) {
        DrawSync(0);
        ot = &font->pPrimBuffersStart[bank];
        TermPrim(ot);
    }

    int count = font->curNumLetters;
    SPRT_16 *packets = font->pPrimBuffers[bank];
    for (int i = 0; i < count; ++i)
        func_800317E0(ot, &packets[i]);

    AddPrim(ot, &font->texpageSettings[bank]);
    if (font->flags & 0x10)
        func_80031804(ot, &font->bgTiles[bank]);
    FontResetLetterRendering();
    if (internal)
        DrawOTag(ot);
}

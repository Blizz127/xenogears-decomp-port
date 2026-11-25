static int memberChangeLabelCoordinate(u32 bits)
{
    if (bits <= 0x7fffffffu)
        return (int)bits;
    return -1 - (int)(~bits);
}

static volatile POLY_FT4 *memberChangeNameStrip(MenuCharacter *character)
{
    volatile SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
    uintptr_t packet = (uintptr_t)character->polys4B0;

    return (POLY_FT4 *)(packet + (uintptr_t)(u32)menu->renderContext * sizeof(POLY_FT4));
}

void func_801C9A08(MenuCharacter *character, u_char characterIndex, u_char localIndex,
                   int *positionsX, int *positionsY, int verticalSpacing)
{
    volatile u_char *length = &character->descriptionStringsLength;
    const volatile int *x = positionsX;
    const volatile int *y = positionsY;
    const u32 rowOffset = (u32)localIndex * (u32)verticalSpacing;
    const unsigned int textureIndex = characterIndex / 2u;
    volatile SystemMenu *menu;
    unsigned int slot;

    *length = 0;
    for (slot = 0; slot < 9; ++slot) {
        s32 glyph = ((const volatile s32 *)D_801CB3DC)[slot];
        if (glyph != 65535) {
            menu = *(SystemMenu *volatile *)&g_Menu;
            void *font = menu->unk2DC;
            uintptr_t address = (uintptr_t)character->polysDescriptionStrings
                                + 2u * *length * sizeof(POLY_FT4);
            int page = menu->renderContext;
            int glyphX = x[slot];
            int glyphY = memberChangeLabelCoordinate((u32)y[slot] + rowOffset);
            int increment = func_8002675C(font, glyph, (POLY_FT4 *)address, page,
                                         glyphX, glyphY, 4096);
            *length = (u_char)((u32)*length + (u32)increment);
        }
    }
    menu = *(SystemMenu *volatile *)&g_Menu;
    void *font = menu->unk2DC;
    int page = menu->renderContext;
    int portraitX = x[9];
    int portraitY = memberChangeLabelCoordinate((u32)y[9] + rowOffset);
    func_8002675C(font, characterIndex + 334, character->polysPortraitSmall, page,
                  portraitX, portraitY, 4096);

    SetPolyFT4((POLY_FT4 *)memberChangeNameStrip(character));
    memberChangeNameStrip(character)->r0 = 128;
    memberChangeNameStrip(character)->g0 = 128;
    memberChangeNameStrip(character)->b0 = 128;
    SetSemiTrans((POLY_FT4 *)memberChangeNameStrip(character), 0);
    u16 tpage = GetTPage(0, 0, 384, 0);
    memberChangeNameStrip(character)->tpage = tpage;
    u16 palette = characterIndex & 1 ? *(const volatile u16 *)&g_SystemPalette2
                                     : *(const volatile u16 *)&g_SystemPalette1;
    memberChangeNameStrip(character)->clut = palette;

    const volatile int *textureU = &g_MemberChangeMenuCharTexcoordsU[textureIndex];
    const volatile int *textureV = &g_MemberChangeMenuCharTexcoordsV[textureIndex];

    *(volatile u16 *)&memberChangeNameStrip(character)->x0 = (u16)((u32)x[16]);
    *(volatile u16 *)&memberChangeNameStrip(character)->y0 = (u16)((u32)y[16] + rowOffset);
    *(volatile u16 *)&memberChangeNameStrip(character)->x1 = (u16)((u32)x[16] + 72u);
    *(volatile u16 *)&memberChangeNameStrip(character)->y1 = (u16)((u32)y[16] + rowOffset);
    *(volatile u16 *)&memberChangeNameStrip(character)->x2 = (u16)((u32)x[16]);
    *(volatile u16 *)&memberChangeNameStrip(character)->y2 = (u16)((u32)y[16] + rowOffset + 13u);
    *(volatile u16 *)&memberChangeNameStrip(character)->x3 = (u16)((u32)x[16] + 72u);
    *(volatile u16 *)&memberChangeNameStrip(character)->y3 = (u16)((u32)y[16] + rowOffset + 13u);

    memberChangeNameStrip(character)->u0 = (u8)(4u * (u32)*textureU);
    memberChangeNameStrip(character)->v0 = (u8)((u32)*textureV);
    memberChangeNameStrip(character)->u1 = (u8)(4u * (u32)*textureU + 72u);
    memberChangeNameStrip(character)->v1 = (u8)((u32)*textureV);
    memberChangeNameStrip(character)->u2 = (u8)(4u * (u32)*textureU);
    memberChangeNameStrip(character)->v2 = (u8)((u32)*textureV + 13u);
    memberChangeNameStrip(character)->u3 = (u8)(4u * (u32)*textureU + 72u);
    memberChangeNameStrip(character)->v3 = (u8)((u32)*textureV + 13u);
}

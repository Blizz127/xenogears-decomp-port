static int memberChangeCoordinate(u32 bits)
{
    if (bits <= 0x7fffffffu)
        return (int)bits;
    return -1 - (int)(~bits);
}

static POLY_FT4 *memberChangeGlyphPolygon(POLY_FT4 *polygons, unsigned int index)
{
    return (POLY_FT4 *)((uintptr_t)polygons + (uintptr_t)index * sizeof(*polygons));
}

static void memberChangeStatDigits(unsigned int value, volatile u_char *length,
                                   POLY_FT4 *polygons, unsigned int firstDigit, int packed,
                                   const volatile int *x, const volatile int *y, u32 rowOffset)
{
    unsigned int digitIndex;
    unsigned int drawn = 0;

    MemberChangeMenuParseNumberToString(value);
    *length = 0;
    for (digitIndex = firstDigit; digitIndex < 9; ++digitIndex) {
        volatile SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
        unsigned int digit = menu->digits[digitIndex];

        if (digit != 255) {
            unsigned int position = packed ? drawn : digitIndex - firstDigit;
            void *font = menu->unk2DC;
            POLY_FT4 *polygon = memberChangeGlyphPolygon(polygons, 2u * *length);
            int page = menu->renderContext;
            int glyphX = memberChangeCoordinate((u32)*x + 8u * position);
            int glyphY = memberChangeCoordinate((u32)*y + rowOffset);
            int width = func_8002675C(font, digit, polygon, page, glyphX, glyphY, 4096);

            *length = (u_char)((u32)*length + (u32)width);
            ++drawn;
        }
    }
}

static POLY_FT4 *memberChangeLevelTintPolygon(MenuCharacter *character, unsigned int pair)
{
    volatile SystemMenu *menu = *(SystemMenu *volatile *)&g_Menu;
    unsigned int index = 2u * pair + (u32)menu->renderContext;

    return memberChangeGlyphPolygon(character->polys5F0, index);
}

void MemberChangeMenuSetCharacterLevelStrings(MenuCharacter *character, u_char characterIndex,
                                              u_char localIndex, int *x, int *y, int verticalSpacing)
{
    volatile GameCharacter *stats = &g_GameState.characters[characterIndex];
    volatile MenuCharacter *state = character;
    u32 rowOffset = (u32)verticalSpacing * localIndex;
    unsigned int pair;

    memberChangeStatDigits(stats->level, &state->levelStringLength, character->polysLevelString,
                          6, 0, x + 10, y + 10, rowOffset);
    memberChangeStatDigits(stats->unk63, &state->unkBE1, character->polys5F0,
                          6, 1, x + 11, y + 11, rowOffset);

    for (pair = 0; pair < state->unkBE1; ++pair) {
        SetShadeTex(memberChangeLevelTintPolygon(character, pair), 0);
        ((volatile POLY_FT4 *)memberChangeLevelTintPolygon(character, pair))->r0 = 0;
        ((volatile POLY_FT4 *)memberChangeLevelTintPolygon(character, pair))->g0 = 128;
        ((volatile POLY_FT4 *)memberChangeLevelTintPolygon(character, pair))->b0 = 0;
    }
}

void MemberChangeMenuSetCharacterHpAndMpStrings(MenuCharacter *character, u_char characterIndex,
                                                u_char localIndex, int *x, int *y, int verticalSpacing)
{
    volatile GameCharacter *stats = &g_GameState.characters[characterIndex];
    volatile MenuCharacter *state = character;
    u32 rowOffset = (u32)verticalSpacing * localIndex;

    memberChangeStatDigits(stats->hp, &state->hpStringLength, character->polysHpString,
                          6, 0, x + 12, y + 12, rowOffset);
    memberChangeStatDigits(stats->maxHp, &state->maxHpStringLength, character->polysMaxHpString,
                          6, 1, x + 13, y + 13, rowOffset);
    memberChangeStatDigits(stats->mp, &state->mpStringLength, character->polysMpString,
                          7, 0, x + 14, y + 14, rowOffset);
    memberChangeStatDigits(stats->maxMp, &state->maxMpStringLength, character->polysMaxMpString,
                          7, 1, x + 15, y + 15, rowOffset);
}

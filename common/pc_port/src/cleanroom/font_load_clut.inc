typedef uint16_t FontPaletteColor __attribute__((may_alias));

void FontLoadClut(short foreground, short background)
{
    volatile FontPaletteColor *colors = (volatile FontPaletteColor *)g_FontClutData;
    for (unsigned int group = 0; group < 4; ++group) {
        unsigned int mask = 1u << group;
        for (unsigned int i = 0; i < 16; ++i) {
            colors[group * 16 + i] = (uint16_t)((i & mask) ? foreground : background);
        }
    }
    LoadImage(&g_FontClutDest, (u_long *)(void *)g_FontClutData);
}

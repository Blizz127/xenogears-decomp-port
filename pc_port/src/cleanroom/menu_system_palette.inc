static inline void MenuSetSystemPalette(void *primitive, u8 selector) {
    u16 palette = selector ? (u16)g_SystemPalette2 : (u16)g_SystemPalette1;
    size_t offset = __builtin_offsetof(POLY_FT4, clut);
    __builtin_memcpy((u8 *)primitive + offset, &palette, sizeof(palette));
}

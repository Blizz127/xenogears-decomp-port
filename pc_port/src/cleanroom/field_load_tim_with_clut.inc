void FieldLoadTIMWithClut(u_long *pTimData, short x, short y, short clutX,
                          short clutY, short clutWidth, short clutHeight)
{
    TIM_IMAGE image;

    OpenTIM(pTimData);
    if (ReadTIM(&image) == 0)
        return;

    if (image.caddr != 0) {
        volatile RECT *palette = image.crect;
        if (clutY != -1) {
            palette->x = clutX;
            palette->y = clutY;
        }
        if (clutWidth != 0)
            palette->w = clutWidth;
        if (clutHeight != 0)
            palette->h = clutHeight;
        LoadImage(image.crect, (u_long *)image.caddr);
    }

    volatile RECT *pixels = image.prect;
    if (image.paddr != 0)
        pixels->x = x;
    pixels->y = y;
    LoadImage(image.prect, (u_long *)image.paddr);
}

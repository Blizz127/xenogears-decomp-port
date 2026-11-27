void FieldSetClipDimensions(short x, short y, short w, short h)
{
    volatile RECT *const first = &g_FieldRenderContexts[0].drawEnvs[0].clip;
    volatile RECT *const second = &g_FieldRenderContexts[1].drawEnvs[0].clip;

    first->y = y;
    first->x = x;
    first->w = w;
    first->h = h;
    second->x = x;
    *(volatile unsigned short *)&second->y = (unsigned short)((unsigned short)y + 256u);
    second->w = w;
    second->h = h;
}

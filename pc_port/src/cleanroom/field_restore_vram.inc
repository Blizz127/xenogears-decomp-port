void func_80078C5C(void)
{
    if (D_800B2344 == 0)
        return;

    volatile RenderContext *contexts = g_FieldRenderContexts;
    contexts[0].drawEnvs[0].dtd = 0;
    contexts[1].drawEnvs[0].dtd = 0;

    RECT strip = { 0, 480, 256, 32 };
    enum { pairCount = 256 * 32 / 2 };
    void *buffer = HeapAlloc(pairCount * sizeof(u32), 0);
    volatile u32 *pixels = buffer;
    StoreImage(&strip, buffer);
    DrawSync(0);

    for (int i = 0; i < pairCount; ++i) {
        u32 pair = pixels[i];
        if (pair & 0xffffu) {
            pixels[i] = pair | 0x0c63u;
            pair = pixels[i];
        }
        if (pair >> 16)
            pixels[i] = pair | 0x0c630000u;
    }

    LoadImage(&strip, buffer);
    DrawSync(0);
    HeapFree(buffer);
}

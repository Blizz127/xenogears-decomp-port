#include <stdint.h>

static RenderContext *FieldDisplayActiveContext(void)
{
    RenderContext *volatile *binding = &g_FieldCurRenderContext;
    return *binding;
}

void FieldDisplay(void)
{
    DrawSync(0);
    Vsync(2);
    ClearImage(&FieldDisplayActiveContext()->drawEnvs->clip, 0, 0, 0);
    PutDrawEnv(FieldDisplayActiveContext()->drawEnvs);
    PutDispEnv(&FieldDisplayActiveContext()->dispEnv);
    DrawOTag(FIELD_CTX_OT3_HEAD(FieldDisplayActiveContext()));
}

void func_800A5774(int x, int y, int h)
{
    RECT region = { (short)x, (short)y, 64, (short)h };
    uint32_t bytes = (uint32_t)h * 128u;
    uint32_t *pixels = HeapAlloc(bytes, 1);
    if (pixels == 0)
        return;

    StoreImage(&region, (u_long *)pixels);
    DrawSync(0);
    int32_t wordCount = (int32_t)((uint32_t)h * 32u);
    if (wordCount > 0) {
        uint32_t limit = bytes / sizeof(*pixels);
        if ((uint32_t)wordCount < limit)
            limit = (uint32_t)wordCount;
        for (uint32_t word = 0; word < limit; ++word)
            pixels[word] |= UINT32_C(0x80008000);
    }
    LoadImage(&region, (u_long *)pixels);
    DrawSync(0);
    HeapFree(pixels);
}

static void FieldPresentZoomFadePair(void)
{
    for (int frame = 0; frame < 2; ++frame) {
        FieldClearAndSwapOTag();
        FieldZoomFadeEffectUpdate();
        FieldDisplay();
    }
}

void func_800A5884(int semiTrans, int abr)
{
    FieldZoomFadeEffectInitialize(semiTrans, abr);
    FieldPresentZoomFadePair();
    for (int strip = 0; strip < 5; ++strip)
        func_800A5774(704 + strip * 64, 256, 224);
    FieldPresentZoomFadePair();
}

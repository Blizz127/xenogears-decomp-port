#include <stdint.h>

static void fieldStartUniformFade(int duration, int color, int change)
{
    volatile FieldFade *fade = &g_FieldEffects.fades[0];
    int ticks = (int)(((uint32_t)duration + 1u) & 0xffffu);
    if (ticks >= 0x8000)
        ticks -= 0x10000;
    int step = duration ? change / duration : (change < 0 ? 1 : -1);

    fade->b0 = color;
    fade->g0 = color;
    fade->r0 = color;
    fade->duration = (short)ticks;
    fade->isVisible = 1;
    fade->semitransparency = 1;
    fade->blueDelta = step;
    fade->greenDelta = step;
    fade->redDelta = step;
}

void func_800A56A8(int duration)
{
    fieldStartUniformFade(duration, 0xff00, -65536);
}

void func_800A5710(int duration)
{
    fieldStartUniformFade(duration, 0, 65536);
}

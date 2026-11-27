#include <stdint.h>

static int fieldFadeDelta(uint32_t pattern, int duration)
{
    int value = pattern <= INT32_MAX ? (int)pattern : -1 - (int)(UINT32_MAX - pattern);
    if (duration == 0)
        return value < 0 ? 1 : -1;
    if (value == INT32_MIN && duration == -1)
        return INT32_MIN;
    return value / duration;
}

void FieldFadeSetParameters(int index, int duration, int red, int green, int blue, int semitransparency)
{
    if ((unsigned int)index >= 2u)
        return;
    volatile FieldFade *fade = &g_FieldEffects.fades[index];
    uint32_t startR = (uint32_t)fade->r0;
    uint32_t startG = (uint32_t)fade->g0;
    uint32_t startB = (uint32_t)fade->b0;
    fade->duration = (short)(uint16_t)duration;
    fade->isVisible = 1;
    fade->semitransparency = (short)(uint16_t)semitransparency;
    fade->redDelta = fieldFadeDelta(((uint32_t)red << 8) - startR, duration);
    fade->greenDelta = fieldFadeDelta(((uint32_t)green << 8) - startG, duration);
    fade->blueDelta = fieldFadeDelta(((uint32_t)blue << 8) - startB, duration);
}

static void fieldFadeMonochrome(int duration, int white)
{
    volatile short *state = &D_800ADC08;
    if (*state == white)
        return;
    int mode = *(volatile int *)&D_800ADC04;
    *state = (short)white;
    if (mode != 2)
        return;
    volatile FieldFade *fade = &g_FieldEffects.fades[0];
    int start = white ? 0 : 0xff00;
    fade->b0 = start;
    fade->g0 = start;
    fade->r0 = start;
    if (white) {
        fade->duration = (short)(uint16_t)duration;
        fade->isVisible = 1;
    } else {
        fade->isVisible = 1;
        fade->duration = (short)(uint16_t)duration;
    }
    fade->semitransparency = 2;
    int change = fieldFadeDelta(white ? 0xff00u : 0xffff0000u, duration);
    fade->blueDelta = change;
    fade->greenDelta = change;
    fade->redDelta = change;
}

void FieldFadeToWhite(int duration)
{
    fieldFadeMonochrome(duration, 1);
}

void FieldFadeToBlack(int duration)
{
    fieldFadeMonochrome(duration, 0);
}

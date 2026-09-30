#include "common.h"
#include "field/main.h"
#include "field/effects.h"

extern int D_800ADC04; // Fade Status?
extern short D_800ADC08;

void FieldFadeInitialize(void) {
    FieldFadeInitializePrimitives(0);
    FieldFadeInitializePrimitives(1);
}

void FieldFadeUpdate(int index) {
    if (g_FieldEffects.fades[index].isVisible) {  

        if (g_FieldEffects.fades[index].duration <= 0) {
            g_FieldEffects.fades[index].duration = 0;
            if (
                D_800ADC08 != 1 &&
                g_FieldEffects.fades[index].r0 == 0 &&
                g_FieldEffects.fades[index].g0 == 0 &&
                g_FieldEffects.fades[index].b0 == 0
            ) {
                g_FieldEffects.fades[index].isVisible = 0;
            }
        } else {
            g_FieldEffects.fades[index].r0 += g_FieldEffects.fades[index].redDelta;
            if ((g_FieldEffects.fades[index].r0 >> 8) >= 0x100) {
                g_FieldEffects.fades[index].r0 = 0xFF00;
            }
            if (g_FieldEffects.fades[index].r0 < 0x0) {
                g_FieldEffects.fades[index].r0 = 0x0;
            }
            
            g_FieldEffects.fades[index].g0 += g_FieldEffects.fades[index].greenDelta;
            if ((g_FieldEffects.fades[index].g0 >> 8) >= 0x100) {
                g_FieldEffects.fades[index].g0 = 0xFF00;
            }
            if (g_FieldEffects.fades[index].g0 < 0x0) {
                g_FieldEffects.fades[index].g0 = 0x0;
            }

            g_FieldEffects.fades[index].b0 += g_FieldEffects.fades[index].blueDelta;
            if ((g_FieldEffects.fades[index].b0 >> 8) >= 0x100) {
                g_FieldEffects.fades[index].b0 = 0xFF00;
            }
            if (g_FieldEffects.fades[index].b0 < 0x0) {
                g_FieldEffects.fades[index].b0 = 0x0;
            }

            g_FieldEffects.fades[index].duration--;
        }
    }
}

void FieldFadeUpdateAndDraw(u_long *ot) {
    if (D_800ADC04 == 2) {
        FieldFadeUpdate(0);
        FieldFadeUpdate(1);
    }
    FieldFadeDraw(ot, g_FieldCurRenderContextIndex);
}

void FieldFadeSetParameters(int index, int duration, int red, int green, int blue, int semitransparency) {
    g_FieldEffects.fades[index].duration = duration;
    g_FieldEffects.fades[index].isVisible = 1;
    g_FieldEffects.fades[index].semitransparency = semitransparency;
    g_FieldEffects.fades[index].redDelta = (red * 0x100 - g_FieldEffects.fades[index].r0) / duration;
    g_FieldEffects.fades[index].greenDelta = (green * 0x100 - g_FieldEffects.fades[index].g0) / duration;
    g_FieldEffects.fades[index].blueDelta = (blue * 0x100 - g_FieldEffects.fades[index].b0) / duration;
}

void FieldFadeToWhite(int duration) {
    if (D_800ADC08 != 1) {
        D_800ADC08 = 1;
        if (D_800ADC04 == 2) {
            g_FieldEffects.fades[0].b0 = 0;
            g_FieldEffects.fades[0].g0 = 0;
            g_FieldEffects.fades[0].r0 = 0;
            g_FieldEffects.fades[0].duration = duration;
            g_FieldEffects.fades[0].isVisible = 1;
            g_FieldEffects.fades[0].semitransparency = 2;
            g_FieldEffects.fades[0].blueDelta = 0xFF00 / duration;
            g_FieldEffects.fades[0].greenDelta = 0xFF00 / duration;
            g_FieldEffects.fades[0].redDelta = 0xFF00 / duration;
        }
    }
}

void FieldFadeToBlack(int duration) {
    if (D_800ADC08) {
        D_800ADC08 = 0;
        if (D_800ADC04 == 2) {
            g_FieldEffects.fades[0].b0 = 0xFF00;
            g_FieldEffects.fades[0].g0 = 0xFF00;
            g_FieldEffects.fades[0].r0 = 0xFF00;
            g_FieldEffects.fades[0].duration = duration;
            g_FieldEffects.fades[0].isVisible = 1;
            g_FieldEffects.fades[0].semitransparency = 2;
            g_FieldEffects.fades[0].blueDelta = -0x10000 / duration;
            g_FieldEffects.fades[0].greenDelta = -0x10000 / duration;
            g_FieldEffects.fades[0].redDelta = -0x10000 / duration;
        }
    }
}

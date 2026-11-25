#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef struct { uintptr_t link; u8 r0, g0, b0, code; u8 tail[32]; } POLY_FT4;
static struct { POLY_FT4 polygons[10]; } g_FieldZoomFadeEffect;
static int g_FieldCurRenderContextIndex;
#include "field_zoom_fade_colors.inc"

int main(void)
{
    const int contexts[] = {0, 1, 0x40000200, 0x60002201, -2, -1};
    for (unsigned i = 0; i < sizeof contexts / sizeof contexts[0]; ++i) {
        for (unsigned value = 0; value <= 255; ++value) {
            memset(&g_FieldZoomFadeEffect, 0xa7, sizeof g_FieldZoomFadeEffect);
            POLY_FT4 expected[10];
            memcpy(expected, g_FieldZoomFadeEffect.polygons, sizeof expected);
            g_FieldCurRenderContextIndex = contexts[i];
            for (unsigned p = i % 2 ? 0 : 1; p < 10; p += 2) {
                expected[p].r0 = expected[p].g0 = expected[p].b0 = (u8)value;
            }
            func_800A5600((u8)value);
            assert(memcmp(expected, g_FieldZoomFadeEffect.polygons, sizeof expected) == 0);
            assert(g_FieldCurRenderContextIndex == contexts[i]);
        }
    }
    puts("1536 synthetic fixed-context calls passed");
}

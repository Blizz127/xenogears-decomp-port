#include "psyq_normal_light_col.h"

#include <string.h>

#include <psx/types.h>
#include <psx/inline_c.h>
#include <psx/gtemac.h>

void NormalLightCol(void *normal, void *input_color, void *output_color)
{
    /* PsyCross operates on aligned words. The game also passes packed packets. */
    uint normal_words[2];
    uint color;
    uint shaded;

    memcpy(normal_words, normal, sizeof(normal_words));
    memcpy(&color, input_color, sizeof(color));
    gte_NormalColorCol(normal_words, &color, &shaded);
    memcpy(output_color, &shaded, sizeof(shaded));
}

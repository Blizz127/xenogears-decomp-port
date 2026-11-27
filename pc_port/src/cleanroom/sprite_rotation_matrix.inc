/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>
#include "psyq/libgte.h"

/* Bind the renderer entry points directly. Game headers give these names
 * the reverse trigonometric meanings in some host translation units. */
extern int sprite_rotation_sine(int angle) __asm__("rsin");
extern int sprite_rotation_cosine(int angle) __asm__("rcos");

/* Each multiplication of two signed Q12 basis components rounds down. */
static int sprite_rotation_product(int a, int b)
{
    const int n = a * b;
    return n / 4096 - (n % 4096 < 0);
}

void Retail8003F738RotMatrix(const SVECTOR *angles, MATRIX *out)
{
    const int sx = sprite_rotation_sine(angles->vx);
    const int cx = sprite_rotation_cosine(angles->vx);
    const int sy = sprite_rotation_sine(angles->vy);
    const int cy = sprite_rotation_cosine(angles->vy);
    const int sz = sprite_rotation_sine(angles->vz);
    const int cz = sprite_rotation_cosine(angles->vz);

    /* Form Rx * Ry * Rz. Keep the signed y/z projections in Q12 before
     * applying x so each basis component rounds at the same stage. */
    const int neg_sy_cz = sprite_rotation_product(-sy, cz);
    const int neg_sy_sz = sprite_rotation_product(-sy, sz);
    const int16_t basis[3][3] = {
        {sprite_rotation_product(cy, cz),
         sprite_rotation_product(-cy, sz),
         sy},
        {sprite_rotation_product(cx, sz) - sprite_rotation_product(sx, neg_sy_cz),
         sprite_rotation_product(cx, cz) + sprite_rotation_product(sx, neg_sy_sz),
         sprite_rotation_product(-sx, cy)},
        {sprite_rotation_product(sx, sz) + sprite_rotation_product(cx, neg_sy_cz),
         sprite_rotation_product(sx, cz) - sprite_rotation_product(cx, neg_sy_sz),
         sprite_rotation_product(cx, cy)}
    };
    memcpy(out->m, basis, sizeof(basis));
}

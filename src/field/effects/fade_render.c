#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"

void FieldFadeInitializePrimitives(int index) {
    SetTile(&g_FieldEffects.fades[index].tiles[0]);
    SetSemiTrans(&g_FieldEffects.fades[index].tiles[0], 1);
    g_FieldEffects.fades[index].tiles[0].w = 0x140;
    g_FieldEffects.fades[index].tiles[0].h = 0xE0;
    g_FieldEffects.fades[index].tiles[0].x0 = 0x0;
    g_FieldEffects.fades[index].tiles[0].y0 = 0x0;
    g_FieldEffects.fades[index].tiles[1] = g_FieldEffects.fades[index].tiles[0];
    g_FieldEffects.fades[index].isVisible = 0;
    g_FieldEffects.fades[index].duration = 0;
    setRGB(g_FieldEffects.fades[index], 0x0);
    g_FieldEffects.fades[index].semitransparency = 2;
}

extern RECT D_800AFE3C[];
void FieldFadeDraw(u_long* ot, int renderContext) {
    int i;
    int ctx;

    for (i = 0; i < 2; i++) {
        if (g_FieldEffects.fades[i].isVisible) {
            D_800AFE3C[i].w = 0x140;
            D_800AFE3C[i].x = 0x0;
            D_800AFE3C[i].y = 0x0;
            D_800AFE3C[i].h = 0xE0;

            SetDrawMode(
                &g_FieldEffects.fades[i].drawModes[renderContext], 
                0, 0, 
                GetTPage(0, g_FieldEffects.fades[i].semitransparency, 0, 0), 
                &D_800AFE3C[i]
            );

            setRGB0(&g_FieldEffects.fades[i].tiles[renderContext],
                g_FieldEffects.fades[i].r0 >> 8,
                g_FieldEffects.fades[i].g0 >> 8, 
                g_FieldEffects.fades[i].b0 >> 8
            );

            ctx = (i == 1);
            addPrim(ot + ctx, &g_FieldEffects.fades[i].tiles[renderContext]);
            addPrim(ot + ctx, &g_FieldEffects.fades[i].drawModes[renderContext]);

            if (
                (g_FieldEffects.fades[i].r0 >> 8 == 0) && 
                (g_FieldEffects.fades[i].g0 >> 8 == 0) &&
                (g_FieldEffects.fades[i].b0 >> 8 == 0)
            ) {
                g_FieldEffects.fades[i].isVisible = 0;
                g_FieldEffects.fades[i].duration = 0;
            }
        }
    }
}

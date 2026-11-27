/* SPDX-License-Identifier: MIT */
#include "model_polygon_projection.inc"
#include "model_polygon_lighting.inc"

s32 func_8002ED20(u8 *pCmd, s32 count)
{
    return PcPort_DrawLitModelPolygons(pCmd, count, PCPORT_MODEL_LIT_F3);
}

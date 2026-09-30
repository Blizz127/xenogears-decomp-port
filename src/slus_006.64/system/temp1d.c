#include "common.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#include "psx_memory.h"
#include "guest_prim_link.h"
#endif
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

/* Retail TU 0x80025044..0x800250E0 (image-list upload), one of the TUs that
 * were merged into system/temp1.c (the groups reach the shared Gfx globals
 * differently: %gp_rel for their own .sbss, absolute for other TUs'). This is
 * the only copy of these bodies; the port builds it too. */

/* Queued VRAM upload; addr/pNext are 32-bit fields in both builds (retail
 * pointers, and what the port's queue writes). */
typedef struct GfxImageEntry {
    RECT rect;
    u32 addr;
    u32 pNext;
} GfxImageEntry;

/* This TU's own .sbss word (retail %gp_rel); the port keeps it elsewhere. */
#ifdef XENO_PC_PORT
extern s32 g_GfxCurContext;
#else
s32 g_GfxCurContext;
#endif
extern u32 g_GfxImageList[2];

/* Walks g_GfxImageList[g_GfxCurContext]: LoadImage entries with pixel data,
 * ClearImage the rest, then empties the list. */
void func_80025044(void) {
    GfxImageEntry* pImage;

    for (pImage = (GfxImageEntry*)(uintptr_t)g_GfxImageList[g_GfxCurContext]; pImage != NULL;
         pImage = (GfxImageEntry*)(uintptr_t)pImage->pNext) {
        if (pImage->addr) {
            LoadImage(&pImage->rect, (u_long*)(uintptr_t)pImage->addr);
        } else {
            ClearImage(&pImage->rect, 0x0, 0x0, 0x0);
        }
    }

    g_GfxImageList[g_GfxCurContext] = 0;
}

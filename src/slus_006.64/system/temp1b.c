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

/* Retail TU 0x80024F64..0x80024FF4 (work buffers / current OT), one of the TUs that were merged into
 * system/temp1.c (the groups reach the shared Gfx globals differently:
 * %gp_rel for their own .sbss, absolute for other TUs'). This is the only
 * copy of these bodies; the port builds it too. */

extern void* g_GfxWorkBuffer2;
extern void func_8001D298(void);
extern void func_8001D2A4(void);

/* This TU's own .sbss: retail reaches it %gp_rel, which cc1 only emits for a
 * same-TU definition. The port keeps the storage elsewhere. */
#ifdef XENO_PC_PORT
#define TEMP1B_SBSS extern
#else
#define TEMP1B_SBSS
#endif
TEMP1B_SBSS s32 g_GfxWorkBufferSize;
TEMP1B_SBSS u32 D_80059300; /* linked lists of SpriteTileData pointers */
TEMP1B_SBSS u32 D_80059304;
TEMP1B_SBSS void* g_GfxWorkBuffers;
TEMP1B_SBSS u32 g_GfxImageList[1]; /* [0] only: this TU clears context 0 */

#define NUM_RENDER_CONTEXTS 2

void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag) {
    void* pWorkBuffers;

    g_GfxWorkBufferSize = workBufferSize;
    pWorkBuffers = HeapAlloc(workBufferSize * NUM_RENDER_CONTEXTS, allocFlag);
    g_GfxWorkBuffers = pWorkBuffers;
    g_GfxWorkBuffer2 = (u8*)pWorkBuffers + workBufferSize;
    D_80059304 = 0;
    D_80059300 = 0;
    g_GfxImageList[0] = 0;
    func_8001D298();
}

/* Frees the work buffers and resets the sprite/image lists. The port keeps
 * its own owner in pc_port/src/world_map_teardown_7299c.c, so this
 * definition is weak there and the port's strong one wins the link. */
#ifdef XENO_PC_PORT
__attribute__((weak))
#endif
void GfxFreeWorkBuffers(void) {
    HeapFree(g_GfxWorkBuffers);
    func_8001D2A4();
}

extern u_long* g_GfxCurOT;

void GfxSetCurrentOT(u_long* ot) {
    g_GfxCurOT = ot;
}

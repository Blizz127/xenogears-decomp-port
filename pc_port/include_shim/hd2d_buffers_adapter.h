#ifndef PC_PORT_HD2D_BUFFERS_ADAPTER_H
#define PC_PORT_HD2D_BUFFERS_ADAPTER_H
#include "../include/xg_plat/hd2d.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Backend adapter API proposal; implementation waits for landed HOOK4 header.
 * Consume exactly once in the immediately following PostProcess, including
 * out-of-scope maps: expire first, then validate path/drawable/viewport metadata.
 * Preserve the frame's field/viewport and projected items; fill only attachment
 * handles, explicit modes, dimensions and raw-slot order scale. No GL calls.
 * OFF returns zero before inspecting pending state or output. */
int PcPort_Hd2dBuffersConsume(int path,int drawable_width,int drawable_height,
    XgHd2dFrame *frame);
/* Explicit CPU-only invalidation on master enable-state transitions. */
void PcPort_Hd2dBuffersReset(void);
#ifdef __cplusplus
}
#endif
#endif

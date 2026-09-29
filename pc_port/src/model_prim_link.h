#ifndef XENO_MODEL_PRIM_LINK_H
#define XENO_MODEL_PRIM_LINK_H

#include "common.h"

/* Publish a built model primitive into an OT while preserving the OT's pointer
 * domain.  Model work buffers are native pointers into g_PsxRam on the PC;
 * guest world OTs must receive the corresponding 24-bit guest address. */
void PcPort_LinkModelPrim(u32 *ot, s32 index, void *packet, u32 tag_length);

#endif

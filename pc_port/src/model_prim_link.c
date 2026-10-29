#include <stdint.h>

#include "guest_prim_link.h"
#include "model_prim_link.h"
#include "hd2d_world_packets.h"

/* Keep existing standalone model certificates linkable without HD2D. */
#pragma weak PcPort_Hd2dWorldPacketLinked

void PcPort_LinkModelPrim(u32 *ot, s32 index, void *packet, u32 tag_length)
{
    u32 *tag = (u32 *)packet;
    const int rejected_before = PcPort_Hd2dWorldPacketLinked ?
        PcPort_PrimLinkRejectCount() : 0;

#if !defined(MODEL_PRIM_LINK_MUTANT_MISSING_TAG_LENGTH)
    *tag = (*tag & 0x00FFFFFFu) | (tag_length & 0xFF000000u);
#else
    (void)tag_length;
    (void)tag;
#endif
#if defined(MODEL_PRIM_LINK_MUTANT_RAW_NATIVE)
    *tag = (*tag & 0xFF000000u) | (ot[index] & 0x00FFFFFFu);
    ot[index] = (ot[index] & 0xFF000000u) |
                ((u32)(uintptr_t)packet & 0x00FFFFFFu);
#else
    PcPort_AddPrimDomainAware(&ot[index], packet);
#endif
    /* Snapshot only a successfully linked packet, after the real tag write. */
    if (PcPort_Hd2dWorldPacketLinked &&
        PcPort_PrimLinkRejectCount() == rejected_before)
        PcPort_Hd2dWorldPacketLinked(packet);
}

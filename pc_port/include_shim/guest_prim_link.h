/* Destination-domain-aware primitive linkage for native/guest OT users. */
#ifndef GUEST_PRIM_LINK_H
#define GUEST_PRIM_LINK_H

void PcPort_AddPrimDomainAware(void *ot, void *prim);
/* Save old OT, replace its link, then store old|tag. Preserves alias order. */
void PcPort_LinkTaggedPrimDomainAware(void *ot, void *prim, unsigned int tag);
void PcPort_PrimLinkReset(void);
int PcPort_PrimLinkGuestCount(void);
int PcPort_PrimLinkNativeCount(void);
int PcPort_PrimLinkRejectCount(void);

#endif /* GUEST_PRIM_LINK_H */

/* Destination-domain-aware primitive linkage for native/guest OT users. */
#ifndef GUEST_PRIM_LINK_H
#define GUEST_PRIM_LINK_H

void PcPort_AddPrimDomainAware(void *ot, void *prim);
void PcPort_PrimLinkReset(void);
int PcPort_PrimLinkGuestCount(void);
int PcPort_PrimLinkNativeCount(void);
int PcPort_PrimLinkRejectCount(void);

#endif /* GUEST_PRIM_LINK_H */

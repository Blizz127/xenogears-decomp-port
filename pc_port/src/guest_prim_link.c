/*
 * Link a primitive according to the destination OT's pointer domain.
 *
 * Native field OTs retain the generic addPrim behavior. World-map OTs live
 * inside g_PsxRam and are consumed as retail 24-bit guest links, so host
 * pointers into that buffer must be translated back to guest addresses.
 */
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "guest_prim_link.h"

static int s_guest_links;
static int s_native_links;
static int s_rejected_links;

static void pc_port_link_prim(u32 *ot, u32 *prim, u32 link)
{
    *prim = (*prim & 0xFF000000u) | (*ot & 0x00FFFFFFu);
    *ot = (*ot & 0xFF000000u) | (link & 0x00FFFFFFu);
}

static int pc_port_in_guest_ram(const void *pointer)
{
    uintptr_t value = (uintptr_t)pointer;
    uintptr_t base = (uintptr_t)g_PsxRam;
    return value >= base && value < base + (uintptr_t)PSX_RAM_SIZE;
}

void PcPort_PrimLinkReset(void)
{
    s_guest_links = 0;
    s_native_links = 0;
    s_rejected_links = 0;
}

int PcPort_PrimLinkGuestCount(void) { return s_guest_links; }
int PcPort_PrimLinkNativeCount(void) { return s_native_links; }
int PcPort_PrimLinkRejectCount(void) { return s_rejected_links; }

void PcPort_AddPrimDomainAware(void *ot, void *prim)
{
    if (pc_port_in_guest_ram(ot)) {
        u32 prim_guest;

        if (!pc_port_in_guest_ram(prim)) {
            s_rejected_links++;
            fprintf(stderr,
                    "[prim-link] reject native prim=%p for guest ot=%p "
                    "count=%d\n",
                    prim, ot, s_rejected_links);
            return;
        }
        prim_guest = PsxMemory_GuestAddr(prim);
#if defined(GUEST_PRIM_LINK_MUTANT_M1)
        prim_guest = (u32)(uintptr_t)prim;
#endif
#if !defined(GUEST_PRIM_LINK_MUTANT_M2)
        *(u32 *)prim = (*(u32 *)prim & 0xFF000000u) |
                       (*(u32 *)ot & 0x00FFFFFFu);
#endif
#if !defined(GUEST_PRIM_LINK_MUTANT_M3)
        *(u32 *)ot = (*(u32 *)ot & 0xFF000000u) |
                     (prim_guest & 0x00FFFFFFu);
#else
        (void)prim_guest;
#endif
        s_guest_links++;
        return;
    }

#if defined(GUEST_PRIM_LINK_MUTANT_M4)
    pc_port_link_prim((u32 *)ot, (u32 *)prim, 0u);
#else
    pc_port_link_prim((u32 *)ot, (u32 *)prim, (u32)(uintptr_t)prim);
#endif
    s_native_links++;
}

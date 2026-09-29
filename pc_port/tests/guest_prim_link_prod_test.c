#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static int check_u32(const char *name, u32 expected, u32 actual)
{
    if (expected == actual)
        return 1;
    fprintf(stderr, "ASSERTION %s expected=0x%08x actual=0x%08x\n",
            name, expected, actual);
    return 0;
}

static int check_int(const char *name, int expected, int actual)
{
    if (expected == actual)
        return 1;
    fprintf(stderr, "ASSERTION %s expected=%d actual=%d\n",
            name, expected, actual);
    return 0;
}

int main(void)
{
    u32 *guest_ot = (u32 *)PSX_ADDR(0x800A1000u);
    u32 *guest_prim = (u32 *)PSX_ADDR(0x800F2000u);
    static u32 native_ot;
    static u32 native_prim;
    static u32 native_bad_prim;
    u32 native_prim_link = (u32)(uintptr_t)&native_prim & 0x00FFFFFFu;
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    PcPort_PrimLinkReset();

    *guest_ot = 0x000A0FFCu;
    *guest_prim = 0x09000000u;
    PcPort_AddPrimDomainAware(guest_ot, guest_prim);
    ok &= check_u32("guest.ot.receives_guest_address", 0x000F2000u,
                    *guest_ot);
    ok &= check_u32("guest.prim.retains_len_and_predecessor", 0x090A0FFCu,
                    *guest_prim);
    ok &= check_int("guest.count", 1, PcPort_PrimLinkGuestCount());

    native_ot = 0x00123456u;
    native_prim = 0x09000000u;
    PcPort_AddPrimDomainAware(&native_ot, &native_prim);
    ok &= check_u32("native.ot.retains_host_link", native_prim_link,
                    native_ot);
    ok &= check_u32("native.prim.retains_len_and_predecessor", 0x09123456u,
                    native_prim);
    ok &= check_int("native.count", 1, PcPort_PrimLinkNativeCount());

    *guest_ot = 0x000A0FFCu;
    native_bad_prim = 0x09000000u;
    PcPort_AddPrimDomainAware(guest_ot, &native_bad_prim);
    ok &= check_u32("mixed.guest_ot_unchanged", 0x000A0FFCu, *guest_ot);
    ok &= check_u32("mixed.native_prim_unchanged", 0x09000000u,
                    native_bad_prim);
    ok &= check_int("mixed.rejected", 1, PcPort_PrimLinkRejectCount());

    if (!ok)
        return EXIT_FAILURE;
    puts("GUEST PRIM LINK CERTIFICATE PASS");
    return EXIT_SUCCESS;
}

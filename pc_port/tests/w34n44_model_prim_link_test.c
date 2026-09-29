/* W34N44 — certificate for model-packet OT domain publication. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "model_prim_link.h"
#include "psx_memory.h"

#define GUEST_OT     0x80001000u
#define GUEST_PACKET 0x80002000u

uint8_t g_PsxRam[PSX_RAM_SIZE];

static u32 s_native_ot[4];
static u32 s_native_packet[4];
static int s_failures;

static u32 ld32(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static void check_guest_domain(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    PcPort_PrimLinkReset();
    st32(GUEST_OT + 3u * 4u, 0xAA001234u);
    st32(GUEST_PACKET, 0xCC005678u);
    PcPort_LinkModelPrim((u32 *)PSX_ADDR(GUEST_OT), 3,
                         PSX_ADDR(GUEST_PACKET), 0x07000000u);
    check("guest-packet-address",
          ld32(GUEST_OT + 3u * 4u) == 0xAA002000u);
    check("guest-old-link-and-length",
          ld32(GUEST_PACKET) == 0x07001234u);
    check("guest-domain-counts",
          PcPort_PrimLinkGuestCount() == 1 &&
          PcPort_PrimLinkNativeCount() == 0 &&
          PcPort_PrimLinkRejectCount() == 0);
}

static void check_native_domain(void)
{
    u32 expected_link;

    memset(s_native_ot, 0, sizeof(s_native_ot));
    memset(s_native_packet, 0, sizeof(s_native_packet));
    PcPort_PrimLinkReset();
    s_native_ot[2] = 0xBB00ABCDu;
    s_native_packet[0] = 0xCC005678u;
    expected_link = (u32)(uintptr_t)s_native_packet & 0x00FFFFFFu;
    PcPort_LinkModelPrim(s_native_ot, 2, s_native_packet, 0x09000000u);
    check("native-packet-address",
          s_native_ot[2] == (0xBB000000u | expected_link));
    check("native-old-link-and-length",
          s_native_packet[0] == 0x0900ABCDu);
    check("native-domain-counts",
          PcPort_PrimLinkGuestCount() == 0 &&
          PcPort_PrimLinkNativeCount() == 1 &&
          PcPort_PrimLinkRejectCount() == 0);
}

static void check_mixed_rejected(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_native_packet, 0, sizeof(s_native_packet));
    PcPort_PrimLinkReset();
    st32(GUEST_OT, 0xDD001111u);
    s_native_packet[0] = 0xCC002222u;
    PcPort_LinkModelPrim((u32 *)PSX_ADDR(GUEST_OT), 0,
                         s_native_packet, 0x04000000u);
    check("mixed-domain-rejected", ld32(GUEST_OT) == 0xDD001111u);
    check("mixed-domain-count",
          PcPort_PrimLinkRejectCount() == 1 &&
          PcPort_PrimLinkGuestCount() == 0);
}

int main(void)
{
    check_guest_domain();
    check_native_domain();
    check_mixed_rejected();
    if (s_failures != 0)
        return 1;
    puts("W34N44 model primitive domain-link certificate PASS");
    return 0;
}

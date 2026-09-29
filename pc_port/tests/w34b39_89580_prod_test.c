/* W34B39 — production-linked certificate for the 0x80089580 slot-table load.
 * The retail cleanup path must load its record base from 0x8009BCC0, not the
 * nearby particle/global word at 0x8009BDE0.  Fixtures deliberately keep the
 * two bases asymmetric and put the wrong-base target on the live OT bucket. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_89580.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

#define PARTICLE_BASE 0x80100000u
#define SLOT_BASE     0x80080000u
#define WRONG_BASE    0x800A2E9Eu
#define OT_BUCKET     0x800A2EA8u
#define BCC0          0x8009BCC0u
#define BDE0          0x8009BDE0u
#define BDF4          0x8009BDF4u
#define STRIDE        0x4Cu
#define COUNT         256u

static void st8(u32 a, u8 v) { memcpy(PSX_ADDR(a), &v, 1); }
static void st16(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void st32(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 ld16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u32 ld32(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }

static int s_pass;
static int s_total;

static void check(const char *name, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
        printf("PASS %s\n", name);
    } else {
        printf("FAIL %s\n", name);
    }
}

static void reset_fixture(void)
{
    u32 i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    st32(BDF4, PARTICLE_BASE);
    st32(BCC0, SLOT_BASE);
    st32(BDE0, WRONG_BASE);
    for (i = 0u; i < COUNT; i++) {
        /* The first record owns slot 0 and has expired (nonzero owner in the
         * packed high half, zero remaining life in the low half).  All later
         * entries are unowned. */
        st16(PARTICLE_BASE + i * STRIDE, 0u);
        st32(PARTICLE_BASE + i * STRIDE + 4u,
             i == 0u ? (1u << 16) : 0u);
    }
    st16(SLOT_BASE + 0x0Au, 0x1234u);
    st16(WRONG_BASE + 0x0Au, 0xBEEFu);
}

static void test_cleanup_uses_bcc0(void)
{
    reset_fixture();
    wm_80089580();
    check("cleanup-decrements-BCC0-record",
          ld16(SLOT_BASE + 0x0Au) == 0x1233u);
    check("cleanup-does-not-touch-wrong-or-OT-record",
          ld16(WRONG_BASE + 0x0Au) == 0xBEEFu &&
          ld16(OT_BUCKET) == 0xBEEFu);
    check("cleanup-clears-first-particle-word",
          ld32(PARTICLE_BASE) == 0u);
}

static void test_active_signed_fixture(void)
{
    u32 entry = PARTICLE_BASE + 4u;

    reset_fixture();
    st16(PARTICLE_BASE, 0xFFFFu);
    st32(entry, (5u << 16) | 3u);
    st32(entry + 4u, 0x01020304u);
    st32(entry + 8u, 0x11121314u);
    st32(entry + 0x0Cu, 0x21222324u);
    st32(entry + 0x14u, 0x00010002u);
    st32(entry + 0x18u, 0x00030004u);
    st32(entry + 0x1Cu, 0x00050006u);
    st32(entry + 0x24u, 0x00070008u);
    st32(entry + 0x28u, 0x0009000Au);
    st32(entry + 0x2Cu, 0x000B000Cu);
    st32(entry + 0x3Cu, 0x04030201u);
    st8(entry + 0x40u, 1u);
    st16(entry + 0x34u, 0x0100u);
    st16(entry + 0x36u, 0x0200u);
    st16(entry + 0x38u, 3u);
    st16(entry + 0x3Au, 4u);
    wm_80089580();
    check("active-counter-decrement",
          (ld32(entry) & 0xFFFFu) == 2u &&
          (ld32(entry) >> 16) == 5u);
    check("active-position-update-widths",
          ld32(entry + 4u) == 0x01030306u &&
          ld32(entry + 8u) == 0x11151318u &&
          ld32(entry + 0x0Cu) == 0x2127232Au);
    check("active-uv-update",
          ld16(entry + 0x34u) == 0x0103u &&
          ld16(entry + 0x36u) == 0x0204u);
}

int main(void)
{
    test_cleanup_uses_bcc0();
    test_active_signed_fixture();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_pass == s_total ? 0 : 1;
}

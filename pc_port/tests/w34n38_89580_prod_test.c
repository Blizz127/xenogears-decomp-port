/* W34N38 — production-linked certificate for full retail wm_80089580. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_89580.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

#define PARTICLE_BASE  0x80100000u
#define SLOT_BASE      0x80110000u
#define WRONG_BASE     0x80120000u
#define OFFSET_BASE    0x80130000u
#define BCC0           0x8009BCC0u
#define BDE0           0x8009BDE0u
#define BDF4           0x8009BDF4u
#define STRIDE         0x4Cu
#define SLOT_STRIDE    0x54u
#define COUNT          256u

static int s_failures;

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 ld16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static u32 record_base(u32 index)
{
    return PARTICLE_BASE + index * STRIDE;
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    st32(BDF4, PARTICLE_BASE);
    st32(BCC0, SLOT_BASE);
    st32(BCC0 + 4u, OFFSET_BASE);
    st32(BDE0, WRONG_BASE);
}

static void test_unowned_record_is_untouched(void)
{
    u32 base = record_base(0u);
    u32 entry = base + 4u;
    u8 before[STRIDE];

    reset_fixture();
    st16(base, 3u);
    st32(entry, 1u); /* owner=0, remaining=1 */
    st32(entry + 0x04u, 0x11223344u);
    st32(entry + 0x14u, 0x01020304u);
    st32(entry + 0x3Cu, 0xAABBCCDDu);
    memcpy(before, PSX_ADDR(base), sizeof(before));
    st16(SLOT_BASE + 3u * SLOT_STRIDE + 0x0Au, 0x3456u);

    wm_80089580();

    check("owner-zero-record-is-read-only",
          memcmp(before, PSX_ADDR(base), sizeof(before)) == 0 &&
          ld16(SLOT_BASE + 3u * SLOT_STRIDE + 0x0Au) == 0x3456u);
}

static void test_active_record_exact_updates(void)
{
    u32 entry;

    reset_fixture();
    entry = record_base(0u) + 4u;
    st32(entry, (0x1234u << 16) | 2u);
    st32(entry + 0x04u, 100u);
    st32(entry + 0x08u, 200u);
    st32(entry + 0x0Cu, 300u);
    st32(entry + 0x14u, 10u);
    st32(entry + 0x18u, 20u);
    st32(entry + 0x1Cu, 30u);
    st32(entry + 0x24u, 1u);
    st32(entry + 0x28u, 2u);
    st32(entry + 0x2Cu, 3u);
    st16(entry + 0x34u, 1000u);
    st16(entry + 0x36u, 2000u);
    st16(entry + 0x38u, 3u);
    st16(entry + 0x3Au, 4u);
    st32(entry + 0x3Cu, 0xAA6403FAu);
    st32(entry + 0x40u, 0x0014F60Au); /* +10, -10, +20 */

    wm_80089580();

    check("remaining-low-halfword-decrements",
          ld32(entry) == ((0x1234u << 16) | 1u));
    check("first-vector-integration-is-retail-addu",
          ld32(entry + 0x04u) == 110u &&
          ld32(entry + 0x08u) == 220u &&
          ld32(entry + 0x0Cu) == 330u);
    check("second-vector-integration-is-retail-addu",
          ld32(entry + 0x14u) == 11u &&
          ld32(entry + 0x18u) == 22u &&
          ld32(entry + 0x1Cu) == 33u);
    check("uv-pairs-use-38-and-3a-deltas",
          ld16(entry + 0x34u) == 1003u &&
          ld16(entry + 0x36u) == 2004u);
    check("rgb-deltas-come-from-word-40-and-clamp",
          ld32(entry + 0x3Cu) == 0xAA7800FFu);
}

static void test_expired_record_releases_exact_owner(void)
{
    u32 base;
    u32 entry;
    u32 target;

    reset_fixture();
    base = record_base(3u);
    entry = base + 4u;
    target = SLOT_BASE + 2u * SLOT_STRIDE + 0x0Au;
    st16(base, 2u);
    st32(entry, 7u << 16); /* owned, remaining=0 */
    st16(target, 0x1234u);
    st16(SLOT_BASE + 2u * 0x2A0u + 0x0Au, 0x5678u);
    st16(WRONG_BASE + 2u * SLOT_STRIDE + 0x0Au, 0x6789u);
    st16(OFFSET_BASE + 2u * SLOT_STRIDE + 0x0Au, 0x789Au);

    wm_80089580();

    check("expired-record-uses-bcc0-and-54-byte-owner-stride",
          ld16(target) == 0x1233u &&
          ld16(SLOT_BASE + 2u * 0x2A0u + 0x0Au) == 0x5678u &&
          ld16(WRONG_BASE + 2u * SLOT_STRIDE + 0x0Au) == 0x6789u &&
          ld16(OFFSET_BASE + 2u * SLOT_STRIDE + 0x0Au) == 0x789Au);
    check("expired-record-clears-owner-and-packed-life",
          ld16(base) == 0u && ld32(entry) == 0u);
}

static void test_last_record_proves_count_and_stride(void)
{
    u32 entry;

    reset_fixture();
    entry = record_base(COUNT - 1u) + 4u;
    st32(entry, (1u << 16) | 1u);
    st32(entry + 0x04u, 4u);
    st32(entry + 0x14u, 5u);
    st32(entry + 0x3Cu, 0x01020304u);

    wm_80089580();

    check("all-256-records-use-4c-byte-stride",
          ld32(entry) == (1u << 16) && ld32(entry + 0x04u) == 9u);
}

int main(void)
{
    test_unowned_record_is_untouched();
    test_active_record_exact_updates();
    test_expired_record_releases_exact_owner();
    test_last_record_proves_count_and_stride();
    if (s_failures != 0)
        return EXIT_FAILURE;
    puts("W34N38 0x80089580 full-body certificate PASS");
    return EXIT_SUCCESS;
}

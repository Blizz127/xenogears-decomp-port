#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_75d4c.h"

#define POOL_PTR      0x8009BE24u
#define POOL          0x800A8000u
#define OLD_PRESENCE  0x8006EE70u
#define RESYNC_TIMER  0x8006EF8Eu
#define PRIMARY       0x8006F368u
#define NEW_PRESENCE  0x8006F8E5u
#define INPUT_FLAGS   0x8006EE68u
#define MODE          0x8009BE10u

static int failures;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void seed_slot(u32 channel, u32 active_base, u32 backup_base)
{
    u32 slot = POOL + channel * 0x80u;

    write16(slot + 0x224u, 0x7777u);
    write32(slot + 0xA8u, active_base + 0u);
    write32(slot + 0xACu, active_base + 1u);
    write32(slot + 0xB0u, active_base + 2u);
    write32(slot + 0xD8u, active_base + 3u);
    write32(slot + 0x228u, backup_base + 0u);
    write32(slot + 0x22Cu, backup_base + 1u);
    write32(slot + 0x230u, backup_base + 2u);
    write32(slot + 0x258u, backup_base + 3u);
    write16(RESYNC_TIMER + channel * 6u, 0x1111u);
}

static void seed_common(void)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    write32(POOL_PTR, POOL);
    seed_slot(0u, 0x1000u, 0x2000u);
    seed_slot(1u, 0x3000u, 0x4000u);
    seed_slot(2u, 0x5000u, 0x6000u);
}

static void test_reconciliation(void)
{
    u32 slot0 = POOL;
    u32 slot1 = POOL + 0x80u;

    seed_common();
    *(u8 *)PSX_ADDR(OLD_PRESENCE + 0u) = 1u;
    *(u8 *)PSX_ADDR(NEW_PRESENCE + 0u) = 0u;
    *(u8 *)PSX_ADDR(OLD_PRESENCE + 2u) = 0u;
    *(u8 *)PSX_ADDR(NEW_PRESENCE + 1u) = 1u;
    *(u8 *)PSX_ADDR(OLD_PRESENCE + 4u) = 1u;
    *(u8 *)PSX_ADDR(NEW_PRESENCE + 2u) = 1u;
    *(u8 *)PSX_ADDR(PRIMARY + 0u) = 0xFFu;
    *(u8 *)PSX_ADDR(PRIMARY + 1u) = 3u;
    *(u8 *)PSX_ADDR(PRIMARY + 2u) = 4u;

    wm_80075D4C();

    check(read32(slot0 + 0xA8u) == 0x2000u &&
              read32(slot0 + 0xACu) == 0x2001u &&
              read32(slot0 + 0xB0u) == 0x2002u &&
              read32(slot0 + 0xD8u) == 0x2003u,
          "removal.restores.backup.to.active");
    check(read32(slot1 + 0x228u) == 0x3000u &&
              read32(slot1 + 0x22Cu) == 0x3001u &&
              read32(slot1 + 0x230u) == 0x3002u &&
              read32(slot1 + 0x258u) == 0x3003u,
          "addition.saves.active.to.backup");
    check(read16(slot1 + 0x224u) == 0u,
          "addition.clears.slot.control");
    check(read16(RESYNC_TIMER + 6u) == 0x400u,
          "addition.arms.resync.timer");
    check(read32(MODE) == 2u, "active.party.selects.mode2");
    check(read32(POOL + 2u * 0x80u + 0xA8u) == 0x5000u &&
              read32(POOL + 2u * 0x80u + 0x228u) == 0x6000u,
          "equal.presence.leaves.slot.unchanged");
}

static void test_idle_and_freeze(void)
{
    seed_common();
    *(u8 *)PSX_ADDR(PRIMARY + 0u) = 0xFFu;
    *(u8 *)PSX_ADDR(PRIMARY + 1u) = 0xFFu;
    *(u8 *)PSX_ADDR(PRIMARY + 2u) = 0xFFu;
    wm_80075D4C();
    check(read32(MODE) == 1u, "no.active.party.selects.mode1");

    write16(INPUT_FLAGS, 0x4000u);
    write32(MODE, 0x12345678u);
    *(u8 *)PSX_ADDR(PRIMARY) = 1u;
    *(u8 *)PSX_ADDR(NEW_PRESENCE) = 1u;
    wm_80075D4C();
    check(read32(MODE) == 0x12345678u,
          "input.freeze.preserves.mode");
}

int main(void)
{
    test_reconciliation();
    test_idle_and_freeze();
    if (failures != 0)
        return 1;
    puts("W34N14 75D4C PRODUCTION CERTIFICATE PASS");
    return 0;
}

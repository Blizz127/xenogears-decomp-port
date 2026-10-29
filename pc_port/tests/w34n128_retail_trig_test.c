/* W34N128: world-map retail trig entry points.
 *
 * The decomp symbol map names SLUS 0x8003F8B0 `rcos` and 0x8003F8CC `rsin`,
 * but 0x8003F8B0 returns the SINE halfword of the retail table and 0x8003F8CC
 * the COSINE halfword.  The world-map transcriptions call them by the decomp
 * names (world_map_retail_trig.h).  This test executes the pinned retail
 * instructions and table from the user's SLUS_006.64 in the MIPS interpreter
 * and requires wm_retail_rcos / wm_retail_rsin, backed by the real PsyCross
 * rsin / rcos, to return the same v0 for every angle of several 32-bit pages.
 * With -DW34N128_MUTANT_CONVENTIONAL the header is bypassed and the
 * conventional PsyCross functions are called under the decomp names, which is
 * what the world map did before; the test must then fail. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "world_map_retail_trig.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define HALT_PC UINT32_C(0x80001000)

#if defined(W34N128_MUTANT_CONVENTIONAL)
static int port_rcos(int a) { return rcos(a); }
static int port_rsin(int a) { return rsin(a); }
#else
static int port_rcos(int a) { return wm_retail_rcos(a); }
static int port_rsin(int a) { return wm_retail_rsin(a); }
#endif

static int bus_read(void* opaque, uint32_t address, unsigned width, uint32_t* value)
{
    (void)opaque;
    if (address < 0x80000000u || address + width > 0x80200000u) return -1;
    *value = 0;
    memcpy(value, PSX_ADDR(address), width);
    return 0;
}

static int bus_write(void* opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque; (void)address; (void)width; (void)value;
    return -1; /* the trig leaves never store */
}

static int32_t retail(uint32_t entry, uint32_t angle)
{
    PcPortMipsBus bus = {0};
    PcPortMipsCpu cpu;

    bus.read = bus_read;
    bus.write = bus_write;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = angle;
    cpu.gpr[31] = HALT_PC;
    if (PcPortMipsRun(&cpu, entry, HALT_PC, 64u) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "W34N128 FAIL retail entry %08x did not return\n", entry);
        return INT32_MIN;
    }
    return (int32_t)cpu.gpr[2];
}

int main(void)
{
    static const uint32_t pages[] = {
        0x00000000u, 0x00001000u, 0xFFFFF000u, 0x7FFFF000u, 0x80000000u, 0xFFFF8000u,
    };
    FILE* image = fopen("disc/SLUS_006.64", "rb");
    unsigned long tested = 0, bad = 0;
    unsigned p;

    if (image == NULL) { fputs("W34N128 FAIL disc/SLUS_006.64 missing\n", stderr); return 2; }
    /* SLUS file offset = vaddr - 0x80010000 + 0x800. */
    if (fseek(image, (long)(0x8003F8B0u - 0x8000F800u), SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x8003F8B0u), 1, 0x38u, image) != 0x38u ||
        fseek(image, (long)(0x800523F0u - 0x8000F800u), SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x800523F0u), 1, 0x4000u, image) != 0x4000u) {
        fputs("W34N128 FAIL could not read the retail trig code/table\n", stderr);
        fclose(image);
        return 2;
    }
    fclose(image);

    for (p = 0; p < sizeof(pages) / sizeof(pages[0]); p++) {
        uint32_t low;
        for (low = 0; low < 0x1000u; low++) {
            uint32_t angle = pages[p] + low;
            int32_t r_rcos = retail(0x8003F8B0u, angle);
            int32_t r_rsin = retail(0x8003F8CCu, angle);
            int32_t n_rcos = port_rcos((int)angle);
            int32_t n_rsin = port_rsin((int)angle);
            tested += 2;
            if (r_rcos != n_rcos || r_rsin != n_rsin) {
                if (bad < 6)
                    fprintf(stderr, "W34N128 FAIL angle=%08x retail rcos/rsin=%d/%d port=%d/%d\n",
                            angle, r_rcos, r_rsin, n_rcos, n_rsin);
                bad++;
            }
        }
    }
    /* The retail table itself: entry 0x400 is {sin 4096, cos 0}. */
    if (retail(0x8003F8B0u, 0x400u) != 4096 || retail(0x8003F8CCu, 0x400u) != 0) {
        fputs("W34N128 FAIL 0x8003F8B0 is not the sine entry\n", stderr);
        bad++;
    }
    if (bad) {
        fprintf(stderr, "W34N128 FAIL %lu of %lu comparisons differ\n", bad, tested);
        return 1;
    }
    printf("W34N128 PASS %lu comparisons: wm_retail_rcos/rsin (real PsyCross rsin/rcos) "
           "equal the retail 0x8003F8B0/0x8003F8CC bodies\n", tested);
    return 0;
}

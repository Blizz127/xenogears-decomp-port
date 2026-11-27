/* Compare the renderer's 12-bit phase convention with two guest SLUS
 * entries executed by the test interpreter against the owner's image. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <psx/libgte.h>
#include "psx_memory.h"
#include "battle_mips_adapter.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define HALT_PC UINT32_C(0x80001000)

static int bus_read(void* opaque, uint32_t address, unsigned width, uint32_t* value)
{
    (void)opaque;
    if (address < 0x80000000u || width > sizeof(*value) || address > 0x80200000u - width) return -1;
    *value = 0;
    memcpy(value, PSX_ADDR(address), width);
    return 0;
}

static int bus_write(void* opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque; (void)address; (void)width; (void)value;
    return -1; /* the trig leaves never store */
}

static uint64_t guest_steps;

static int32_t retail(uint32_t entry, uint32_t angle)
{
    PcPortMipsBus bus = {0};
    PcPortMipsCpu cpu;

    bus.read = bus_read;
    bus.write = bus_write;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = angle;
    cpu.gpr[31] = HALT_PC;
    if (PcPortMipsRun(&cpu, entry, HALT_PC, 64u) != PC_PORT_MIPS_HALTED ||
        cpu.pc != HALT_PC || cpu.steps == 0u) {
        fprintf(stderr, "W34N128 FAIL retail entry %08x did not return\n", entry);
        return INT32_MIN;
    }
    guest_steps += cpu.steps;
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
            int32_t guest_sine = retail(0x8003F8B0u, angle);
            int32_t guest_cosine = retail(0x8003F8CCu, angle);
            int phase = (int)(angle & 0xfffu);
            int32_t renderer_sine = rsin(phase);
            int32_t renderer_cosine = rcos(phase);
#ifdef W34N128_SWAP_RENDERER_RESULTS
            int32_t swapped = renderer_sine;
            renderer_sine = renderer_cosine;
            renderer_cosine = swapped;
#endif
            tested += 2;
            if (guest_sine != renderer_sine || guest_cosine != renderer_cosine) {
                if (bad < 6)
                    fprintf(stderr, "W34N128 FAIL angle=%08x guest sine/cosine=%d/%d renderer=%d/%d\n",
                            angle, guest_sine, guest_cosine, renderer_sine, renderer_cosine);
                bad += (unsigned long)(guest_sine != renderer_sine) +
                       (unsigned long)(guest_cosine != renderer_cosine);
            }
        }
    }
    /* The retail table itself: entry 0x400 is {sin 4096, cos 0}. */
    if (retail(0x8003F8B0u, 0x400u) != 4096 || retail(0x8003F8CCu, 0x400u) != 0) {
        fputs("W34N128 FAIL 0x8003F8B0 is not the sine entry\n", stderr);
        return 1;
    }
    if (bad) {
        fprintf(stderr, "W34N128 FAIL %lu of %lu comparisons differ\n", bad, tested);
        return 1;
    }
    printf("W34N128 PASS %lu comparisons of renderer sine/cosine and owned SLUS results"
           " (%" PRIu64 " guest instructions, including the quarter-turn checks)\n",
           tested, guest_steps);
    return 0;
}

/* Run four collision probes from the user's world overlay with scripted
 * terrain services. Write a canonical trace of calls, results and scratch RAM. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { overlay_size = 180422 };
static const uint32_t overlay = 0x8006faf0u, scratch_base = 0x1f800000u;
static const uint32_t base = 0x800d7000u, stack = 0x801ff000u, halt = 0x80001000u;
static uint8_t ram[2 * 1024 * 1024], scratch[1024];
static uint32_t random_state = 777;
static uint32_t terrain[2], blocked[2];
static unsigned terrain_calls, blocked_calls, calls;

static uint32_t random_word(void)
{
    random_state = random_state * 1103515245u + 12345u;
    return random_state >> 8;
}

static uint8_t *mapped(uint32_t address, unsigned width)
{
    uint32_t ram_offset = address - 0x80000000u;
    uint32_t scratch_offset = address - scratch_base;
    if (width <= sizeof ram && ram_offset <= sizeof ram - width)
        return ram + ram_offset;
    if (width <= sizeof scratch && scratch_offset <= sizeof scratch - width)
        return scratch + scratch_offset;
    return NULL;
}

static int read_word(void *unused, uint32_t a, unsigned width, uint32_t *v)
{
    (void)unused;
    const uint8_t *p = mapped(a, width);
    if (!p || !width || width > 4) return -1;
    *v = 0;
    for (unsigned i = 0; i < width; ++i) *v |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int write_word(void *unused, uint32_t a, unsigned width, uint32_t v)
{
    (void)unused;
    uint8_t *p = mapped(a, width);
    if (!p || !width || width > 4) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(v >> (i * 8));
    return 0;
}

static void trace_word(uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i)
        if (putchar((int)((v >> (i * 8)) & 255)) == EOF) exit(2);
}

static int service(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    switch (target) {
    case 0x80093354u:
    case 0x80093f18u: {
        const uint8_t *point = mapped(cpu->gpr[4], 16);
        if (!point) return -1;
        trace_word(target);
        trace_word(cpu->gpr[4]);
        if (fwrite(point, 1, 16, stdout) != 16) exit(2);
        cpu->gpr[2] = target == 0x80093354u ? 0 : terrain[terrain_calls++ % 2];
        break;
    }
    case 0x80094060u:
        trace_word(target);
        trace_word(cpu->gpr[4]);
        trace_word(cpu->gpr[5]);
        cpu->gpr[2] = blocked[blocked_calls++ % 2];
        break;
    default:
        return 0;
    }
    ++calls;
    return 1;
}

static uint32_t random_coord(uint32_t origin)
{
    uint32_t cell = random_word() & 63;
    return origin + cell * 0x80000 + random_word() % 0x80000;
}

int main(int argc, char **argv)
{
    static const uint32_t entries[] = {
        0x8009443cu, 0x800945c8u, 0x80094750u, 0x800948d8u
    };
    if (argc != 2) return 2;
    FILE *disc = fopen(argv[1], "rb");
    if (!disc) return 2;
    int loaded = fread(mapped(overlay, overlay_size), 1, overlay_size, disc) == overlay_size &&
                 fgetc(disc) == EOF && !ferror(disc);
    if (fclose(disc) || !loaded) return 2;

    unsigned tested[4] = {0};
    PcPortMipsBus bus = { .read = read_word, .write = write_word, .bridge = service };
    for (unsigned n = 0; n < 40000; ++n) {
        unsigned probe = random_word() & 3u;
        uint32_t x = random_coord(0x7000000u);
        uint32_t z = random_coord(0x2800000u);
        int32_t dx = (int32_t)(random_word() % 8193) - 4096;
        int32_t dz = (int32_t)(random_word() % 8193) - 4096;
        uint32_t mode = random_word() % 3;
        if ((probe < 2 && !dx) || (probe >= 2 && !dz)) continue;
        if ((probe == 0 && dx < 0) || (probe == 1 && dx > 0)) dx = -dx;
        if ((probe == 2 && dz < 0) || (probe == 3 && dz > 0)) dz = -dz;
        for (unsigned i = 0; i < 64; ++i) scratch[i] = (uint8_t)random_word();
        write_word(NULL, scratch_base + 0x10, 4, x + (uint32_t)(dx * 8));
        write_word(NULL, scratch_base + 0x18, 4, z + (uint32_t)(dz * 8));
        uint32_t inputs[8] = {x, 0xffed0000u, z, 0, (uint32_t)dx, 0, (uint32_t)dz, 0};
        for (unsigned i = 0; i < 8; ++i) write_word(NULL, base + 4 * i, 4, inputs[i]);
        terrain[0] = random_word() % 5;
        terrain[1] = random_word() % 5;
        uint32_t collision = random_word() & 1;
        blocked[0] = collision * ((random_word() & 1) ? 1u : 0x10000u);
        blocked[1] = random_word() & 1;
        calls = terrain_calls = blocked_calls = 0;
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = base;
        cpu.gpr[5] = base + 16;
        cpu.gpr[6] = scratch_base;
        cpu.gpr[7] = mode;
        cpu.gpr[29] = stack;
        cpu.gpr[31] = halt;
        trace_word(n);
        trace_word(entries[probe]);
        if (PcPortMipsRun(&cpu, entries[probe], halt, 5000) != PC_PORT_MIPS_HALTED ||
            cpu.pc != halt || cpu.gpr[29] != stack || cpu.gpr[31] != halt) {
            fprintf(stderr, "collision probe %u failed to return in case %u\n", probe, n);
            return 1;
        }
        trace_word(calls);
        trace_word(cpu.gpr[2]);
        trace_word((uint32_t)cpu.steps);
        if (fwrite(scratch, 1, 64, stdout) != 64) return 2;
        ++tested[probe];
    }
    fprintf(stderr, "Guest collision probe cases: %u, %u, %u, %u\n",
            tested[0], tested[1], tested[2], tested[3]);
    return fflush(stdout) == EOF;
}

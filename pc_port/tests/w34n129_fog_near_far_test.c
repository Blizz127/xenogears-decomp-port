/* Observe the fog service call made by the user's World overlay fragment. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>

#define ENTRY 0x80072d74u
#define RETURN_PC 0x80072d9cu
#define WORLD_BASE 0x8006faf0u

typedef struct {
    uint8_t code[RETURN_PC - ENTRY];
    uint32_t state, screen_distance;
    uint32_t near_depth, far_depth, height;
    unsigned calls;
} FogCase;

static int read_guest(void *ctx, uint32_t address, unsigned width, uint32_t *out)
{
    FogCase *fog = ctx;
    if (width != 4)
        return -1;
    if (address == 0x8009d7ccu) {
        *out = fog->state;
    } else if (address == 0x8009bcdcu) {
        *out = fog->screen_distance;
    } else {
        if (address < ENTRY || address > RETURN_PC - width || (address & 3u))
            return -1;
        const uint8_t *p = fog->code + (address - ENTRY);
        *out = p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    }
    return 0;
}

static int capture_fog(void *ctx, PcPortMipsCpu *cpu, uint32_t address)
{
    if (address != 0x80048ab0u)
        return 0;
    FogCase *fog = ctx;
    ++fog->calls;
    fog->near_depth = cpu->gpr[4];
    fog->far_depth = cpu->gpr[5];
    fog->height = cpu->gpr[6];
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FogCase fog = {0};
    FILE *file = fopen(argv[1], "rb");
    if (!file) return 2;
    int ok = !fseek(file, ENTRY - WORLD_BASE, SEEK_SET) &&
             fread(fog.code, 1, sizeof fog.code, file) == sizeof fog.code;
    if (fclose(file) || !ok) return 2;

    const uint32_t states[] = {0, 1, 2, 3, UINT32_MAX};
    const uint32_t heights[] = {256, 512};
    const int disabled = PcPortMipsInterpreterDisabled();
    unsigned cases = 0;
    for (size_t i = 0; i < sizeof states / sizeof *states; ++i) {
        for (size_t j = 0; j < sizeof heights / sizeof *heights; ++j) {
            fog.state = states[i];
            fog.screen_distance = heights[j];
            fog.calls = 0;
            PcPortMipsBus bus = {.opaque = &fog, .read = read_guest, .bridge = capture_fog};
            PcPortMipsCpu cpu;
            PcPortMipsCpuInit(&cpu, &bus);
            cpu.gpr[31] = RETURN_PC;
            int status = PcPortMipsRun(&cpu, ENTRY, RETURN_PC, 64);
            if (disabled) {
                ok = status == PC_PORT_MIPS_INTERPRETER_DISABLED && !cpu.steps && !fog.calls;
            } else {
                ok = status == PC_PORT_MIPS_HALTED && cpu.pc == RETURN_PC && cpu.steps &&
                     fog.calls == 1 && fog.near_depth == (fog.state == 2 ? 2816u : 2048u) &&
                     fog.far_depth == 3712u && fog.height == fog.screen_distance;
            }
            if (!ok) {
                fprintf(stderr, "fog state=%u height=%u status=%d calls=%u observed=%u/%u/%u steps=%llu\n",
                        fog.state, fog.screen_distance, status, fog.calls, fog.near_depth,
                        fog.far_depth, fog.height, (unsigned long long)cpu.steps);
                return 1;
            }
            ++cases;
        }
    }
    printf("%u World fog cases: %s\n", cases, disabled ? "refused before executing" :
           "one guest fog call each with verified near, far and screen distance");
    return 0;
}

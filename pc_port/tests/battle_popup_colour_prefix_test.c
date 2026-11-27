/* Check the guest popup shade and the resident sine/cosine lookup at quarter turns. */
#include "lib/owned_battle_bus.h"

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[1], 0x800, 0x80010000u, 0x49800u);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    const int32_t sin_quarter[] = {0, 4096, 0, -4096};
    const uint8_t shade_quarter[] = {192, 255, 192, 128};
    uint32_t sprite = 0x80160000u;
    for (unsigned phase = 0; phase < 4; phase++) {
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[4] = 0xba000000u | (phase * 1024);
        test_run(&cpu, 0x8003f8b0u);
        assert((int32_t)cpu.gpr[2] == sin_quarter[phase]);
        test_init(&cpu);
        cpu.gpr[4] = phase * 1024;
        test_run(&cpu, 0x8003f8ccu);
        assert((int32_t)cpu.gpr[2] == sin_quarter[(phase + 1) % 4]);

        uint8_t *rgb = test_ram + (sprite - TEST_RAM_BASE) + 0x28;
        memset(rgb - 1, 0xa5, 5);
        test_init(&cpu);
        cpu.gpr[4] = sprite;
        cpu.gpr[5] = phase * 16;
        int rc = PcPortMipsRun(&cpu, 0x800bcafcu, 0x8001f6b0u, 200);
        assert(rc == PC_PORT_MIPS_HALTED && cpu.gpr[4] == sprite);
        assert(rgb[-1] == 0xa5 && rgb[3] == 0xa5);
        for (unsigned c = 0; c < 3; c++)
            assert(rgb[c] == shade_quarter[phase]);
    }
    puts("Verified guest popup RGB at four pulse phases and both resident trigonometric lookups.");
}

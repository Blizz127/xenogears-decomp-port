#include "lib/owned_battle_bus.h"

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    const uint32_t owners = 0x800ccb68u;
    for (unsigned i = 0; i < 3; i++) {
        memset(test_ram + (owners - TEST_RAM_BASE), 0, 44);
        if (i < 2)
            test_put(owners + (i ? 4u : 12u), 4, 0x80140000u);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[17] = 0x800c3eb0u;
        cpu.gpr[18] = 8u;
        cpu.gpr[20] = 0x800c0000u;
        cpu.gpr[21] = 0x8cb8u;
        cpu.gpr[29] -= 48;
        for (unsigned j = 0; j < 7; j++)
            test_put(cpu.gpr[29] + 16 + 4*j, 4, j == 6 ? 0x8008067cu : 0x44550000u+j);
        uint32_t end = i < 2 ? 0x8001d0a4u : 0x8008067cu;
        assert(PcPortMipsRun(&cpu, 0x800bce4cu, end, 1000) == PC_PORT_MIPS_HALTED);
        if (i < 2)
            assert(cpu.gpr[4] == 0x80140000u);
        else
            assert(cpu.gpr[29] == 0x801ff000u && cpu.gpr[31] == end);
    }
    puts("Verified the guest popup owner search at two occupied slots and its all-empty return.");
}

#include "lib/owned_battle_bus.h"

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    for (unsigned i = 0; i < 2; i++) {
        const uint32_t owner = 0x80140000u + 0x100u * i;
        test_put(0x800ccb68u, 4, owner);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[18] = i ? 0x18u : 8u;
        cpu.gpr[29] -= 48;
        assert(PcPortMipsRun(&cpu, 0x800bcdd4u, 0x8001d0a4u, 1000) == PC_PORT_MIPS_HALTED);
        assert(cpu.gpr[4] == owner && cpu.gpr[31] == 0x800bce4cu);
        assert(test_get(0x800ccb68u, 4) == owner);
    }
    puts("Verified the first guest popup owner lookup and its argument at the call boundary.");
}

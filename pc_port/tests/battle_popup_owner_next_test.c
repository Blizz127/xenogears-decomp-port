#include "lib/owned_battle_bus.h"

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    const uint32_t table = 0x80160000u;
    for (unsigned empty = 0; empty < 2; empty++) {
        uint32_t owner = empty ? 0 : 0x80161000u;
        test_put(table, 4, owner);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[17] = table - 0x100u;
        cpu.gpr[21] = 0x100u;
        cpu.gpr[2] = empty;
        uint32_t end = empty ? 0x800bce68u : 0x800bce44u;
        assert(PcPortMipsRun(&cpu, 0x800bcdfcu, end, 200) == PC_PORT_MIPS_HALTED);
        assert(cpu.gpr[4] == owner && cpu.gpr[16] == owner);
        assert(test_get(table, 4) == owner);
    }
    puts("Verified the guest popup owner branches for occupied and empty records.");
}

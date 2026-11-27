#include "lib/owned_battle_bus.h"

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[1], 0x800, 0x80010000u, 0x49800u);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    const uint32_t task = 0x80140000u, object = 0x80141000u;
    for (unsigned active = 0; active < 2; active++) {
        memset(test_ram + (task - TEST_RAM_BASE), 0x55, 0x80);
        memset(test_ram + (object - TEST_RAM_BASE), 0x55, 0x80);
        test_put(0x800c4922u, 1, 0);
        test_put(0x800ccb3cu, 4, object);
        test_put(0x800591acu, 1, active);
        test_put(0x80059464u, 4, 9);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[2] = task;
        cpu.gpr[17] = 0x800c3eb0u;
        cpu.gpr[18] = 0x800c0000u;
        cpu.gpr[29] -= 32;
        for (unsigned j=0; j<4; j++)
            test_put(cpu.gpr[29] + 16 + 4*j, 4, j==3 ? cpu.gpr[31] : 0x50200000u+j);
        test_run(&cpu, 0x800bccd4u);
        assert(test_get(0x800c3748u, 4) == task);
        assert(test_get(task+8, 4) == 0x800bcbb4u);
        assert(test_get(task+12, 4) == 0x800bcb54u);
        assert(test_get(task+0x1c, 4) == object);
        assert(test_get(task+0x24, 4) == 0);
        assert((test_get(object+0x2b, 1) & 1) == 0);
        assert(test_get(0x80059464u, 4) == 9u-active);
        assert(cpu.gpr[29] == 0x801ff000u);
    }
    puts("Verified the guest popup task publication, callback registration and active count using original setters.");
}

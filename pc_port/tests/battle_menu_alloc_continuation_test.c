/* The original allocation returns a new menu record in V0. The continuation
 * publishes and initializes that record before returning to its caller. */
#include "lib/owned_battle_bus.h"

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[1], 0x800, 0x80010000u, 0x49800u);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    const unsigned size = 0x5da4u;
    for (unsigned n = 0; n < 2; n++) {
        uint32_t record = 0x80140000u + n * 0x8000u;
        uint8_t *data = test_ram + (record - TEST_RAM_BASE);
        memset(data - 1, 0x5a, size + 2);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[29] -= 24;
        cpu.gpr[2] = record;
        test_put(cpu.gpr[29] + 16, 4, cpu.gpr[31]);
        test_run(&cpu, 0x8007fed8u);
        assert(cpu.gpr[29] == 0x801ff000u);
        assert(test_get(0x800d2db4u, 4) == record);
        assert(test_get(record + 0x5d9c, 2) == 160);
        assert(test_get(record + 0x5d9e, 2) == 100);
        assert(data[-1] == 0x5a && data[size] == 0x5a);
        for (unsigned i = 0; i < size; i++)
            if (i != 0x5d9c && i != 0x5d9e)
                assert(data[i] == 0);
    }
    puts("Verified two guest menu allocations with the original zeroing service, dimensions and redzones.");
}

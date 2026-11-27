/* Verify the actual production table hydration and the disc's frame-size query. */
#include "lib/owned_battle_bus.h"

extern unsigned char D_8001C76C[0x170];

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[1], 0x800, 0x80010000u, 0x49800u);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    uint32_t header = 0x8001c778u;
    assert(memcmp(D_8001C76C, test_ram + 0x1c76c, sizeof(D_8001C76C)) == 0);
    uint32_t offset = test_get(header + 16, 4);
    uint32_t count = test_get(header + 20, 4);
    assert(offset == 28 && count == 8);
    uint32_t cursor = header + offset;
    uint32_t size = 0;
    for (uint32_t i = 0; i < count; i++) {
        assert(cursor + 2 <= 0x8001c76c + sizeof(D_8001C76C));
        size += 4 * (1 + test_get(cursor, 1));
        cursor += 4 * (1 + test_get(cursor + 1, 1));
    }
    PcPortMipsCpu cpu;
    test_init(&cpu);
    cpu.gpr[4] = header;
    test_run(&cpu, 0x800b16a4u);
    assert(size == 208 && cpu.gpr[2] == size);
    assert(memcmp(D_8001C76C, test_ram + 0x1c76c, sizeof(D_8001C76C)) == 0);
    puts("Verified owned popup table hydration, eight records and 208-byte guest frame size.");
}

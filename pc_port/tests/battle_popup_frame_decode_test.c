/* Exercise the actual owned decoder and ShadeTex service with both original modes. */
#include "lib/owned_battle_bus.h"

extern unsigned char D_8001C76C[0x170];

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[1], 0x800, 0x80010000u, 0x49800u);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    uint32_t header = 0x8001c778u;
    uint32_t input = header + test_get(header + 16, 4);
    uint32_t count = test_get(header + 20, 4);
    assert(count == 8);
    uint8_t first[208];
    for (uint32_t mode = 0; mode < 2; mode++) {
        const uint32_t base = 0x80140020u;
        uint8_t *guarded = test_ram + (base - TEST_RAM_BASE) - 32;
        memset(guarded, 0xa5, 32 + 208 + 32);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[4] = header;
        cpu.gpr[5] = base;
        cpu.gpr[7] = mode;
        test_run(&cpu, 0x800b1720u);
        assert(memcmp(D_8001C76C, test_ram + 0x1c76c, sizeof(D_8001C76C)) == 0);
        for (unsigned i = 0; i < 32; i++)
            assert(guarded[i] == 0xa5 && guarded[32 + 208 + i] == 0xa5);
        uint32_t packet = base;
        uint32_t record = input;
        for (uint32_t n = 0; n < count; n++) {
            uint32_t words = test_get(record, 1);
            assert(words > 0 && test_get(packet + 3, 1) == words);
            unsigned length = 4 * (words + 1);
            uint8_t *data = test_ram + (packet - TEST_RAM_BASE);
            if (mode == 0) {
                memcpy(first + (packet - base), data, length);
            } else {
                /* Changing the texture-shading mode changes bit zero of
                 * the command byte in each primitive and leaves its data intact. */
                for (unsigned i = 0; i < length; i++)
                    assert((first[packet - base + i] ^ data[i]) == (i == 7));
            }
            packet += length;
            record += 4 * (1 + test_get(record + 1, 1));
        }
        assert(packet == base + 208);
    }
    puts("Verified both guest decoder modes, eight packet lengths, input immutability and output bounds.");
}

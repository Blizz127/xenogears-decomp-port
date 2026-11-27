/* Exercise the public whole-dispatch oracle with a short set of opening-opcode
 * cases. The caller supplies the original US executable. */
#define main exhaustive_c1_main
#include "sprite_dispatch_c1_retail_test.c"
#undef main

GUARD(Retail8003F738RotMatrix)
extern uint32_t PcPort_UnhandledSpriteCommandCount(void);

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <SLUS_006.64>\n", argv[0]);
        return 2;
    }
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    assert((uintptr_t)&g_RandomSeed + 4 <= UINT32_MAX);
    FILE *image = fopen(argv[1], "rb");
    if (!image) {
        perror(argv[1]);
        return 2;
    }
    int loaded = fseek(image, 0x800, SEEK_SET) == 0 &&
        fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000;
    if (fclose(image) || !loaded) return 2;

    const unsigned operands[] = {0, 1, 0x7f, 0x80, 0xff, 0x40};
    const int16_t scales[] = {0, 1, -1, 4095, -4095, 4096, 8192, 32767, -32768};
    const uint16_t timers[] = {0, 1, 1023, 1024, 1025, 32767, 32768, 65535};
    const uint32_t seeds[] = {0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u,
                              0x12345678u, 0x80000001u, 0xff0000ffu};
    uint32_t unhandled = PcPort_UnhandledSpriteCommandCount();
    compare(0x40, 8192, 0, 1, 4, 0, 0);

    for (unsigned byte = 0; byte < 256; ++byte) {
        for (unsigned alias = 0; alias < 21; ++alias) {
            unsigned sample = byte + 3 * alias;
            compare(byte, scales[sample % 9], timers[(byte / 3 + alias) % 8],
                    seeds[(byte + alias) % 8], (byte + alias) % 5, alias, sample % 4);
        }
    }
    for (unsigned operand = 0; operand < sizeof(operands) / sizeof(operands[0]); ++operand) {
        for (unsigned scale = 0; scale < sizeof(scales) / sizeof(scales[0]); ++scale) {
            for (unsigned timer = 0; timer < sizeof(timers) / sizeof(timers[0]); ++timer) {
                compare(operands[operand], scales[scale], timers[timer],
                        seeds[(operand + 3 * scale + timer) % 8],
                        (scale + timer) % 5, 0, (operand + timer) % 4);
            }
        }
    }
    assert(cases == 5809 && PcPort_UnhandledSpriteCommandCount() == unhandled);
    printf("PASS %u focused owner-backed C1 comparisons\n", cases);
    return 0;
}

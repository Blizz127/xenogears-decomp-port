/* Focused coverage at the same host and owner-CPU entry as the exhaustive test. */
#define main exhaustive_bb_main
#include "sprite_dispatch_bb_retail_test.c"
#undef main

/* The release translation unit also references the rotation service on other
 * branches. This fixture exercises no operation that calls it. */
GUARD(Retail8003F738RotMatrix)
extern uint32_t PcPort_UnhandledSpriteCommandCount(void);

static void check_nearby_commands(void)
{
    uint8_t *sprite = (uint8_t *)fixture.sprite;
    uint8_t *operand = (uint8_t *)fixture.ops;
    memset(&fixture, 0, sizeof(fixture));
    sprite[0x8c] = 5;
    operand[0] = 7;
    expected = fixture;
    ((uint8_t *)expected.sprite)[0x8c] = 254;
    func_8001FBE4(sprite, 0xb8, operand);
    assert(memcmp(&fixture, &expected, sizeof(fixture)) == 0);
    func_8001FBE4(sprite, 0xbe, operand);
    assert(memcmp(&fixture, &expected, sizeof(fixture)) == 0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen(argv[1], "rb");
    assert(image && fseek(image, 0x800, SEEK_SET) == 0);
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x12000);
    assert(fclose(image) == 0);

    uint32_t unhandled = PcPort_UnhandledSpriteCommandCount();
    check_nearby_commands();
    compare(0, 0xea, 0, 0);
    const unsigned words[] = {0, 1, 0x017f, 0x7fff, 0x8000, 0xff80, 0xffff};
    const unsigned operands[] = {0, 1, 0x7f, 0x80, 0xea, 0xff};
    for (unsigned i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        unsigned old = words[i];
        for (unsigned j = 0; j < sizeof(operands) / sizeof(operands[0]); ++j)
            compare(old, operands[j], 0, (i + j) % 4);
        compare(old, old & 255u, 1, 0);
        compare(old, old >> 8, 2, 1);
        compare(old, 0x80, 3, 2);
        compare(old, 0x7f, 4, 3);
    }
    assert(PcPort_UnhandledSpriteCommandCount() == unhandled);
    printf("PASS %u owner-backed BB comparisons and two neighboring commands\n", cases);
    return 0;
}

/* Observable menu state for eight scalar operations in the player's battle overlay. */
#include "lib/owned_battle_bus.h"

static const uint32_t frame = 0x80100000u;

static void test_actors(void)
{
    const uint8_t slots[] = {0, 1, 10, 255};
    test_put(0x800c34b0u, 4, frame);
    for (unsigned i = 0; i < sizeof(slots); i++) {
        uint32_t mask = 0x8500u + slots[i];
        test_put(0x800c3448u + 2u * slots[i], 2, mask);
        PcPortMipsCpu cpu;
        test_init(&cpu);
        cpu.gpr[4] = 0xabcdef00u | slots[i];
        test_run(&cpu, 0x80089c08u);
        assert(cpu.gpr[2] == mask);
        uint32_t flag = frame + 0x15a + 0x170u * slots[i];
        uint32_t before = i & 1 ? 0xa4 : 0xa5;
        test_put(flag, 1, before);
        test_init(&cpu);
        cpu.gpr[4] = 0xabcdef00u | slots[i];
        test_run(&cpu, 0x8009ab00u);
        assert(test_get(flag, 1) == (before & ~1u));
    }
}

static void test_selection_reset(void)
{
    uint8_t *state = test_ram + (frame - TEST_RAM_BASE) + 0x2c0;
    test_put(0x800c3eacu, 4, frame);
    memset(state, 0x52, 32);
    PcPortMipsCpu cpu;
    test_init(&cpu);
    test_run(&cpu, 0x80085e78u);
    for (unsigned i = 0; i < 32; i++) {
        if (i >= 12 && i < 19)
            assert(state[i] == 0xff);
        else if (i == 22)
            assert(state[i] == 0);
        else
            assert(state[i] == 0x52);
    }
}

static void test_inactive_cleanup(void)
{
    test_put(0x800d2d28u, 4, frame);
    const uint32_t entries[] = {0x8007fce8u, 0x8007fdecu, 0x8007fb70u};
    const uint8_t states[] = {0, 3, 4, 255};
    uint8_t *data = test_ram + (frame - TEST_RAM_BASE);
    uint8_t before[256];
    memset(data, 0x5a, sizeof(before));
    data[0x8e] = data[0x96] = data[0xae] = 0;
    memcpy(before, data, sizeof(before));
    for (unsigned i = 0; i < sizeof(states); i++) {
        for (unsigned j = 0; j < sizeof(entries) / sizeof(entries[0]); j++) {
            PcPortMipsCpu cpu;
            test_init(&cpu);
            uint32_t stack = cpu.gpr[29];
            cpu.gpr[4] = 0xabcdef02u;
            test_put(0x800d2d26u, 1, states[i]);
            test_run(&cpu, entries[j]);
            assert(stack == cpu.gpr[29]);
            assert(memcmp(data, before, sizeof(before)) == 0);
            assert(test_get(0x800d2d26u, 1) == states[i]);
        }
    }
}

static void test_cursor_and_empty(void)
{
    test_put(0x800d2d28u, 4, frame);
    test_put(0x800ccb34u, 1, 0xa7);
    for (uint32_t count = 0; count < 2; count++) {
        for (uint32_t selected = 0; selected < 2; selected++) {
            PcPortMipsCpu cpu;
            test_init(&cpu);
            cpu.gpr[4] = 0xabcd0000u | count;
            cpu.gpr[5] = 0x12340000u | selected;
            test_run(&cpu, 0x8007fbe0u);
            uint8_t expected = (uint8_t)(selected - (count == selected));
            assert(test_get(frame + 0x97, 1) == expected);
            assert(test_get(frame + 0x98, 1) == 0xa7);
        }
    }
    PcPortMipsCpu cpu;
    test_init(&cpu);
    uint32_t before[32];
    memcpy(before, cpu.gpr, sizeof(before));
    test_run(&cpu, 0x800baf40u);
    assert(memcmp(before, cpu.gpr, sizeof(before)) == 0);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_image(argv[2], 0, 0x8006faf0u, 343936u);
    test_actors();
    test_selection_reset();
    test_inactive_cleanup();
    test_cursor_and_empty();
    puts("Verified eight guest menu operations: actor masks, flags, reset, inactive cleanup, cursor and empty helper.");
}

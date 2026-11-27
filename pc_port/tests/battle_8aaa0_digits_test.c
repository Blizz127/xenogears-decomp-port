#include "battle_guest_numeric_bus.h"

enum { DIGIT_COUNT = 9 };
static const uint32_t digit_address = 0x800c3cf4u;

static int digit_write(void *context, uint32_t address, unsigned width, uint32_t value)
{
    if (address < digit_address || address - digit_address >= DIGIT_COUNT || width != 1)
        return -1;
    return guest_write(context, address, width, value);
}

static void check_digits(uint32_t number)
{
    uint8_t expected[DIGIT_COUNT];
    uint32_t remaining = number;
    for (int i = DIGIT_COUNT - 1; i > 0; --i) {
        expected[i] = remaining % 10;
        remaining /= 10;
    }
    /* There are nine cells. For a ten-digit number the first cell
       carries the unsplit leading pair. A displayed zero occupies one cell. */
    /* A zero ten-millions digit blanks the leading cell too.
       For example, 100000001 is displayed as a single 1. */
    expected[0] = expected[1] ? remaining : 0;
    for (unsigned i = 0; i < DIGIT_COUNT - 1 && expected[i] == 0; ++i)
        expected[i] = UINT8_MAX;

    memset(guest_ram + (digit_address - GUEST_RAM_BASE), 0xa5, DIGIT_COUNT);
    PcPortMipsBus bus = {.read = guest_read, .write = digit_write};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = number;
    cpu.gpr[29] = 0x801ff000u;
    cpu.gpr[31] = 0x8000deacu;
    assert(PcPortMipsRun(&cpu, 0x8008aaa0u, cpu.gpr[31], 1000) == PC_PORT_MIPS_HALTED);
    assert(cpu.steps && cpu.pc == 0x8000deacu && cpu.gpr[29] == 0x801ff000u);
    assert(!memcmp(guest_ram + (digit_address - GUEST_RAM_BASE), expected, sizeof expected));
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    guest_load(argv[1], BATTLE_OVERLAY_BASE, 0, 0x8008aaa0u, 172);
    const uint32_t inputs[] = {0, 1, 9, 10, 65535, 123456, 99999999,
                               100000000, 100000001, 101234567, 110000000,
                               999999999, 1000000000, UINT32_MAX};
    for (unsigned i = 0; i < sizeof inputs / sizeof inputs[0]; ++i)
        check_digits(inputs[i]);
    printf("Battle decimal display: %zu cases passed using owner-supplied guest code.\n",
           sizeof inputs / sizeof inputs[0]);
    return 0;
}

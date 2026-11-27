#include "lib/world_guest_memory.h"

int main(int argc, char **argv)
{
    assert(argc == 2);
    load_overlay(argv[1]);
    PcPortMipsBus bus = {.read = read_guest, .write = write_guest};
    for (unsigned index = 0; index < 16; ++index) {
        uint32_t selector = index == 15 ? UINT32_MAX : index;
        for (unsigned i = 0; i < 0x2000; ++i) guest[0x100000 + i] = (uint8_t)(i * 37 + 5);
        put(0x8009be24u, 4, 0x80100000u);
        put(0x8009c610u, 4, selector);
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        assert(PcPortMipsRun(&cpu, 0x80072784u, 0x8007290cu, 2000) == PC_PORT_MIPS_HALTED);
        assert(cpu.pc == 0x8007290cu);
    }
    puts("World resume selector reached its guest endpoint for 16 inputs");
    return 0;
}

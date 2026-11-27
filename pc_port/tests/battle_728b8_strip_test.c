#include "battle_guest_numeric_bus.h"

enum { PACKET_SIZE = 40, PACKET_COUNT = 8 };
static const uint32_t packets = 0x80100000u;
static const uint32_t table = 0x80190000u;
static const uint32_t strip_entry = 0x800728b8u;
static const uint32_t add_prim_entry = 0x80043b48u;

typedef struct StripCalls {
    unsigned count;
    uint32_t packet[PACKET_COUNT];
} StripCalls;

/* Observe AddPrim on entry, then let the adapter execute the original callee. */
static int observe(void *context, PcPortMipsCpu *cpu, uint32_t target)
{
    StripCalls *calls = context;
    if (target == add_prim_entry) {
        assert(calls->count < PACKET_COUNT);
        assert(cpu->gpr[4] == table + 4);
        calls->packet[calls->count++] = cpu->gpr[5];
    }
    return 0;
}

static uint32_t read_word(uint32_t address)
{
    uint32_t value;
    assert(guest_read(NULL, address, 4, &value) == 0);
    return value;
}

static void write_word(uint32_t address, uint32_t value)
{
    assert(guest_write(NULL, address, 4, value) == 0);
}

static void check_strip(unsigned first, unsigned count)
{
    const uint32_t table_word = 0x12abcdefu;
    const uint32_t packet_word = 0x07010203u;
    uint32_t expected[PACKET_COUNT];
    StripCalls calls = {0};
    write_word(0x800ccb04u, table);
    write_word(table + 4, table_word);
    for (unsigned i = 0; i < PACKET_COUNT; ++i) {
        expected[i] = packet_word;
        write_word(packets + i * PACKET_SIZE, packet_word);
    }
    PcPortMipsBus bus = {.opaque = &calls, .read = guest_read,
                         .write = guest_write, .bridge = observe};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = packets;
    cpu.gpr[5] = count;
    cpu.gpr[6] = first;
    cpu.gpr[29] = 0x801ff000u;
    cpu.gpr[31] = 0x80073c1cu;
    assert(PcPortMipsRun(&cpu, strip_entry, cpu.gpr[31], 5000) == PC_PORT_MIPS_HALTED);
    assert(cpu.steps && cpu.pc == 0x80073c1cu && cpu.gpr[29] == 0x801ff000u);
    assert(calls.count == count);

    uint32_t head = table_word & 0x00ffffffu;
    for (unsigned i = 0; i < count; ++i) {
        unsigned index = first + 2 * i;
        uint32_t address = packets + index * PACKET_SIZE;
        assert(index < PACKET_COUNT && calls.packet[i] == address);
        expected[index] = (packet_word & 0xff000000u) | head;
        head = address & 0x00ffffffu;
    }
    assert(read_word(table + 4) == ((table_word & 0xff000000u) | head));
    for (unsigned i = 0; i < PACKET_COUNT; ++i)
        assert(read_word(packets + i * PACKET_SIZE) == expected[i]);
}

static void check_add_prim(int same_address, int uncached)
{
    uint32_t head = table + 4, packet = same_address ? head : packets;
    const uint32_t old_head = 0x12abcdefu;
    const uint32_t old_packet = 0x07010203u;
    write_word(head, old_head);
    if (!same_address) write_word(packet, old_packet);
    PcPortMipsBus bus = {.read = guest_read, .write = guest_write};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    uint32_t segment = uncached ? 0x20000000u : 0;
    cpu.gpr[4] = head | segment;
    cpu.gpr[5] = packet | segment;
    cpu.gpr[31] = 0x80073b24u;
    assert(PcPortMipsRun(&cpu, add_prim_entry, cpu.gpr[31], 100) == PC_PORT_MIPS_HALTED);
    assert(cpu.steps && cpu.pc == 0x80073b24u);
    assert(read_word(head) == ((old_head & 0xff000000u) | (packet & 0x00ffffffu)));
    if (!same_address)
        assert(read_word(packet) == ((old_packet & 0xff000000u) | (old_head & 0x00ffffffu)));
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    guest_load(argv[1], BATTLE_OVERLAY_BASE, 0, strip_entry, 128);
    guest_load(argv[2], SLUS_TEXT_BASE, SLUS_HEADER_BYTES, add_prim_entry, 60);
    check_strip(0, 0);
    check_strip(0, 1);
    check_strip(2, 2);
    check_strip(1, 3);
    for (int same = 0; same < 2; ++same)
        for (int uncached = 0; uncached < 2; ++uncached)
            check_add_prim(same, uncached);
    puts("Battle HUD strip: 4 ordering-table chains and 4 AddPrim alias cases passed using owner-supplied guest code.");
    return 0;
}

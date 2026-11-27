/* Exercise the owned guest routines on the port CPU using an isolated memory bus. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char image[2 << 20], ram[sizeof(image)];
static const uint32_t base = 0x80000000u;
static const uint32_t exit_pc = 0x801ff800u;
static unsigned shade_calls, mutate_popup;
static uint32_t stop_at, stopped_at;

static int span(uint32_t a, unsigned width)
{
    return (width == 1 || width == 2 || width == 4) &&
           a >= base && a - base <= sizeof(ram) - width;
}

static int read_bus(void *ctx, uint32_t a, unsigned width, uint32_t *v)
{
    (void)ctx;
    if (!span(a, width)) return -1;
    *v = 0;
    for (unsigned i = 0; i < width; i++)
        *v |= (uint32_t)ram[a - base + i] << (8 * i);
    return 0;
}

static int write_bus(void *ctx, uint32_t a, unsigned width, uint32_t v)
{
    (void)ctx;
    if (!span(a, width)) return -1;
    for (unsigned i = 0; i < width; i++)
        ram[a - base + i] = v >> (8 * i);
    return 0;
}

static void put(uint32_t a, unsigned width, uint32_t v)
{
    assert(write_bus(NULL, a, width, v) == 0);
}

static uint32_t get(uint32_t a, unsigned width)
{
    uint32_t v;
    assert(read_bus(NULL, a, width, &v) == 0);
    return v;
}

static int observe(void *ctx, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)ctx;
    if (target == 0x80043c24u) {
        shade_calls++;
        if (mutate_popup && shade_calls == 4) {
            uint32_t record = cpu->gpr[17];
            put(record + 2, 1, get(record + 2, 1) ^ 1);
            put(record + 3, 1, get(record + 3, 1) ^ 2);
        }
    }
    if (target == stop_at) {
        stopped_at = target;
        return -1;
    }
    return 0;
}

static void run(uint32_t entry, uint32_t halt, uint32_t gpr[32], int expected)
{
    const PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = observe};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    memcpy(cpu.gpr, gpr, sizeof(cpu.gpr));
    stopped_at = 0;
    int result = PcPortMipsRun(&cpu, entry, halt, 20000);
    if (result != expected) fprintf(stderr, "%08x returned %d at %08x (%s)\n", entry, result, cpu.pc, cpu.error);
    assert(result == expected && cpu.steps > 0);
    if (expected == PC_PORT_MIPS_UNRESOLVED_CALL) assert(stop_at && stopped_at == stop_at);
}

static void load(const char *path, size_t skip, uint32_t dest, size_t size)
{
    FILE *fp = fopen(path, "rb");
    assert(fp && fseek(fp, (long)skip, SEEK_SET) == 0);
    assert(dest >= base && dest - base + size <= sizeof(image));
    assert(fread(image + (dest - base), 1, size, fp) == size);
    assert(fclose(fp) == 0);
}

static void popup_decoder(void)
{
    const uint32_t args = 0x8001c778u, packets = 0x80140020u, halt = 0x800bd190u;
    for (int prefix = 0; prefix < 2; prefix++) {
        for (unsigned mode = 0; mode < 2; mode++) {
            memcpy(ram, image, sizeof(ram));
            uint32_t gpr[32] = {0};
            gpr[4] = args; gpr[5] = packets; gpr[7] = mode;
            gpr[29] = 0x801ff000u; gpr[31] = halt;
            memset(ram + packets - base, 0xa5, 0x180);
            stop_at = prefix ? 0x80043c24u : 0;
            mutate_popup = mode;
            shade_calls = 0;
            run(0x800b1720u, halt, gpr, prefix ? PC_PORT_MIPS_UNRESOLVED_CALL : PC_PORT_MIPS_HALTED);
            assert(shade_calls == (prefix ? 1u : 8u));
            if (!prefix) {
                unsigned changed = 0;
                for (unsigned i = 0; i < 0x180u; i++)
                    changed += get(packets + i, 1) != 0xa5;
                assert(changed > shade_calls);
            }
            assert(get(packets - 1, 1) == image[packets - 1 - base]);
        }
    }
}

static void popup_suffix(void)
{
    for (unsigned selection = 0; selection < 2; selection++) {
        memcpy(ram, image, sizeof(ram));
        uint32_t gpr[32] = {0};
        const uint32_t task = 0x80140000u, state = 0x80141000u;
        put(0x800591acu, 1, selection);
        put(0x80059464u, 4, 9u);
        put(task + 0x14, 4, 0xc1234567u);
        put(state + 0x36, 2, 0x1234u + selection);
        memset(ram + 0x1c778u, 0, 0x80);
        put(0x8001c788u, 4, 0x40);
        put(0x8001c78cu, 4, selection ? 2 : 0);
        put(0x8001c7b8u, 1, 2); put(0x8001c7b9u, 1, 1);
        put(0x8001c7c0u, 1, 3); put(0x8001c7c1u, 1, 1);
        gpr[2] = task; gpr[21] = state; gpr[29] = 0x801ff000u - 56; gpr[31] = 0x800bd0e4u;
        stop_at = 0x80031bdcu;
        run(0x800bd0e4u, 0x800bce2cu, gpr, PC_PORT_MIPS_UNRESOLVED_CALL);
    }
}

static void voice_slots(void)
{
    const uint32_t slot = 0x8006252cu, current = 0x80141000u, other = 0x80141100u;
    const uint32_t dirty = 0x80059554u, key = 0x800594fcu;
    const unsigned index = 5;
    uint32_t gpr[32] = {0};
    gpr[4] = current; gpr[5] = index; gpr[29] = 0x801ff000u; gpr[31] = exit_pc;
    stop_at = 0;
    memcpy(ram, image, sizeof(ram));
    put(slot + 4*index, 4, other);
    put(current + 4, 2, 3); put(other + 4, 2, 9);
    put(dirty, 4, 0); put(key, 4, ~0u);
    run(0x8003e724u, exit_pc, gpr, PC_PORT_MIPS_HALTED);
    assert(get(slot + 4*index, 4) == other && get(dirty, 4) == 0);
    put(current + 4, 2, 10);
    run(0x8003e724u, exit_pc, gpr, PC_PORT_MIPS_HALTED);
    assert(get(slot + 4*index, 4) == current && get(current, 2) == index);
    assert(get(dirty, 4) == (1u << index) && !(get(key, 4) & (1u << index)));
    gpr[4] = other;
    run(0x8003e83cu, exit_pc, gpr, PC_PORT_MIPS_HALTED);
    assert(get(slot + 4*index, 4) == current);
    gpr[4] = current;
    run(0x8003e83cu, exit_pc, gpr, PC_PORT_MIPS_HALTED);
    assert(get(slot + 4*index, 4) == 0);
    gpr[5] = 24;
    put(slot + 24*4, 4, other);
    run(0x8003e83cu, exit_pc, gpr, PC_PORT_MIPS_HALTED);
    assert(get(slot + 24*4, 4) == other);
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    assert(!PcPortMipsInterpreterDisabled());
    load(argv[1], 0x800, 0x80010000u, 0x49800);
    load(argv[2], 0, 0x8006faf0u, 343936);
    popup_decoder(); popup_suffix(); voice_slots();
    puts("Owned guest battle checks passed: four popup decoder cases, two allocation boundaries, five voice slot operations.");
    return 0;
}

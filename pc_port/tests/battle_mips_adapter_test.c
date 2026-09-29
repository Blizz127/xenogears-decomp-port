#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "battle_mips_adapter.h"

#define RAM_BASE 0x80001000u
#define RAM_SIZE 0x2000u
#define HALT_PC  0xfffffffcu

typedef struct TestBus {
    uint8_t ram[RAM_SIZE];
    unsigned bridge_calls;
} TestBus;

static uint32_t enc_i(unsigned op, unsigned rs, unsigned rt, int imm)
{
    return (op << 26) | (rs << 21) | (rt << 16) | ((uint16_t)imm);
}

static uint32_t enc_r(unsigned rs, unsigned rt, unsigned rd,
                      unsigned shamt, unsigned funct)
{
    return (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | funct;
}

static uint32_t enc_j(unsigned op, uint32_t target)
{
    return (op << 26) | ((target >> 2) & 0x03ffffffu);
}

static void put32(TestBus *bus, uint32_t address, uint32_t value)
{
    unsigned off = address - RAM_BASE;
    bus->ram[off + 0] = (uint8_t)value;
    bus->ram[off + 1] = (uint8_t)(value >> 8);
    bus->ram[off + 2] = (uint8_t)(value >> 16);
    bus->ram[off + 3] = (uint8_t)(value >> 24);
}

static int test_read(void *opaque, uint32_t address, unsigned width,
                     uint32_t *value)
{
    TestBus *bus = opaque;
    unsigned off;

    if (address < RAM_BASE || address + width > RAM_BASE + RAM_SIZE)
        return -1;
    off = address - RAM_BASE;
    *value = bus->ram[off];
    if (width >= 2)
        *value |= (uint32_t)bus->ram[off + 1] << 8;
    if (width == 4) {
        *value |= (uint32_t)bus->ram[off + 2] << 16;
        *value |= (uint32_t)bus->ram[off + 3] << 24;
    }
    return 0;
}

static int test_write(void *opaque, uint32_t address, unsigned width,
                      uint32_t value)
{
    TestBus *bus = opaque;
    unsigned off;

    if (address < RAM_BASE || address + width > RAM_BASE + RAM_SIZE)
        return -1;
    off = address - RAM_BASE;
    bus->ram[off] = (uint8_t)value;
    if (width >= 2)
        bus->ram[off + 1] = (uint8_t)(value >> 8);
    if (width == 4) {
        bus->ram[off + 2] = (uint8_t)(value >> 16);
        bus->ram[off + 3] = (uint8_t)(value >> 24);
    }
    return 0;
}

static int test_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    TestBus *bus = opaque;

    if (target != 0x80002000u)
        return 0;
    bus->bridge_calls++;
    cpu->gpr[2] = cpu->gpr[4] * 3u;
    return 1;
}

static int check_core_execution(void)
{
    TestBus bus;
    PcPortMipsCpu cpu;
    PcPortMipsBus ops;
    uint32_t stored;
    int rc;

    memset(&bus, 0, sizeof(bus));
    memset(&ops, 0, sizeof(ops));
    ops.opaque = &bus;
    ops.read = test_read;
    ops.write = test_write;
    ops.bridge = test_bridge;

    /*
     * t0=5; t1=7; v0=t0+t1; a0=v0+1 in jal delay slot.
     * The internal callee returns 23.  The external bridge sees a0=25 in
     * its delay slot and returns 75.  The final addu produces 100.
     */
    put32(&bus, 0x80001000u, enc_i(9, 0, 8, 5));
    put32(&bus, 0x80001004u, enc_i(9, 0, 9, 7));
    put32(&bus, 0x80001008u, enc_r(8, 9, 2, 0, 0x21));
    put32(&bus, 0x8000100cu, enc_j(3, 0x80001030u));
    put32(&bus, 0x80001010u, enc_i(9, 2, 4, 1));
    put32(&bus, 0x80001014u, enc_i(0x2b, 16, 2, 0));
    put32(&bus, 0x80001018u, enc_j(3, 0x80002000u));
    put32(&bus, 0x8000101cu, enc_i(9, 2, 4, 2));
    put32(&bus, 0x80001020u, enc_r(2, 4, 2, 0, 0x21));
    put32(&bus, 0x80001024u, enc_r(17, 0, 0, 0, 8));
    put32(&bus, 0x80001028u, 0);

    put32(&bus, 0x80001030u, enc_i(9, 4, 2, 10));
    put32(&bus, 0x80001034u, enc_r(31, 0, 0, 0, 8));
    put32(&bus, 0x80001038u, 0);

    PcPortMipsCpuInit(&cpu, &ops);
    cpu.gpr[16] = 0x80001100u;
    cpu.gpr[17] = HALT_PC;
    rc = PcPortMipsRun(&cpu, 0x80001000u, HALT_PC, 1000u);
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "battle mips adapter: execution rc=%d error=%s\n",
                rc, cpu.error);
        return 1;
    }
    if (cpu.gpr[2] != 100u || cpu.gpr[4] != 25u || bus.bridge_calls != 1u) {
        fprintf(stderr,
                "battle mips adapter: regs v0=%u a0=%u bridges=%u\n",
                cpu.gpr[2], cpu.gpr[4], bus.bridge_calls);
        return 1;
    }
    if (test_read(&bus, 0x80001100u, 4, &stored) != 0 || stored != 23u) {
        fprintf(stderr, "battle mips adapter: stored=%u, expected 23\n",
                stored);
        return 1;
    }
    return 0;
}

static int check_unknown_instruction_fails_closed(void)
{
    TestBus bus;
    PcPortMipsCpu cpu;
    PcPortMipsBus ops;
    int rc;

    memset(&bus, 0, sizeof(bus));
    memset(&ops, 0, sizeof(ops));
    ops.opaque = &bus;
    ops.read = test_read;
    ops.write = test_write;
    ops.bridge = test_bridge;
    put32(&bus, RAM_BASE, 0x44000000u); /* COP1 is not a PS1 game opcode. */

    PcPortMipsCpuInit(&cpu, &ops);
    rc = PcPortMipsRun(&cpu, RAM_BASE, HALT_PC, 4u);
    if (rc != PC_PORT_MIPS_UNSUPPORTED || cpu.error[0] == '\0') {
        fprintf(stderr,
                "battle mips adapter: unsupported instruction did not fail "
                "closed (rc=%d error=%s)\n",
                rc, cpu.error);
        return 1;
    }
    return 0;
}

static int check_load_delay_and_unaligned_pair(void)
{
    TestBus bus;
    PcPortMipsCpu cpu;
    PcPortMipsBus ops;
    int rc;

    memset(&bus, 0, sizeof(bus));
    memset(&ops, 0, sizeof(ops));
    ops.opaque = &bus;
    ops.read = test_read;
    ops.write = test_write;
    ops.bridge = test_bridge;

    put32(&bus, 0x80001100u, 0x44332211u);
    put32(&bus, 0x80001104u, 0x88776655u);

    /* A normal load is invisible to the immediately following instruction.
     * A direct write in that slot cancels the pending load.  Consecutive
     * little-endian LWL/LWR still merge through the delayed-load latch. */
    put32(&bus, 0x80001000u, enc_i(9, 0, 9, 0x55));
    put32(&bus, 0x80001004u, enc_i(0x23, 8, 9, 0));
    put32(&bus, 0x80001008u, enc_r(9, 0, 10, 0, 0x21));
    put32(&bus, 0x8000100cu, enc_r(9, 0, 11, 0, 0x21));
    put32(&bus, 0x80001010u, enc_i(0x23, 8, 9, 0));
    put32(&bus, 0x80001014u, enc_i(9, 0, 9, 7));
    put32(&bus, 0x80001018u, enc_r(9, 0, 12, 0, 0x21));
    put32(&bus, 0x8000101cu, enc_i(0x22, 13, 14, 3));
    put32(&bus, 0x80001020u, enc_i(0x26, 13, 14, 0));
    put32(&bus, 0x80001024u, 0);
    put32(&bus, 0x80001028u, enc_r(17, 0, 0, 0, 8));
    put32(&bus, 0x8000102cu, 0);

    PcPortMipsCpuInit(&cpu, &ops);
    cpu.gpr[8] = 0x80001100u;
    cpu.gpr[13] = 0x80001101u;
    cpu.gpr[14] = 0xA5A5A5A5u;
    cpu.gpr[17] = HALT_PC;
    rc = PcPortMipsRun(&cpu, 0x80001000u, HALT_PC, 1000u);
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "battle mips adapter: load test rc=%d error=%s\n",
                rc, cpu.error);
        return 1;
    }
    if (cpu.gpr[10] != 0x55u || cpu.gpr[11] != 0x44332211u ||
        cpu.gpr[12] != 7u || cpu.gpr[14] != 0x55443322u) {
        fprintf(stderr,
                "battle mips adapter: load delay t2=%08x t3=%08x "
                "t4=%08x t6=%08x\n",
                cpu.gpr[10], cpu.gpr[11], cpu.gpr[12], cpu.gpr[14]);
        return 1;
    }
    return 0;
}

int main(void)
{
    if (check_core_execution() != 0)
        return 1;
    if (check_unknown_instruction_fails_closed() != 0)
        return 1;
    if (check_load_delay_and_unaligned_pair() != 0)
        return 1;
    puts("BATTLE MIPS ADAPTER PASS");
    return 0;
}

#include "lib/world_guest_memory.h"

#define OBJECT 0x80150000u
static uint32_t channel_result;
static unsigned mock_calls;

static int mock_service(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    uint32_t *r = cpu->gpr;
    switch (target) {
    case 0x80024524u: r[2] = OBJECT; break;
    case 0x800245d8u:
    case 0x80022000u:
    case 0x800848b4u: break;
    case 0x8008c364u:
        put(r[4] + 0x50u, 4, 0x1234u + r[5]);
        r[2] = channel_result;
        break;
    case 0x80093978u: r[2] = (uint32_t)-1234567; break;
    case 0x8004a92cu: {
        uint8_t *matrix = region(r[5], 32);
        assert(matrix);
        memset(matrix, 0x5a, 32);
        r[2] = r[5];
        break;
    }
    case 0x80043a1cu: r[2] = 0x176; break;
    case 0x8003f968u: {
        void *dst = region(r[4], r[6]);
        const void *src = region(r[5], r[6]);
        assert(dst && src);
        memmove(dst, src, r[6]);
        r[2] = r[4];
        return 1;
    }
    default: return 0;
    }
    assert(++mock_calls <= 256);
    return 1;
}

static void seed(unsigned k)
{
    static const uint32_t modes[] = {0,1,2,3,4,5,6,7,8,UINT32_MAX,0x80000000u,0x7fffffffu};
    static const uint32_t results[] = {3, 1, UINT32_MAX};
    uint32_t slot = 0x80100000u + (k % 64) * 0x80;
    memset(scratch, 0xa7, sizeof scratch);
    for (unsigned i = 0; i < 0x60000; ++i) guest[0x100000 + i] = (uint8_t)(i * 37 + k * 13);
    channel_result = results[(k / 12) % 3];
    mock_calls = 0;
    put(0x8009be24, 4, 0x80100000);
    put(0x8009be10, 4, modes[k % 12]);
    put(0x8009c620, 4, 0x80110000);
    put(0x8009c610, 4, k % 9);
    put(0x8009cd34, 4, k & 1 ? 0x80160000 : 0);
    put(0x8009cd38, 4, 0x80161000);
    put(0x8009cd3c, 4, 0x80162000);
    put(0x8006f369, 1, k & 1 ? 0xff : 2);
    put(0x8006f36a, 1, k & 1 ? 3 : 0xff);
    put(0x8006ee68, 2, (k / 12) & 1 ? 0xe000 : 0xe001);
    put(0x8009bd3c, 2, 0x1234 + k);
    put(slot + 0x70, 4, k & 1 ? 0x80000fff : 0x7fffffff);
    for (unsigned i = 0; i < 9; ++i) {
        put(0x8009b674 + 2 * i, 2, i * 8);
        put(0x8009b688 + 2 * i, 2, i * 7);
        put(0x8009b64c + 4 * i, 2, 2);
        put(0x8009b64e + 4 * i, 2, 5);
    }
    put(0x8009afa0, 2, k & 1 ? 0xffff : 0xfffe);
    put(0x8009afa2, 2, 3);
    put(0x8009afa4, 2, 4);
    put(0x8009afa6, 2, 0x8000);
    put(0x8009afa8, 2, 0xffff);
    put(0x8009afdc, 2, k & 1 ? 0xffff : 0xfffe);
    put(0x8009afde, 2, 6);
    put(0x8009afe0, 2, 0xffff);
    for (unsigned i = 0; i < 2; ++i) {
        uint32_t record = 0x80110000u + (i ? 5u : 2u) * 0x54u;
        put(record + 0x40, 4, 0x80120000 + i * 0x100);
        put(0x80120004 + i * 0x100, 2, k % 4);
        put(record + 0x48, 4, 0x80130000 + i * 0x1000);
        put(record + 0x4c, 4, 0x80140000 + i * 0x1000);
    }
}

int main(int argc, char **argv)
{
    static uint8_t starting_ram[sizeof guest];
    static const uint32_t entries[] = {
        0x8008a52c, 0x8008b498, 0x8008bd1c, 0x8008c6ec, 0x8008d520, 0x8008de9c,
        0x8008e4f4, 0x800907c4, 0x80087f60, 0x8008868c, 0x800879e0, 0x80088c90
    };
    assert(argc == 2);
    load_overlay(argv[1]);
    memcpy(starting_ram, guest, sizeof guest);
    PcPortMipsBus bus = {.read=read_guest, .write=write_guest, .bridge=mock_service};
    for (unsigned test = 0; test < sizeof entries / sizeof entries[0]; ++test) {
        for (unsigned k = 0; k < 96; ++k) {
            memcpy(guest, starting_ram, sizeof guest);
            seed(k);
            PcPortMipsCpu cpu;
            PcPortMipsCpuInit(&cpu, &bus);
            cpu.gpr[4] = k % 64;
            cpu.gpr[29] = 0x801ff000u;
            cpu.gpr[31] = 0x80001000u;
            int status = PcPortMipsRun(&cpu, entries[test], cpu.gpr[31], 20000);
            if (status != PC_PORT_MIPS_HALTED || cpu.pc != 0x80001000u) {
                fprintf(stderr, "Guest entry %08x, case %u, status %d, PC %08x\n",
                        entries[test], k, status, cpu.pc);
                return 1;
            }
        }
    }
    puts("12 owner-image initializers returned across 96 synthetic inputs each (mocked child services)");
    return 0;
}

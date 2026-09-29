/* Pure retail eligibility leaf: exact read sequence and no permitted writes.
 * Group and status names do not assert unproven gameplay meanings. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef XBT_TEST_PACKED_RAM
#include "battle_target_eligibility_ram.h"
#endif
static uint8_t ram[0x200000];
static uint32_t reads[12];
static unsigned widths[12], nreads, cases;
#ifdef XBT_TEST_PACKED_RAM
static uint8_t saved_ram[sizeof(ram)];
void PcPortBattleTargetRamObserve(uint32_t address, unsigned width) {
    if (nreads == 12) { fprintf(stderr, "ELIGIBILITY FAIL excess packed reads\n"); exit(1); }
    reads[nreads] = address & 0x1FFFFFFF;
    widths[nreads++] = width;
}
#endif
extern uint32_t func_80083FF4(uint32_t, uint32_t) __attribute__((weak));
uint8_t D_800D2DCC[256], D_800C3EB7[256][28], D_800D32A1[256][8];
uint8_t D_800C3EB4[256][28], *D_800D3364;
uint16_t D_800CCD64[256][184], D_800CCE08[256][184];
static uint8_t matrix_data[0x5000];
static void fail(const char *s) {
    fprintf(stderr, "ELIGIBILITY FAIL %s case=%u\n", s, cases); exit(1);
}
static int read_bus(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p; a &= 0x1fffffff;
    if ((uint64_t)a+w > sizeof(ram)) return -1;
    if (!(a >= 0x83ff4 && a < 0x84108)) {
        if (nreads == 12) fail("excess data reads");
        reads[nreads] = a; widths[nreads++] = w;
    }
    *v = 0;
    for (unsigned i=0; i<w; ++i) *v |= (uint32_t)ram[a+i] << (8*i);
    return 0;
}
static int write_bus(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p; (void)a; (void)w; (void)v;
    fail("retail leaf attempted write"); return -1;
}
static void put(unsigned a, unsigned w, uint32_t value) {
    for (unsigned i=0; i<w; ++i) ram[a+i] = (uint8_t)(value >> (8*i));
}
static void check(uint32_t actor_arg, uint32_t target_arg, unsigned present,
                  unsigned blocked, unsigned alternate, unsigned matrix,
                  unsigned ag, unsigned tg, unsigned status) {
    unsigned a = actor_arg&255, t = target_arg&255;
    ram[0xd2dcc+t] = (uint8_t)present;
    ram[0xc3eb7+t*28] = (uint8_t)blocked;
    ram[0xd32a1+t*8] = (uint8_t)alternate;
    ram[0xc3eb4+a*28] = (uint8_t)ag;
    ram[0xc3eb4+t*28] = (uint8_t)tg;
    /* Same actor/target means one physical group byte. */
    if (a == t) ag = tg;
    unsigned slot = 0x100000+0x140+ag*64+tg*8;
    memset(ram+0x100000, 0xa5, 0x5000);
    ram[slot] = (uint8_t)matrix;
    put(0xd3364, 4, 0x80100000);
    unsigned inverse = (status & 0xc001) ? 0 : 1;
    put(0xccd64+t*0x170, 2, alternate ? inverse : status);
    put(0xcce08+t*0x170, 2, alternate ? status : inverse);
    uint32_t expected[12]; unsigned expected_width[12], count = 0;
#define READ(addr, width) do { expected[count] = (addr); expected_width[count++] = (width); } while (0)
    READ(0xd2dcc+t, 1);
    unsigned result = 0;
    if (present) {
        READ(0xc3eb7+t*28, 1);
        if (!blocked) {
            READ(0xd32a1+t*8, 1);
            if (alternate) {
                READ(0xcce08+t*0x170, 2);
                result = !(status & 0xc001);
            } else {
                READ(0xc3eb4+t*28, 1); READ(0xc3eb4+a*28, 1);
                READ(0xd3364, 4); READ(slot, 1);
                if (!matrix) {
                    READ(0xccd64+t*0x170, 2);
                    result = !(status & 0xc001);
                }
            }
        }
    }
#undef READ
    PcPortMipsBus bus = {.read=read_bus, .write=write_bus};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = actor_arg; cpu.gpr[5] = target_arg;
    cpu.gpr[29] = 0x801ff000; cpu.gpr[31] = 0xfffffffcu; nreads = 0;
    if (PcPortMipsRun(&cpu, 0x80083ff4, 0xfffffffcu, 200)) fail(cpu.error);
    if (cpu.gpr[2] != result || count != nreads ||
        memcmp(reads, expected, count*sizeof(*reads)) ||
        memcmp(widths, expected_width, count*sizeof(*widths)))
        fail("retail result/read sequence");
#ifdef XBT_TEST_PACKED_RAM
    uint32_t packed_result = 0xDEADBEEF;
    memcpy(saved_ram, ram, sizeof(ram));
    nreads = 0;
    if (PcPortBattleTargetEligibilityRam(ram, sizeof(ram), actor_arg,
                                        target_arg, &packed_result) ||
        packed_result != cpu.gpr[2]) fail("packed RAM differs from retail");
    if (nreads != count || memcmp(reads, expected, count*sizeof(*reads)) ||
        memcmp(widths, expected_width, count*sizeof(*widths)))
        fail("packed RAM read trace differs from retail");
    if (memcmp(saved_ram, ram, sizeof(ram))) fail("packed RAM changed input");
#endif
    if (func_80083FF4) {
        D_800D2DCC[t] = (uint8_t)present;
        D_800C3EB7[t][0] = (uint8_t)blocked;
        D_800D32A1[t][0] = (uint8_t)alternate;
        D_800C3EB4[a][0] = (uint8_t)ag; D_800C3EB4[t][0] = (uint8_t)tg;
        D_800CCD64[t][0] = (uint16_t)(alternate ? inverse : status);
        D_800CCE08[t][0] = (uint16_t)(alternate ? status : inverse);
        memcpy(matrix_data, ram+0x100000, sizeof(matrix_data));
        /* Early exits and the alternate path must not dereference this. */
        D_800D3364 = !present || blocked || alternate ? NULL : matrix_data;
        if (func_80083FF4(actor_arg, target_arg) != result)
            fail("native differs from retail");
        if (memcmp(matrix_data, ram+0x100000, sizeof(matrix_data)) ||
            D_800D2DCC[t] != present || D_800C3EB7[t][0] != blocked ||
            D_800D32A1[t][0] != alternate || D_800C3EB4[a][0] != ag ||
            D_800C3EB4[t][0] != tg ||
            D_800CCD64[t][0] != (alternate ? inverse : status) ||
            D_800CCE08[t][0] != (alternate ? status : inverse))
            fail("native changed input");
    }
    ++cases;
}
int main(void) {
    FILE *f = fopen("disc/battle.bin", "rb");
    if (!f || fseek(f, 0x14504, SEEK_SET) ||
        fread(ram+0x83ff4, 1, 0x114, f) != 0x114) fail("retail load");
    fclose(f);
    for (unsigned status=0; status<65536; ++status)
        for (unsigned alt=0; alt<2; ++alt)
            check(0x101, 0x103, 255, 0, alt ? 255 : 0, 0, 7, 2, status);
    const unsigned ids[] = {0, 3, 10, 255, 0x103};
    const unsigned bytes[] = {0, 1, 255}, groups[] = {0, 1, 7, 255};
    for (unsigned a=0; a<5; ++a) for (unsigned t=0; t<5; ++t)
    for (unsigned ag=0; ag<4; ++ag) for (unsigned tg=0; tg<4; ++tg)
    for (unsigned p=0; p<3; ++p) for (unsigned b=0; b<3; ++b)
    for (unsigned alt=0; alt<3; ++alt) for (unsigned m=0; m<3; ++m)
        check(ids[a], ids[t], bytes[p], bytes[b], bytes[alt], bytes[m],
              groups[ag], groups[tg], 0);
    printf("ELIGIBILITY retail contract PASS %u cases\n", cases);
#ifdef XBT_TEST_PACKED_RAM
    puts("ELIGIBILITY packed RAM differential PASS");
#else
    if (!func_80083FF4) fail("missing native owner 83FF4");
    puts("ELIGIBILITY native differential PASS");
#endif
    return 0;
}

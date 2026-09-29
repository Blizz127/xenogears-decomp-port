/* Retail target selection, with only list-building callee 841E0 controlled. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void func_80084A7C(uint32_t) __attribute__((weak));
struct BattleCommandContext;
struct BattleCommandContext *D_800C3EAC;
uint8_t D_800D3274, D_800C3E90[16];
enum { OLD = 0x100000, NEW = 0x110000, SIZE = 0x4100 };
static uint8_t ram[0x200000];
static _Alignas(4) uint8_t original[SIZE], replacement[SIZE];
static unsigned mode, actor, target, count, native_phase, calls;
static void fail(const char *s) {
    fprintf(stderr, "TARGET SELECTION FAIL %s actor=%u target=%u count=%u mode=%u\n",
            s, actor, target, count, mode);
    exit(1);
}
static int read_bus(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p; a &= 0x1fffffff;
    if ((uint64_t)a+w > sizeof(ram)) return -1;
    *v = 0;
    for (unsigned i=0; i<w; ++i) *v |= (uint32_t)ram[a+i] << (8*i);
    return 0;
}
static int write_bus(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p; a &= 0x1fffffff;
    if ((uint64_t)a+w > sizeof(ram)) return -1;
    for (unsigned i=0; i<w; ++i) ram[a+i] = (uint8_t)(v >> (8*i));
    return 0;
}
uint32_t func_800841E0(uint32_t a) {
    uint8_t *old = native_phase ? original : ram+OLD;
    uint8_t *next = native_phase ? replacement : ram+NEW;
    if (++calls != 1 || a != actor || old[0x2e8] != target)
        fail("callee must observe pre-call selection and masked actor");
    if (mode == 1) old[actor*64+0x3c] = (uint8_t)(target+1);
    if (mode == 2) {
        next[actor*64+0x3c] = (uint8_t)(target+1);
        if (native_phase) D_800C3EAC = (struct BattleCommandContext *)next;
        else write_bus(NULL, 0x800c3eac, 4, 0x80000000u+NEW);
    }
    if (native_phase) D_800D3274 = mode == 3 ? 0 : (uint8_t)count;
    else ram[0xd3274] = mode == 3 ? 0 : (uint8_t)count;
    return 0;
}
static int bridge(void *p, PcPortMipsCpu *cpu, uint32_t address) {
    (void)p;
    if (address != 0x800841e0) return 0;
    func_800841E0(cpu->gpr[4]);
    for (unsigned r=2; r<=15; ++r) cpu->gpr[r] = 0xa55a0000u+r;
    cpu->gpr[24] = 0xa55a0018; cpu->gpr[25] = 0xa55a0019;
    return 1;
}
int main(int argc, char **argv) {
    int oracle_only = argc == 2 && !strcmp(argv[1], "--oracle-only");
    FILE *f = fopen("disc/battle.bin", "rb");
    if (!f || fseek(f, 0x14f8c, SEEK_SET) ||
        fread(ram+0x84a7c, 1, 0xc4, f) != 0xc4) fail("retail load");
    fclose(f);
    const uint32_t actors[] = {0, 1, 255, 0x101};
    unsigned cases = 0;
    for (unsigned ai=0; ai<4; ++ai)
    for (target=0; target<256; ++target)
    for (count=0; count<=16; ++count)
    for (mode=0; mode<4; ++mode) {
        actor = actors[ai]&255;
        memset(original, 0xa5, SIZE); memset(replacement, 0x6b, SIZE);
        original[actor*64+0x3c] = (uint8_t)target;
        memcpy(ram+OLD, original, SIZE); memcpy(ram+NEW, replacement, SIZE);
        for (unsigned i=0; i<16; ++i)
            ram[0xc3e90+i] = D_800C3E90[i] = (uint8_t)(i*17);
        ram[0xd3274] = D_800D3274 = 0xff; /* callee supplies final count */
        write_bus(NULL, 0x800c3eac, 4, 0x80000000u+OLD);
        calls = native_phase = 0;
        PcPortMipsBus bus = {.read=read_bus, .write=write_bus, .bridge=bridge};
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = actors[ai]; cpu.gpr[29] = 0x801ff000;
        cpu.gpr[31] = 0xfffffffcu;
        if (PcPortMipsRun(&cpu, 0x80084a7c, 0xfffffffcu, 2000)) fail(cpu.error);
        unsigned selected = mode == 1 || mode == 2 ? (target+1)&255 : target;
        unsigned found = 0;
        for (unsigned i=0; i<(mode == 3 ? 0 : count); ++i)
            if (D_800C3E90[i] == selected) found = 1;
        unsigned expected = found ? (mode == 2 ? 0x6b : target) : D_800C3E90[0];
        if (calls != 1 || ram[(mode == 2 ? NEW : OLD)+0x2e8] != expected)
            fail("retail selection contract");
        if (!oracle_only && func_80084A7C) {
            D_800C3EAC = (struct BattleCommandContext *)original;
            D_800D3274 = 0xff; calls = 0; native_phase = 1;
            func_80084A7C(actors[ai]);
            if (calls != 1 || memcmp(original, ram+OLD, SIZE) ||
                memcmp(replacement, ram+NEW, SIZE) || D_800D3274 != ram[0xd3274] ||
                memcmp(D_800C3E90, ram+0xc3e90, 16) ||
                D_800C3EAC != (struct BattleCommandContext *)(mode == 2 ? replacement : original))
                fail("native differs from retail");
        }
        ++cases;
    }
    printf("TARGET SELECTION retail contract PASS %u cases\n", cases);
    if (!oracle_only && !func_80084A7C) fail("missing native owner 80084A7C");
    if (!oracle_only) puts("TARGET SELECTION native differential PASS");
    return 0;
}

/* Contract for retail 841E0. Only eligibility predicate 83FF4 is intercepted.
 * Group/rank names describe byte comparisons, not inferred game semantics. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t ram[0x200000];
static unsigned actor, mask, groups, rank_mode, calls;
extern uint32_t func_800841E0(uint32_t) __attribute__((weak));
extern void func_80084A7C(uint32_t) __attribute__((weak));
struct BattleCommandContext;
struct BattleCommandContext *D_800C3EAC;
enum { CONTEXT = 0x100000, CONTEXT_SIZE = 0x4100 };
static _Alignas(4) uint8_t native_context[CONTEXT_SIZE];
static uint8_t expected_context[CONTEXT_SIZE];
static unsigned in_chain, seed_target, chain_cases;
uint8_t D_800D3274, D_800C3E90[12], D_800C3EB4[256][28];
uint16_t D_800CCD34[11][184];
static void fail(const char *s) {
    fprintf(stderr, "TARGET LIST FAIL %s actor=%u mask=%u groups=%u rank=%u\n",
            s, actor, mask, groups, rank_mode); exit(1);
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
static int bridge(void *p, PcPortMipsCpu *cpu, uint32_t address) {
    (void)p;
    if (address != 0x80083ff4) return 0;
    unsigned start = actor < 3 ? 3 : 0;
    if (cpu->gpr[4] != actor || cpu->gpr[5] != start+calls)
        fail("eligibility call order/arguments");
    if (in_chain && ram[CONTEXT+0x2e8] != seed_target)
        fail("retail chain pre-call selection");
    unsigned result = (mask >> calls++) & 1;
    for (unsigned r=2; r<=15; ++r) cpu->gpr[r] = 0xa55a0000u+r;
    cpu->gpr[24] = 0xa55a0018; cpu->gpr[25] = 0xa55a0019;
    /* Retail masks the predicate result to a byte, not a C truth value. */
    cpu->gpr[2] = 0x80000100u | (result ? 0xff : 0);
    return 1;
}
uint32_t func_80083FF4(uint32_t a, uint32_t b) {
    unsigned start = actor < 3 ? 3 : 0;
    if (a != actor || b != start+calls) fail("native eligibility arguments");
    if (in_chain && native_context[0x2e8] != seed_target)
        fail("native chain pre-call selection");
    if (!calls) {
        if (D_800D3274 != 0) fail("native count not reset before calls");
        for (unsigned i=0; i<12; ++i)
            if (D_800C3E90[i] != 255) fail("native list not reset before calls");
    }
    return 0x80000100u | ((mask >> calls++) & 1 ? 0xff : 0);
}
static void check_chain(unsigned argument, const uint8_t *list, unsigned count) {
    if (!func_80084A7C || !func_800841E0) fail("missing native chain owner");
    in_chain = 1;
    /* Exercise all byte targets and explicitly preserve a listed tail target,
     * so returning list[0] unconditionally cannot pass integration. */
    for (unsigned mode=0; mode<2; ++mode) {
        seed_target = mode && count ? list[count-1] : mask;
        memset(expected_context, 0xa5, sizeof(expected_context));
        expected_context[actor*64+0x3c] = (uint8_t)seed_target;
        memcpy(native_context, expected_context, sizeof(native_context));
        memcpy(ram+CONTEXT, expected_context, sizeof(expected_context));
        unsigned selected = list[0];
        for (unsigned i=0; i<count; ++i)
            if (list[i] == seed_target) selected = seed_target;
        expected_context[0x2e8] = (uint8_t)selected;
        write_bus(NULL, 0x800c3eac, 4, 0x80000000u+CONTEXT);
        memset(ram+0xc3e90, 0x5a, 12); ram[0xd3274] = 0xa5;
        PcPortMipsBus bus = {.read=read_bus, .write=write_bus, .bridge=bridge};
        PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = argument; cpu.gpr[29] = 0x801ff000;
        cpu.gpr[31] = 0xfffffffcu; calls = 0;
        if (PcPortMipsRun(&cpu, 0x80084a7c, 0xfffffffcu, 6000)) fail(cpu.error);
        unsigned expected_calls = actor < 3 ? 8 : 3;
        if (calls != expected_calls || ram[0xd3274] != count ||
            memcmp(ram+0xc3e90, list, 12) ||
            memcmp(ram+CONTEXT, expected_context, CONTEXT_SIZE))
            fail("retail chain contract");
        D_800C3EAC = (struct BattleCommandContext *)native_context;
        memset(D_800C3E90, 0x5a, 12); D_800D3274 = 0xa5; calls = 0;
        func_80084A7C(argument);
        if (calls != expected_calls || D_800D3274 != count ||
            memcmp(D_800C3E90, ram+0xc3e90, 12) ||
            memcmp(native_context, ram+CONTEXT, CONTEXT_SIZE) ||
            D_800C3EAC != (struct BattleCommandContext *)native_context)
            fail("native chain differs from retail");
        ++chain_cases;
    }
    in_chain = 0;
}
int main(void) {
    FILE *f = fopen("disc/battle.bin", "rb");
    if (!f || fseek(f, 0x146f0, SEEK_SET) ||
        fread(ram+0x841e0, 1, 0x368, f) != 0x368) fail("retail load");
    if (fseek(f, 0x14f8c, SEEK_SET) ||
        fread(ram+0x84a7c, 1, 0xc4, f) != 0xc4) fail("retail caller load");
    fclose(f);
    const unsigned actors[] = {0, 2, 3, 255, 0x101};
    unsigned cases = 0;
    for (unsigned ai=0; ai<5; ++ai)
    for (mask=0; mask<256; ++mask)
    for (groups=0; groups<16; ++groups)
    for (rank_mode=0; rank_mode<4; ++rank_mode) {
        actor = actors[ai]&255;
        uint8_t group[256], expected[12]; uint16_t rank[11];
        memset(group, 7, sizeof(group));
        memset(expected, 0xff, sizeof(expected));
        for (unsigned i=0; i<11; ++i) {
            group[i] = (groups >> (i%4)) & 1 ? 7 : 0xff;
            rank[i] = rank_mode == 0 ? (uint16_t)(i*5000) :
                      rank_mode == 1 ? (uint16_t)(65535-i*5000) :
                      rank_mode == 2 ? 0x8000 : (i&1 ? 0xffff : 0);
            write_bus(NULL, 0x800ccd34+i*0x170, 2, rank[i]);
        }
        for (unsigned i=0; i<256; ++i) ram[0xc3eb4+i*28] = group[i];
        memset(ram+0xc3e8f, 0x5a, 14);
        ram[0xd3273] = 0x6b; ram[0xd3274] = 0xa5; ram[0xd3275] = 0x7c;
        unsigned first = actor < 3 ? 3 : 0, end = actor < 3 ? 11 : 3;
        unsigned count = 0, same = 0;
        /* Stable partition, then the retail one-pass slot-zero swaps. */
        for (unsigned pass=0; pass<2; ++pass)
            for (unsigned i=first; i<end; ++i)
                if ((mask & (1u << (i-first))) &&
                    ((group[i] == group[actor]) == (pass == 0))) {
                    expected[count++] = (uint8_t)i;
                    if (pass == 0) ++same;
                }
        unsigned limit = same ? same : count;
        for (unsigned i=1; i<limit; ++i)
            if (rank[expected[i]] < rank[expected[0]]) {
                uint8_t temp = expected[0]; expected[0] = expected[i]; expected[i] = temp;
            }
        PcPortMipsBus bus = {.read=read_bus, .write=write_bus, .bridge=bridge};
        PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = actors[ai]; cpu.gpr[29] = 0x801ff000;
        cpu.gpr[31] = 0xfffffffcu; calls = 0;
        if (PcPortMipsRun(&cpu, 0x800841e0, 0xfffffffcu, 5000)) fail(cpu.error);
        if (calls != end-first || ram[0xd3274] != count ||
            memcmp(ram+0xc3e90, expected, 12) || cpu.gpr[2] != expected[0])
            fail("retail ordering/count/return contract");
        if (ram[0xc3e8f] != 0x5a || ram[0xc3e9c] != 0x5a ||
            ram[0xd3273] != 0x6b || ram[0xd3275] != 0x7c)
            fail("list/count canary");
        if (func_800841E0) {
            memset(D_800C3EB4, 0x5a, sizeof(D_800C3EB4));
            memset(D_800CCD34, 0x5a, sizeof(D_800CCD34));
            for (unsigned i=0; i<256; ++i) D_800C3EB4[i][0] = group[i];
            for (unsigned i=0; i<11; ++i) D_800CCD34[i][0] = rank[i];
            memset(D_800C3E90, 0x5a, 12); D_800D3274 = 0xa5; calls = 0;
            uint32_t result = func_800841E0(actors[ai]);
            if (result != expected[0] || calls != end-first || D_800D3274 != count ||
                memcmp(D_800C3E90, ram+0xc3e90, 12)) fail("native differs from retail");
            check_chain(actors[ai], expected, count);
        }
        ++cases;
    }
    printf("TARGET LIST retail contract PASS %u cases\n", cases);
    if (!func_800841E0) fail("missing native owner 841E0");
    puts("TARGET LIST native differential PASS");
    printf("TARGET CHAIN native/retail PASS %u cases (real 84A7C -> 841E0)\n", chain_cases);
    return 0;
}

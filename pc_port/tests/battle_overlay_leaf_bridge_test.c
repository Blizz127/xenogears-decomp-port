#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "battle_overlay_guest_ram.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
void CTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PcPort_ServiceVblank(void) {}
void PcPort_PadVblankPump(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }

__attribute__((used)) void func_80079934(uint32_t *pValue) { *pValue += 4; }
__attribute__((used)) void func_800A3484(int16_t *p) { *p = -1; }
__attribute__((used)) void func_800AEEEC(uint8_t *p) { *(int16_t *)(p + 0x98) = -1; }
__attribute__((used)) uint16_t func_80089B50(uint16_t lo, uint16_t hi)
{
    if (lo == 0xFFFF) return 0xFFFF;
    if (hi == 0) return 0;
    if (lo == hi) return lo;
    return lo;
}
__attribute__((used)) uint32_t func_800B16A4(const uint8_t *data)
{
    uint32_t i = 0, total = 0;
    const uint8_t *record = data + *(const uint32_t *)(data + 0x10);
    uint32_t count = *(const uint32_t *)(data + 0x14);
    while (i != count) {
        total += ((uint32_t)record[0] + 1u) * 4u;
        record += ((uint32_t)record[1] + 1u) * 4u;
        i++;
    }
    return total;
}
__attribute__((used)) void func_800B6930(uint32_t *dst, uint8_t *p)
{
    uint32_t n = (uint32_t)((int8_t)p[1] << 8) | p[0];
    p += n;
    dst[0] = (uint32_t)(((int8_t)p[1] << 8) | p[0]) << 16;
    dst[1] = (uint32_t)(((int8_t)p[3] << 8) | p[2]) << 16;
    dst[2] = (uint32_t)(((int8_t)p[5] << 8) | p[4]) << 16;
}
__attribute__((used)) void func_800B6990(uint16_t *dst, uint8_t *p)
{
    uint32_t n = (uint32_t)((int8_t)p[1] << 8) | p[0];
    p += n;
    dst[0] = (uint16_t)(((int8_t)p[1] << 8) | p[0]);
    dst[1] = (uint16_t)(((int8_t)p[3] << 8) | p[2]);
    dst[2] = (uint16_t)(((int8_t)p[5] << 8) | p[4]);
}
__attribute__((used)) void func_800B69E4(uint16_t *a0, uint8_t *a1)
{
    uint8_t *r = a1 + (((int32_t)(int8_t)a1[1] << 8) | a1[0]);
    a0[0] = (uint16_t)(a0[0] + (((int32_t)(int8_t)r[1] << 8) | r[0]));
    a0[1] = (uint16_t)(a0[1] + (((int32_t)(int8_t)r[3] << 8) | r[2]));
    a0[2] = (uint16_t)(a0[2] + (((int32_t)(int8_t)r[5] << 8) | r[4]));
}
__attribute__((used)) uint8_t func_80079ED8(uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3)
{
    uint8_t *p = D_800CCCEC + a0 * 368;
    (void)a1;
    if (a3 == 0) {
        *p = a2;
        return 0;
    }
    return *p;
}
__attribute__((used)) uint16_t func_8007A280(uint8_t a0, uint8_t a1, uint16_t a2, uint8_t a3)
{
    uint8_t *p = D_800CCD36 + a0 * 368;
    (void)a1;
    if (a3 == 0) {
        p[0] = (uint8_t)a2;
        p[1] = (uint8_t)(a2 >> 8);
        return 0;
    }
    return (uint16_t)(p[0] | (p[1] << 8));
}
__attribute__((used)) void func_80079E18(uint8_t index)
{
    D_800D2D28[0xB4] = 1;
    *(uint8_t *)(D_800D3725 + (((uint32_t)index * 3u) << 5)) = 1;
}
__attribute__((used)) void func_80079E4C(uint8_t index)
{
    D_800D2D28[0xB4] = 0;
    *(uint8_t *)(D_800D3725 + (((uint32_t)index * 3u) << 5)) = 0;
}
__attribute__((used)) uint32_t func_800AA898(uint8_t *p, uint32_t a1, uint32_t a2, uint32_t a3)
{
    (void)a1;
    *(uint16_t *)(p + 0x3C) = 0xFFFF;
    p[0x5C] = 0xFF;
    *(uint32_t *)(p + 0x8) = a2;
    *(uint32_t *)(p + 0x14) = a3;
    *(uint16_t *)(p + 0x8E) = 1;
    return 1;
}

#define WORD 0x80020100u
#define HALF 0x80020200u
#define REC  0x80020300u
#define SRC  0x80020400u
#define DST  0x80020500u
#define STACK 0x801ff000u

static BattleMipsRuntime rt;
static PcPortMipsCpu cpu;
static unsigned checks;

#define CHECK(label, condition) do { checks++; if (!(condition)) { \
    fprintf(stderr, "BATTLE OVERLAY LEAF FAIL %s line=%d\n", label, __LINE__); \
    return 1; \
} } while (0)

static uint32_t word(uint32_t a)
{
    return load_le(PSX_ADDR(a), 4);
}

static uint16_t half(uint32_t a)
{
    return (uint16_t)load_le(PSX_ADDR(a), 2);
}

static void emit(uint32_t a, uint32_t w)
{
    store_le(PSX_ADDR(a), 4, w);
}

static void setup(void)
{
    memset(&rt, 0, sizeof(rt));
    memset(&cpu, 0, sizeof(cpu));
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    rt.initialized = 1;
    initialize_cpu(&cpu, &rt);
    cpu.gpr[29] = STACK;
    cpu.gpr[31] = BATTLE_HALT_PC;
    rt.bridge_cpu = &cpu;
    g_ActiveBattleRuntime = &rt;
    static const struct {
        uint32_t address;
        void *host;
        const char *name;
    } leaves[] = {
        { 0x80079934u, (void *)func_80079934, "func_80079934" },
        { 0x80079e18u, (void *)func_80079E18, "func_80079E18" },
        { 0x80079e4cu, (void *)func_80079E4C, "func_80079E4C" },
        { 0x80079ed8u, (void *)func_80079ED8, "func_80079ED8" },
        { 0x8007a280u, (void *)func_8007A280, "func_8007A280" },
        { 0x80089b50u, (void *)func_80089B50, "func_80089B50" },
        { 0x800a3484u, (void *)func_800A3484, "func_800A3484" },
        { 0x800aa898u, (void *)func_800AA898, "func_800AA898" },
        { 0x800aeeeCu, (void *)func_800AEEEC, "func_800AEEEC" },
        { 0x800b16a4u, (void *)func_800B16A4, "func_800B16A4" },
        { 0x800b6930u, (void *)func_800B6930, "func_800B6930" },
        { 0x800b6990u, (void *)func_800B6990, "func_800B6990" },
        { 0x800b69e4u, (void *)func_800B69E4, "func_800B69E4" },
    };
    unsigned n;
    rt.function_count = sizeof(leaves) / sizeof(leaves[0]);
    for (n = 0; n < rt.function_count; n++) {
        rt.functions[n].address = leaves[n].address;
        rt.functions[n].host = leaves[n].host;
        rt.functions[n].name = leaves[n].name;
        emit(leaves[n].address, 0x03e00008u);
        emit(leaves[n].address + 4u, 0);
    }
}

int main(void)
{
    int rc;
    volatile void *keep = (void *)func_80079934;
    keep = (void *)func_800A3484;
    keep = (void *)func_800AEEEC;
    keep = (void *)func_80089B50;
    keep = (void *)func_800B16A4;
    keep = (void *)func_800B6930;
    keep = (void *)func_800B6990;
    keep = (void *)func_800B69E4;
    keep = (void *)func_800AA898;
    keep = (void *)func_80079ED8;
    keep = (void *)func_8007A280;
    keep = (void *)func_80079E18;
    keep = (void *)func_80079E4C;
    (void)keep;

    setup();
    store_le(PSX_ADDR(WORD), 4, 10);
    cpu.gpr[4] = WORD;
    rc = runtime_bridge(&rt, &cpu, 0x80079934u);
    CHECK("79934 bridged", rc == 1);
    CHECK("79934 *p += 4", word(WORD) == 14);

    setup();
    store_le(PSX_ADDR(HALF), 2, 0x1234);
    cpu.gpr[4] = HALF;
    rc = runtime_bridge(&rt, &cpu, 0x800a3484u);
    CHECK("A3484 bridged", rc == 1);
    CHECK("A3484 *p = -1", half(HALF) == 0xffff);

    setup();
    store_le(PSX_ADDR(REC + 0x98), 2, 0x7777);
    cpu.gpr[4] = REC;
    rc = runtime_bridge(&rt, &cpu, 0x800aeeeCu);
    CHECK("AEEEC bridged", rc == 1);
    CHECK("AEEEC *(p+0x98) = -1", half(REC + 0x98) == 0xffff);

    setup();
    cpu.gpr[4] = 7;
    cpu.gpr[5] = 7;
    rc = runtime_bridge(&rt, &cpu, 0x80089b50u);
    CHECK("89B50 bridged", rc == 1);
    CHECK("89B50 lo==hi", cpu.gpr[2] == 7);
    cpu.gpr[4] = 0xffff;
    cpu.gpr[5] = 3;
    rc = runtime_bridge(&rt, &cpu, 0x80089b50u);
    CHECK("89B50 lo==FFFF", cpu.gpr[2] == 0xffff);

    setup();
    store_le(PSX_ADDR(SRC + 0x10), 4, 0x18);
    store_le(PSX_ADDR(SRC + 0x14), 4, 1);
    store_le(PSX_ADDR(SRC + 0x18), 4, 0); /* record[0]=0, [1]=0 */
    cpu.gpr[4] = SRC;
    rc = runtime_bridge(&rt, &cpu, 0x800b16a4u);
    CHECK("B16A4 bridged", rc == 1);
    CHECK("B16A4 size", cpu.gpr[2] == 4);

    setup();
    store_le(PSX_ADDR(SRC), 2, 2); /* offset 2 */
    store_le(PSX_ADDR(SRC + 2), 2, 1);
    store_le(PSX_ADDR(SRC + 4), 2, 2);
    store_le(PSX_ADDR(SRC + 6), 2, 3);
    cpu.gpr[4] = DST;
    cpu.gpr[5] = SRC;
    rc = runtime_bridge(&rt, &cpu, 0x800b6930u);
    CHECK("B6930 bridged", rc == 1);
    CHECK("B6930 dst0", word(DST) == 0x00010000u);
    CHECK("B6930 dst1", word(DST + 4) == 0x00020000u);
    CHECK("B6930 dst2", word(DST + 8) == 0x00030000u);

    setup();
    store_le(PSX_ADDR(SRC), 2, 2);
    store_le(PSX_ADDR(SRC + 2), 2, 0x0011);
    store_le(PSX_ADDR(SRC + 4), 2, 0x0022);
    store_le(PSX_ADDR(SRC + 6), 2, 0x0033);
    cpu.gpr[4] = DST;
    cpu.gpr[5] = SRC;
    rc = runtime_bridge(&rt, &cpu, 0x800b6990u);
    CHECK("B6990 bridged", rc == 1);
    CHECK("B6990 dst0", half(DST) == 0x0011);
    CHECK("B6990 dst1", half(DST + 2) == 0x0022);
    CHECK("B6990 dst2", half(DST + 4) == 0x0033);

    setup();
    store_le(PSX_ADDR(DST), 2, 10);
    store_le(PSX_ADDR(DST + 2), 2, 20);
    store_le(PSX_ADDR(DST + 4), 2, 30);
    store_le(PSX_ADDR(SRC), 2, 2);
    store_le(PSX_ADDR(SRC + 2), 2, 1);
    store_le(PSX_ADDR(SRC + 4), 2, 2);
    store_le(PSX_ADDR(SRC + 6), 2, 3);
    cpu.gpr[4] = DST;
    cpu.gpr[5] = SRC;
    rc = runtime_bridge(&rt, &cpu, 0x800b69e4u);
    CHECK("B69E4 bridged", rc == 1);
    CHECK("B69E4 a0[0]", half(DST) == 11);
    CHECK("B69E4 a0[1]", half(DST + 2) == 22);
    CHECK("B69E4 a0[2]", half(DST + 4) == 33);

    setup();
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0;
    cpu.gpr[6] = 0xAB;
    cpu.gpr[7] = 0;
    rc = runtime_bridge(&rt, &cpu, 0x80079ed8u);
    CHECK("79ED8 store bridged", rc == 1);
    CHECK("79ED8 wrote D_800CCCEC", load_le(PSX_ADDR(0x800CCCEC), 1) == 0xAB);
    cpu.gpr[6] = 0;
    cpu.gpr[7] = 1;
    rc = runtime_bridge(&rt, &cpu, 0x80079ed8u);
    CHECK("79ED8 load bridged", rc == 1);
    CHECK("79ED8 read D_800CCCEC", (cpu.gpr[2] & 0xffu) == 0xAB);

    setup();
    cpu.gpr[4] = 0;
    cpu.gpr[5] = 0;
    cpu.gpr[6] = 0x3344;
    cpu.gpr[7] = 0;
    rc = runtime_bridge(&rt, &cpu, 0x8007a280u);
    CHECK("7A280 store bridged", rc == 1);
    CHECK("7A280 wrote D_800CCD36", half(0x800CCD36u) == 0x3344);

    setup();
    store_le(PSX_ADDR(0x800D2D28), 4, 0x801F0800u);
    cpu.gpr[4] = 1;
    rc = runtime_bridge(&rt, &cpu, 0x80079e18u);
    CHECK("79E18 bridged", rc == 1);
    CHECK("79E18 ui+0xB4", load_le(PSX_ADDR(0x801F0800u + 0xB4), 1) == 1);
    CHECK("79E18 table", load_le(PSX_ADDR(0x800D3725u + (3u << 5)), 1) == 1);
    rc = runtime_bridge(&rt, &cpu, 0x80079e4cu);
    CHECK("79E4C bridged", rc == 1);
    CHECK("79E4C ui+0xB4", load_le(PSX_ADDR(0x801F0800u + 0xB4), 1) == 0);

    setup();
    cpu.gpr[4] = DST;
    cpu.gpr[5] = 0;
    cpu.gpr[6] = 0x11111111u;
    cpu.gpr[7] = 0x22222222u;
    rc = runtime_bridge(&rt, &cpu, 0x800aa898u);
    CHECK("AA898 bridged", rc == 1);
    CHECK("AA898 v0", cpu.gpr[2] == 1);
    CHECK("AA898 +8", word(DST + 8) == 0x11111111u);
    CHECK("AA898 +14", word(DST + 0x14) == 0x22222222u);

    /* A non-allowlisted overlay target still interprets (jr ra / nop). */
    setup();
    store_le(PSX_ADDR(WORD), 4, 10);
    cpu.gpr[4] = WORD;
    cpu.pc = 0x80071000u;
    rc = runtime_bridge(&rt, &cpu, 0x80071000u);
    CHECK("unadopted overlay interprets", rc == 0);
    CHECK("unadopted does not touch WORD", word(WORD) == 10);

    printf("BATTLE OVERLAY LEAF PASS checks=%u\n", checks);
    return 0;
}

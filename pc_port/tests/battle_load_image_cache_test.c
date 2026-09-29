/* Regression test for two battle-bridge fixes in pc_port/src/battle_mips_runtime.c.
 *
 * 1. LoadImage pointer translation.  The interpreted battle overlay calls
 *    LoadImage(RECT*, u_long*) at 0x800b798c with a low physical-RAM source
 *    (0x1000) and a KSEG0 RECT.  translate_argument deliberately leaves low
 *    values alone because it is shared with scalar arguments, so the raw
 *    0x1000 reached PsyCross and GR_CopyVRAM memmoved from host address 0x1000
 *    (SIGSEGV).  bridge_load_image resolves the RECT as a guest pointer (OR-ing
 *    the KSEG0 bit for low addresses, as the CompMatrix bridge does) and maps a
 *    low source to g_PsxRam + address.
 *
 * 2. The resolved-call cache.  Every call to a guest routine absent from the
 *    bridge table used to re-run a func_%08X dlsym plus an O(n) host-pointer
 *    scan, which put a stalled battle inside do_lookup_x.  The runtime now
 *    caches the verdict per target.  This test proves a hit by resolving once,
 *    then swapping the table's host for a different function and asserting the
 *    cached entry is still the one invoked (a cache that never stores its key
 *    would pick up the new function and fail here).
 *
 * Modelled on battle_guest_call_test.c: this TU #includes the production
 * source and supplies the external stubs.  Failure prints
 * "LOADIMAGE CACHE FAIL <label> line=N"; success prints one PASS line.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* ---- external stubs for the production TU ---- */
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
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target)
{
    (void)cpu;
    (void)target;
}

/* Strong xeno_port_is_generated_stub override lives in the runner-emitted TU. */
const char *g_test_generated_stub_names[8];
unsigned g_test_generated_stub_count;

#define LOADIMAGE_ADDR 0x80044894u

static BattleMipsRuntime rt;
static unsigned checks;

#define CHECK(label, condition) do { checks++; if (!(condition)) { \
    fprintf(stderr, "LOADIMAGE CACHE FAIL %s line=%d\n", label, __LINE__); \
    return 0; } } while (0)

static unsigned a_calls, b_calls;
static unsigned hot_calls;

/* The generic bridge calls a host owner through the 12-argument
 * GenericHostFunction pointer, so the "hot" entries need that exact shape for
 * UBSan's indirect-call type check. */
static uintptr_t fake_hot_host(uintptr_t a0, uintptr_t a1, uintptr_t a2,
                               uintptr_t a3, uintptr_t a4, uintptr_t a5,
                               uintptr_t a6, uintptr_t a7, uintptr_t a8,
                               uintptr_t a9, uintptr_t a10, uintptr_t a11)
{
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    (void)a6; (void)a7; (void)a8; (void)a9; (void)a10; (void)a11;
    hot_calls++;
    return 0;
}
static void *a_rect, *a_data;

static int fake_load_image_a(void *rect, void *data)
{
    a_calls++;
    a_rect = rect;
    a_data = data;
    return 0;
}

static int fake_load_image_b(void *rect, void *data)
{
    (void)rect;
    (void)data;
    b_calls++;
    return 0;
}

static void setup(void)
{
    memset(&rt, 0, sizeof(rt));
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    rt.initialized = 1;
    g_ActiveBattleRuntime = &rt;
    g_test_generated_stub_count = 0;
    a_calls = b_calls = 0;
    a_rect = a_data = NULL;
    rt.function_count = 1;
    rt.functions[0].address = LOADIMAGE_ADDR;
    rt.functions[0].host = (void *)fake_load_image_a;
    rt.functions[0].name = "LoadImage";
}

static int call_load_image(uint32_t rect, uint32_t data)
{
    PcPortMipsCpu cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.gpr[4] = rect;
    cpu.gpr[5] = data;
    return runtime_bridge_call(&rt, &cpu, LOADIMAGE_ADDR);
}

/* 1. KSEG0 RECT + low physical source both land inside g_PsxRam. */
static int kseg0_and_low_source(void)
{
    setup();
    CHECK("kseg0-return", call_load_image(0x800C4A90u, 0x80001000u) == 1);
    CHECK("kseg0-calls", a_calls == 1);
    CHECK("kseg0-rect", a_rect == (void *)(g_PsxRam + 0xC4A90u));
    CHECK("kseg0-data", a_data == (void *)(g_PsxRam + 0x1000u));
    return 1;
}

/* 2. Bare physical/low values mean the same thing (the live failure used a low
 *    source; the RECT may arrive either way). */
static int low_addresses(void)
{
    setup();
    CHECK("low-return", call_load_image(0x000C4A90u, 0x00001000u) == 1);
    CHECK("low-rect", a_rect == (void *)(g_PsxRam + 0xC4A90u));
    CHECK("low-data", a_data == (void *)(g_PsxRam + 0x1000u));
    return 1;
}

/* 3. KSEG1 (uncached mirror) source behaves like KSEG0. */
static int kseg1_source(void)
{
    setup();
    CHECK("kseg1-return", call_load_image(0x800C4A90u, 0xA0001000u) == 1);
    CHECK("kseg1-data", a_data == (void *)(g_PsxRam + 0x1000u));
    return 1;
}

/* 4. Cache hit: the second call must reuse the first resolution.  Swapping the
 *    table's host between calls is observable only if the cache works. */
static int cache_hit_reuses_entry(void)
{
    setup();
    CHECK("cache-first", call_load_image(0x800C4A90u, 0x80001000u) == 1);
    CHECK("cache-a-once", a_calls == 1 && b_calls == 0);
    rt.functions[0].host = (void *)fake_load_image_b;
    CHECK("cache-second", call_load_image(0x800C4A90u, 0x80001000u) == 1);
    CHECK("cache-still-a", a_calls == 2 && b_calls == 0);
    return 1;
}

/* 5. A different target is not served by another target's slot. */
static int cache_is_keyed(void)
{
    unsigned i;
    int found = 0;
    setup();
    CHECK("key-first", call_load_image(0x800C4A90u, 0x80001000u) == 1);
    for (i = 0; i < BRIDGE_CALL_CACHE_SIZE; i++) {
        if (rt.call_cache[i].state != 0 &&
            rt.call_cache[i].target == LOADIMAGE_ADDR)
            found = 1;
    }
    CHECK("key-stored", found == 1);
    /* Unknown *main-executable* target (below BATTLE_BASE, outside the
     * 0x801d0000..0x80300000 interpreter range) still reports unresolved. */
    CHECK("key-unknown", runtime_bridge_call(&rt, &(PcPortMipsCpu){0}, 0x80044000u) == -1);
    return 1;
}

/* 6. Cache eviction must not serve another target's entry.  Five guest-code
 * targets are found that share a cache base; the first four fill the four ways
 * with resolvable hosts, then the fifth (unresolvable) must re-resolve and be
 * interpreted rather than dispatched to a stale neighbour. */
static unsigned collision_base(uint32_t target)
{
    return (unsigned)((target * 2654435761u) >> 23) & (BRIDGE_CALL_CACHE_SIZE - 1);
}

static int cache_eviction_re_resolves(void)
{
    /* Four non-guest (main-executable) addresses resolve to a host owner without
     * the overlay leaf gate, and are placed first so they occupy the four ways
     * of one base.  The fifth is guest overlay code, which must be interpreted
     * when unresolved -- with the eviction bug it is handed one of the four. */
    uint32_t hot[4];
    uint32_t victim_target = 0;
    unsigned found = 0, i, j;
    uint32_t candidate;
    PcPortMipsCpu cpu;

    setup();
    for (i = 0; i < 4; i++)
        hot[i] = 0;
    /* Collect non-guest candidates first. */
    for (candidate = 0x80010000u; candidate < 0x8006F000u && found < 8; candidate += 4u) {
        unsigned b = collision_base(candidate);
        (void)b;
        break;
    }
    {
        /* Bucket non-guest candidates by base, then look for a base that also
         * has a guest-code candidate. */
        static uint32_t bucket[2][BRIDGE_CALL_CACHE_SIZE][4];
        static unsigned bucket_n[2][BRIDGE_CALL_CACHE_SIZE];
        unsigned r;
        memset(bucket, 0, sizeof(bucket));
        for (r = 0; r < 2; r++) {
            uint32_t lo = (r == 0) ? 0x80010000u : 0x801D0000u;
            uint32_t hi = (r == 0) ? 0x8006F000u : 0x80300000u;
            for (candidate = lo; candidate < hi; candidate += 4u) {
                unsigned b = collision_base(candidate);
                if (bucket_n[r][b] < 4)
                    bucket[r][b][bucket_n[r][b]++] = candidate;
            }
        }
        for (candidate = 0x80070000u; candidate < 0x800C0000u; candidate += 4u) {
            unsigned b = collision_base(candidate);
            if (bucket_n[0][b] >= 4 || bucket_n[1][b] >= 4) {
                unsigned r = (bucket_n[0][b] >= 4) ? 0u : 1u;
                for (i = 0; i < 4; i++)
                    hot[i] = bucket[r][b][i];
                victim_target = candidate;
                found = 1;
                break;
            }
        }
    }
    CHECK("evict-group", found == 1 && victim_target != 0);
    CHECK("evict-distinct", hot[0] != hot[1] && hot[1] != hot[2] && hot[2] != hot[3]);

    for (i = 0; i < 4; i++)
        for (j = i + 1; j < 4; j++)
            if (hot[j] < hot[i]) {
                uint32_t t = hot[i];
                hot[i] = hot[j];
                hot[j] = t;
            }
    rt.function_count = 4;
    for (i = 0; i < 4; i++) {
        rt.functions[i].address = hot[i];
        rt.functions[i].host = (void *)fake_hot_host;
        rt.functions[i].name = "hot";
    }
    hot_calls = 0;
    for (i = 0; i < 4; i++) {
        memset(&cpu, 0, sizeof(cpu));
        CHECK("evict-fill", runtime_bridge_call(&rt, &cpu, hot[i]) == 1);
    }
    CHECK("evict-host-called", hot_calls == 4);
    hot_calls = 0;
    memset(&cpu, 0, sizeof(cpu));
    CHECK("evict-fallback", runtime_bridge_call(&rt, &cpu, victim_target) == 0);
    CHECK("evict-no-wrong-host", hot_calls == 0);
    return 1;
}

int main(void)
{
    if (!kseg0_and_low_source()) return 1;
    if (!low_addresses()) return 1;
    if (!kseg1_source()) return 1;
    if (!cache_hit_reuses_entry()) return 1;
    if (!cache_is_keyed()) return 1;
    if (!cache_eviction_re_resolves()) return 1;
    printf("LOADIMAGE CACHE PASS checks=%u translate/cache\n", checks);
    return 0;
}

/* Shared-address-space retail differential harness for menu/shop overlays.
 *
 * The PSX main RAM image is mapped at its real guest address (0x80000000,
 * 2 MiB) inside the -no-pie test process, so a guest pointer value is also a
 * valid host pointer.  Retail MIPS runs in the interpreter
 * (pc_port/src/battle_mips_adapter.c) against that RAM; the native port body
 * runs against the same RAM restored from the same snapshot.  Globals the body
 * touches are bound to their retail addresses with `-Wl,--defsym` (compile the
 * body with -fPIC so references go through the GOT), and structs whose native
 * layout differs (SystemMenu, which holds 8-byte host pointers) are mirrored
 * with pc_port/tests/lib/gen_struct_mirror.py tables.
 *
 * Callees: every jal target is a "callee" listed in hx_callees[] by the test.
 * Retail calls are serviced by the bridge, native calls by same-named stubs
 * that call hx_native(); both record an ordered event (target + arguments,
 * including stack arguments) and run the test's hx_effect() hook, which acts
 * on the shared RAM, so both worlds see identical callee behaviour.
 *
 * Not for ASan (its shadow region overlaps 0x80000000); UBSan is fine. */
#ifndef XENO_MENU_RETAIL_HARNESS_H
#define XENO_MENU_RETAIL_HARNESS_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "battle_mips_adapter.h"

#define HX_BASE 0x80000000u
#define HX_SIZE 0x200000u
#define HX_HALT 0xFFFFFFFCu
#define HX_STACK_TOP 0x801FFF00u
#define HX_STACK_LOW 0x801F0000u
#define HX_MAX_EVENTS 8192
#define HX_MAX_ARGS 10

typedef struct HxCallee {
    uint32_t address;
    unsigned nargs;
    const char* name;
} HxCallee;

typedef struct HxEvent {
    uint32_t fn;
    unsigned n;
    uint32_t a[HX_MAX_ARGS];
} HxEvent;

/* Provided by the test. */
extern const HxCallee hx_callees[];
extern const unsigned hx_ncallees;
/* Side effects + return value of a callee; identical for both worlds.
 * side 0 = retail, 1 = native; index = per-world call ordinal. */
uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side);

static HxEvent hx_events[2][HX_MAX_EVENTS];
static unsigned hx_nevents[2];
static int hx_side;
static unsigned hx_checks, hx_case;
static uint8_t* hx_snapshot;
static uint8_t* hx_retail_after;
static uint32_t hx_cov_lo, hx_cov_hi;
static uint32_t* hx_cov;
static PcPortMipsCpu* hx_cpu;

#define HX_REQUIRE(ok, ...) do { ++hx_checks; if (!(ok)) { \
    fprintf(stderr, "HX FAIL case=%u %s:%d: ", hx_case, __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); exit(1); } } while (0)

static inline uint8_t* hx_p(uint32_t a)
{
    if (a < HX_BASE || (uint64_t)a >= (uint64_t)HX_BASE + HX_SIZE) {
        fprintf(stderr, "HX FAIL case=%u guest address %08x out of RAM\n", hx_case, a);
        exit(1);
    }
    return (uint8_t*)(uintptr_t)a;
}
static inline uint32_t hx_r32(uint32_t a) { uint32_t v; memcpy(&v, hx_p(a), 4); return v; }
static inline uint16_t hx_r16(uint32_t a) { uint16_t v; memcpy(&v, hx_p(a), 2); return v; }
static inline uint8_t hx_r8(uint32_t a) { return *hx_p(a); }
static inline void hx_w32(uint32_t a, uint32_t v) { memcpy(hx_p(a), &v, 4); }
static inline void hx_w16(uint32_t a, uint16_t v) { memcpy(hx_p(a), &v, 2); }
static inline void hx_w8(uint32_t a, uint8_t v) { *hx_p(a) = v; }
static inline uint32_t hx_addr(const void* p)
{
    uintptr_t v = (uintptr_t)p;
    if (v == 0) return 0;
    if (v < HX_BASE || v >= (uintptr_t)HX_BASE + HX_SIZE) {
        fprintf(stderr, "HX FAIL case=%u host pointer %p outside guest RAM\n", hx_case, p);
        exit(1);
    }
    return (uint32_t)v;
}

static void hx_map(void)
{
    void* p = mmap((void*)(uintptr_t)HX_BASE, HX_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void*)(uintptr_t)HX_BASE) {
        fprintf(stderr, "HX FAIL cannot map guest RAM at %08x\n", HX_BASE);
        exit(1);
    }
    hx_snapshot = (uint8_t*)malloc(HX_SIZE);
    hx_retail_after = (uint8_t*)malloc(HX_SIZE);
    if (!hx_snapshot || !hx_retail_after) exit(1);
}

/* Load a file image at a guest address; returns its size. */
static size_t hx_load(const char* path, uint32_t address, size_t skip)
{
    FILE* f = fopen(path, "rb");
    size_t n;
    if (!f) { fprintf(stderr, "HX FAIL open %s\n", path); exit(1); }
    fseek(f, (long)skip, SEEK_SET);
    n = fread(hx_p(address), 1, HX_BASE + HX_SIZE - address, f);
    fclose(f);
    return n;
}

static void hx_coverage(uint32_t lo, uint32_t hi)
{
    hx_cov_lo = lo;
    hx_cov_hi = hi;
    hx_cov = (uint32_t*)calloc((hi - lo) / 4, sizeof(uint32_t));
}

static unsigned hx_coverage_report(const char* what, const uint32_t* unreachable, unsigned nun)
{
    unsigned i, hit = 0, total = (hx_cov_hi - hx_cov_lo) / 4;
    for (i = 0; i < total; ++i) {
        uint32_t pc = hx_cov_lo + i * 4;
        unsigned k, skip = 0;
        for (k = 0; k < nun; ++k) skip |= unreachable[k] == pc;
        if (hx_cov[i]) ++hit;
        else if (!skip) fprintf(stderr, "  %s: uncovered %08x\n", what, pc);
    }
    printf("%s coverage %u/%u instructions\n", what, hit, total);
    return hit;
}

static const HxCallee* hx_find(uint32_t address)
{
    unsigned i;
    for (i = 0; i < hx_ncallees; ++i)
        if (hx_callees[i].address == address) return &hx_callees[i];
    return NULL;
}

static uint32_t hx_record(uint32_t fn, const uint32_t* a, unsigned n)
{
    HxEvent* e;
    unsigned index = hx_nevents[hx_side];
    HX_REQUIRE(index < HX_MAX_EVENTS, "event overflow");
    e = &hx_events[hx_side][index];
    memset(e, 0, sizeof(*e));
    e->fn = fn;
    e->n = n;
    memcpy(e->a, a, n * sizeof(uint32_t));
    hx_nevents[hx_side]++;
    return hx_effect(fn, e->a, index, hx_side);
}

/* Native stub body: hx_native(0xADDR, n, arg0, arg1, ...), args as uint32_t. */
static uint32_t hx_native(uint32_t fn, unsigned n, ...)
{
    uint32_t a[HX_MAX_ARGS];
    unsigned i;
    va_list ap;
    va_start(ap, n);
    for (i = 0; i < n; ++i) a[i] = va_arg(ap, uint32_t);
    va_end(ap);
    return hx_record(fn, a, n);
}

static int hx_read(void* o, uint32_t address, unsigned width, uint32_t* value)
{
    unsigned i;
    (void)o;
    if (address < HX_BASE || (uint64_t)address + width > (uint64_t)HX_BASE + HX_SIZE) return -1;
    if (hx_cov && hx_cpu && width == 4 && address == hx_cpu->pc &&
        address >= hx_cov_lo && address < hx_cov_hi)
        hx_cov[(address - hx_cov_lo) / 4]++;
    *value = 0;
    for (i = 0; i < width; ++i) *value |= (uint32_t)hx_p(address)[i] << (8 * i);
    return 0;
}

static int hx_write(void* o, uint32_t address, unsigned width, uint32_t value)
{
    unsigned i;
    (void)o;
    if (address < HX_BASE || (uint64_t)address + width > (uint64_t)HX_BASE + HX_SIZE) return -1;
    for (i = 0; i < width; ++i) hx_p(address)[i] = (uint8_t)(value >> (8 * i));
    return 0;
}

static int hx_bridge(void* o, PcPortMipsCpu* c, uint32_t target)
{
    const HxCallee* callee = hx_find(target);
    uint32_t a[HX_MAX_ARGS];
    unsigned i;
    (void)o;
    if (!callee) return 0; /* guest code (the function under test) */
    for (i = 0; i < callee->nargs; ++i)
        a[i] = i < 4 ? c->gpr[4 + i] : hx_r32(c->gpr[29] + 4 * i);
    c->gpr[2] = hx_record(target, a, callee->nargs);
    return 1;
}

/* Run retail from `entry` (which must not itself be a listed callee). */
static uint32_t hx_run_retail(uint32_t entry, uint32_t a0, uint32_t a1, uint32_t a2,
                              uint32_t a3, uint64_t budget)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    int rc;
    memset(&bus, 0, sizeof(bus));
    bus.read = hx_read;
    bus.write = hx_write;
    bus.bridge = hx_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0; cpu.gpr[5] = a1; cpu.gpr[6] = a2; cpu.gpr[7] = a3;
    cpu.gpr[29] = HX_STACK_TOP;
    cpu.gpr[31] = HX_HALT;
    hx_side = 0;
    hx_nevents[0] = 0;
    hx_cpu = &cpu;
    rc = PcPortMipsRun(&cpu, entry, HX_HALT, budget);
    hx_cpu = NULL;
    HX_REQUIRE(rc == PC_PORT_MIPS_HALTED, "retail run rc=%d pc=%08x %s", rc, cpu.pc, cpu.error);
    return cpu.gpr[2];
}

/* Snapshot -> retail -> save -> restore snapshot -> caller runs native. */
static void hx_begin_case(void) { memcpy(hx_snapshot, hx_p(HX_BASE), HX_SIZE); }
static void hx_retail_done(void)
{
    memcpy(hx_retail_after, hx_p(HX_BASE), HX_SIZE);
    memcpy(hx_p(HX_BASE), hx_snapshot, HX_SIZE);
    hx_side = 1;
    hx_nevents[1] = 0;
}

static void hx_compare_events(void)
{
    unsigned i, k;
    HX_REQUIRE(hx_nevents[0] == hx_nevents[1], "event count retail=%u native=%u",
               hx_nevents[0], hx_nevents[1]);
    for (i = 0; i < hx_nevents[0]; ++i) {
        const HxEvent* r = &hx_events[0][i];
        const HxEvent* n = &hx_events[1][i];
        const HxCallee* c = hx_find(r->fn);
        HX_REQUIRE(r->fn == n->fn, "event %u: retail %s native %s", i,
                   c ? c->name : "?", hx_find(n->fn) ? hx_find(n->fn)->name : "?");
        for (k = 0; k < r->n; ++k)
            HX_REQUIRE(r->a[k] == n->a[k], "event %u %s arg%u retail=%08x native=%08x",
                       i, c ? c->name : "?", k, r->a[k], n->a[k]);
    }
}

/* Compare RAM after native with RAM after retail, skipping the retail stack
 * and caller-listed ranges (e.g. the retail-layout mirror of a native struct,
 * which is compared field-wise instead). */
static void hx_compare_ram(const uint32_t (*skip)[2], unsigned nskip)
{
    uint32_t a;
    for (a = HX_BASE; a < HX_BASE + HX_SIZE; ++a) {
        unsigned k, skipped = a >= HX_STACK_LOW && a < HX_STACK_TOP + 0x100;
        for (k = 0; k < nskip && !skipped; ++k) skipped = a >= skip[k][0] && a < skip[k][1];
        if (skipped) continue;
        if (hx_retail_after[a - HX_BASE] != *hx_p(a)) {
            HX_REQUIRE(0, "RAM %08x retail=%02x native=%02x", a,
                       hx_retail_after[a - HX_BASE], *hx_p(a));
        }
    }
    ++hx_checks;
}

static inline uint32_t hx_retail_r32(uint32_t a) { uint32_t v; memcpy(&v, hx_retail_after + (a - HX_BASE), 4); return v; }
static inline uint8_t hx_retail_r8(uint32_t a) { return hx_retail_after[a - HX_BASE]; }

/* ---- native-struct mirroring ------------------------------------------ */
typedef struct HxField {
    uint32_t retail_off;
    size_t native_off;
    uint32_t retail_span;
    size_t native_elem;
    size_t count;
    int is_ptr;
    const char* name;
} HxField;

/* Copy a native struct into a retail-layout guest block.  Nested members
 * whose native element size differs from the retail stride are skipped (the
 * test must not rely on them). */
static void hx_mirror_out(const HxField* f, unsigned n, const void* native, uint32_t guest)
{
    unsigned i;
    size_t k;
    for (i = 0; i < n; ++i) {
        uint32_t stride = (uint32_t)(f[i].retail_span / f[i].count);
        for (k = 0; k < f[i].count; ++k) {
            const uint8_t* src = (const uint8_t*)native + f[i].native_off + k * f[i].native_elem;
            uint32_t dst = guest + f[i].retail_off + (uint32_t)k * stride;
            if (f[i].is_ptr) {
                void* p;
                memcpy(&p, src, sizeof(p));
                hx_w32(dst, hx_addr(p));
            } else if (f[i].native_elem <= stride) {
                memcpy(hx_p(dst), src, f[i].native_elem);
            }
        }
    }
}

/* Compare a native struct with a retail-layout block in the retail-after
 * RAM image. */
static void hx_mirror_compare(const HxField* f, unsigned n, const void* native, uint32_t guest)
{
    unsigned i;
    size_t k;
    for (i = 0; i < n; ++i) {
        uint32_t stride = (uint32_t)(f[i].retail_span / f[i].count);
        for (k = 0; k < f[i].count; ++k) {
            const uint8_t* src = (const uint8_t*)native + f[i].native_off + k * f[i].native_elem;
            uint32_t dst = guest + f[i].retail_off + (uint32_t)k * stride;
            if (f[i].is_ptr) {
                void* p;
                memcpy(&p, src, sizeof(p));
                HX_REQUIRE(hx_retail_r32(dst) == hx_addr(p), "struct field %s[%zu] pointer", f[i].name, k);
            } else if (f[i].native_elem <= stride) {
                HX_REQUIRE(memcmp(hx_retail_after + (dst - HX_BASE), src, f[i].native_elem) == 0,
                           "struct field %s[%zu] retail=%02x.. native=%02x..", f[i].name, k,
                           hx_retail_after[dst - HX_BASE], src[0]);
            }
        }
    }
}

#endif

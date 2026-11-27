#define _GNU_SOURCE
#define XENO_BATTLE_SHARED_DATA_OWNER 1

#include "god_mode.h"
/* Host hooks: weak so the tests that #include/link this runtime without the
 * port's god_mode.c / game_overrides.c still link. */
#pragma weak PcPort_GodModeBeforeGuest
#pragma weak PcPort_BattleStateHashBeforeGuest
#pragma weak PcPort_BattleResultsTraceBeforeGuest
#include "battle_mips_adapter.h"
void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu*, uint32_t);
void PcPort_BattleResultsTraceBeforeGuest(PcPortMipsCpu*, uint32_t);
#include "battle_mips_runtime_internal.h"
#include "psx_memory.h"
#include "system_window_guard.h"
#pragma weak PcPort_SystemWindowGuardSwap
#include <setjmp.h>
#include <limits.h>
#include "battle_guest_resolver.h"
#include "types.h"
#include "system/graphics.h"
#include "system/memory.h"
#include "system/sound.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "PsyX/PsyX_public.h"
#include "gpu/PsyX_GPU.h"

#ifdef _WIN32
#include "battle_win32_host.h"
#define RTLD_DEFAULT ((void *)0)
static void *dlsym(void *scope, const char *name)
{
    return scope == RTLD_DEFAULT ? PcPort_Win32MainImageSymbol(name) : NULL;
}
#else
#include <dlfcn.h>
#endif
#if defined(__linux__)
#include <link.h>
#include <pthread.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BATTLE_BASE       0x8006faf0u
#define BATTLE_END        0x800c3a6cu
#define BATTLE_ENTRY      0x80070f40u
#define BATTLE_STACK_TOP  0x801fff00u
#define BATTLE_HALT_PC    0xfffffffcu
#define BATTLE_STEP_LIMIT UINT64_C(20000000000)

/* XENO_BATTLE_STEP_LIMIT overrides the per-battle guest instruction budget
 * (default 2e10; the old 2e9 was ~2 minutes of battle at 60 fps); a long play session otherwise
 * ends with "instruction budget exhausted". */
static uint64_t battle_step_limit(void)
{
    const char *e = getenv("XENO_BATTLE_STEP_LIMIT");
    if (e && e[0]) {
        unsigned long long v = strtoull(e, NULL, 0);
        if (v > 0) return (uint64_t)v;
    }
    return BATTLE_STEP_LIMIT;
}
#define CALLBACK_STEP_LIMIT UINT64_C(50000000)

typedef struct PcPortBattleSymbol {
    uint32_t address;
    uint32_t size;
    uint8_t is_function;
    const char *name;
} PcPortBattleSymbol;

#include "host_only_bss.h"
#include "battle_bridge_map.inc"

typedef struct ResolvedFunction {
    uint32_t address;
    void *host;
    const char *name;
} ResolvedFunction;

typedef struct ResolvedData {
    uint32_t address;
    uint32_t size;
    uint8_t *host;
    const char *name;
} ResolvedData;

typedef struct HostRange {
    uintptr_t begin;
    uintptr_t end;
    int writable;
} HostRange;

typedef struct BattleMipsRuntime BattleMipsRuntime;
#include "battle_file1_controller.h"
#include "battle_results_gate.h"

/* One resolved-call cache slot.  Every guest call that is not found in the
 * bridge table used to fall back to a `func_%08X` dlsym on EVERY call, plus a
 * linear scan of the whole function table; a battle that calls an
 * interpreter-only overlay routine in a loop then spends all its time inside
 * the dynamic linker (observed live: an attack stalled for minutes with the
 * stack repeatedly in do_lookup_x / _dl_lookup_symbol_x).  Cache the verdict
 * per target address instead -- resolved host entry, or "no host owner, run the
 * retail bytes".  This changes no resolution outcome, only its cost. */
#define BRIDGE_CALL_CACHE_SIZE 512
typedef struct BridgeCallCacheSlot {
    uint32_t target;
    int state;              /* 0 empty, 1 resolved, 2 unresolved */
    ResolvedFunction entry; /* storage when the fallback path built one */
    char name[32];          /* backing store for entry.name */
} BridgeCallCacheSlot;

struct BattleMipsRuntime {
    int initialized;
#if defined(__linux__)
    pthread_t owner_thread;
    int owner_thread_recorded;
#endif
    int resolving_write; /* write flag of the access being resolved (diagnostic only) */
    int trace_calls;
    /* Verification hook, off in production. When set, overlay targets are never
     * adopted: every guest call resolves to the interpreter. The differential
     * harness sets it to run the retail bytes for a function and then the host
     * C body on identical guest RAM and compare the two. */
    int force_interpret;
    BattleFile1Identity file1;
    /* Identity gate for the adopted RESULTS module routines. */
    BattleResultsIdentity results;
    PcPortMipsCpu *bridge_cpu;
    ResolvedFunction functions[1024];
    size_t function_count;
    ResolvedData data[1024];
    size_t data_count;
    HostRange host_ranges[256];
    size_t host_range_count;
    BridgeCallCacheSlot call_cache[BRIDGE_CALL_CACHE_SIZE];
};

typedef uintptr_t (*GenericHostFunction)(
    uintptr_t, uintptr_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t);

static BattleMipsRuntime g_BattleRuntime;
static BattleMipsRuntime *g_ActiveBattleRuntime;

extern unsigned int MFC2(int reg);
extern void MTC2(unsigned int value, int reg);
extern unsigned int CFC2(int reg);
extern void CTC2(unsigned int value, int reg);
extern int doCOP2(int op);

/* The generated stub object provides a strong definition. The weak fallback
 * keeps the trial link self-contained; final builds reject every generated
 * function stub before it can be called from retail guest code. */
__attribute__((weak)) int xeno_port_is_generated_stub(const char *name)
{
    (void)name;
    return 0;
}

static uint32_t load_le(const uint8_t *p, unsigned width)
{
    uint32_t value = p[0];
    if (width >= 2)
        value |= (uint32_t)p[1] << 8;
    if (width == 4) {
        value |= (uint32_t)p[2] << 16;
        value |= (uint32_t)p[3] << 24;
    }
    return value;
}

static void store_le(uint8_t *p, unsigned width, uint32_t value)
{
    p[0] = (uint8_t)value;
    if (width >= 2)
        p[1] = (uint8_t)(value >> 8);
    if (width == 4) {
        p[2] = (uint8_t)(value >> 16);
        p[3] = (uint8_t)(value >> 24);
    }
}

static int function_compare(const void *lhs, const void *rhs)
{
    const ResolvedFunction *a = lhs;
    const ResolvedFunction *b = rhs;
    if (a->address < b->address) return -1;
    if (a->address > b->address) return 1;
    return strcmp(a->name, b->name);
}

static int data_compare(const void *lhs, const void *rhs)
{
    const ResolvedData *a = lhs;
    const ResolvedData *b = rhs;
    if (a->address < b->address) return -1;
    if (a->address > b->address) return 1;
    if (a->size > b->size) return -1;
    if (a->size < b->size) return 1;
    return strcmp(a->name, b->name);
}

static uint32_t inferred_data_size(size_t index)
{
    uint32_t address = g_BattleBridgeSymbols[index].address;
    size_t i;

    if (g_BattleBridgeSymbols[index].size != 0)
        return g_BattleBridgeSymbols[index].size;
    for (i = index + 1; i < g_BattleBridgeSymbolCount; i++) {
        uint32_t next = g_BattleBridgeSymbols[i].address;
        if (g_BattleBridgeSymbols[i].is_function || next <= address)
            continue;
        if (next - address <= 0x10000u)
            return next - address;
        break;
    }
    return 4;
}

static void load_host_ranges(BattleMipsRuntime *runtime)
{
#ifdef _WIN32
    (void)runtime;
#else
    FILE *maps = fopen("/proc/self/maps", "r");
    char line[512];

    if (maps == NULL)
        return;
    while (runtime->host_range_count <
           sizeof(runtime->host_ranges) / sizeof(runtime->host_ranges[0]) &&
           fgets(line, sizeof(line), maps) != NULL) {
        unsigned long begin;
        unsigned long end;
        char perms[5];
        HostRange *range;

        if (sscanf(line, "%lx-%lx %4s", &begin, &end, perms) != 3 ||
            perms[0] != 'r')
            continue;
        range = &runtime->host_ranges[runtime->host_range_count++];
        range->begin = (uintptr_t)begin;
        range->end = (uintptr_t)end;
        range->writable = perms[1] == 'w';
    }
    fclose(maps);
#endif
}

static int host_address_valid(BattleMipsRuntime *runtime, uintptr_t address,
                              unsigned width, int write)
{
#ifdef _WIN32
    (void)runtime;
    return PcPort_Win32HostAddressValid(address, width, write);
#else
    size_t i;
    for (i = 0; i < runtime->host_range_count; i++) {
        const HostRange *range = &runtime->host_ranges[i];
        if (address >= range->begin && address <= range->end &&
            width <= range->end - address &&
            (!write || range->writable))
            return 1;
    }
    return 0;
#endif
}

/* These four scalar renderer globals are missing from the generated map.
 * Exact bindings prevent writes from landing in an inferred neighboring
 * symbol's host allocation, whose layout is unrelated to guest adjacency. */
static void initialize_model_node_data(BattleMipsRuntime *runtime)
{
    static const struct {uint32_t address;const char *name;} globals[]={
        {0x80050108u,"D_80050108"},{0x8005010cu,"D_8005010C"},
        {0x80059310u,"D_80059310"},{0x80059314u,"D_80059314"}
    };
    for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);i++) {
        uint8_t *host=dlsym(RTLD_DEFAULT,globals[i].name);
        if(!host)continue;
        if(runtime->data_count>=sizeof(runtime->data)/sizeof(runtime->data[0]))abort();
        runtime->data[runtime->data_count++]=(ResolvedData){globals[i].address,4u,host,globals[i].name};
    }
}

/* The sound manager's element count is initialized by native sound setup.
 * It is absent from the generated guest map, so without this exact scalar
 * binding the battle wrapper reads stale guest RAM and can select slot 254. */
static void initialize_sound_count_data(BattleMipsRuntime *runtime)
{
    uint8_t *host = dlsym(RTLD_DEFAULT, "D_80059478");
    if (!host) return;
    size_t slot = 0;
    while (slot < runtime->data_count && runtime->data[slot].address != 0x80059478u)
        slot++;
    if (slot == runtime->data_count) {
        if (slot >= sizeof(runtime->data) / sizeof(runtime->data[0])) abort();
        runtime->data_count++;
    }
    runtime->data[slot] = (ResolvedData){0x80059478u, 4u, host, "D_80059478"};
}

/* Original descriptor rows and working pointers belong to the converted
 * packet cluster. Native legacy pointer globals are a separate, unconverted
 * interface; exposing their LP64 layout through this bus would corrupt data. */
static void initialize_model_packet_data(BattleMipsRuntime *runtime)
{
    static const struct {uint32_t address,size;const char *name;} bindings[]={
        {0x80018980u,24u,NULL},
        {0x8004fe50u,680u,NULL},
        {0x80059424u,4u,NULL},{0x80059528u,4u,NULL},{0x80059538u,4u,NULL},
        {0x8005952cu,4u,NULL},{0x8005953cu,4u,NULL},{0x80059498u,4u,NULL},
        {0x80059308u,2u,"D_80059308"},{0x8005930cu,2u,"D_8005930C"},
        {0x800595c0u,4u,"D_800595C0"},
        {0x80059568u,4u,NULL},{0x80050104u,4u,"D_80050104"},
        {0x800500f8u,4u,"D_800500F8"},{0x800500fcu,4u,"D_800500FC"},
        {0x80050100u,4u,"D_80050100"},{0x80059578u,4u,"D_80059578"}
    };
    for(unsigned i=0;i<sizeof(bindings)/sizeof(bindings[0]);i++){
        uint8_t *host=bindings[i].name?dlsym(RTLD_DEFAULT,bindings[i].name):PSX_ADDR(bindings[i].address);
        if(!host)continue;
        size_t slot=0;while(slot<runtime->data_count&&runtime->data[slot].address!=bindings[i].address)slot++;
        if(slot==runtime->data_count){
            if(slot>=sizeof(runtime->data)/sizeof(runtime->data[0]))abort();
            runtime->data_count++;
        }
        runtime->data[slot]=(ResolvedData){bindings[i].address,bindings[i].size,host,
            bindings[i].name?bindings[i].name:"model-packet-canonical"};
    }
}

/* A binding wider than the host object it lands on would let a guest access
 * past the object's end reach its link neighbours: inferred_data_size runs to
 * the next guest symbol, while the native object (a 32-byte generated stub, a
 * narrower C global) can be smaller.  The port links -rdynamic, so dladdr1
 * finds each bound object and its ELF size, and the binding is clamped to
 * it: the guest bytes past the object stay in guest RAM, as in retail, where
 * they are separate memory.  The clamped list is logged once per process and
 * checked against pc_port/tests/battle_binding_sizes.allow by
 * pc_port/tests/check_battle_binding_sizes.py.
 * glibc declares Dl_info / dladdr1 only under _GNU_SOURCE, which the port's
 * "-include assert.h" settles before this file's own define; the struct
 * layout and RTLD_DL_SYMENT (1) are glibc ABI, reached through an asm label
 * so a TU that does see the glibc declaration does not conflict. */
#if defined(__linux__)
typedef struct BindingDlInfo {
    const char *fname;
    void *fbase;
    const char *sname;
    void *saddr;
} BindingDlInfo;
extern int binding_dladdr1(const void *address, BindingDlInfo *info, void **extra,
                           int flags) __asm__("dladdr1");

static void runtime_binding_size_guard(BattleMipsRuntime *runtime)
{
    static int reported;
    unsigned checked = 0, oversized = 0, unknown = 0;
    int report = !reported;
    size_t i;

    reported = 1;
    for (i = 0; i < runtime->data_count; i++) {
        ResolvedData *d = &runtime->data[i];
        uintptr_t host = (uintptr_t)d->host, end;
        uintptr_t ram = (uintptr_t)g_PsxRam, pad = (uintptr_t)g_PsxScratchpad;
        const ElfW(Sym) *sym = NULL;
        BindingDlInfo info;

        if (d->host == NULL || d->size == 0 ||
            (host >= ram && host < ram + PSX_RAM_SIZE) || (host >= pad && host < pad + 0x1000u))
            continue;   /* guest RAM / scratchpad: no host neighbour */
        if (binding_dladdr1(d->host, &info, (void **)&sym, 1 /* RTLD_DL_SYMENT */) == 0 ||
            sym == NULL || info.saddr == NULL || host >= (uintptr_t)info.saddr + sym->st_size) {
            unknown++;
            continue;
        }
        checked++;
        end = (uintptr_t)info.saddr + sym->st_size;
        if (host + d->size > end) {
            oversized++;
            if (d->name == NULL || strcmp(info.sname, d->name) != 0) {
                /* Another symbol at this address: report, do not guess. */
                if (report)
                    fprintf(stderr, "[xeno-port][battle-bindings] oversized binding 0x%08x %s lands "
                            "on %s; not clamped\n", d->address, d->name ? d->name : "?",
                            info.sname ? info.sname : "?");
                continue;
            }
            if (report)
                fprintf(stderr, "[xeno-port][battle-bindings] oversized binding 0x%08x size 0x%x %s: "
                    "host object %s is %zu bytes (binding at +0x%zx); clamped to 0x%zx\n",
                    d->address, d->size, d->name ? d->name : "?", info.sname,
                    (size_t)sym->st_size, (size_t)(host - (uintptr_t)info.saddr),
                    (size_t)(end - host));
            d->size = (uint32_t)(end - host);
        }
    }
    if (report)
        fprintf(stderr, "[xeno-port][battle-bindings] checked=%u clamped=%u not-exported=%u\n",
                checked, oversized, unknown);
}
#elif defined(_WIN32)
static void runtime_binding_size_guard(BattleMipsRuntime *runtime)
{
    for (size_t i = 0; i < runtime->data_count; ++i) {
        ResolvedData *data = &runtime->data[i];
        if (!data->name || PcPort_Win32MainImageSymbol(data->name) != data->host)
            continue;
        size_t bytes = PcPort_Win32DataSymbolSize(data->name);
        if (bytes && bytes < data->size) data->size = (uint32_t)bytes;
    }
}
#else
/* dladdr1 and ELF symbol sizes are glibc/Linux; elsewhere bindings keep their
 * inferred sizes (pc_port/tests/check_battle_binding_sizes.py still checks). */
static void runtime_binding_size_guard(BattleMipsRuntime *runtime)
{
    (void)runtime;
}
#endif

static void initialize_runtime(BattleMipsRuntime *runtime)
{
    size_t i;

    if (runtime->initialized)
        return;
#if defined(__linux__)
    runtime->owner_thread=pthread_self();
    runtime->owner_thread_recorded=1;
#endif
    runtime->trace_calls = getenv("XENO_BATTLE_MIPS_TRACE") != NULL;
    load_host_ranges(runtime);
    /* The call cache stores resolved entries for the table built below. */
    memset(runtime->call_cache, 0, sizeof(runtime->call_cache));

    for (i = 0; i < g_BattleBridgeSymbolCount; i++) {
        const __typeof__(g_BattleBridgeSymbols[0]) *symbol = &g_BattleBridgeSymbols[i];
        void *host;

        /* Overlay-owned code and data must stay in the loaded retail image,
         * never bind to the native build's generated placeholders. */
        if (symbol->address >= BATTLE_BASE)
            continue;
        /* The decomp names these two retail table entries opposite to
         * PsyCross's conventional sine/cosine exports (see the libgte shim). */
        if (symbol->is_function && symbol->address == 0x8003f8b0u)
            host = dlsym(RTLD_DEFAULT, "rsin");
        else if (symbol->is_function && symbol->address == 0x8003f8ccu)
            host = dlsym(RTLD_DEFAULT, "rcos");
        else
            host = dlsym(RTLD_DEFAULT, symbol->name);
        if (host == NULL)
            continue;
        if (symbol->is_function) {
            ResolvedFunction *entry;
            if (runtime->function_count >=
                sizeof(runtime->functions) / sizeof(runtime->functions[0]))
                abort();
            entry = &runtime->functions[runtime->function_count++];
            entry->address = symbol->address;
            entry->host = host;
            entry->name = symbol->name;
        } else {
            ResolvedData *entry;
            if (runtime->data_count >=
                sizeof(runtime->data) / sizeof(runtime->data[0]))
                abort();
            entry = &runtime->data[runtime->data_count++];
            entry->address = symbol->address;
            entry->size = inferred_data_size(i);
            /* This main-executable data block ends at 8001C8DC. Its next
             * bridge symbol is a function, so the generic data-size guess
             * would otherwise extend the mapping beyond the host object. */
            if (symbol->address == 0x8001c76cu &&
                strcmp(symbol->name, "D_8001C76C") == 0)
                entry->size = 0x170u;
            /* The symbol table sizes g_GameState at 0x2300, but the retail
             * state runs to 0x2358: the new-game template copies 0x2358
             * bytes and the field stores map/camera/entrance/song at
             * +0x231A..+0x2322 (virtual_machine.c).  The native build mirrors
             * that tail with the contiguous globals D_8006F934..D_8006F958
             * (data_game_state.c), so guest reads of those fields must hit
             * the native copy, not the raw guest RAM behind it.  Do NOT go
             * past 0x2358: the rest of the 0x4600 array is reservation
             * padding no native code writes, while the retail overlays
             * themselves write D_8006F9DC.. (no native owner) there -- that
             * block must stay guest-resident.  Widening to 0x4600 once fed
             * the battle zeros for it and it picked the wrong backdrop. */
            if (strcmp(symbol->name, "g_GameState") == 0 &&
                entry->size < 0x2358u)
                entry->size = 0x2358u;
            entry->host = host;
            entry->name = symbol->name;
        }
    }
    initialize_model_node_data(runtime);
    initialize_model_packet_data(runtime);
    initialize_sound_count_data(runtime);
    qsort(runtime->functions, runtime->function_count,
          sizeof(runtime->functions[0]), function_compare);
    qsort(runtime->data, runtime->data_count,
          sizeof(runtime->data[0]), data_compare);
    runtime->initialized = 1;
    runtime_binding_size_guard(runtime);
    fprintf(stderr,
            "[xeno-port][battle-mips] retail adapter ready: functions=%zu "
            "shared-data=%zu host-ranges=%zu\n",
            runtime->function_count, runtime->data_count,
            runtime->host_range_count);
}

static const ResolvedFunction *find_function(BattleMipsRuntime *runtime,
                                             uint32_t address)
{
    size_t lo = 0;
    size_t hi = runtime->function_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (runtime->functions[mid].address < address)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < runtime->function_count &&
        runtime->functions[lo].address == address)
        return &runtime->functions[lo];
    return NULL;
}

static ResolvedData *find_data(BattleMipsRuntime *runtime, uint32_t address,
                               unsigned width)
{
    size_t lo = 0;
    size_t hi = runtime->data_count;
    size_t cursor;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (runtime->data[mid].address <= address)
            lo = mid + 1;
        else
            hi = mid;
    }
    cursor = lo;
    while (cursor > 0) {
        ResolvedData *entry = &runtime->data[--cursor];
        uint32_t offset = address - entry->address;
        if (offset <= 0x10000u && offset + width <= entry->size)
            return entry;
        if (entry->address + 0x10000u < address)
            break;
    }
    return NULL;
}

/* XENO_BATTLE_UNSHARED_DIAG=1: log the first access to every main-exe data
 * address (sdata/sbss/bss below the battle overlay) that the guest touches
 * without a shared native binding.  Such accesses land in raw g_PsxRam, which
 * native code never updates, so they read stale zeros -- the silent failure
 * mode behind "the battle sees an empty stat".  Diagnostic only. */
static void note_unshared_access(uint32_t address, unsigned width, int write)
{
    static int enabled = -1;
    static uint32_t seen[4096];
    static size_t seen_count;
    size_t lo = 0;
    size_t hi;

    if (enabled < 0)
        enabled = getenv("XENO_BATTLE_UNSHARED_DIAG") != NULL;
    if (!enabled)
        return;
    hi = seen_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (seen[mid] < address)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < seen_count && seen[lo] == address)
        return;
    if (seen_count >= sizeof(seen) / sizeof(seen[0]))
        return;
    memmove(&seen[lo + 1], &seen[lo], (seen_count - lo) * sizeof(seen[0]));
    seen[lo] = address;
    seen_count++;
    fprintf(stderr, "[xeno-port][battle-mips][unshared] %s addr=0x%08x width=%u\n",
            write ? "write" : "read", address, width);
}

/* Shared-data lookup used by the one guest resolver (battle_guest_resolver.h)
 * on behalf of the interpreter (context = its runtime). */
static uint8_t *runtime_shared_lookup(void *context, uint32_t canonical,
                                      unsigned width)
{
    BattleMipsRuntime *runtime = context;
    ResolvedData *entry = find_data(runtime, canonical, width);
    if (entry != NULL)
        return entry->host + (canonical - entry->address);
    /* Also flag data accesses into main-exe .text: the port never copies that
     * range into guest RAM (functions are bridged), so a guest reading a table
     * that the compiler placed there sees zeros. */
    if (canonical >= PSX_EXE_SDATA_START ||
        (canonical >= PSX_EXE_TEXT_START && canonical < PSX_EXE_TEXT_END))
        note_unshared_access(canonical, width, runtime->resolving_write);
    return NULL;
}

/* The same lookup for native battle bodies (bg_ptr/GPTR): the active battle's
 * runtime, or none. The PE definition owns the optional consumer symbol. */
uint8_t *PcPort_BattleSharedData(void *context, uint32_t canonical,
                                 unsigned width)
{
    BattleMipsRuntime *runtime = g_ActiveBattleRuntime;
    ResolvedData *entry;
    (void)context;
    if (runtime == NULL || !runtime->initialized)
        return NULL;
    entry = find_data(runtime, canonical, width);
    return entry != NULL ? entry->host + (canonical - entry->address) : NULL;
}

static uint8_t *resolve_memory(BattleMipsRuntime *runtime, uint32_t address,
                               unsigned width, int write)
{
    uint8_t *p = NULL;
    uintptr_t host;

    runtime->resolving_write = write;
    switch (battle_guest_resolve(address, width, runtime_shared_lookup,
                                 runtime, &p)) {
    case BATTLE_GUEST_RESOLVED:
        return p;
    case BATTLE_GUEST_INVALID:
        return NULL;
    default:
        break;
    }
    host = (uintptr_t)address;
    if (host_address_valid(runtime, host, width, write))
        return (uint8_t *)host;
    return NULL;
}

/* KUSEG [0, 2 MB) is the PS1's user-segment alias of main RAM, and the
 * interpreter's bus maps it to KSEG0 as the hardware does.  Retail reaches it
 * through NULL-based pointers: battle AI action opcode 0x70 (func_8007E7C0)
 * stores through D_800D3278, which stays NULL (MainLoop's ClearMemory) in a
 * battle whose descriptor does not load battle_command_file1, and on
 * hardware the store lands in BIOS low RAM and the battle carries on.
 *
 * Only these bus accesses alias it.  The shared resolver -- and so
 * bg_ptr/GPTR in native bodies -- keeps guest NULL as host NULL, so native
 * NULL checks and GADDR round trips stay as they are.  Instruction fetches
 * never get here for KUSEG: the bridge rejects such a pc first ("unresolved
 * call target").  Above 2 MB a word may be a raw -no-pie host address, so the
 * alias stops there.  The interpreter's hot path gains one compare; the
 * alias and its note are a cold path. */
#define BATTLE_BUS_KUSEG_RAM_END 0x00200000u

/* A bus access into KUSEG RAM is legal retail behaviour, but it is also what
 * a port-side NULL looks like: name each distinct address once (up to 32), so
 * a recurring retail site cannot hide a new one. */
static uint8_t *runtime_resolve_kuseg(BattleMipsRuntime *runtime, uint32_t address,
                                      unsigned width, int write)
{
    static uint32_t seen[32];
    static unsigned seen_count;
    uint8_t *p = resolve_memory(runtime, address | 0x80000000u, width, write);
    if (p != NULL) {
        unsigned i;
        for (i = 0; i < seen_count; i++)
            if (seen[i] == address)
                break;
        if (i == seen_count && seen_count < 32u) {
            seen[seen_count++] = address;
            fprintf(stderr,
                    "[xeno-port][battle-mips] KUSEG RAM %s%u at 0x%08x (PS1 "
                    "low-RAM alias, as on hardware; distinct address %u of "
                    "max 32 logged)\n",
                    write ? "write" : "read", width * 8u, address, seen_count);
        }
    }
    return p;
}

static int runtime_read(void *opaque, uint32_t address, unsigned width,
                        uint32_t *value)
{
    BattleMipsRuntime *runtime = opaque;
    uint8_t *p = address < BATTLE_BUS_KUSEG_RAM_END
        ? runtime_resolve_kuseg(runtime, address, width, 0)
        : resolve_memory(runtime, address, width, 0);
    if (p == NULL)
        return -1;
    *value = load_le(p, width);
    return 0;
}

/* Store-side invalidation of the identity-gated modules, for a store about to
 * land at resolved host pointer p.  Shared by the interpreter's bus and by
 * the adopted results bodies' accessor (PcPort_BattleMipsResolve), so a
 * native store has exactly the effect on module identity a retail one has. */
static void runtime_note_store(BattleMipsRuntime *runtime, const uint8_t *p,
                               unsigned width)
{
    if (runtime->file1.verified && (uintptr_t)p + width > (uintptr_t)g_PsxRam + 0x1e5000u &&
        (uintptr_t)p < (uintptr_t)g_PsxRam + 0x1e9b5cu)
        file1_invalidate(runtime);
    results_note_store(runtime, p, width);
}

static int runtime_write(void *opaque, uint32_t address, unsigned width,
                         uint32_t value)
{
    BattleMipsRuntime *runtime = opaque;
    uint8_t *p = address < BATTLE_BUS_KUSEG_RAM_END
        ? runtime_resolve_kuseg(runtime, address, width, 1)
        : resolve_memory(runtime, address, width, 1);
    if (p == NULL)
        return -1;
    runtime_note_store(runtime, p, width);
    store_le(p, width, value);
    return 0;
}

static uintptr_t translate_argument(BattleMipsRuntime *runtime,
                                    uint32_t value)
{
    uint8_t *p;
    if ((value & 0xff800000u) != 0x80000000u &&
        (value & 0xff800000u) != 0xa0000000u &&
        (value & 0xfffff000u) != 0x1f800000u)
        return (uintptr_t)value;
    p = resolve_memory(runtime, value, 1, 0);
    return p != NULL ? (uintptr_t)p : (uintptr_t)value;
}

/* Typed bridge thunks for the battle guest-ABI leaves
 * (pc_port/src/battle_guest_abi_thunks.inc has the contract).  The legacy
 * generic bridge cannot tell an integer from a pointer, so it turns every
 * KSEG0/KSEG1/scratchpad-shaped word into a host pointer (translate_argument).
 * A guest-ABI body takes guest addresses as u32 integers and may hand one back
 * unchanged (func_800A5914 returns its fourth argument when tries <= 0): a
 * translated argument then comes back in the HOST domain and retail stores a
 * host address into guest RAM.  An entry here passes each argument exactly as
 * its declared kind, with no heuristic. */
enum {
    BG_THUNK_ARG_W32 = 1,
    BG_THUNK_ARG_GUEST,
    BG_THUNK_ARG_U8,
    BG_THUNK_ARG_S8,
    BG_THUNK_ARG_U16,
    BG_THUNK_ARG_S16,
    BG_THUNK_ARG_HOST
};
enum {
    BG_THUNK_RET_VOID = 1,
    BG_THUNK_RET_W32,
    BG_THUNK_RET_GUEST,
    BG_THUNK_RET_U8,
    BG_THUNK_RET_S8,
    BG_THUNK_RET_U16,
    BG_THUNK_RET_S16,
    BG_THUNK_RET_HOST
};
/* o32 argument words a thunk can carry: a0..a3 and twelve stack words.  The
 * widest adopted or candidate leaf is 800BA984 (14 words, its 14th read). */
#define BG_THUNK_MAX_ARGS 16u

typedef struct BattleGuestThunk {
    uint32_t address;
    const char *name;
    uint8_t ret;
    uint8_t argc;
    uint8_t args[BG_THUNK_MAX_ARGS];
} BattleGuestThunk;

typedef uintptr_t (*BattleGuestThunkFunction)(
    uintptr_t, uintptr_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t, uintptr_t);

#define BG_THUNK(addr, fname, rkind, nargs, ...) \
    { (addr), (fname), (rkind), (nargs), { __VA_ARGS__ } },
__attribute__((unused)) static const BattleGuestThunk kBattleGuestThunks[] = {
#include "battle_guest_abi_thunks.inc"
};
#undef BG_THUNK
#define BATTLE_GUEST_THUNK_COUNT \
    (sizeof(kBattleGuestThunks) / sizeof(kBattleGuestThunks[0]))

__attribute__((unused)) static const BattleGuestThunk *battle_guest_thunk(uint32_t target)
{
    size_t i;
    for (i = 0; i < BATTLE_GUEST_THUNK_COUNT; i++) {
        if (kBattleGuestThunks[i].address == target)
            return &kBattleGuestThunks[i];
    }
    return NULL;
}

/* Calls `host` (the leaf's native body) with `words` (the o32 argument words,
 * a0..a3 then the stack slots) converted per the thunk's declared kinds, and
 * stores the declared return into *v0 (untouched for void).  -1 when a
 * declared pointer has no backing or a pointer return cannot be put back in
 * the guest domain; nothing is called in the first case. */
__attribute__((unused)) static int battle_guest_thunk_invoke(BattleMipsRuntime *runtime,
                                     const BattleGuestThunk *thunk, void *host,
                                     const uint32_t *words, uint32_t *v0)
{
    uintptr_t args[BG_THUNK_MAX_ARGS] = {0};
    uintptr_t result;
    unsigned i;

    for (i = 0; i < thunk->argc; i++) {
        uint32_t w = words[i];
        switch (thunk->args[i]) {
        case BG_THUNK_ARG_W32:
        case BG_THUNK_ARG_GUEST: args[i] = (uintptr_t)w; break;
        case BG_THUNK_ARG_U8:    args[i] = (uintptr_t)(uint8_t)w; break;
        case BG_THUNK_ARG_S8:    args[i] = (uintptr_t)(uint32_t)(int32_t)(int8_t)w; break;
        case BG_THUNK_ARG_U16:   args[i] = (uintptr_t)(uint16_t)w; break;
        case BG_THUNK_ARG_S16:   args[i] = (uintptr_t)(uint32_t)(int32_t)(int16_t)w; break;
        case BG_THUNK_ARG_HOST:
            if ((w & 0xff800000u) == 0x80000000u ||
                (w & 0xff800000u) == 0xa0000000u ||
                (w & 0xfffff000u) == 0x1f800000u) {
                uint8_t *p = resolve_memory(runtime, w, 1, 0);
                if (p == NULL)
                    return -1;
                args[i] = (uintptr_t)p;
            } else {
                /* NULL, or a raw host address a native main-exe service
                 * handed the overlay (-no-pie; bg_ptr's rule). */
                args[i] = (uintptr_t)w;
            }
            break;
        default:
            return -1;
        }
    }
    result = ((BattleGuestThunkFunction)host)(args[0], args[1], args[2], args[3],
                                               args[4], args[5], args[6], args[7],
                                               args[8], args[9], args[10], args[11],
                                               args[12], args[13], args[14], args[15]);
    switch (thunk->ret) {
    case BG_THUNK_RET_VOID: break;
    case BG_THUNK_RET_W32:
    case BG_THUNK_RET_GUEST: *v0 = (uint32_t)result; break;
    case BG_THUNK_RET_U8:   *v0 = (uint8_t)result; break;
    case BG_THUNK_RET_S8:   *v0 = (uint32_t)(int32_t)(int8_t)result; break;
    case BG_THUNK_RET_U16:  *v0 = (uint16_t)result; break;
    case BG_THUNK_RET_S16:  *v0 = (uint32_t)(int32_t)(int16_t)result; break;
    case BG_THUNK_RET_HOST: {
        const void *pointer = (const void *)result;
        uintptr_t ram = (uintptr_t)g_PsxRam;
        uintptr_t pad = (uintptr_t)g_PsxScratchpad;
        if (pointer == NULL)
            *v0 = 0;
        else if (result >= ram && result < ram + PSX_RAM_SIZE)
            *v0 = PsxMemory_GuestAddr(pointer);
        else if (result >= pad && result < pad + 0x1000u)
            *v0 = 0x1f800000u | (uint32_t)(result - pad);
        else
            return -1;
        break;
    }
    default:
        return -1;
    }
    return 0;
}

static int is_callback_argument(const char *name, unsigned index)
{
    if (index == 0 && strcmp(name, "func_8001D164") == 0)
        return 1;
    if (index == 1 &&
        (strcmp(name, "WorkListSetTaskCallback") == 0 ||
         strcmp(name, "TimerWorkListSetTaskCallback") == 0 ||
         strcmp(name, "func_80021BF8") == 0 ||
         strcmp(name, "func_8001D0A4") == 0 ||
         strcmp(name, "WorkListTaskSetOnFreeCallback") == 0))
        return 1;
    if (strcmp(name, "WorkListsAddTasks") == 0 && index >= 2 && index <= 4)
        return 1;
    return 0;
}

static int target_is_guest_code(uint32_t target)
{
    if (target >= BATTLE_BASE && target < BATTLE_END)
        return 1;
    if (target >= 0x801d0000u && target < 0x80300000u)
        return 1;
    return 0;
}

/* XENO_CPU_FALLBACK_STATS: per-battle interpreter-vs-native meter (stderr). */
#include "battle_cpu_fallback_meter.inc"

/* XENO_BATTLE_EXIT_HASH state (battle_exit_hash_print, below). */
static int g_BattleExitHash = -1;
static unsigned g_BattleExitHashBattle;
static unsigned g_BattleExitHashResults;

/* Overlay C adopted onto the host. Pointer-only leaves plus bodies that
 * touch overlay D_* through battle_overlay_guest_ram.h (guest RAM, not a
 * second host copy). Nested guest pointers loaded from structs stay in
 * the interpreter until they grow an explicit translator. */
static const char *const kOverlayLeaves[] = {
#include "battle_overlay_host_leaves.inc"
};
#define OVERLAY_LEAF_COUNT (sizeof(kOverlayLeaves) / sizeof(kOverlayLeaves[0]))
/* The first leaf of the "Battle guest ABI prototype" section of
 * battle_overlay_host_leaves.inc: every name before it is one of the legacy
 * (pre-guest-ABI) adoptions, every name from it on is a guest-ABI or lifted
 * body. */
#define OVERLAY_FIRST_GUEST_ABI_LEAF "func_800A5A48"

/* TEST SWITCH XENO_BATTLE_NATIVE_LEAVES (verification only; unset is the
 * production default and changes nothing):
 *   0       decline every adopted overlay leaf -- legacy, guest-ABI and lifted
 *           alike -- so the interpreter runs the retail bytes instead;
 *   legacy  keep only the legacy leaves (those before the guest-ABI section);
 *   1/all   the default (every adopted leaf).
 * Any other value aborts, so a typo cannot silently compare native with
 * native.  Read once; one log line when active.  It gates only the overlay
 * leaf allowlist: the identity-gated RESULTS routines and the file-1
 * controller keep their own gates.
 * Returns the number of leading kOverlayLeaves entries that may run natively. */
static size_t g_OverlayLeafLimit = (size_t)-1;

size_t PcPort_BattleNativeLeafLimit(void)
{
    if (g_OverlayLeafLimit == (size_t)-1) {
        const char *v = getenv("XENO_BATTLE_NATIVE_LEAVES");
        size_t limit = OVERLAY_LEAF_COUNT, i;
        const char *mode = NULL;

        if (v == NULL || v[0] == '\0' || strcmp(v, "1") == 0 || strcmp(v, "all") == 0) {
            limit = OVERLAY_LEAF_COUNT;
        } else if (strcmp(v, "0") == 0) {
            limit = 0;
            mode = "0 (all adopted overlay leaves declined; interpreter runs retail)";
        } else if (strcmp(v, "legacy") == 0) {
            for (i = 0; i < OVERLAY_LEAF_COUNT; i++) {
                if (strcmp(kOverlayLeaves[i], OVERLAY_FIRST_GUEST_ABI_LEAF) == 0)
                    break;
            }
            if (i == OVERLAY_LEAF_COUNT) {
                fprintf(stderr, "[xeno-port][battle-mips] XENO_BATTLE_NATIVE_LEAVES=legacy: "
                        "boundary leaf %s missing from the allowlist\n",
                        OVERLAY_FIRST_GUEST_ABI_LEAF);
                abort();
            }
            limit = i;
            mode = "legacy (guest-ABI and lifted leaves declined)";
        } else {
            fprintf(stderr, "[xeno-port][battle-mips] XENO_BATTLE_NATIVE_LEAVES=%s "
                    "is not 0, legacy, 1 or all\n", v);
            abort();
        }
        if (mode != NULL)
            fprintf(stderr, "[xeno-port][battle-mips] TEST XENO_BATTLE_NATIVE_LEAVES=%s: "
                    "native_leaves=%zu/%zu\n", mode, limit, (size_t)OVERLAY_LEAF_COUNT);
        g_OverlayLeafLimit = limit;
    }
    return g_OverlayLeafLimit;
}

static int overlay_leaf_host_ok(const char *name)
{
    size_t i, limit = PcPort_BattleNativeLeafLimit();
    for (i = 0; i < limit; i++) {
        if (strcmp(name, kOverlayLeaves[i]) == 0)
            return 1;
    }
    return 0;
}

/* For native main-executable code that calls an adopted overlay leaf directly
 * (game_overrides.c's func_800BA8F4): non-zero when `name` may run natively
 * under XENO_BATTLE_NATIVE_LEAVES. */
int PcPort_BattleNativeLeafAllowed(const char *name)
{
    return overlay_leaf_host_ok(name);
}

static int graphics_pointer_to_guest(const void *pointer, uint32_t *value)
{
    uintptr_t host = (uintptr_t)pointer;
    uintptr_t ram = (uintptr_t)g_PsxRam;
    if (host >= ram && host < ram + PSX_RAM_SIZE) {
        *value = PsxMemory_GuestAddr(pointer);
        return 0;
    }
    if (host > UINT32_MAX) {
        fprintf(stderr, "[xeno-port][battle-mips] unrepresentable graphics pointer %p\n", pointer);
        return -1;
    }
    *value = (uint32_t)host;
    return 0;
}

/* The shard initializer calls these SDK entry points through resolved symbols.
 * Call their declared C types: the old twelve-uintptr_t call is undefined for
 * int/void/pointer returns and is rejected by Clang's function sanitizer.
 * Scalar arguments stay raw guest integers; only pointers use translation.
 * Void SDK calls have no result to publish. Pointer results return to the guest
 * address space before any caller can store or use them.
 */
static int bridge_shard_sdk(PcPortMipsCpu *cpu, const ResolvedFunction *resolved,
                             const uintptr_t *args)
{
    const char *name = resolved->name;
    void *host = resolved->host;
    uint32_t result;
    if (strcmp(name, "rand") == 0) {
        int (*random_value)(void) = (int (*)(void))host;
        result = (uint32_t)random_value();
    } else if (strcmp(name, "SquareRoot0") == 0) {
        int (*square_root)(int) = (int (*)(int))host;
        result = (uint32_t)square_root((int32_t)cpu->gpr[4]);
    } else if (strcmp(name, "ratan2") == 0) {
        int (*angle)(int, int) = (int (*)(int, int))host;
        result = (uint32_t)angle((int32_t)cpu->gpr[4], (int32_t)cpu->gpr[5]);
    } else if (strcmp(name, "GetTPage") == 0) {
        u_short (*page)(int, int, int, int) = (u_short (*)(int, int, int, int))host;
        result = page((int32_t)cpu->gpr[4], (int32_t)cpu->gpr[5],
                      (int32_t)cpu->gpr[6], (int32_t)cpu->gpr[7]);
    } else if (strcmp(name, "Square0") == 0) {
        VECTOR *(*square)(VECTOR *, VECTOR *) = (VECTOR *(*)(VECTOR *, VECTOR *))host;
        if (graphics_pointer_to_guest(square((VECTOR *)args[0], (VECTOR *)args[1]),
                                      &result) != 0) return -1;
    } else if (strcmp(name, "RotMatrixZYX") == 0) {
        MATRIX *(*rotate)(SVECTOR *, MATRIX *) = (MATRIX *(*)(SVECTOR *, MATRIX *))host;
        if (graphics_pointer_to_guest(rotate((SVECTOR *)args[0], (MATRIX *)args[1]),
                                      &result) != 0) return -1;
    } else if (strcmp(name, "ApplyMatrixLV") == 0) {
        VECTOR *(*apply)(MATRIX *, VECTOR *, VECTOR *) =
            (VECTOR *(*)(MATRIX *, VECTOR *, VECTOR *))host;
        if (graphics_pointer_to_guest(apply((MATRIX *)args[0], (VECTOR *)args[1],
                                            (VECTOR *)args[2]), &result) != 0) return -1;
    } else if (strcmp(name, "SetPolyFT3") == 0) {
        void (*initialize)(POLY_FT3 *) = (void (*)(POLY_FT3 *))host;
        initialize((POLY_FT3 *)args[0]);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
        return 1;
    } else if (strcmp(name, "SetShadeTex") == 0) {
        void (*shade)(void *, int) = (void (*)(void *, int))host;
        shade((void *)args[0], (int32_t)cpu->gpr[5]);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
        return 1;
    } else {
        return 0;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    cpu->gpr[2] = result;
    cpu->gpr[3] = 0;
    return 1;
}

/* LoadImage(RECT* rect, u_long* p): both arguments are guest pointers, and the
 * pixel source may be a *low physical* RAM address -- the battle overlay passes
 * 0x1000 -- which translate_argument leaves alone on purpose because it is
 * shared by scalar arguments.  Translate both explicitly here; otherwise
 * PsyCross's GR_CopyVRAM memmoves from host address 0x1000 and SIGSEGVs
 * (observed live: LoadImage -> GR_CopyVRAM(src=0x1000, w=19088)). */
static int bridge_load_image(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                             void *host_function)
{
    int (*load_image)(RECT16 *, u_long *) = (int (*)(RECT16 *, u_long *))host_function;
    uint32_t address = cpu->gpr[4];
    uint32_t source = cpu->gpr[5];
    void *rect;
    void *data;

    if (address < 0x200000u)
        address |= 0x80000000u;
    rect = resolve_memory(runtime, address, 8u, 0);
    if (rect == NULL)
        return -1;
    if (source < 0x200000u)
        data = g_PsxRam + source;
    else
        data = (void *)translate_argument(runtime, source);
    cpu->gpr[2] = (uint32_t)load_image(rect, data);
    return 1;
}

/* The party-texture path has known pointer/scalar signatures. In particular,
 * its atlas getter takes six writable word pointers, four on the guest stack.
 * Resolve physical RAM pointers as well as KSEG/scratchpad aliases. */
static int bridge_texture_sdk(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                const ResolvedFunction *resolved)
{
    if (strcmp(resolved->name, "OpenTIM") == 0) {
        int (*open_tim)(u_long *) = (int (*)(u_long *))resolved->host;
        uint32_t address = cpu->gpr[4];
        if (address < 0x200000u) address |= 0x80000000u;
        u_long *data = (u_long *)resolve_memory(runtime, address, 8, 0);
        if (data == NULL || (address & 3u)) return -1;
        cpu->gpr[2] = (uint32_t)open_tim(data);
        cpu->gpr[3] = 0;
    } else if (strcmp(resolved->name, "DrawSync") == 0) {
        int (*draw_sync)(int) = (int (*)(int))resolved->host;
        cpu->gpr[2] = (uint32_t)draw_sync((int32_t)cpu->gpr[4]);
        cpu->gpr[3] = 0;
    } else if (resolved->address == 0x80026338u) {
        void (*unpack)(uint8_t *, int32_t, uint32_t *, int32_t *, int32_t *,
                        int32_t *, int32_t *, int32_t *) =
            (void (*)(uint8_t *, int32_t, uint32_t *, int32_t *, int32_t *,
                       int32_t *, int32_t *, int32_t *))resolved->host;
        uint32_t table_address = cpu->gpr[4];
        if (table_address < 0x200000u) table_address |= 0x80000000u;
        uint8_t *table = resolve_memory(runtime, table_address, 1, 0);
        void *outputs[6];
        if (table == NULL) return -1;
        for (unsigned i = 0; i < 6; i++) {
            uint32_t address;
            if (i < 2) address = cpu->gpr[6 + i];
            else if (runtime_read(runtime, cpu->gpr[29] + 0x10u + (i - 2u) * 4u,
                                  4, &address) != 0) return -1;
            if (address < 0x200000u) address |= 0x80000000u;
            outputs[i] = resolve_memory(runtime, address, 4, 1);
            if (outputs[i] == NULL || (address & 3u)) return -1;
        }
        unpack(table, (int32_t)cpu->gpr[5], outputs[0], outputs[1], outputs[2],
               outputs[3], outputs[4], outputs[5]);
        /* The C function is void; leave the guest return registers alone. */
    } else {
        return 0;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

static int bridge_read_tim(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                           void *host_function)
{
    TIM_IMAGE native;
    TIM_IMAGE *(*read_tim)(TIM_IMAGE *) = (TIM_IMAGE *(*)(TIM_IMAGE *))host_function;
    uint32_t words[5];
    unsigned i;

    /* Retail 800475D4/DC and 80047604/0C store four 32-bit pointers at
     * offsets 4/8/12/16. Never let native TIM_IMAGE's 64-bit pointers
     * overwrite the guest's 20-byte stack object or its adjacent locals. */
    memset(&native, 0, sizeof(native));
    if (read_tim(&native) == NULL) {
        cpu->gpr[2] = 0;
        return 1;
    }
    words[0] = native.mode;
    if (graphics_pointer_to_guest(native.cRECT16, &words[1]) != 0 ||
        graphics_pointer_to_guest(native.caddr, &words[2]) != 0 ||
        graphics_pointer_to_guest(native.pRECT16, &words[3]) != 0 ||
        graphics_pointer_to_guest(native.paddr, &words[4]) != 0)
        return -1;
    for (i = 0; i < 5; i++)
        if (runtime_write(runtime, cpu->gpr[4] + i * 4u, 4, words[i]) != 0)
            return -1;
    cpu->gpr[2] = cpu->gpr[4];
    return 1;
}

static int bridge_read_geom_offset(BattleMipsRuntime *runtime,
                                   PcPortMipsCpu *cpu, void *host_function)
{
    long x, y;
    void (*read_offset)(long *, long *) = (void (*)(long *, long *))host_function;
    read_offset(&x, &y);
    /* Retail 8004A0CC/D0 uses SW for each output. Native long is 64 bits. */
    if (runtime_write(runtime, cpu->gpr[4], 4, (uint32_t)x) != 0 ||
        runtime_write(runtime, cpu->gpr[5], 4, (uint32_t)y) != 0)
        return -1;
    return 1;
}

/* These animation leaves use packed game memory. In particular AddPrim must
 * link game addresses, not the low bits of translated host pointers. */
static uint32_t shard_sdk_address(uint32_t address)
{
    return address < 0x200000u ? address | 0x80000000u : address;
}

/* Loader calls with known signatures. Preserve guest pointer return values,
 * and resolve the complete byte range before passing it to host libc. */
static void *module_memory_pointer(BattleMipsRuntime *runtime, uint32_t address,
                                   uint32_t length, int write)
{
    address = shard_sdk_address(address);
    if (((address & 0xff800000u) == 0x80000000u ||
         (address & 0xff800000u) == 0xa0000000u) &&
        /* PSX_ADDR masks to 2 MiB. The allocation also has a guard area,
         * which must not turn a wrapping game range into a linear copy. */
        length > 0x200000u - (address & 0x1fffffu)) return NULL;
    if ((address & 0xfffff000u) == 0x1f800000u &&
        length > 4096u - (address & 0xfffu)) return NULL;
    return resolve_memory(runtime, address, length, write);
}

static int bridge_module_memory(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                 const ResolvedFunction *resolved)
{
    if (resolved->address == 0x80089c08u) {
        uint16_t (*mask)(uint8_t) = (uint16_t (*)(uint8_t))resolved->host;
        cpu->gpr[2] = mask((uint8_t)cpu->gpr[4]);
        cpu->gpr[3] = 0;
    } else if (strcmp(resolved->name, "memmove") == 0 ||
               strcmp(resolved->name, "memcpy") == 0) {
        if(cpu->load_valid && cpu->load_reg!=0)return -1;
        uint32_t length = cpu->gpr[6];
        if (length != 0) {
            void *destination = module_memory_pointer(runtime, cpu->gpr[4], length, 1);
            const void *source = module_memory_pointer(runtime, cpu->gpr[5], length, 0);
            void *(*move)(void *, const void *, size_t) =
                (void *(*)(void *, const void *, size_t))resolved->host;
            if (destination == NULL || source == NULL) return -1;
            move(destination, source, length);
        }
        cpu->gpr[2] = cpu->gpr[4];
        cpu->gpr[3] = 0;
    } else if (strcmp(resolved->name, "bzero") == 0) {
        uint32_t length = cpu->gpr[5];
        if (length != 0) {
            void *destination = module_memory_pointer(runtime, cpu->gpr[4], length, 1);
            void (*zero)(void *, size_t) = (void (*)(void *, size_t))resolved->host;
            if (destination == NULL) return -1;
            zero(destination, length);
        }
        /* bzero is void: no host return register has a guest meaning. */
    } else return 0;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

/* Scoped palette SDK calls: signed memcpy length and explicit packed spans.
 * Complete-span rejection is deliberately stricter than partial CPU faults.
 * The copied bytes follow the original forward order even for overlap. */
static int bridge_palette_update_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                          const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address;
    int copy=target==0x8003f968u&&cpu->gpr[31]==0x800a6b78u;
    int upload=target==0x80044894u&&cpu->gpr[31]==0x800a6f64u;
    if(!copy&&!upload)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    if(copy){
        uint32_t destination=cpu->gpr[4],length=cpu->gpr[6];
        if(destination&&(int32_t)length>0){
            uint8_t *out=module_memory_pointer(runtime,destination,length,1);
            const uint8_t *in=module_memory_pointer(runtime,cpu->gpr[5],length,0);
            if(!out||!in)return -1;
            for(uint32_t i=0;i<length;i++)out[i]=in[i];
        }
        cpu->gpr[2]=destination;cpu->gpr[3]=0;
    }else{
        if((cpu->gpr[4]|cpu->gpr[5])&1u)return -1;
        uint8_t *packed=module_memory_pointer(runtime,cpu->gpr[4],8,0);
        if(!packed)return -1;
        int32_t width=(int16_t)load_le(packed+4,2),height=(int16_t)load_le(packed+6,2);
        if(width<=0||height<=0)return -1;
        uint32_t bytes=(uint32_t)width*(uint32_t)height*2u;
        void *pixels=module_memory_pointer(runtime,cpu->gpr[5],bytes,0);
        if(!pixels)return -1;
        int (*upload_image)(RECT16 *,u_long *)=(int (*)(RECT16 *,u_long *))resolved->host;
        cpu->gpr[2]=(uint32_t)upload_image((RECT16 *)packed,pixels);
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Effect-model and anchor callers compose three packed 32-byte matrices.
 * Keep the stricter span/pending contract scoped to these adopted sites. */
static int bridge_effect_model_compose(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                        const ResolvedFunction *resolved)
{
    if(resolved->address!=0x8004931cu||
       (cpu->gpr[31]!=0x800a497cu&&cpu->gpr[31]!=0x800a456cu))return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    MATRIX *matrices[3];
    for(unsigned i=0;i<3;i++){
        uint32_t token=cpu->gpr[4u+i];
        if(token&3u)return -1;
        matrices[i]=module_memory_pointer(runtime,token,32,i==2);
        if(!matrices[i])return -1;
    }
    MATRIX *out=((MATRIX *(*)(MATRIX *,MATRIX *,MATRIX *))resolved->host)(matrices[0],matrices[1],matrices[2]);
    runtime_note_store(runtime,(const uint8_t *)matrices[2],32);
    if(out!=matrices[2])return -1;
    cpu->gpr[2]=cpu->gpr[6];cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Explicit void service boundaries. Work-list and graphics allocation use
 * native pointer-width globals; translating their internals into four-byte
 * guest stores would change that storage contract. Sound's packed WDS header
 * is resolved as a pointer, never as an integer handle. These bindings prove
 * the ABI, not complete original SDK/audio body or volatile-register parity. */
static int bridge_post_resource_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                         const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address;
    if(target!=0x8001c944u&&target!=0x80024f64u&&target!=0x80038310u&&target!=0x80044534u)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    if(target==0x8001c944u)((void (*)(void))resolved->host)();
    else if(target==0x80024f64u)((void (*)(int,unsigned int))resolved->host)((int32_t)cpu->gpr[4],cpu->gpr[5]);
    else if(target==0x80044534u)((void (*)(int))resolved->host)((int32_t)cpu->gpr[4]);
    else {
        uint32_t token=cpu->gpr[4];
        SoundWDSEntry *sound=token?module_memory_pointer(runtime,token,sizeof(SoundWDSEntry),1):NULL;
        if(token&&(!sound||(token&3u)))return -1;
        ((void (*)(SoundWDSEntry *))resolved->host)(sound);
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Timer records are packed, but their list links contain low-32 native
 * pointers. Keep the allocator's actual pointer token, matching the native
 * list head and consumers. The original callback identity stays a raw word
 * through the separately adopted eight-byte setter. */
struct WorkListEntry;
static int bridge_timer_task_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                      const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address;
    if(target!=0x8001cd08u&&target!=0x800286ccu)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    if(target==0x800286ccu)cpu->gpr[2]=(uint32_t)((int (*)(void))resolved->host)();
    else {
        uint32_t token=cpu->gpr[4];
        void *owner=token?module_memory_pointer(runtime,token,0x1cu,0):NULL;
        if(token&&(!owner||(token&3u)))return -1;
        struct WorkListEntry *(*allocate)(void *,int32_t)=
            (struct WorkListEntry *(*)(void *,int32_t))resolved->host;
        uintptr_t result=(uintptr_t)allocate(owner,(int32_t)cpu->gpr[5]);
        if(result>UINT32_MAX)return -1;
        cpu->gpr[2]=(uint32_t)result;
    }
    cpu->gpr[3]=0;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Light and color matrices are void services reading five packed words. */
static int bridge_effects_matrix(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                       const ResolvedFunction *resolved)
{
    if(resolved->address!=0x80049f2cu&&resolved->address!=0x80049f5cu)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    uint32_t token=cpu->gpr[4];MATRIX *matrix=module_memory_pointer(runtime,token,20,0);
    if((token&3u)||!matrix)return -1;
    ((void (*)(MATRIX *))resolved->host)(matrix);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Actor rendering supplies three RGB bytes. Preserve the SDK's concrete void
 * signature without reading the legacy thunk's unused stack arguments. */
static int bridge_actor_back_color(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                   const ResolvedFunction *resolved)
{
    (void)runtime;
    if(resolved->address!=0x8004a0ecu)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    ((void (*)(int,int,int))resolved->host)((int32_t)cpu->gpr[4],
        (int32_t)cpu->gpr[5],(int32_t)cpu->gpr[6]);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Camera-update math keeps scalars raw and vectors packed. Square0 touches
 * three words, preserves the pad and returns the caller's output token. */
static int bridge_camera_update_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                          const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address,a=cpu->gpr[4],b=cpu->gpr[5],result;
    if(target!=0x8003f8b0u&&target!=0x8003f8ccu&&target!=0x8004a414u&&target!=0x8004b32cu)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    if(target==0x8004a414u){
        VECTOR *input=module_memory_pointer(runtime,a,12,0),*output=module_memory_pointer(runtime,b,12,1);
        if(((a|b)&3u)||!input||!output)return -1;
        VECTOR *out=((VECTOR *(*)(VECTOR *,VECTOR *))resolved->host)(input,output);
        runtime_note_store(runtime,(const uint8_t *)output,12);
        if(out!=output)return -1;
        result=b;
    }else if(target==0x8004b32cu){
        result=(uint32_t)((int (*)(int,int))resolved->host)((int32_t)a,(int32_t)b);
    }else{
        /* initialize_runtime binds the two addresses to the correct sine
         * or cosine export despite the decomp's reversed symbol names. */
        result=(uint32_t)((int (*)(int))resolved->host)((int)(a&0xfffu));
    }
    cpu->gpr[2]=result;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Camera services retain concrete native signatures and packed guest outputs.
 * Validate complete input/output spans before entering the graphics backend;
 * the original SDK's volatile CPU state and hardware faults are not an ABI. */
static int bridge_target_camera_graphics(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                          const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address,a=cpu->gpr[4],b=cpu->gpr[5],c=cpu->gpr[6],d=cpu->gpr[7];
    switch(target){
    case 0x8004abbcu:case 0x80049cecu:case 0x8004a0dcu:case 0x80049efcu:case 0x80049f8cu:
    case 0x8004a64cu:case 0x80048c4cu:case 0x80048d7cu:case 0x8004a480u:case 0x8004960cu:case 0x800496acu:case 0x8003fa08u:break;
    default:return 0;
    }
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    uint32_t result=0;int returns_value=0;
    if(target==0x8003fa08u){
        /* The original memset returns zero for a null destination or a
         * nonpositive signed length. Positive writes use the complete span
         * and return the caller's raw guest token, never a host pointer. */
        if(a&&(int32_t)c>0){
            uint8_t *destination=module_memory_pointer(runtime,a,c,1);
            if(!destination)return -1;
            void *out=((void *(*)(void *,int,size_t))resolved->host)(destination,(int32_t)b,c);
            runtime_note_store(runtime,destination,c);
            if(out!=destination)return -1;
            result=a;
        }
        returns_value=1;
    }else if(target==0x8004abbcu){
        SVECTOR *angles=module_memory_pointer(runtime,a,6,0);MATRIX *matrix=module_memory_pointer(runtime,b,sizeof(MATRIX),1);
        if((a&1u)||(b&3u)||!angles||!matrix)return -1;
        MATRIX *out=((MATRIX *(*)(SVECTOR *,MATRIX *))resolved->host)(angles,matrix);
        runtime_note_store(runtime,(const uint8_t *)matrix,18);
        if(out!=matrix)return -1;
        result=b;returns_value=1;
    }else if(target==0x80049cecu){
        MATRIX *matrix=module_memory_pointer(runtime,a,20,0);SVECTOR *vector=module_memory_pointer(runtime,b,8,0);
        VECTOR *output=module_memory_pointer(runtime,c,12,1);
        if(((a|b|c)&3u)||!matrix||!vector||!output)return -1;
        VECTOR *out=((VECTOR *(*)(MATRIX *,SVECTOR *,VECTOR *))resolved->host)(matrix,vector,output);
        runtime_note_store(runtime,(const uint8_t *)output,12);
        if(out!=output)return -1;
        result=c;returns_value=1;
    }else if(target==0x8004a0dcu){
        result=(uint32_t)((int32_t (*)(void))resolved->host)();returns_value=1;
    }else if(target==0x80049efcu||target==0x80049f8cu){
        MATRIX *matrix=module_memory_pointer(runtime,a,sizeof(MATRIX),0);
        if((a&3u)||!matrix)return -1;
        ((void (*)(MATRIX *))resolved->host)(matrix);
    }else if(target==0x8004a64cu){
        SVECTOR *vector=module_memory_pointer(runtime,a,8,0);
        uint8_t *screen=module_memory_pointer(runtime,b,4,1),*depth=module_memory_pointer(runtime,c,4,1),*flag=module_memory_pointer(runtime,d,4,1);
        if(((a|b|c|d)&3u)||!vector||!screen||!depth||!flag)return -1;
        int xy=0;long native_depth=0,native_flag=0;
        result=(uint32_t)((int (*)(SVECTOR *,int *,long *,long *))resolved->host)(vector,&xy,&native_depth,&native_flag);
        /* Host long may be eight bytes. Every original output is four. */
        store_le(screen,4,(uint32_t)xy);runtime_note_store(runtime,screen,4);
        store_le(depth,4,(uint32_t)native_depth);runtime_note_store(runtime,depth,4);
        store_le(flag,4,(uint32_t)native_flag);runtime_note_store(runtime,flag,4);returns_value=1;
    }else if(target==0x80048c4cu){
        result=(uint32_t)((int (*)(int))resolved->host)((int32_t)a);returns_value=1;
    }else if(target==0x80048d7cu){
        VECTOR *input=module_memory_pointer(runtime,a,12,0),*output=module_memory_pointer(runtime,b,12,1);
        if(((a|b)&3u)||!input||!output)return -1;
        result=(uint32_t)((long (*)(VECTOR *,VECTOR *))resolved->host)(input,output);
        runtime_note_store(runtime,(const uint8_t *)output,12);returns_value=1;
    }else if(target==0x8004a480u){
        VECTOR *left=module_memory_pointer(runtime,a,12,0),*right=module_memory_pointer(runtime,b,12,0),*output=module_memory_pointer(runtime,c,12,1);
        if(((a|b|c)&3u)||!left||!right||!output)return -1;
        ((void (*)(VECTOR *,VECTOR *,VECTOR *))resolved->host)(left,right,output);
        runtime_note_store(runtime,(const uint8_t *)output,12);
    }else ((void (*)(void))resolved->host)();
    if(returns_value)cpu->gpr[2]=result;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* DrawSync has one signed scalar argument. Resolve it before legacy stack
 * marshalling so a valid call never reads eight unrelated argument words. */
static int bridge_draw_sync_service(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                     const ResolvedFunction *resolved)
{
    (void)runtime;
    if(resolved->address!=0x800445d0u)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    cpu->gpr[2]=(uint32_t)((int (*)(int))resolved->host)((int32_t)cpu->gpr[4]);
    cpu->gpr[3]=0;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* StoreImage reads the CPU VRAM mirror into packed 16-bit pixel storage.
 * Its backend does not clip or validate dimensions; refuse unsupported spans
 * before calling it, and preserve registers outside its int return value. */
static int bridge_vram_readback(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                const ResolvedFunction *resolved)
{
    if(resolved->address!=0x800448f8u)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    uint32_t address=cpu->gpr[4],destination=cpu->gpr[5];
    RECT16 *rectangle=module_memory_pointer(runtime,address,sizeof(RECT16),0);
    if(!rectangle||(address&1u)||(destination&1u))return -1;
    int x=rectangle->x,y=rectangle->y,w=rectangle->w,h=rectangle->h;
    if(x<0||y<0||w<0||h<0||x>1024||y>512||w>1024-x||h>512-y)return -1;
    uint32_t bytes=(uint32_t)w*(uint32_t)h*2u;
    u_long *pixels=module_memory_pointer(runtime,destination,bytes?bytes:1u,1);
    if(!pixels)return -1;
    /* Empty reads have no pixel access; avoid the backend forming an
     * out-of-range source pointer for an empty edge rectangle. */
    int result=bytes?((int (*)(RECT16 *,u_long *))resolved->host)(rectangle,pixels):0;
    if(bytes)runtime_note_store(runtime,(const uint8_t *)pixels,bytes);
    cpu->gpr[2]=(uint32_t)result;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
/* Paired fade records remain packed guest storage. The native list owner
 * takes real record pointers and retains its pointer-width list heads. */
static int bridge_color_fade_lists(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                   const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address;
    int add=target==0x8001cc18u||target==0x8001ca58u;
    if(!add&&target!=0x8001cb48u&&target!=0x8001cd94u)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    uint32_t a=cpu->gpr[4],b=cpu->gpr[5];
    if(!add&&!a)return -1;
    struct WorkListEntry *first=a?module_memory_pointer(runtime,a,0x1cu,!add):NULL;
    if(a&&(!first||(a&3u)))return -1;
    if(add) {
        struct WorkListEntry *record=module_memory_pointer(runtime,b,0x1cu,1);
        if(!b||!record||(b&3u))return -1;
        ((void (*)(struct WorkListEntry *,struct WorkListEntry *))resolved->host)(first,record);
    } else ((void (*)(struct WorkListEntry *))resolved->host)(first);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int32_t shard_q12_product(int32_t a, int32_t b)
{
    return (int32_t)((uint32_t)a * (uint32_t)b) >> 12;
}

/* Heap boundaries keep allocation sizes/flags as unsigned words. Only the
 * optional content-name table is a pointer; its pointed-to layout remains the
 * native heap service's responsibility. Preserve void returns and normalize
 * native RAM pointers before a caller stores them in a guest word. */
static int bridge_heap_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                const ResolvedFunction *resolved)
{
    if(resolved->address!=0x80032498u && resolved->address!=0x80031bdcu &&
       resolved->address!=0x800320b8u && resolved->address!=0x800324b8u) return 0;
    if(cpu->load_valid && cpu->load_reg!=0) return -1;
    if(resolved->address==0x800324b8u) {
        ((void (*)(uint16_t))resolved->host)((uint16_t)cpu->gpr[4]);
    } else if(resolved->address==0x800320b8u) {
        /* HeapUnpinBlock clears the pin bit in the preceding eight-byte
         * packed header. Use the canonical bus for physical/KSEG aliases
         * and host allocations; do not treat this void service as a query. */
        uint32_t address=shard_sdk_address(cpu->gpr[4])-4u,value;
        cpu->gpr[3]=0xfdffffffu;
        if((address&3u) || runtime_read(runtime,address,4,&value)!=0) return -1;
        cpu->gpr[2]=value&cpu->gpr[3];
        if(runtime_write(runtime,address,4,cpu->gpr[2])!=0) return -1;
    } else if(resolved->address==0x80032498u) {
        char **names=cpu->gpr[5] ? module_memory_pointer(runtime,cpu->gpr[5],1,0) : NULL;
        if(cpu->gpr[5] && names==NULL) return -1;
        ((void (*)(uint32_t,char **))resolved->host)(cpu->gpr[4],names);
    } else {
        void *p=((void *(*)(uint32_t,uint32_t))resolved->host)(cpu->gpr[4],cpu->gpr[5]);
        uint32_t result;
        if(graphics_pointer_to_guest(p,&result)!=0) return -1;
        cpu->gpr[2]=result;cpu->gpr[3]=0;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Stage-three UI calls have concrete pointer/scalar signatures. GetClut's
 * packed result uses unsigned shifts so negative/wide coordinates have the
 * original low-16-bit behavior without the SDK's signed-left-shift UB. */
/* Resource construction includes thirteen arguments, beyond the legacy
 * twelve-word thunk. Pointer slots and narrow scalars have explicit types;
 * these bindings do not establish the external bodies' semantic coverage. */
static int bridge_resource_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                    const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address,result=0;
    if(target!=0x8002709cu && target!=0x80027d64u && target!=0x8003342cu && target!=0x800aa898u)return 0;
    if(cpu->load_valid && cpu->load_reg!=0)return -1;
    if(target==0x8003342cu) {
        uint32_t *table=module_memory_pointer(runtime,cpu->gpr[4],4,1);
        if(!table || (cpu->gpr[4]&3u))return -1;
        result=((unsigned int (*)(uint32_t *))resolved->host)(table);
    } else if(target==0x800aa898u) {
        uint8_t *actor=module_memory_pointer(runtime,cpu->gpr[4],0x9a,1);
        if(!actor || (cpu->gpr[4]&3u))return -1;
        result=((uint32_t (*)(uint8_t *,uint32_t,uint32_t,uint32_t))resolved->host)
            (actor,cpu->gpr[5],cpu->gpr[6],cpu->gpr[7]);
    } else {
        uint32_t a[13];unsigned argc=target==0x8002709cu?13:9;
        for(unsigned i=0;i<argc;i++) {
            if(i<4)a[i]=cpu->gpr[4+i];
            else if(runtime_read(runtime,cpu->gpr[29]+i*4u,4,&a[i])!=0)return -1;
        }
        void *value;
        if(target==0x8002709cu) {
            int16_t *coords=module_memory_pointer(runtime,a[8],12,0);
            uint8_t *colors=a[9]?module_memory_pointer(runtime,a[9],11,0):NULL;
            if(!coords || (a[8]&3u) || (a[9]&&!colors))return -1;
            void *(*initialize)(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t,
                int32_t,int32_t,int16_t *,uint8_t *,int32_t,int32_t,int32_t)=
                (void *(*)(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t,
                int32_t,int32_t,int16_t *,uint8_t *,int32_t,int32_t,int32_t))resolved->host;
            value=initialize((int32_t)a[0],(int32_t)a[1],(int32_t)a[2],(int32_t)a[3],
                (int32_t)a[4],(int32_t)a[5],(int32_t)a[6],(int32_t)a[7],coords,colors,
                (int32_t)a[10],(int32_t)a[11],(int32_t)a[12]);
        } else {
            LineScroll *line=module_memory_pointer(runtime,a[0],sizeof(LineScroll),1);
            int8_t *data=a[8]?module_memory_pointer(runtime,a[8],1,0):NULL;
            if(!line || (a[0]&3u) || (a[8]&&!data))return -1;
            LineScroll *(*initialize)(LineScroll *,uint16_t,uint16_t,uint16_t,
                int16_t,int16_t,uint16_t,uint16_t,int8_t *)=
                (LineScroll *(*)(LineScroll *,uint16_t,uint16_t,uint16_t,
                int16_t,int16_t,uint16_t,uint16_t,int8_t *))resolved->host;
            value=initialize(line,(uint16_t)a[1],(uint16_t)a[2],(uint16_t)a[3],
                (int16_t)a[4],(int16_t)a[5],(uint16_t)a[6],(uint16_t)a[7],data);
        }
        if(graphics_pointer_to_guest(value,&result)!=0)return -1;
    }
    cpu->gpr[2]=result;cpu->gpr[3]=0;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* The adopted motion updater consumes five raw words and returns void.
 * Its concrete C type must precede the legacy eight-word returning thunk.
 * This binding preserves the existing adoption decision and return contract;
 * it does not establish all motion-script paths or nested service semantics. */
static int bridge_motion_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                   const ResolvedFunction *resolved)
{
    if(resolved->address!=0x800aad54u)return 0;
    uint32_t fifth;
    if((cpu->load_valid&&cpu->load_reg!=0)||(cpu->gpr[29]&3u)||
       runtime_read(runtime,cpu->gpr[29]+0x10u,4,&fifth)!=0)return -1;
    void (*update)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t)=
        (void (*)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t))resolved->host;
    update(cpu->gpr[4],cpu->gpr[5],cpu->gpr[6],cpu->gpr[7],fifth);
    cpu->gpr[3]=0;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Actor-construction services retain their concrete scalar/pointer types.
 * HeapGetNextBlockHeader's historical pointer return encodes a byte count;
 * that result must not undergo pointer normalization. Full original callee
 * policy, including model pinning/relocation, remains separate proof work. */
static int bridge_actor_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                 const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address,result=0,a=cpu->gpr[4];
    if(target!=0x8002c4bcu && target!=0x8002c644u && target!=0x8002dde4u && target!=0x80030988u &&
       target!=0x80031894u && target!=0x80038428u && target!=0x8003864cu && target!=0x80043a1cu)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    if(target==0x80030988u){
        ((void (*)(uint16_t,uint16_t,int16_t,int16_t))resolved->host)
            ((uint16_t)a,(uint16_t)cpu->gpr[5],(int16_t)cpu->gpr[6],(int16_t)cpu->gpr[7]);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
    }
    if(target==0x80038428u){
        SoundFile *sound=module_memory_pointer(runtime,a,sizeof(SoundFile),1);
        if(!sound||(a&3u))return -1;
        ((void (*)(SoundFile *))resolved->host)(sound);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
    }
    if(target==0x8002dde4u){
        uint32_t *image=module_memory_pointer(runtime,a,4,0),stack[3];
        if(!image||(a&3u))return -1;
        for(unsigned i=0;i<3;i++){
            uint32_t address=cpu->gpr[29]+0x10u+i*4u;
            if((address&3u)||runtime_read(runtime,address,4,&stack[i])!=0)return -1;
        }
        result=(uint32_t)((int32_t (*)(uint32_t *,int32_t,int32_t,int32_t,int32_t,uint16_t,uint16_t))resolved->host)
            (image,(int32_t)cpu->gpr[5],(int32_t)cpu->gpr[6],(int32_t)cpu->gpr[7],
             (int32_t)stack[0],(uint16_t)stack[1],(uint16_t)stack[2]);
    }else if(target==0x8003864cu){
        void *sound=a?module_memory_pointer(runtime,a,0x16u,0):NULL;
        if(a&&(!sound||(a&1u)))return -1;
        void *value=((void *(*)(void *,int16_t))resolved->host)(sound,(int16_t)cpu->gpr[5]);
        if(graphics_pointer_to_guest(value,&result)!=0)return -1;
    }else if(target==0x80031894u){
        HeapBlock *block=module_memory_pointer(runtime,a,1,0);
        void *header=module_memory_pointer(runtime,shard_sdk_address(a)-8u,8,0);
        if(!block||!header||(a&3u)||(uint8_t *)header+8u!=(uint8_t *)block)return -1;
        result=(uint32_t)(uintptr_t)((void *(*)(HeapBlock *))resolved->host)(block);
        cpu->gpr[2]=result;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
    }else if(target==0x80043a1cu){
        result=((uint16_t (*)(int,int,int,int))resolved->host)
            ((int32_t)a,(int32_t)cpu->gpr[5],(int32_t)cpu->gpr[6],(int32_t)cpu->gpr[7]);
    }else{
        uint8_t *model=module_memory_pointer(runtime,a,0x28u,1);
        if(!model||(a&3u))return -1;
        result=(uint32_t)((int32_t (*)(uint8_t *))resolved->host)(model);
    }
    cpu->gpr[2]=result;cpu->gpr[3]=0;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#include "battle_window_binding_host.inc"

static int bridge_stage3_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                   const ResolvedFunction *resolved)
{
    const char *name=resolved->name;
    void *host=resolved->host;
    uint32_t result=0,address=cpu->gpr[4];
    if (strcmp(name,"GetClut")==0) {
        result=((cpu->gpr[5]<<6)|((cpu->gpr[4]>>4)&0x3fu))&0xffffu;
    } else if (strcmp(name,"SetPolyGT4")==0 || strcmp(name,"SetPolyFT4")==0 || strcmp(name,"SetPolyG4")==0 ||
               strcmp(name,"SetPolyF4")==0 || strcmp(name,"SetLineF2")==0) {
        unsigned size=strcmp(name,"SetPolyGT4")==0 ? sizeof(POLY_GT4) :
                      strcmp(name,"SetPolyFT4")==0 ? sizeof(POLY_FT4) :
                      strcmp(name,"SetPolyG4")==0 ? sizeof(POLY_G4) :
                      strcmp(name,"SetPolyF4")==0 ? sizeof(POLY_F4) : sizeof(LINE_F2);
        void *p=module_memory_pointer(runtime,address,size,1);
        if (p==NULL || (address&3u)) return -1;
        if (strcmp(name,"SetPolyGT4")==0) ((void (*)(POLY_GT4 *))host)(p);
        else if (strcmp(name,"SetPolyFT4")==0) ((void (*)(POLY_FT4 *))host)(p);
        else if (strcmp(name,"SetPolyG4")==0) ((void (*)(POLY_G4 *))host)(p);
        else if (strcmp(name,"SetPolyF4")==0) ((void (*)(POLY_F4 *))host)(p);
        else ((void (*)(LINE_F2 *))host)(p);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
    } else if (strcmp(name,"SetSemiTrans")==0 || strcmp(name,"SetShadeTex")==0) {
        void *p=module_memory_pointer(runtime,address,8,1);
        if (p==NULL) return -1;
        ((void (*)(void *,int))host)(p,(int32_t)cpu->gpr[5]);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
    } else if (strcmp(name,"SetDrawMode")==0) {
        uint32_t rectangle;
        if (runtime_read(runtime,cpu->gpr[29]+0x10u,4,&rectangle)!=0) return -1;
        DR_MODE *p=module_memory_pointer(runtime,address,sizeof(DR_MODE),1);
        RECT16 *r=rectangle ? module_memory_pointer(runtime,rectangle,sizeof(RECT16),0) : NULL;
        if (p==NULL || (address&3u) || (rectangle && (r==NULL || (rectangle&1u)))) return -1;
        ((void (*)(DR_MODE *,int,int,int,RECT16 *))host)(p,(int32_t)cpu->gpr[5],
             (int32_t)cpu->gpr[6],(int32_t)cpu->gpr[7],r);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
    } else if (resolved->address==0x80076a10u || resolved->address==0x80076a6cu) {
        uint8_t *p=module_memory_pointer(runtime,cpu->gpr[5],1,1);
        if (p==NULL || (cpu->gpr[5]&3u)) return -1;
        int32_t (*glyph)(int32_t,uint8_t *,int16_t,int16_t)=
            (int32_t (*)(int32_t,uint8_t *,int16_t,int16_t))host;
        result=(uint32_t)glyph((int32_t)cpu->gpr[4],p,(int16_t)cpu->gpr[6],(int16_t)cpu->gpr[7]);
    } else if (resolved->address==0x80076c34u) {
        uint8_t *p=module_memory_pointer(runtime,address,40,1);
        if (p==NULL || (address&3u)) return -1;
        ((void (*)(uint8_t *))host)(p);
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
    } else if (resolved->address==0x8008ac00u || resolved->address==0x800338d8u) {
        void *p=((void *(*)(int32_t))host)((int32_t)cpu->gpr[4]);
        if (graphics_pointer_to_guest(p,&result)!=0) return -1;
    } else if (resolved->address==0x80034eacu) {
        void *text=module_memory_pointer(runtime,address,1,0);
        void *buffer=module_memory_pointer(runtime,cpu->gpr[5],1,1);
        if (text==NULL || buffer==NULL) return -1;
        int32_t (*render)(void *,void *,int32_t,int32_t)=
            (int32_t (*)(void *,void *,int32_t,int32_t))host;
        result=(uint32_t)render(text,buffer,(int32_t)cpu->gpr[6],(int32_t)cpu->gpr[7]);
    } else return 0;
    cpu->gpr[2]=result; cpu->gpr[3]=0;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}

static int bridge_shard_animation_sdk(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                       const ResolvedFunction *resolved)
{
    uint32_t a = shard_sdk_address(cpu->gpr[4]);
    uint32_t b = shard_sdk_address(cpu->gpr[5]);
    if (resolved->address == 0x8003f738u) {
        uint32_t angles[3], trig[3];
        if ((a & 3u) || (b & 1u)) return -1;
        for (unsigned i = 0; i < 3; i++)
            if (runtime_read(runtime, a + i * 2u, 2, &angles[i]) != 0) return -1;
        for (unsigned i = 0; i < 3; i++)
            if (runtime_read(runtime, 0x800523f0u + (angles[i] & 0xfffu) * 4u,
                             4, &trig[i]) != 0) return -1;
        int32_t sx = (int16_t)trig[0], cx = (int16_t)(trig[0] >> 16);
        int32_t sy = (int16_t)trig[1], cy = (int16_t)(trig[1] >> 16);
        int32_t sz = (int16_t)trig[2], cz = (int16_t)(trig[2] >> 16);
        int32_t t0 = shard_q12_product(cz, -sy), t1 = shard_q12_product(sz, -sy);
        /* Original 8003F738 rounds each Q12 product separately. PsyCross's
         * trig-identity implementation changes some matrix halfwords. Read
         * the owner's loaded table and keep the original stage boundaries. */
        const unsigned offsets[] = {0, 6, 12, 2, 8, 14, 4, 10, 16};
        const uint32_t values[] = {
            (uint32_t)shard_q12_product(cz, cy),
            (uint32_t)shard_q12_product(sz, cx) - (uint32_t)shard_q12_product(t0, sx),
            (uint32_t)shard_q12_product(t0, cx) + (uint32_t)shard_q12_product(sz, sx),
            (uint32_t)shard_q12_product(-sz, cy),
            (uint32_t)shard_q12_product(cz, cx) + (uint32_t)shard_q12_product(t1, sx),
            (uint32_t)shard_q12_product(cz, sx) - (uint32_t)shard_q12_product(t1, cx),
            (uint32_t)sy, (uint32_t)shard_q12_product(-cy, sx),
            (uint32_t)shard_q12_product(cy, cx)
        };
        for (unsigned i = 0; i < 9; i++)
            if (runtime_write(runtime, b + offsets[i], 2, values[i]) != 0) return -1;
        int64_t last_product = (int64_t)cy * cx;
        cpu->lo = (uint32_t)last_product;
        cpu->hi = (uint32_t)((uint64_t)last_product >> 32);
        cpu->gpr[2] = cpu->gpr[5];
    } else if (resolved->address == 0x80043b48u) {
        uint32_t packet, ot;
        if ((a & 3u) || (b & 3u) || runtime_read(runtime, b, 4, &packet) != 0 ||
            runtime_read(runtime, a, 4, &ot) != 0) return -1;
        packet = (packet & 0xff000000u) | (ot & 0x00ffffffu);
        if (runtime_write(runtime, b, 4, packet) != 0 ||
            runtime_read(runtime, a, 4, &ot) != 0) return -1;
        ot = (ot & 0xff000000u) | (cpu->gpr[5] & 0x00ffffffu);
        if (runtime_write(runtime, a, 4, ot) != 0) return -1;
        cpu->gpr[2] = ot;
        cpu->gpr[3] = packet;
    } else if (resolved->address == 0x80021ad8u) {
        int32_t (*fade)(int32_t, int32_t) = (int32_t (*)(int32_t, int32_t))resolved->host;
        cpu->gpr[2] = (uint32_t)fade((int32_t)cpu->gpr[4], (int32_t)cpu->gpr[5]);
        cpu->gpr[3] = 0;
    } else if (strcmp(resolved->name, "HeapFree") == 0) {
        unsigned int (*release)(void *) = (unsigned int (*)(void *))resolved->host;
        void *pointer = cpu->gpr[4] == 0 ? NULL : resolve_memory(runtime, a, 1, 1);
        if (cpu->gpr[4] != 0 && !pointer) return -1;
        cpu->gpr[2] = release(pointer);
        cpu->gpr[3] = 0;
    } else if (strcmp(resolved->name, "ReadGeomScreen") == 0) {
        int32_t (*read_screen)(void) = (int32_t (*)(void))resolved->host;
        cpu->gpr[2] = (uint32_t)read_screen();
        cpu->gpr[3] = 0;
    } else if (strcmp(resolved->name, "SetGeomOffset") == 0) {
        /* The original shifts raw bits. Native SDK signed left shift is
         * undefined for negative offsets; preserve the exact register bits. */
        CTC2(cpu->gpr[4] << 16, 24);
        CTC2(cpu->gpr[5] << 16, 25);
    } else if (strcmp(resolved->name, "SetGeomScreen") == 0) {
        void (*set_screen)(int) = (void (*)(int))resolved->host;
        set_screen((int32_t)cpu->gpr[4]);
    } else if (strcmp(resolved->name, "SetBackColor") == 0) {
        void (*set_color)(int,int,int)=(void (*)(int,int,int))resolved->host;
        set_color((int32_t)cpu->gpr[4],(int32_t)cpu->gpr[5],(int32_t)cpu->gpr[6]);
    } else if (strcmp(resolved->name, "TransMatrix") == 0 ||
               strcmp(resolved->name, "SetColorMatrix") == 0 ||
               strcmp(resolved->name, "SetRotMatrix") == 0 ||
               strcmp(resolved->name, "SetTransMatrix") == 0) {
        MATRIX *matrix = (MATRIX *)resolve_memory(runtime, a, sizeof(MATRIX), 1);
        if ((a & 3u) || !matrix) return -1;
        if (strcmp(resolved->name, "TransMatrix") == 0) {
            VECTOR *vector = (VECTOR *)resolve_memory(runtime, b, 12, 0);
            MATRIX *(*translate)(MATRIX *, VECTOR *) = (MATRIX *(*)(MATRIX *, VECTOR *))resolved->host;
            if ((b & 3u) || !vector) return -1;
            translate(matrix, vector);
            cpu->gpr[2] = cpu->gpr[4];
            cpu->gpr[3] = 0;
        } else {
            void (*set_matrix)(MATRIX *) = (void (*)(MATRIX *))resolved->host;
            set_matrix(matrix);
        }
    } else return 0;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

static int bridge_rot_trans_pers3(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                  void *host_function, const uintptr_t *args)
{
    int (*project)(SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *) =
        (int (*)(SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *))host_function;
    long native[5] = {0};
    uint32_t output[5];
    int result;
    unsigned i;

    output[0] = cpu->gpr[7];
    for (i = 1; i < 5; i++)
        if (runtime_read(runtime, cpu->gpr[29] + 12u + i * 4u, 4, &output[i]) != 0)
            return -1;
    /* The native GTE wrapper sign-extends FLAG into a host long. Retail
     * stores only 32 bits; a direct call overwrites the adjacent guest local
     * (observed at battle 800B7284: FLAG at sp+4C, saved pointer at sp+50). */
    result = project((SVECTOR *)args[0], (SVECTOR *)args[1], (SVECTOR *)args[2],
                     &native[0], &native[1], &native[2], &native[3], &native[4]);
    for (i = 0; i < 5; i++)
        if (runtime_write(runtime, output[i], 4, (uint32_t)native[i]) != 0)
            return -1;
    cpu->gpr[2] = (uint32_t)result;
    return 1;
}

static int bridge_rot_trans_pers(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                 void *host_function, const uintptr_t *args)
{
    int (*project)(SVECTOR *, int *, long *, long *) =
        (int (*)(SVECTOR *, int *, long *, long *))host_function;
    int xy = 0;
    long p = 0, flag = 0;
    int result = project((SVECTOR *)args[0], &xy, &p, &flag);
    /* Same SDK boundary as RTP3: retail SWC2/SW outputs are four bytes. */
    if (runtime_write(runtime, cpu->gpr[5], 4, (uint32_t)xy) != 0 ||
        runtime_write(runtime, cpu->gpr[6], 4, (uint32_t)p) != 0 ||
        runtime_write(runtime, cpu->gpr[7], 4, (uint32_t)flag) != 0)
        return -1;
    cpu->gpr[2] = (uint32_t)result;
    return 1;
}

static int bridge_rot_trans_pers4(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                  void *host_function, const uintptr_t *args)
{
    int (*project)(SVECTOR *, SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *, long *) =
        (int (*)(SVECTOR *, SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *, long *))host_function;
    long native[6] = {0};
    uint32_t output[6];
    int result;
    unsigned i;
    for (i = 0; i < 6; i++)
        if (runtime_read(runtime, cpu->gpr[29] + 16u + i * 4u, 4, &output[i]) != 0)
            return -1;
    result = project((SVECTOR *)args[0], (SVECTOR *)args[1], (SVECTOR *)args[2], (SVECTOR *)args[3],
                     &native[0], &native[1], &native[2], &native[3], &native[4], &native[5]);
    for (i = 0; i < 6; i++)
        if (runtime_write(runtime, output[i], 4, (uint32_t)native[i]) != 0)
            return -1;
    cpu->gpr[2] = (uint32_t)result;
    return 1;
}

static int bridge_rot_average4(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                               void *host_function, const uintptr_t *args)
{
    long (*project)(SVECTOR *, SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *, long *) =
        (long (*)(SVECTOR *, SVECTOR *, SVECTOR *, SVECTOR *, long *, long *, long *, long *, long *, long *))host_function;
    long native[6] = {0};
    uint32_t output[6];
    unsigned i;
    for (i = 0; i < 6; i++)
        if (runtime_read(runtime, cpu->gpr[29] + 16u + i * 4u, 4, &output[i]) != 0)
            return -1;
    long result = project((SVECTOR *)args[0], (SVECTOR *)args[1],
                          (SVECTOR *)args[2], (SVECTOR *)args[3], &native[0],
                          &native[1], &native[2], &native[3], &native[4], &native[5]);
    /* Retail writes XY0..3, then IR0, then FLAG, each as one word. Keep this
     * order for aliased p/FLAG without overwriting adjacent guest locals. */
    for (i = 0; i < 6; i++)
        if (runtime_write(runtime, output[i], 4, (uint32_t)native[i]) != 0)
            return -1;
    cpu->gpr[2] = (uint32_t)result;
    return 1;
}

static int bridge_rot_average_nclip4(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    uint32_t output[7], value, flags, opz;
    for (unsigned i = 0; i < 7; i++)
        if (runtime_read(runtime, cpu->gpr[29] + 16u + i * 4u, 4, &output[i]) != 0)
            return -1;
    /* Retail 8004A83C..8004A8EC, including the conditional output stores.
     * The native SDK helper overwrites FLAG after the fourth projection;
     * 8004A8C8 instead ORs it with the first three vertices' RTPT flags. */
    for (unsigned i = 0; i < 3; i++) {
        if (runtime_read(runtime, cpu->gpr[4 + i], 4, &value) != 0) return -1;
        MTC2(value, (int)i * 2);
        if (runtime_read(runtime, cpu->gpr[4 + i] + 4u, 4, &value) != 0) return -1;
        MTC2(value, (int)i * 2 + 1);
    }
    doCOP2(0x4a280030); /* RTPT */
    flags = CFC2(31);
    if (runtime_write(runtime, output[6], 4, flags) != 0) return -1;
    doCOP2(0x4b400006); /* NCLIP */
    opz = MFC2(24);
    if ((int32_t)opz > 0) {
        for (unsigned i = 0; i < 3; i++)
            if (runtime_write(runtime, output[i], 4, MFC2(12 + (int)i)) != 0) return -1;
        if (runtime_read(runtime, cpu->gpr[7], 4, &value) != 0) return -1;
        MTC2(value, 0);
        if (runtime_read(runtime, cpu->gpr[7] + 4u, 4, &value) != 0) return -1;
        MTC2(value, 1);
        doCOP2(0x4a180001); /* RTPS */
        if (runtime_write(runtime, output[3], 4, MFC2(14)) != 0 ||
            runtime_write(runtime, output[4], 4, MFC2(8)) != 0 ||
            runtime_write(runtime, output[6], 4, CFC2(31) | flags) != 0) return -1;
        doCOP2(0x4b68002e); /* AVSZ4 */
        if (runtime_write(runtime, output[5], 4, MFC2(7)) != 0) return -1;
    }
    cpu->gpr[2] = opz;
    return 1;
}

static int bridge_clear_otag_r(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    uint32_t base, count = cpu->gpr[5];
    if (graphics_pointer_to_guest((void *)translate_argument(runtime, cpu->gpr[4]), &base) != 0 ||
        (base & 3u) != 0 || base < 0x80000000u || base >= 0x80200000u ||
        count == 0 || count > (0x80200000u - base) / 4u)
        return -1;
    /* DMA6 clears four-byte guest words. PsyCross's native OT_TAG is padded
     * to eight bytes for compiled u_long[] tables and would overwrite the
     * next battle render context. 80044B3C..54 links entry 0 to the loaded
     * EXE sentinel, not an invented terminator packet. */
    if (runtime_write(runtime, base, 4, 0x0005698cu) != 0) return -1;
    for (uint32_t i = 1; i < count; i++)
        if (runtime_write(runtime, base + i * 4u, 4,
                          (base + (i - 1u) * 4u) & 0x00ffffffu) != 0) return -1;
    cpu->gpr[2] = cpu->gpr[4];
    return 1;
}

/* Frame timing and context selection take raw scalar arguments. The native
 * void services must not manufacture V0/V1 from an unrelated return register.
 * Contexts beyond the two implemented native buffers are explicit refusals. */
static int bridge_frame_services(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,
                                 const ResolvedFunction *resolved)
{
    uint32_t target=resolved->address;
    if(target!=0x80019ca0u&&target!=0x800250e0u&&target!=0x80044ad8u&&target!=0x8004b54cu)return 0;
    if(cpu->load_valid&&cpu->load_reg!=0)return -1;
    if(target==0x80019ca0u){
        void (*check_reset)(void)=(void (*)(void))resolved->host;check_reset();
    }else if(target==0x800250e0u){
        if(cpu->gpr[4]>1u)return -1;
        void (*begin_context)(int)=(void (*)(int))resolved->host;begin_context((int32_t)cpu->gpr[4]);
    }else if(target==0x8004b54cu){
        int (*sync)(int)=(int (*)(int))resolved->host;cpu->gpr[2]=(uint32_t)sync((int32_t)cpu->gpr[4]);
    }else return bridge_clear_otag_r(runtime,cpu);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static uint8_t *dma_packet_memory(uint32_t address, unsigned bytes)
{
    const uint32_t ram_size = 0x200000u;
    uint32_t offset = address & 0x1fffffffu;
    uintptr_t host = address, ram = (uintptr_t)g_PsxRam;
    if (address < ram_size || (address & 0xffe00000u) == 0x80000000u ||
        (address & 0xffe00000u) == 0xa0000000u) {
        if ((offset & 3u) == 0 && bytes <= ram_size - offset)
            return g_PsxRam + offset;
    } else if (host >= ram && host < ram + ram_size && (host & 3u) == 0 &&
               bytes <= ram + ram_size - host) {
        /* HeapAlloc currently returns native addresses into emulated RAM.
         * Inline retail tag writers can therefore produce either domain. */
        return (uint8_t *)host;
    }
    return NULL;
}

/* TEMP-DIAG (docs/evidence/lahan-fire-gear-20260906): XENO_BATTLE_OT_DIAG=
 * "from:to" prints, for battle DrawOTag frames in [from,to] (1-based count
 * of bridge_draw_otag calls), one histogram line of the first GP0 command
 * byte of every non-NOP packet, and with XENO_BATTLE_OT_DIAG_SAMPLES=n the
 * first n packets of each frame verbatim (guest words). Read-only; the
 * packet payload and the walk are untouched. Off unless the env is set. */
extern unsigned g_PcPortPresentedFrames __attribute__((weak)); /* game_overrides.c */
static unsigned PcPort_PresentedFramesOrZero(void)
{
    const unsigned* volatile p = &g_PcPortPresentedFrames;
    return p ? *p : 0u;
}
static int s_otDiagState = -1;
static unsigned s_otDiagFrom, s_otDiagTo, s_otDiagSamples;
static unsigned s_otDiagFrame;

static int battle_ot_diag_enabled(void)
{
    if (s_otDiagState < 0) {
        const char *env = getenv("XENO_BATTLE_OT_DIAG");
        const char *samples = getenv("XENO_BATTLE_OT_DIAG_SAMPLES");
        s_otDiagState = 0;
        if (env != NULL && *env != '\0') {
            char *end = NULL;
            s_otDiagFrom = (unsigned)strtoul(env, &end, 0);
            s_otDiagTo = (end != NULL && *end == ':')
                ? (unsigned)strtoul(end + 1, NULL, 0) : 0xFFFFFFFFu;
            s_otDiagSamples = (samples != NULL && *samples != '\0')
                ? (unsigned)strtoul(samples, NULL, 0) : 0u;
            s_otDiagState = 1;
        }
    }
    return s_otDiagState;
}

static int bridge_draw_otag(PcPortMipsCpu *cpu)
{
    uint32_t address = cpu->gpr[4];
    unsigned otDiag = 0;
    unsigned otHist[256];
    unsigned otPackets = 0, otSampled = 0;
    if (g_GPUDisabledState) {
        ClearSplits();
        return 1;
    }
    if (battle_ot_diag_enabled()) {
        s_otDiagFrame++;
        if (s_otDiagFrame >= s_otDiagFrom && s_otDiagFrame <= s_otDiagTo) {
            otDiag = 1;
            memset(otHist, 0, sizeof(otHist));
        }
    }
    if (PsyX_BeginScene()) ClearSplits();
    /* At most one tag per RAM word; a longer walk necessarily repeats a
     * node. Stop with an error instead of hanging on a corrupt DMA chain. */
    for (uint32_t step = 0; step < 0x80000u; step++) {
        uint8_t *source = dma_packet_memory(address, 4);
        uint32_t tag, len, link, nonzero = 0;
        uint32_t packet[256] = {0};
        if (source == NULL) break;
        tag = load_le(source, 4);
        len = tag >> 24;
        link = tag & 0xffffffu;
        source = dma_packet_memory(address, (len + 1u) * 4u);
        if (source == NULL) break;
        memcpy(packet, source, (len + 1u) * 4u);
        for (uint32_t i = 1; i <= len; i++) nonzero |= packet[i];
        /* Zero GP0 words are NOPs, including the four words in the retail
         * 8005698C sentinel. Walk their link without inventing a replacement
         * packet. All other payloads pass through byte-for-byte. */
        if (nonzero != 0) {
            if (otDiag) {
                otHist[packet[1] >> 24]++;
                otPackets++;
                if (otSampled < s_otDiagSamples) {
                    uint32_t w;
                    otSampled++;
                    fprintf(stderr, "[battle-ot] f=%u addr=%08x len=%u:",
                            s_otDiagFrame, address, len);
                    for (w = 1; w <= len && w < 14; w++)
                        fprintf(stderr, " %08x", packet[w]);
                    fputc('\n', stderr);
                }
            }
            packet[0] = (tag & 0xff000000u) | 0xffffffu;
            /* Use linked-list mode for the complete DMA payload, not DrawPrim
             * (which only consumes one command in a merged packet). Only
             * this temporary host tag changes; guest bytes stay untouched. */
            ParsePrimitivesLinkedList((u_long *)packet, 0);
        }
        if (link == 0xffffffu) {
            if (otDiag) {
                unsigned c;
                fprintf(stderr, "[battle-ot] frame=%u presented=%u packets=%u hist:",
                        s_otDiagFrame, PcPort_PresentedFramesOrZero(), otPackets);
                for (c = 0; c < 256; c++)
                    if (otHist[c] != 0)
                        fprintf(stderr, " %02x=%u", c, otHist[c]);
                fputc('\n', stderr);
            }
            DrawAllSplits();
            return 1;
        }
        address = link;
    }
    ClearSplits();
    fprintf(stderr, "[xeno-port][battle-mips] invalid/cyclic DMA chain at %08x\n", address);
    return -1;
}

/* Direct-mapped, 4-way probe cache keyed by retail target address. */
static BridgeCallCacheSlot *bridge_cache_slot(BattleMipsRuntime *runtime,
                                              uint32_t target)
{
    unsigned base = (unsigned)((target * 2654435761u) >> 23) &
                    (BRIDGE_CALL_CACHE_SIZE - 1);
    unsigned i;

    for (i = 0; i < 4; i++) {
        BridgeCallCacheSlot *s = &runtime->call_cache[(base + i) &
                                 (BRIDGE_CALL_CACHE_SIZE - 1)];
        if (s->state == 0 || s->target == target)
            return s;
    }
    /* All four ways belong to other targets: reuse the primary slot, but mark
     * it empty first.  Returning a live slot here would let the caller treat
     * another target's verdict as a HIT, and the bridge would then dispatch
     * that target's host function for this address -- observed live as the
     * overlay PC 0x800b798c being dispatched to LoadImage in a tight loop. */
    {
        BridgeCallCacheSlot *victim = &runtime->call_cache[base];
        victim->state = 0;
        return victim;
    }
}

#define BRIDGE_HOOKS_NONE INT_MIN

#include "battle_resident_boundary.inc"

static int runtime_bridge_hooks(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                uint32_t target)
{
    /* Adopted RESULTS module routines (battle_results_gate.inc).  Ahead of the
     * per-target cache below, which keys on the address alone: other overlays
     * load over 0x801DE000.., so only the module-identity gate may decide
     * whether the retail results code is what is resident.  Returns 0 (run
     * the retail bytes) for anything not allowlisted or not proven. */
    {
        int adopted = results_try_host(runtime, cpu, target);
        if (adopted > 0)
            cpu_meter_note_kind(CPU_METER_KIND_RESULTS);
        if (adopted != 0) return adopted;
    }

    if (target == 0x801e6ce8u) {
        int adopted = file1_try_controller(runtime, cpu);
        if (adopted != 0) return adopted;
    }

    return BRIDGE_HOOKS_NONE;
}

static int battle_prepare_workspaces(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu);
static int battle_use_field_formation(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu);

static int runtime_bridge_call(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    BridgeCallCacheSlot *slot;
    BattleMipsRuntime *runtime = opaque;
    const ResolvedFunction *resolved;
    GenericHostFunction function;
    uintptr_t args[12];
    uintptr_t result;
    uint32_t raw;
    unsigned i;
    char fallback[32];
    void *fallback_host;
    int file1_load_candidate = 0;

    /* Verification hook (see the field comment). Placed above the file-1
     * controller so the always-interpret control in
     * run_battle_overlay_leaf_bridge_test.sh keeps its injection anchor. */
    if (runtime->force_interpret && target_is_guest_code(target))
        return 0;

    if (target == BATTLE_ENTRY)
        return battle_prepare_workspaces(runtime, cpu);
    if (target == 0x80070fc8u)
        return battle_use_field_formation(runtime, cpu);

    /* The renderer callbacks at these sites require native packed data.
     * A guest call without a verified argument adapter cannot enter them. */
    if ((cpu->gpr[31] == 0x8002c86cu ||
         (cpu->gpr[31] == 0x800b371cu && target != 0x800b383cu) ||
         cpu->gpr[31] == 0x800bea88u ||
         (cpu->gpr[31] == 0x80071700u && target == 0x8028022cu)) &&
        !target_is_guest_code(target))
        return -1;

    {
        int result = battle_resident_boundary(runtime, cpu, target);
        if (result != BRIDGE_HOOKS_NONE)
            return result;
    }

    /* Validate the loaded module before consulting the native symbol cache. */
    {
        int result = runtime_bridge_hooks(runtime, cpu, target);
        if (result != BRIDGE_HOOKS_NONE &&
            (result != 0 || target_is_guest_code(target)))
            return result;
    }

    resolved = NULL;
    slot = bridge_cache_slot(runtime, target);
    if (slot->state == 0) {
        resolved = find_function(runtime, target);
        if (resolved == NULL) {
            /* Native task allocators put registered host callbacks in their
             * packed callback slots. Retail battle code reads those slots and
             * uses JALR (observed at 800BB5E0 for TimerWorkListDeleteTask).
             * Resolve back to the existing symbol entry so normal ABI handling
             * and generated-stub rejection still apply. Never call arbitrary
             * host addresses, and never match by truncating a wider pointer. */
            for (size_t index = 0; index < runtime->function_count; index++) {
                if ((uintptr_t)runtime->functions[index].host == (uintptr_t)target) {
                    resolved = &runtime->functions[index];
                    break;
                }
            }
        }
        if (resolved == NULL) {
            snprintf(fallback, sizeof(fallback), "func_%08X", target);
            fallback_host = NULL;
            if (target_is_guest_code(target)) {
                /* A battle function whose name another overlay also defines
                 * is linked as battle_func_<addr> (build_port.sh); the plain
                 * name would be the other overlay's body. */
                char battle_name[40];
                snprintf(battle_name, sizeof(battle_name), "battle_%s", fallback);
                fallback_host = dlsym(RTLD_DEFAULT, battle_name);
            }
            if (fallback_host == NULL)
                fallback_host = dlsym(RTLD_DEFAULT, fallback);
            if (fallback_host != NULL &&
                (!target_is_guest_code(target) || overlay_leaf_host_ok(fallback))) {
                memcpy(slot->name, fallback, sizeof(slot->name));
                slot->name[sizeof(slot->name) - 1] = '\0';
                slot->entry.address = target;
                slot->entry.host = fallback_host;
                slot->entry.name = slot->name;
                resolved = &slot->entry;
            }
        }
        if (resolved != NULL && target_is_guest_code(target) &&
            !overlay_leaf_host_ok(resolved->name))
            resolved = NULL;
        if (resolved != NULL && xeno_port_is_generated_stub(resolved->name)) {
            fprintf(stderr,
                    "[xeno-port][battle-mips] refusing generated stub %s "
                    "at retail target 0x%08x\n",
                    resolved->name, target);
            return -1;
        }
        if (resolved != NULL && resolved != &slot->entry) {
            slot->entry = *resolved;
            resolved = &slot->entry;
        }
        slot->target = target;
        slot->state = (resolved != NULL) ? 1 : 2;
    } else if (slot->state == 1) {
        resolved = &slot->entry;
    }
    if (resolved == NULL) {
        if (target_is_guest_code(target))
            return 0;
        fprintf(stderr,
                "[xeno-port][battle-mips] unresolved native call "
                "target=0x%08x guest-pc=0x%08x\n",
                target, cpu->gpr[31] - 8u);
        return -1;
    }

    /* A native work-list may store a registered host callback address. Its
     * canonical guest address still determines the argument ABI. */
    if (resolved->address != target) {
        int result = battle_resident_boundary(runtime, cpu, resolved->address);
        if (result != BRIDGE_HOOKS_NONE)
            return result;
    }

    /* Concrete signatures precede the legacy word-based thunks. In
     * particular, the void glyph-style service must not be intercepted by
     * the old eight-word returning call ABI. Resolution/adoption above is
     * still required; these bindings do not allowlist unproven bodies. */
    {
        int handled = bridge_frame_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_camera_update_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_effects_matrix(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_actor_back_color(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_palette_update_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_effect_model_compose(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_target_camera_graphics(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_draw_sync_service(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_vram_readback(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_color_fade_lists(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_timer_task_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_post_resource_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_heap_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_resource_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_motion_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_actor_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_window_render(runtime, cpu, resolved);
        if (handled != 0) return handled;
        handled = bridge_stage3_services(runtime, cpu, resolved);
        if (handled != 0) return handled;
    }

#ifndef BATTLE_BRIDGE_LEGACY_ARGS
    /* Guest-ABI leaves: typed thunk, never the translate_argument heuristic.
     * (BATTLE_BRIDGE_LEGACY_ARGS rebuilds the pre-thunk bridge; only the
     * overlay differential's old-bridge control defines it.) */
    if (target_is_guest_code(target)) {
        const BattleGuestThunk *thunk = battle_guest_thunk(target);
        if (thunk != NULL) {
            uint32_t words[BG_THUNK_MAX_ARGS] = {0};
            for (i = 0; i < thunk->argc; i++) {
                if (i < 4)
                    words[i] = cpu->gpr[4 + i];
                else if (runtime_read(runtime, cpu->gpr[29] + 0x10u + (i - 4u) * 4u,
                                      4, &words[i]) != 0)
                    return -1;
            }
            if (runtime->trace_calls)
                fprintf(stderr,
                        "[xeno-port][battle-mips] typed call %s target=0x%08x "
                        "a0=%08x a1=%08x a2=%08x a3=%08x\n",
                        resolved->name, target, cpu->gpr[4], cpu->gpr[5],
                        cpu->gpr[6], cpu->gpr[7]);
            cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
            if (battle_guest_thunk_invoke(runtime, thunk, resolved->host, words,
                                          &cpu->gpr[2]) != 0) {
                fprintf(stderr,
                        "[xeno-port][battle-mips] typed thunk %s refused its "
                        "arguments at 0x%08x\n", thunk->name, target);
                return -1;
            }
            cpu->gpr[3] = 0;
            return 1;
        }
    }
#endif

    if (resolved->address == 0x800a3490u || resolved->address == 0x800a3514u ||
        resolved->address == 0x800a3578u || resolved->address == 0x800a35c8u) {
        /* Effect callbacks take three scalar halfwords, never guest pointers.
         * Let retail execute BREAK 7 for a zero divisor. */
        if ((int16_t)cpu->gpr[5] == 0) return 0;
        if (resolved->address == 0x800a3514u) {
            int32_t (*effect)(int16_t, int16_t, int16_t) =
                (int32_t (*)(int16_t, int16_t, int16_t))resolved->host;
            cpu->gpr[2] = (uint32_t)effect((int16_t)cpu->gpr[4],
                                         (int16_t)cpu->gpr[5],
                                         (int16_t)cpu->gpr[6]);
        } else {
            int16_t (*effect)(int16_t, int16_t, int16_t) =
                (int16_t (*)(int16_t, int16_t, int16_t))resolved->host;
            cpu->gpr[2] = (uint32_t)(int32_t)effect((int16_t)cpu->gpr[4],
                                                  (int16_t)cpu->gpr[5],
                                                  (int16_t)cpu->gpr[6]);
        }
        return 1;
    }

    if (resolved->address == 0x8003f8b0u ||
        resolved->address == 0x8003f8ccu) {
        /* Retail masks the scalar angle before its table lookup. Do this
         * before generic pointer translation or host signed arithmetic. */
        int angle = (int)(cpu->gpr[4] & 0xfffu);
        int (*trig)(int) = (int (*)(int))resolved->host;
        cpu->gpr[2] = (uint32_t)trig(angle);
        return 1;
    }

    if (resolved->address == 0x8004931cu) {
        /* CompMatrix has three matrix-pointer arguments. Unlike generic
         * scalar/pointer inference, its typed boundary must accept physical
         * RAM addresses, including zero. The Lahan destruction callback
         * 800AFCBC supplied 0xC and 0x1408 in the captured failure;
         * retail reads those RAM locations instead of dereferencing host
         * null-page addresses. Preserve the original destination in v0. */
        MATRIX *matrices[3];
        MATRIX *(*compose)(MATRIX *, MATRIX *, MATRIX *) =
            (MATRIX *(*)(MATRIX *, MATRIX *, MATRIX *))resolved->host;
        for (i = 0; i < 3; ++i) {
            uint32_t address = cpu->gpr[4 + i];
            if ((address & 3u) != 0)
                return -1;
            if (address < 0x200000u) {
                if (address > 0x200000u - sizeof(MATRIX))
                    return -1;
                address |= 0x80000000u;
            }
            matrices[i] = (MATRIX *)resolve_memory(
                runtime, address, sizeof(MATRIX), i == 2);
            if (matrices[i] == NULL)
                return -1;
        }
        compose(matrices[0], matrices[1], matrices[2]);
        cpu->gpr[2] = cpu->gpr[6];
        return 1;
    }

    if (resolved->address == 0x8008abb8u) {
        /* The battle allocator wrapper takes two scalars and returns a host
         * RAM pointer. Keep its scalar bits out of pointer translation, and
         * restore the guest return domain before retail stores the result. */
        void *(*allocate)(int32_t, int32_t) =
            (void *(*)(int32_t, int32_t))resolved->host;
        /* A load in the jal delay slot has not landed yet. No static retail
         * call site does this; leave any indirect one to retail. */
        if (cpu->load_valid && cpu->load_reg != 0)
            return 0;
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
        result = PsxMemory_GuestAddr(allocate((int32_t)cpu->gpr[4], (int32_t)cpu->gpr[5]));
        cpu->gpr[2] = (uint32_t)result;
        cpu->gpr[3] = 0;
        return 1;
    }

    if (resolved->address == 0x8001bd40u) {
        uint8_t (*random_range)(uint8_t,uint8_t) =
            (uint8_t (*)(uint8_t,uint8_t))resolved->host;
        cpu->gpr[2] = random_range((uint8_t)cpu->gpr[4],(uint8_t)cpu->gpr[5]);
        cpu->gpr[3] = 0;
        cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
        return 1;
    }

    for (i = 0; i < 4; i++) {
        raw = cpu->gpr[4 + i];
        args[i] = is_callback_argument(resolved->name, i)
                      ? (uintptr_t)raw
                      : translate_argument(runtime, raw);
    }
    for (i = 4; i < 12; i++) {
        if (runtime_read(runtime, cpu->gpr[29] + 0x10u + (i - 4u) * 4u,
                         4, &raw) != 0)
            raw = 0;
        args[i] = is_callback_argument(resolved->name, i)
                      ? (uintptr_t)raw
                      : translate_argument(runtime, raw);
    }

    if (resolved->address == 0x80032f54u) {
        uint32_t height;
        /* Retail window construction consumes seven arguments: its seventh
         * stack value is stored as a halfword, then used as a signed row count
         * (80032F9C, 80032FD8, 80032FFC). The native constructor retains a dead
         * mode argument before height, also supplied by native field callers.
         * Read the untranslated guest scalar and insert that
         * placeholder here, preserving the existing native calling convention. */
        if (runtime_read(runtime, cpu->gpr[29] + 0x18u, 2, &height) != 0)
            return -1;
        args[6] = 0;
        args[7] = (uintptr_t)(intptr_t)(int16_t)height;
    }

    if (runtime->trace_calls)
        fprintf(stderr,
                "[xeno-port][battle-mips] call %s target=0x%08x "
                "a0=%08x a1=%08x a2=%08x a3=%08x\n",
                resolved->name, target, cpu->gpr[4], cpu->gpr[5],
                cpu->gpr[6], cpu->gpr[7]);
    if (strcmp(resolved->name, "LoadImage") == 0)
        return bridge_load_image(runtime, cpu, resolved->host);
    if (strcmp(resolved->name, "ReadTIM") == 0)
        return bridge_read_tim(runtime, cpu, resolved->host);
    {
        int handled = bridge_module_memory(runtime, cpu, resolved);
        if (handled != 0) return handled;
    }
    {
        int handled = bridge_texture_sdk(runtime, cpu, resolved);
        if (handled != 0) return handled;
    }
    {
        int handled = bridge_shard_animation_sdk(runtime, cpu, resolved);
        if (handled != 0) return handled;
    }
    if (strcmp(resolved->name, "ReadGeomOffset") == 0)
        return bridge_read_geom_offset(runtime, cpu, resolved->host);
    if (strcmp(resolved->name, "RotTransPers3") == 0)
        return bridge_rot_trans_pers3(runtime, cpu, resolved->host, args);
    if (strcmp(resolved->name, "RotTransPers") == 0)
        return bridge_rot_trans_pers(runtime, cpu, resolved->host, args);
    if (strcmp(resolved->name, "RotTransPers4") == 0)
        return bridge_rot_trans_pers4(runtime, cpu, resolved->host, args);
    if (strcmp(resolved->name, "RotAverage4") == 0)
        return bridge_rot_average4(runtime, cpu, resolved->host, args);
    if (strcmp(resolved->name, "RotAverageNclip4") == 0)
        return bridge_rot_average_nclip4(runtime, cpu);
    if (strcmp(resolved->name, "ClearOTagR") == 0)
        return bridge_clear_otag_r(runtime, cpu);
    if (strcmp(resolved->name, "DrawOTag") == 0)
        return bridge_draw_otag(cpu);
    {
        int handled = bridge_shard_sdk(cpu, resolved, args);
        if (handled != 0) return handled;
    }
    file1_before_archive(runtime, cpu, resolved->address, &file1_load_candidate);
    results_note_archive(runtime, resolved->address);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    function = (GenericHostFunction)resolved->host;
    result = function(args[0], args[1], args[2], args[3],
                      args[4], args[5], args[6], args[7],
                      args[8], args[9], args[10], args[11]);
    file1_after_archive(runtime, cpu, resolved->address, result, file1_load_candidate);
    cpu->gpr[2] = (uint32_t)result;
    cpu->gpr[3] = (uint32_t)(result >> 32);
    return 1;
}

static int runtime_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    BattleMipsRuntime *runtime = opaque;
    /* Service elapsed vblank callbacks on the game thread while the guest is
     * running, including pause busy-waits. Shares the Vsync counter cursor;
     * repeated bridge calls in the same tick cannot push duplicate states. */
    { extern void PcPort_PadVblankPump(void); PcPort_PadVblankPump(); }
    PcPortMipsCpu *previous = runtime->bridge_cpu;
    CpuMeterBridgeState meter = {0};
    runtime->bridge_cpu = cpu;
    {
        void (*volatile hash_hook)(PcPortMipsCpu*, uint32_t) = PcPort_BattleStateHashBeforeGuest;
        void (*volatile results_hook)(PcPortMipsCpu*, uint32_t) = PcPort_BattleResultsTraceBeforeGuest;
        if (hash_hook) hash_hook(cpu, target);
        if (results_hook) results_hook(cpu, target);
    }
    {
        void (*volatile god_hook)(PcPortMipsCpu*, uint32_t) = PcPort_GodModeBeforeGuest;
        if (god_hook)
            god_hook(cpu, target);
    }
    if (g_BattleExitHash > 0 && target == 0x801E252Cu)
        g_BattleExitHashResults++;
    cpu_meter_bridge_enter(runtime, cpu, target, &meter);
    int result = runtime_bridge_call(opaque, cpu, target);
    cpu_meter_bridge_leave(cpu, target, result, &meter);
    runtime->bridge_cpu = previous;
    return result;
}

static uint32_t runtime_cop2_read(void *opaque, int control, unsigned reg)
{
    (void)opaque;
    return control ? CFC2((int)reg) : MFC2((int)reg);
}

static void runtime_cop2_write(void *opaque, int control, unsigned reg,
                               uint32_t value)
{
    (void)opaque;
    if (control)
        CTC2(value, (int)reg);
    else
        MTC2(value, (int)reg);
}

static int runtime_cop2_command(void *opaque, uint32_t instruction)
{
    (void)opaque;
    doCOP2((int)instruction);
    return 0;
}

static void initialize_cpu(PcPortMipsCpu *cpu, BattleMipsRuntime *runtime)
{
    PcPortMipsBus bus;
    memset(&bus, 0, sizeof(bus));
    bus.opaque = runtime;
    bus.read = runtime_read;
    bus.write = runtime_write;
    bus.bridge = runtime_bridge;
    bus.cop2_read = runtime_cop2_read;
    bus.cop2_write = runtime_cop2_write;
    bus.cop2_command = runtime_cop2_command;
    PcPortMipsCpuInit(cpu, &bus);
    cpu->gpr[29] = BATTLE_STACK_TOP;
    cpu->gpr[31] = BATTLE_HALT_PC;
}

/* A retail battle overlay is called from the main executable and inherits its
 * small-data base. The native main loop has no guest GP to pass, so establish
 * the configured retail overlay ABI at this entry only. Nested guest calls
 * preserve their caller's register context separately. */
static void initialize_battle_entry_cpu(PcPortMipsCpu *cpu, BattleMipsRuntime *runtime)
{
    initialize_cpu(cpu, runtime);
    cpu->gpr[28] = 0x80059170u; /* config/battle.yaml: gp_value */
}

/* The host pointer the guest-to-native bridge would hand a native callee for
 * the raw guest word `value` (translate_argument): an adopted host body that
 * passes a guest word to a main-executable routine must see exactly what the
 * interpreted caller's callee sees, including shared-data redirects and raw
 * pass-through of non-address values. */
void *PcPort_BattleGuestWordToHost(uint32_t value)
{
    BattleMipsRuntime *runtime = g_ActiveBattleRuntime;
    if (runtime == NULL || !runtime->initialized)
        return value != 0 ? (void *)PSX_ADDR(value) : NULL;
    return (void *)translate_argument(runtime, value);
}

/* Guest-address -> native map for indirect calls from native code (bg_jalr,
 * and any later GCALL): a target that is an adopted guest-ABI leaf with a
 * typed thunk runs its native body instead of the interpreter.  Exactly the
 * leaves the bridge would adopt for a JAL: allowlisted, a linked host body
 * (battle_<name> first, as the bridge looks it up), never a generated stub.
 * dlsym results are process-wide, so the cache is too. */
__attribute__((unused)) static void *g_GuestNativeHost[BATTLE_GUEST_THUNK_COUNT];
__attribute__((unused)) static uint8_t g_GuestNativeState[BATTLE_GUEST_THUNK_COUNT]; /* 0 ?, 1 yes, 2 no */
/* Indirect calls served natively; the differential reads it. */
__attribute__((unused)) static unsigned long g_GuestNativeIndirectCalls;

__attribute__((unused)) static void *battle_guest_native_host(const BattleGuestThunk *thunk)
{
    size_t index = (size_t)(thunk - kBattleGuestThunks);
    if (g_GuestNativeState[index] == 0) {
        char battle_name[40];
        void *host;
        snprintf(battle_name, sizeof(battle_name), "battle_%s", thunk->name);
        host = dlsym(RTLD_DEFAULT, battle_name);
        if (host == NULL)
            host = dlsym(RTLD_DEFAULT, thunk->name);
        if (host != NULL && (!overlay_leaf_host_ok(thunk->name) ||
                             xeno_port_is_generated_stub(thunk->name)))
            host = NULL;
        g_GuestNativeHost[index] = host;
        g_GuestNativeState[index] = host != NULL ? 1 : 2;
    }
    return g_GuestNativeState[index] == 1 ? g_GuestNativeHost[index] : NULL;
}

/* A guest $sp a native-to-guest call may build its callee frame below.
 * Retail battle code runs some work on stacks of its own: 800A4654 switches
 * $sp into the scratchpad (0x1F8003F8), 800B62C8 / 800BEE2C / 800BB65C into
 * a HeapAlloc'd block (4-byte aligned only), so a native body reached from
 * there sees that $sp.  An interpreted `jal` accepts any of them; so must the
 * native call services: the frame window [sp - below, sp + above) has to lie
 * in one guest region (main RAM through KSEG0/KSEG1, or the 1 KiB
 * scratchpad) and $sp must be word aligned (o32 only promises that; retail's
 * own stack switches keep 4).  BATTLE_JAL_LEGACY_SP_CHECK restores the old
 * rule (8-aligned, KSEG0 RAM at or below BATTLE_STACK_TOP) for the
 * differential's control only. */
static int guest_call_stack_ok(uint32_t sp, uint32_t below, uint32_t above)
{
#ifdef BATTLE_JAL_LEGACY_SP_CHECK
    (void)above;
    return (sp & 7u) == 0 && sp >= 0x80000000u + below && sp <= BATTLE_STACK_TOP;
#else
    uint32_t offset;
    if ((sp & 3u) != 0)
        return 0;
    if ((sp & 0xfffff000u) == 0x1f800000u) {
        offset = sp & 0xfffu;
        return offset >= below && offset + above <= 0x400u;
    }
    if ((sp & 0xff800000u) != 0x80000000u && (sp & 0xff800000u) != 0xa0000000u)
        return 0;
    offset = sp & 0x7fffffu;
    return offset >= below && offset + above <= 0x200000u;
#endif
}

int PcPort_BattleMipsCallGuest(uint32_t target, const uint32_t *args,
                              unsigned argc, uint32_t *result)
{
    BattleMipsRuntime *runtime = g_ActiveBattleRuntime;
    PcPortMipsCpu *caller;
    PcPortMipsCpu cpu;
    uint32_t words[7];
    uint32_t sp;
    int rc;

    if (runtime == NULL || !runtime->initialized ||
        runtime->bridge_cpu == NULL || argc > 7 || (argc != 0 && args == NULL))
        return -1;
    /* The older callback target domain deliberately includes a wider overlay
     * window. Do not change it here, and do not let this service mask its
     * out-of-RAM addresses (or a native pointer) through PSX_ADDR. */
    if (!target_is_guest_code(target) || (target & 3u) != 0 ||
        target >= 0x80200000u)
        return -1;
#ifndef BATTLE_BRIDGE_LEGACY_ARGS
    /* Native map: an adopted guest-ABI leaf stays native.  force_interpret
     * (verification only) keeps the interpreter, as the bridge does.  The
     * caller must supply every declared argument word; otherwise the retail
     * callee would read registers the native caller never set, so it is left
     * to the interpreter. */
    if (!runtime->force_interpret) {
        const BattleGuestThunk *thunk = battle_guest_thunk(target);
        void *host = thunk != NULL ? battle_guest_native_host(thunk) : NULL;
        if (host != NULL && argc >= thunk->argc) {
            uint32_t v0 = 0;
            cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
            if (battle_guest_thunk_invoke(runtime, thunk, host, args, &v0) != 0) {
                fprintf(stderr, "[xeno-port][battle-mips] typed thunk %s refused "
                        "its arguments (indirect call)\n", thunk->name);
                return -1;
            }
            g_GuestNativeIndirectCalls++;
            if (result != NULL)
                *result = v0;
            return 0;
        }
    }
#endif
    caller = runtime->bridge_cpu;
    sp = caller->gpr[29];
    /* Retail file-1 801E6CE8 starts with ADDIU SP,SP,-0x50. Its native
     * replacement will lack that guest frame, including the o32 arg slots.
     * Validate before subtracting or resolving any addresses. */
    if (!guest_call_stack_ok(sp, 0x50u, 0u)) {
        fprintf(stderr, "[xeno-port][battle-mips] guest call 0x%08x: caller sp "
                "0x%08x has no room for a frame\n", target, sp);
        return -1;
    }
    sp -= 0x50u;
    if (argc != 0)
        memcpy(words, args, argc * sizeof(words[0]));

    initialize_cpu(&cpu, runtime);
    /* Guest helpers can use GP-relative data; a fresh callback's zero GP is
     * not the environment of a direct call from the native controller. */
    memcpy(cpu.gpr, caller->gpr, sizeof(cpu.gpr));
    memcpy(cpu.cp0, caller->cp0, sizeof(cpu.cp0));
    cpu.hi = caller->hi;
    cpu.lo = caller->lo;
    cpu.gpr[0] = 0;
    cpu.gpr[29] = sp;
    cpu.gpr[31] = BATTLE_HALT_PC;
    for (unsigned i = 0; i < 4; i++)
        cpu.gpr[4 + i] = i < argc ? words[i] : 0;
    for (unsigned i = 4; i < argc; i++) {
        if (runtime_write(runtime, sp + 0x10u + (i - 4u) * 4u, 4,
                          words[i]) != 0)
            return -1;
    }

    runtime->bridge_cpu = &cpu;
    cpu_meter_mark_entry(&cpu, target);
    rc = PcPortMipsRun(&cpu, target, BATTLE_HALT_PC, CALLBACK_STEP_LIMIT);
    runtime->bridge_cpu = caller;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "[xeno-port][battle-mips] guest call 0x%08x failed: %s\n",
                target, cpu.error);
        return -1;
    }
    if (result != NULL)
        *result = cpu.gpr[2];
    return 0;
}

/* A `jal target` made by a native battle body exactly as the interpreter
 * makes it (include/battle_lift.h, lifted port bodies): the same
 * runtime_bridge() entry -- RESULTS/file-1 gates, the archive identity hooks,
 * the typed and special-cased bindings, translate_argument for the generic
 * path, HeapAlloc's guest-domain return -- on a CPU copied from the current
 * bridge CPU, with a0..a3 and the o32 stack words below its SP.  A target
 * the bridge does not bind natively runs in the interpreter, as an
 * interpreted jal would.  v0/v1 are what the guest would see after the call.
 * args: a0..a3, then the words the caller's frame holds at sp+0x10.. (a
 * caller passes as many as its callees can read; a retail callee such as
 * func_800A3640 takes fifteen).  Returns 0, or -1 when the call cannot be
 * made. */
#define BATTLE_JAL_STACK_WORDS 16u
/* Host callbacks stored in packed task slots need not have MIPS instruction
 * alignment. Admit an unaligned value only by exact full-width registration,
 * then use its retail identity so the normal ABI/stub/guest-body gates apply.
 * Never infer a callable host address from a truncated pointer or guest word. */
static int battle_jal_target(BattleMipsRuntime *runtime, uint32_t *target)
{
    if ((*target & 3u) == 0)
        return 1;
    if (*target < BATTLE_BUS_KUSEG_RAM_END ||
        (*target & 0xE0000000u) == 0x80000000u ||
        (*target & 0xE0000000u) == 0xA0000000u ||
        (*target & 0xFFFFF000u) == 0x1F800000u)
        return 0;
    for (size_t i = 0; i < runtime->function_count; i++) {
        const ResolvedFunction *entry = &runtime->functions[i];
        if ((uintptr_t)entry->host == (uintptr_t)*target &&
            (entry->address & 3u) == 0) {
            *target = entry->address;
            return 1;
        }
    }
    return 0;
}

static int battle_call_jal(uint32_t target,const uint32_t *args,unsigned argc,
                           uint32_t *v0,uint32_t *v1,const uint32_t *incoming_s1)
{
    BattleMipsRuntime *runtime = g_ActiveBattleRuntime;
    PcPortMipsCpu *caller;
    PcPortMipsCpu cpu;
    uint32_t sp;
    int rc;

    if (runtime == NULL || !runtime->initialized ||
        runtime->bridge_cpu == NULL || argc > 4u + BATTLE_JAL_STACK_WORDS ||
        (argc != 0 && args == NULL) || !battle_jal_target(runtime, &target)) {
        fprintf(stderr, "[xeno-port][battle-mips] native jal 0x%08x: no active "
                "battle CPU or bad call shape\n", target);
        return -1;
    }
    caller = runtime->bridge_cpu;
    sp = caller->gpr[29];
    /* Room below the live guest frame for the callee's o32 home area and
     * up to BATTLE_JAL_STACK_WORDS stack argument words. */
    if (!guest_call_stack_ok(sp, 0x100u, 0u)) {
        fprintf(stderr, "[xeno-port][battle-mips] native jal 0x%08x: caller sp "
                "0x%08x has no room for a frame\n", target, sp);
        return -1;
    }
    sp -= 0x100u;
    initialize_cpu(&cpu, runtime);
    memcpy(cpu.gpr, caller->gpr, sizeof(cpu.gpr));
    memcpy(cpu.cp0, caller->cp0, sizeof(cpu.cp0));
    cpu.hi = caller->hi;
    cpu.lo = caller->lo;
    cpu.gpr[0] = 0;
    if (incoming_s1 != NULL)
        cpu.gpr[17] = *incoming_s1;
    cpu.gpr[29] = sp;
    cpu.gpr[31] = BATTLE_HALT_PC;
    for (unsigned i = 0; i < 4; i++)
        cpu.gpr[4 + i] = i < argc ? args[i] : 0;
    for (unsigned i = 4; i < argc; i++) {
        if (runtime_write(runtime, sp + 0x10u + (i - 4u) * 4u, 4, args[i]) != 0)
            return -1;
    }
    rc = runtime_bridge(runtime, &cpu, target);
    if (rc < 0) {
        fprintf(stderr, "[xeno-port][battle-mips] native jal 0x%08x refused\n", target);
        return -1;
    }
    if (rc == 0) {
        runtime->bridge_cpu = &cpu;
        cpu_meter_mark_entry(&cpu, target);
        rc = PcPortMipsRun(&cpu, target, BATTLE_HALT_PC, CALLBACK_STEP_LIMIT);
        runtime->bridge_cpu = caller;
        if (rc != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "[xeno-port][battle-mips] native jal 0x%08x failed: %s\n",
                    target, cpu.error);
            return -1;
        }
    }
    if (v0 != NULL)
        *v0 = cpu.gpr[2];
    if (v1 != NULL)
        *v1 = cpu.gpr[3];
    return 0;
}

int PcPort_BattleMipsCallJal(uint32_t target,const uint32_t *args,unsigned argc,
                            uint32_t *v0,uint32_t *v1)
{
    return battle_call_jal(target,args,argc,v0,v1,NULL);
}

/* The effect constructor retains incoming S1 for negative stripe rows.
 * Lifted callers keep that register in a C local, so pass it explicitly
 * into the copied callee CPU without modifying the live bridge caller. */
int PcPort_BattleMipsCallJalS1(uint32_t target,const uint32_t *args,unsigned argc,
                              uint32_t s1,uint32_t *v0,uint32_t *v1)
{
    return battle_call_jal(target,args,argc,v0,v1,&s1);
}

static int run_guest_callback(BattleMipsRuntime *runtime, uint32_t callback,
                              void *argument)
{
    PcPortMipsCpu cpu;
    int rc;

    initialize_battle_entry_cpu(&cpu, runtime);
    /* A nested callback inherits the live caller's GP; the native work-list
     * pump otherwise invokes it in the configured retail overlay context. */
    if (runtime->bridge_cpu != NULL)
        cpu.gpr[28] = runtime->bridge_cpu->gpr[28];
    /* A native work-list pump can call back while an outer guest frame is
     * still live. Continue below that caller's SP, as a normal nested call
     * does, instead of restarting at the top and overwriting saved locals/RA.
     * The bridge wrapper restores the parent CPU across deeper callbacks. */
    if (runtime->bridge_cpu != NULL)
        cpu.gpr[29] = runtime->bridge_cpu->gpr[29];
    cpu.gpr[4] = (uint32_t)(uintptr_t)argument;
    cpu_meter_mark_entry(&cpu, callback);
    rc = PcPortMipsRun(&cpu, callback, BATTLE_HALT_PC, CALLBACK_STEP_LIMIT);
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr,
                "[xeno-port][battle-mips] callback 0x%08x failed: %s\n",
                callback, cpu.error);
        return -1;
    }
    return 0;
}

/* Main-executable code that the interpreted battle overlay stores in a packed
 * work-list callback slot. func_800B6438 (mainc88.c) does exactly this: it
 * WorkListSetTaskCallback's the retail address 0x80025A88, the main-exe effect
 * renderer func_80025A88. The native work list then has a guest callback it
 * must invoke. Resolve it to the host owner with the same two lookups
 * runtime_bridge_call uses (bridge entry first, then the func_%08X symbol),
 * and refuse a generated stub the same way. Returns 1 if called. */
static int run_main_exe_callback(BattleMipsRuntime *runtime, uint32_t callback,
                                 void *argument)
{
    const ResolvedFunction *resolved = find_function(runtime, callback);
    void *host = NULL;

    if (resolved != NULL && resolved->host != NULL &&
        !xeno_port_is_generated_stub(resolved->name)) {
        host = resolved->host;
    }
    if (host == NULL) {
        char fallback[32];
        snprintf(fallback, sizeof(fallback), "func_%08X", callback);
        host = dlsym(RTLD_DEFAULT, fallback);
        if (host == NULL || xeno_port_is_generated_stub(fallback))
            return 0;
    }
    ((void (*)(void *))host)(argument);
    return 1;
}

/* Called by the packed native work-list adapter when a retail task carries a
 * guest callback address. Returns 1 if dispatched, 0 if it is not ours. */
/* Non-zero while a retail battle overlay is the active runtime (native
 * main-executable callers of adopted battle leaves check it, as the dispatch
 * below does). */
int PcPort_BattleMipsActive(void)
{
    return g_ActiveBattleRuntime != NULL;
}

int PcPort_BattleMipsDispatchCallback(uint32_t callback, void *argument)
{
    if (g_ActiveBattleRuntime == NULL)
        return 0;
    if (target_is_guest_code(callback)) {
        if (run_guest_callback(g_ActiveBattleRuntime, callback, argument) != 0) {
            /* The abort skips the battle-end report: print the meter first
             * (no-op unless XENO_CPU_FALLBACK_STATS is set). */
            cpu_meter_end_battle("aborted");
            abort();
        }
        return 1;
    }
    return run_main_exe_callback(g_ActiveBattleRuntime, callback, argument);
}

/* The adapter includes the same decomp-owned controller body as the PSX
 * build; its bindings and identity gate remain private to this runtime. */
#include "battle_file1_controller.inc"

/* Identity-gated dispatch for the adopted battle RESULTS module routines and
 * the guest accessor their native bodies use; after file-1 for its digest. */
#include "battle_results_gate.inc"

/* TEST TOOLING XENO_BATTLE_EXIT_HASH=1 (off unless set; read once): at battle
 * exit print one line with a SHA-256 per guest-RAM game-state region, read
 * through the interpreter's bus (resolve_memory: the canonical view, native
 * copies included), so a native-leaves run and an XENO_BATTLE_NATIVE_LEAVES=0
 * run of one deterministic route can be compared line for line.  The line
 * carries no instruction or timing counts on purpose.  Regions:
 *   gamestate   g_GameState 0x8006D634, 0x235C (incl. field-transition tuple)
 *   characters  g_GameState+0x26C  11 x 0xA4 character stats
 *   gears       g_GameState+0x978  20 x 0xA4 gear stats
 *   money       g_GameState+0x1924 gold
 *   field_vars  g_GameState+0x1930 0x400 field-script variables / story flags
 *               (FieldLoad copies them to g_FieldScriptMemory, misc3.c)
 *   party       g_GameState+0x1D30 FrMask + party members
 *   inventory   g_GameState+0x1D38..0x22B8 (all five families + gear ride)
 *   rng         g_RandomSeed 0x8005A1FC
 *   results_ws  RESULTS module mutable words 0x801E44C0..0x801E4500
 *   battlers    D_800CCCE8 11 x 0x170 battler records at exit
 * plus the battle number, how the battle ended and the number of RESULTS
 * module (0x801E252C) entries seen by the bridge. */

static int battle_exit_hash_enabled(void)
{
    if (g_BattleExitHash < 0) {
        const char *v = getenv("XENO_BATTLE_EXIT_HASH");
        g_BattleExitHash = v != NULL && v[0] != '\0' && strcmp(v, "0") != 0;
    }
    return g_BattleExitHash;
}

static void battle_exit_hash_print(BattleMipsRuntime *runtime, const char *how)
{
    static const struct { const char *name; uint32_t address, size; } kRegions[] = {
        {"gamestate",  0x8006D634u,           0x235Cu},
        {"characters", 0x8006D634u + 0x26Cu,  11u * 0xA4u},
        {"gears",      0x8006D634u + 0x978u,  20u * 0xA4u},
        {"money",      0x8006D634u + 0x1924u, 4u},
        {"field_vars", 0x8006D634u + 0x1930u, 0x400u},
        {"party",      0x8006D634u + 0x1D30u, 8u},
        {"inventory",  0x8006D634u + 0x1D38u, 0x22B8u - 0x1D38u},
        {"rng",        0x8005A1FCu,           4u},
        {"results_ws", 0x801E44C0u,           0x40u},
        {"battlers",   0x800CCCE8u,           11u * 0x170u},
    };
    static uint8_t buffer[0x2400];
    char line[2048];
    size_t used;
    unsigned r;
    uint32_t money = 0, rng = 0;

    used = (size_t)snprintf(line, sizeof(line),
                            "[xeno-port][battle-exit-hash] battle=%u end=%s results_calls=%u",
                            g_BattleExitHashBattle, how, g_BattleExitHashResults);
    if (runtime_read(runtime, 0x8006D634u + 0x1924u, 4, &money) != 0)
        money = 0xffffffffu;
    if (runtime_read(runtime, 0x8005A1FCu, 4, &rng) != 0)
        rng = 0xffffffffu;
    used += (size_t)snprintf(line + used, sizeof(line) - used,
                             " money_value=%u rng_value=%08x", money, rng);
    for (r = 0; r < sizeof(kRegions) / sizeof(kRegions[0]); r++) {
        uint8_t digest[32];
        uint32_t j, value;
        unsigned k;
        int ok = 1;
        for (j = 0; j < kRegions[r].size; j++) {
            if (runtime_read(runtime, kRegions[r].address + j, 1, &value) != 0) {
                ok = 0;
                break;
            }
            buffer[j] = (uint8_t)value;
        }
        used += (size_t)snprintf(line + used, sizeof(line) - used, " %s=",
                                 kRegions[r].name);
        if (!ok) {
            used += (size_t)snprintf(line + used, sizeof(line) - used,
                                     "UNREADABLE@%08x", kRegions[r].address + j);
            continue;
        }
        file1_digest(buffer, kRegions[r].size, digest);
        for (k = 0; k < 32 && used + 3 < sizeof(line); k++) {
            line[used++] = file1_hex_digit(digest[k] >> 4);
            line[used++] = file1_hex_digit(digest[k] & 15u);
        }
        line[used] = '\0';
    }
    fprintf(stderr, "%s\n", line);
    fflush(stderr);
}

void PcPort_DetVblankBattleEnter(void) __attribute__((weak));
void PcPort_DetVblankBattleReturn(void) __attribute__((weak));

/* Verify the owner-supplied battle entry before the CPU is started. */
static int battle_entry_present(const uint8_t *entry)
{
    static const uint8_t expected[32] = {
        0x07, 0xef, 0xcf, 0xb5, 0xb6, 0xbb, 0x51, 0x0e,
        0xb6, 0x4f, 0xca, 0xe3, 0x2e, 0x1e, 0x38, 0x90,
        0x6c, 0xd8, 0xf6, 0xdf, 0x72, 0xb9, 0xf4, 0xbf,
        0x54, 0x49, 0xa2, 0x08, 0x43, 0x82, 0x25, 0x9c,
    };
    uint8_t digest[sizeof(expected)];
    file1_digest(entry, 4, digest);
    return memcmp(digest, expected, sizeof(expected)) == 0;
}

/* The opening encounter workspaces are allocated in one phase, then cleared
 * in a second phase. The remaining battle code consumes their guest pointers. */
static int battle_prepare_workspaces(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    static const struct { uint32_t slot, bytes; } workspace[] = {
        {0x800c3ea4u, 41652u}, {0x800d2d28u, 268u}, {0x800c3eacu, 760u}
    };
    const uint32_t resume = 0x80070fc8u;
    static const uint8_t expected[32] = {
        0xe6, 0xa2, 0x97, 0x14, 0x81, 0xcc, 0xb5, 0x3d,
        0xc0, 0xc5, 0x63, 0xb1, 0x55, 0xde, 0xe1, 0x87,
        0x53, 0xa3, 0x74, 0x50, 0xae, 0xc6, 0xfc, 0xcd,
        0xe1, 0x46, 0xad, 0x23, 0x08, 0x91, 0x47, 0xe1
    };
    uint8_t digest[sizeof(expected)];
    if (cpu->gpr[29] != BATTLE_STACK_TOP || cpu->gpr[31] != BATTLE_HALT_PC ||
        cpu->steps != 0 || cpu->next_pc != BATTLE_ENTRY + 4u || cpu->load_valid ||
        !overlay_leaf_host_ok("func_8008ABB8"))
        return 0;

    /* The wrapper checks the first word. This partial implementation depends
     * on the identity of the entire prefix through the last workspace clear. */
    file1_digest(PSX_ADDR(BATTLE_ENTRY), resume - BATTLE_ENTRY, digest);
    if (memcmp(expected, digest, sizeof(expected)) != 0)
        return 0;
    const ResolvedFunction *clear = find_function(runtime, 0x8003f8e8u);
    if (!clear || strcmp(clear->name, "bzero") != 0 ||
        xeno_port_is_generated_stub("func_8008ABB8"))
        return 0;
    void *(*allocate)(int32_t, int32_t) = dlsym(RTLD_DEFAULT, "battle_func_8008ABB8");
    if (!allocate)
        allocate = dlsym(RTLD_DEFAULT, "func_8008ABB8");
    if (!allocate)
        return 0;

    /* The rest of the encounter still runs with a guest stack. Construct its
     * o32 frame so its epilogue can restore the original caller. */
    const uint32_t frame = cpu->gpr[29] - 48u;
    for (unsigned i = 0; i < 4; ++i)
        if (runtime_write(runtime, frame + 24u + 4u * i, 4, cpu->gpr[16 + i]) != 0)
            return -1;
    if (runtime_write(runtime, frame + 40u, 4, cpu->gpr[31]) != 0)
        return -1;
    cpu->gpr[29] = frame;

    for (unsigned i = 0; i < sizeof(workspace) / sizeof(workspace[0]); ++i) {
        cpu->gpr[4] = workspace[i].bytes;
        cpu->gpr[5] = 0;
        uint32_t address = PsxMemory_GuestAddr(allocate(workspace[i].bytes, 0));
        if (runtime_write(runtime, workspace[i].slot, 4, address) != 0)
            return -1;
        cpu->gpr[2] = address;
    }
    cpu->gpr[3] = 0;  /* high word of the allocator's guest pointer result */
    for (unsigned i = 0; i < sizeof(workspace) / sizeof(workspace[0]); ++i) {
        if (runtime_read(runtime, workspace[i].slot, 4, &cpu->gpr[4]) != 0)
            return -1;
        cpu->gpr[5] = workspace[i].bytes;
        if (bridge_module_memory(runtime, cpu, clear) != 1)
            return -1;
    }

    /* State observed at the final void call's return. The ordinary bridge
     * already resumes at ra, so the next phase uses the usual dispatch. */
    cpu->gpr[1] = workspace[2].slot & 0xffff0000u;
    cpu->gpr[19] = 1;
    cpu->gpr[31] = resume;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}

/* Adopt only the ordinary field-selected record. An archive transition or a
 * scripted override can call other services before this copy takes place. */
static int battle_use_field_formation(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    const uint32_t entry = 0x80070fc8u, resume = 0x80071158u;
    const uint32_t active_record = 0x8006f9dcu;
    enum { record_bytes = 32, field_slots = 16 };
    static const uint8_t expected[32] = {
        0x1e, 0xc2, 0x51, 0x3d, 0x24, 0x69, 0x9d, 0x09,
        0x55, 0xaf, 0x4a, 0x53, 0x15, 0x8f, 0xd8, 0x88,
        0xe0, 0xf6, 0xb1, 0xc0, 0x16, 0x69, 0x20, 0x5f,
        0x7b, 0xbb, 0xd1, 0xdd, 0xcf, 0x89, 0xbc, 0xa6
    };
    if (cpu->next_pc != entry + 4 || cpu->gpr[31] != entry ||
        cpu->gpr[29] != BATTLE_STACK_TOP - 48 || cpu->gpr[28] != 0x80059170u ||
        cpu->gpr[19] != 1 || cpu->load_valid || !PcPort_BattleNativeLeafLimit())
        return 0;
    uint8_t digest[sizeof(expected)];
    file1_digest(PSX_ADDR(entry), resume - entry, digest);
    if (memcmp(digest, expected, sizeof(expected)) != 0)
        return 0;

    /* Read the field-owned selector and the resident settings through the
     * same bindings as the rest of the game. The boot sentinel pointer may
     * also point directly at the port's compact native data symbol. */
    enum { override, phase, slot, manager, sentinel_pointer, sentinel, reads };
    struct { uint32_t addr; unsigned size; } input[reads] = {
        [override] = {0x8005947cu, 1}, [phase] = {0x800594f8u, 1},
        [slot] = {0x80059508u, 1}, [manager] = {0x80062528u, 4},
        [sentinel_pointer] = {0x8005917cu, 4}, [sentinel] = {0, 4}
    };
    const uint8_t *input_value[reads];
    for (unsigned i = 0; i < reads; ++i) {
        if (i == sentinel) {
            uint32_t token = load_le(input_value[sentinel_pointer], 4);
            if (!token || (token & 3u) ||
                (!battle_resident_guest_pointer(token) &&
                 (uintptr_t)dlsym(RTLD_DEFAULT, "D_80010000") != token))
                return 0;
            input[i].addr = token;
        }
        input_value[i] = module_memory_pointer(runtime, input[i].addr, input[i].size, 0);
        if (!input_value[i]) return 0;
    }
    if (*input_value[override] || *input_value[phase] ||
        *input_value[slot] >= field_slots || load_le(input_value[sentinel], 4) != UINT32_MAX)
        return 0;

    /* These entry flags and the retained sound manager are written before
     * the original path chooses its record. A sentinel or selector aliasing a
     * write could change that choice. Validate every span before committing. */
    struct { uint32_t addr; unsigned size; uint32_t value; } initial[] = {
        {0x8005959cu, 1, 0}, {0x800c3e29u, 1, UINT8_MAX},
        {0x800c3e28u, 1, UINT8_MAX}, {0x800d366cu, 1, 0},
        {0x800c3e54u, 4, load_le(input_value[manager], 4)}
    };
    enum { writes = sizeof(initial) / sizeof(initial[0]) };
    uint8_t *output[writes];
    for (unsigned i = 0; i < writes; ++i) {
        output[i] = module_memory_pointer(runtime, initial[i].addr, initial[i].size, 1);
        if (!output[i]) return 0;
        for (unsigned j = 0; j < reads; ++j) {
            uintptr_t a = (uintptr_t)output[i], b = (uintptr_t)input_value[j];
            if (a <= b ? b - a < initial[i].size : a - b < input[j].size)
                return 0;
        }
    }

    uint32_t selected = 0x800658dcu + *input_value[slot] * record_bytes;
    const uint8_t *record = module_memory_pointer(runtime, selected, record_bytes, 0);
    uint8_t *destination = module_memory_pointer(runtime, active_record, record_bytes, 1);
    if (!record || destination != PSX_ADDR(active_record))
        return 0;
    for (unsigned i = 0; i < record_bytes; ++i)
        if (module_memory_pointer(runtime, selected + i, 1, 0) != record + i ||
            module_memory_pointer(runtime, active_record + i, 1, 1) != destination + i)
            return 0;
    const ResolvedFunction *move = find_function(runtime, 0x8003f99cu);
    if (!move || !move->host || strcmp(move->name, "memmove") != 0)
        return 0;

    for (unsigned i = 0; i < writes; ++i) {
        runtime_note_store(runtime, output[i], initial[i].size);
        store_le(output[i], initial[i].size, initial[i].value);
    }
    /* Supply the same guest ABI at the resident memmove boundary. Its typed
     * bridge preserves the guest destination as the return value. */
    cpu->gpr[1] = 0x800c0000u;
    cpu->gpr[2] = 0x800658dcu;
    cpu->gpr[3] = UINT32_MAX;
    cpu->gpr[4] = active_record;
    cpu->gpr[5] = selected;
    cpu->gpr[6] = record_bytes;
    cpu->gpr[31] = resume;
    return bridge_module_memory(runtime, cpu, move);
}

void func_80070F40(void)
{
    BattleMipsRuntime *runtime = &g_BattleRuntime;
    PcPortMipsCpu cpu;
    int rc;

    initialize_runtime(runtime);
    file1_invalidate(runtime);
    results_invalidate(runtime);
    runtime->file1.archive20_selected = 0;
    if (!battle_entry_present((const uint8_t *)PSX_ADDR(BATTLE_ENTRY))) {
        fprintf(stderr, "[xeno-port][battle-mips] expected battle image is missing\n");
        abort();
    }

    initialize_battle_entry_cpu(&cpu, runtime);
    g_ActiveBattleRuntime = runtime;
    fprintf(stderr,
            "[xeno-port][battle-mips] enter retail battle.bin at 0x%08x\n",
            BATTLE_ENTRY);
    /* TEST TOOLING: XENO_DETERMINISTIC_VBLANK_BATTLE (psyq_compat.c); inert
     * unset.  Weak: tests that link this TU without psyq_compat.c. */
    if (PcPort_DetVblankBattleEnter)
        PcPort_DetVblankBattleEnter();
    cpu_meter_begin_battle();
    cpu_meter_mark_entry(&cpu, BATTLE_ENTRY);
    if (battle_exit_hash_enabled()) {
        g_BattleExitHashBattle++;
        g_BattleExitHashResults = 0;
    }
    rc = PcPortMipsRun(&cpu, BATTLE_ENTRY, BATTLE_HALT_PC,
                       battle_step_limit());
    if (PcPort_DetVblankBattleReturn)
        PcPort_DetVblankBattleReturn();
    cpu_meter_end_battle(rc == PC_PORT_MIPS_HALTED ? "returned" : "stopped");
    if (g_BattleExitHash > 0)
        battle_exit_hash_print(runtime, rc == PC_PORT_MIPS_HALTED ? "returned" : "stopped");
    g_ActiveBattleRuntime = NULL;
    file1_invalidate(runtime);
    results_invalidate(runtime);
    runtime->file1.archive20_selected = 0;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr,
                "[xeno-port][battle-mips] stopped after %llu instructions: "
                "%s\n", (unsigned long long)cpu.steps, cpu.error);
        abort();
    }
    fprintf(stderr,
            "[xeno-port][battle-mips] retail battle returned after %llu "
            "instructions\n", (unsigned long long)cpu.steps);
}

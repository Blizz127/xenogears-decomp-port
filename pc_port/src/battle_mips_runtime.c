#define _GNU_SOURCE

#include "god_mode.h"
/* Host hooks: weak so the tests that #include/link this runtime without the
 * port's god_mode.c / game_overrides.c still link. */
#pragma weak PcPort_GodModeBeforeGuest
#include "battle_mips_adapter.h"
#include "battle_mips_runtime_internal.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "PsyX/PsyX_public.h"
#include "../extern/PsyCross/src/gpu/PsyX_GPU.h"

#include <dlfcn.h>
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
    int trace_calls;
    /* Verification hook, off in production. When set, overlay targets are never
     * adopted: every guest call resolves to the interpreter. The differential
     * harness sets it to run the retail bytes for a function and then the host
     * C body on identical guest RAM and compare the two. */
    int force_interpret;
    BattleFile1Identity file1;
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
}

static int host_address_valid(BattleMipsRuntime *runtime, uintptr_t address,
                              unsigned width, int write)
{
    size_t i;
    for (i = 0; i < runtime->host_range_count; i++) {
        const HostRange *range = &runtime->host_ranges[i];
        if (address >= range->begin && address + width <= range->end &&
            (!write || range->writable))
            return 1;
    }
    return 0;
}

static void initialize_runtime(BattleMipsRuntime *runtime)
{
    size_t i;

    if (runtime->initialized)
        return;
    runtime->trace_calls = getenv("XENO_BATTLE_MIPS_TRACE") != NULL;
    load_host_ranges(runtime);
    /* The call cache stores resolved entries for the table built below. */
    memset(runtime->call_cache, 0, sizeof(runtime->call_cache));

    for (i = 0; i < g_BattleBridgeSymbolCount; i++) {
        const PcPortBattleSymbol *symbol = &g_BattleBridgeSymbols[i];
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
    qsort(runtime->functions, runtime->function_count,
          sizeof(runtime->functions[0]), function_compare);
    qsort(runtime->data, runtime->data_count,
          sizeof(runtime->data[0]), data_compare);
    runtime->initialized = 1;
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

static uint8_t *resolve_memory(BattleMipsRuntime *runtime, uint32_t address,
                               unsigned width, int write)
{
    uintptr_t host;
    uint32_t canonical = address;

    if ((address & 0xfffff000u) == 0x1f800000u) {
        unsigned offset = address & 0xfffu;
        if (offset + width > 4096u)
            return NULL;
        return g_PsxScratchpad + offset;
    }

    if ((address & 0xff800000u) == 0x80000000u ||
        (address & 0xff800000u) == 0xa0000000u) {
        /* KSEG0 and KSEG1 are cached/uncached aliases of the same physical
         * RAM.  The retail symbol table is KSEG0-shaped, so canonicalize the
         * KSEG1 form before resolving a native shared global. */
        if ((address & 0xff800000u) == 0xa0000000u)
            canonical = address - 0x20000000u;
        if ((canonical & 0x1fffffffu) <
            (BATTLE_BASE & 0x1fffffffu)) {
            ResolvedData *entry = find_data(runtime, canonical, width);
            if (entry != NULL)
                return entry->host + (canonical - entry->address);
            /* Also flag data accesses into main-exe .text: the port never
             * copies that range into guest RAM (functions are bridged), so
             * a guest reading a table that the compiler placed there sees
             * zeros. */
            if (canonical >= PSX_EXE_SDATA_START ||
                (canonical >= PSX_EXE_TEXT_START &&
                 canonical < PSX_EXE_TEXT_END))
                note_unshared_access(canonical, width, write);
        }
        return (uint8_t *)PSX_ADDR(canonical);
    }

    host = (uintptr_t)address;
    if (host_address_valid(runtime, host, width, write))
        return (uint8_t *)host;
    return NULL;
}

static int runtime_read(void *opaque, uint32_t address, unsigned width,
                        uint32_t *value)
{
    BattleMipsRuntime *runtime = opaque;
    uint8_t *p = resolve_memory(runtime, address, width, 0);
    if (p == NULL)
        return -1;
    *value = load_le(p, width);
    return 0;
}

static int runtime_write(void *opaque, uint32_t address, unsigned width,
                         uint32_t value)
{
    BattleMipsRuntime *runtime = opaque;
    uint8_t *p = resolve_memory(runtime, address, width, 1);
    if (p == NULL)
        return -1;
    if (runtime->file1.verified && (uintptr_t)p + width > (uintptr_t)g_PsxRam + 0x1e5000u &&
        (uintptr_t)p < (uintptr_t)g_PsxRam + 0x1e9b5cu)
        file1_invalidate(runtime);
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

/* Overlay C adopted onto the host. Pointer-only leaves plus bodies that
 * touch overlay D_* through battle_overlay_guest_ram.h (guest RAM, not a
 * second host copy). Nested guest pointers loaded from structs stay in
 * the interpreter until they grow an explicit translator. */
static int overlay_leaf_host_ok(const char *name)
{
    static const char *const kLeaves[] = {
#include "battle_overlay_host_leaves.inc"
    };
    size_t i;
    for (i = 0; i < sizeof(kLeaves) / sizeof(kLeaves[0]); i++) {
        if (strcmp(name, kLeaves[i]) == 0)
            return 1;
    }
    return 0;
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

/* LoadImage(RECT* rect, u_long* p): both arguments are guest pointers, and the
 * pixel source may be a *low physical* RAM address -- the battle overlay passes
 * 0x1000 -- which translate_argument leaves alone on purpose because it is
 * shared by scalar arguments.  Translate both explicitly here; otherwise
 * PsyCross's GR_CopyVRAM memmoves from host address 0x1000 and SIGSEGVs
 * (observed live: LoadImage -> GR_CopyVRAM(src=0x1000, w=19088)). */
static int bridge_load_image(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                             void *host_function)
{
    int (*load_image)(void *, void *) = (int (*)(void *, void *))host_function;
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

    if (target == 0x801e6ce8u) {
        int adopted = file1_try_controller(runtime, cpu);
        if (adopted != 0) return adopted;
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
    file1_before_archive(runtime, cpu, resolved->address, &file1_load_candidate);
    function = (GenericHostFunction)resolved->host;
    result = function(args[0], args[1], args[2], args[3],
                      args[4], args[5], args[6], args[7],
                      args[8], args[9], args[10], args[11]);
    file1_after_archive(runtime, cpu, resolved->address, result, file1_load_candidate);
    if (resolved->address == 0x80031bdcu) {
        /* HeapAlloc returns native RAM pointers. Retail 80070E54..6C
         * subtracts a fixed guest address from v0 to reserve overlay space,
         * so restore the guest pointer before retail uses or stores it. */
        result = PsxMemory_GuestAddr((void *)result);
    }
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
    runtime->bridge_cpu = cpu;
    {
        void (*volatile god_hook)(PcPortMipsCpu*, uint32_t) = PcPort_GodModeBeforeGuest;
        if (god_hook)
            god_hook(cpu, target);
    }
    int result = runtime_bridge_call(opaque, cpu, target);
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
    caller = runtime->bridge_cpu;
    sp = caller->gpr[29];
    /* Retail file-1 801E6CE8 starts with ADDIU SP,SP,-0x50. Its native
     * replacement will lack that guest frame, including the o32 arg slots.
     * Validate before subtracting or resolving any addresses. */
    if ((sp & 7u) != 0 || sp < 0x80000050u || sp > BATTLE_STACK_TOP)
        return -1;
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

static int run_guest_callback(BattleMipsRuntime *runtime, uint32_t callback,
                              void *argument)
{
    PcPortMipsCpu cpu;
    int rc;

    initialize_cpu(&cpu, runtime);
    /* A native work-list pump can call back while an outer guest frame is
     * still live. Continue below that caller's SP, as a normal nested call
     * does, instead of restarting at the top and overwriting saved locals/RA.
     * The bridge wrapper restores the parent CPU across deeper callbacks. */
    if (runtime->bridge_cpu != NULL)
        cpu.gpr[29] = runtime->bridge_cpu->gpr[29];
    cpu.gpr[4] = (uint32_t)(uintptr_t)argument;
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
int PcPort_BattleMipsDispatchCallback(uint32_t callback, void *argument)
{
    if (g_ActiveBattleRuntime == NULL)
        return 0;
    if (target_is_guest_code(callback)) {
        if (run_guest_callback(g_ActiveBattleRuntime, callback, argument) != 0)
            abort();
        return 1;
    }
    return run_main_exe_callback(g_ActiveBattleRuntime, callback, argument);
}

/* Native main-executable sprite code re-enters the currently loaded battle
 * overlay here (retail SLUS call at 800248FC). Keep its actual interpreter
 * and state; never substitute the field animation interpreter or a no-op. */
void func_800C11CC(void* sprite)
{
    if (!PcPort_BattleMipsDispatchCallback(0x800c11ccu, sprite)) {
        fputs("[xeno-port][battle-mips] sprite animation called without an active retail battle overlay\n", stderr);
        abort();
    }
}

/* The adapter includes the same decomp-owned controller body as the PSX
 * build; its bindings and identity gate remain private to this runtime. */
#include "battle_file1_controller.inc"

void func_80070F40(void)
{
    BattleMipsRuntime *runtime = &g_BattleRuntime;
    PcPortMipsCpu cpu;
    uint32_t first;
    int rc;

    initialize_runtime(runtime);
    file1_invalidate(runtime);
    runtime->file1.archive20_selected = 0;
    first = load_le((const uint8_t *)PSX_ADDR(BATTLE_ENTRY), 4);
    if (first != 0x27bdffd0u) {
        fprintf(stderr,
                "[xeno-port][battle-mips] retail battle image missing: "
                "entry word=%08x expected=27bdffd0\n", first);
        abort();
    }

    initialize_cpu(&cpu, runtime);
    g_ActiveBattleRuntime = runtime;
    fprintf(stderr,
            "[xeno-port][battle-mips] enter retail battle.bin at 0x%08x\n",
            BATTLE_ENTRY);
    rc = PcPortMipsRun(&cpu, BATTLE_ENTRY, BATTLE_HALT_PC,
                       battle_step_limit());
    g_ActiveBattleRuntime = NULL;
    file1_invalidate(runtime);
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

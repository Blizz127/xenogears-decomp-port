/* The native packed task adapter versus retail Allocate/Add/Remove/Delete.
 * Heap calls are deterministic boundary doubles, not game allocation data. */
#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct WorkListEntry WorkListEntry;
#define WL_U32(pointer) ((u32)(uintptr_t)(pointer))
u8 g_PsxRam[PSX_RAM_SIZE];
s32 D_80059190, g_NumTimerWorkListEntries, g_NumWorkListEntries, g_WorkListCurTimer;
WorkListEntry *D_800594C0, *g_TimerWorkList, *D_80059590, *g_WorkList;
short D_80059494;
extern s32 D_80059184, D_80059464;
extern u8 D_800591AC, D_800591AF;
extern WorkListEntry *TimerWorkListAllocateTask(void *, s32) __attribute__((weak));
extern void TimerWorkListDeleteTask(WorkListEntry *) __attribute__((weak));
extern void *func_8001D0A4(void *, void *) __attribute__((weak));
extern void *func_8001D164(void *) __attribute__((weak));

#define AREA 0x80100000u
#define NODE 0x80100020u
#define OWNER 0x80100200u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu
static unsigned allocations, frees;
static u32 allocated_size, allocated_flags, freed_pointer;

void *HeapAlloc(u_int size, u_int flags)
{
    allocations++;
    allocated_size = size;
    allocated_flags = flags;
    return PSX_ADDR(NODE);
}
u_int HeapFree(void *pointer)
{
    frees++;
    freed_pointer = (uintptr_t)pointer;
    return 17;
}

static u8 *address_bytes(u32 address, unsigned width)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (address >= base && (uint64_t)address + width <= base + sizeof(g_PsxRam))
        return (u8 *)(uintptr_t)address;
    if ((address < 0x200000u && (uint64_t)address + width <= 0x200000u) ||
        (address >= 0x80000000u && (uint64_t)address + width <= 0x80200000u))
        return PSX_ADDR(address);
    return NULL;
}
static int read_bus(void *context, u32 address, unsigned width, u32 *value)
{
    (void)context;
    const u8 *p = address_bytes(address, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; i++) *value |= (u32)p[i] << (i * 8);
    return 0;
}
static int write_bus(void *context, u32 address, unsigned width, u32 value)
{
    (void)context;
    u8 *p = address_bytes(address, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; i++) p[i] = value >> (i * 8);
    return 0;
}
static u32 word(u32 address)
{ u32 value; assert(read_bus(NULL, address, 4, &value) == 0); return value; }
static void put(u32 address, u32 value)
{ assert(write_bus(NULL, address, 4, value) == 0); }
static int heap_bridge(void *context, PcPortMipsCpu *cpu, u32 target)
{
    (void)context;
    if (target == 0x80031bdcu) {
        cpu->gpr[2] = (uintptr_t)HeapAlloc(cpu->gpr[4], cpu->gpr[5]);
        return 1;
    }
    if (target == 0x800320e8u) {
        cpu->gpr[2] = HeapFree((void *)(uintptr_t)cpu->gpr[4]);
        return 1;
    }
    return 0;
}
static u32 retail(u32 entry, u32 a, u32 b)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = heap_bridge};
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a; cpu.gpr[5] = b;
    cpu.gpr[28] = 0x80059170u; cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    if (PcPortMipsRun(&cpu, entry, HALT, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "TIMER WORK LIST retail %08x failed: %s\n", entry, cpu.error);
        assert(0);
    }
    return cpu.gpr[2];
}

typedef struct Snapshot {
    u32 id, count, marked, head, cursor;
    u32 allocs, size, flags, free_calls, free_pointer;
    u8 bytes[0x240];
} Snapshot;

static Snapshot snapshot(int native)
{
    Snapshot state;
    memset(&state, 0, sizeof(state));
    state.id = native ? (u32)D_80059184 : word(0x80059184u);
    state.count = native ? (u32)g_NumTimerWorkListEntries : word(0x80059188u);
    state.marked = native ? (u32)D_80059464 : word(0x80059464u);
    state.head = native ? WL_U32(g_TimerWorkList) : word(0x8005958cu);
    state.cursor = native ? WL_U32(D_80059590) : word(0x80059590u);
    state.allocs = allocations; state.size = allocated_size; state.flags = allocated_flags;
    state.free_calls = frees; state.free_pointer = freed_pointer;
    memcpy(state.bytes, PSX_ADDR(AREA), sizeof(state.bytes));
    if (native) {
        /* Only the callback address differs between equivalent executable
         * domains. Require the actual native delete owner before normalizing. */
        assert(word(NODE + 0xc) == WL_U32(TimerWorkListDeleteTask));
        u32 callback = 0x8001ce44u;
        memcpy(state.bytes + NODE - AREA + 0xc, &callback, 4);
    }
    return state;
}

static void reset(unsigned flags, unsigned fill, u32 id, int marked, int linked, unsigned deletion)
{
    allocations = frees = allocated_size = allocated_flags = freed_pointer = 0;
    memset(PSX_ADDR(AREA), fill, 0x240);
    put(OWNER + 0x10, 0xfabc0123u);
    put(0x10, 0xb56789abu); /* Retail null-owner KUSEG RAM read. */
    put(NODE + 0x80 + 0x18, (uintptr_t)PSX_ADDR(NODE + 0x100));
    put(NODE + 0x100 + 0x18, 0);
    D_80059184 = (s32)id;
    D_800591AC = marked;
    D_800591AF = flags;
    D_80059464 = 9;
    g_NumTimerWorkListEntries = linked ? 2 : 0;
    g_TimerWorkList = linked ? PSX_ADDR(NODE + 0x80) : NULL;
    D_80059590 = PSX_ADDR(NODE + deletion * 0x80);
    put(0x80059184u, id);
    put(0x80059188u, g_NumTimerWorkListEntries);
    put(0x80059464u, D_80059464);
    put(0x8005958cu, WL_U32(g_TimerWorkList));
    put(0x80059590u, WL_U32(D_80059590));
    assert(write_bus(NULL, 0x800591acu, 1, D_800591AC) == 0);
    assert(write_bus(NULL, 0x800591afu, 1, D_800591AF) == 0);
}

static int compare_case(unsigned flags, unsigned fill, u32 id, int marked,
                        int linked, unsigned deletion, s32 size, int null_owner)
{
    WorkListEntry *owner = null_owner ? NULL : PSX_ADDR(OWNER);
    WorkListEntry *target = PSX_ADDR(NODE + deletion * 0x80);
    reset(flags, fill, id, marked, linked, deletion);
    u32 expected_pointer = retail(0x8001cd08u, WL_U32(owner), size);
    Snapshot allocated = snapshot(0);
    retail(0x8001ce44u, WL_U32(target), 0);
    Snapshot deleted = snapshot(0);
    reset(flags, fill, id, marked, linked, deletion);
    WorkListEntry *result = TimerWorkListAllocateTask(owner, size);
    Snapshot actual = snapshot(1);
    if (WL_U32(result) != expected_pointer || memcmp(&actual, &allocated, sizeof(actual))) {
        fprintf(stderr, "TIMER WORK LIST allocation mismatch flags=%u id=%08x\n", flags, id);
        return 0;
    }
    TimerWorkListDeleteTask(target);
    actual = snapshot(1);
    if (memcmp(&actual, &deleted, sizeof(actual))) {
        fprintf(stderr, "TIMER WORK LIST delete mismatch target=%u linked=%d\n", deletion, linked);
        return 0;
    }
    return 1;
}

static int check_lookup(void)
{
    if (!func_8001D0A4) {
        fputs("TIMER LOOKUP FAIL missing native owner\n", stderr);
        return 0;
    }
    const u32 callbacks[] = {0, 0x800bcbb4u, 0x800bcb54u, 0x00412340u, 0x800bcfacu};
    const unsigned callback_count = sizeof(callbacks) / sizeof(*callbacks);
    const u32 ids[] = {0, 1, 0x1fffffffu};
    unsigned cases = 0;
    for (unsigned length = 0; length <= 3; length++)
    for (unsigned match = 0; match <= 3; match++)
    for (unsigned id = 0; id < 3; id++)
    for (unsigned flags = 0; flags < 64; flags++)
    for (unsigned cb = 0; cb < callback_count; cb++)
    for (unsigned null_owner = 0; null_owner < 2; null_owner++, cases++) {
        u8 before[0x240];
        const u32 owner = null_owner ? 0 : WL_U32(PSX_ADDR(OWNER));
        memset(PSX_ADDR(AREA), 0xa5, sizeof(before));
        put((null_owner ? 0 : OWNER) + 0x10,
            ids[id] | ((flags & 7u) << 29));
        for (unsigned n = 0; n < length; n++) {
            const u32 node = NODE + n * 0x40;
            /* Nonmatching nodes individually exercise each predicate. */
            put(node, n == match || n != 0 ? owner : WL_U32(PSX_ADDR(OWNER + 0x20)));
            put(node + 8, n == match || n != 2 ? callbacks[cb] : callbacks[(cb + 1) % callback_count]);
            put(node + 0x14, (n == match || n != 1 ? ids[id] : ids[id] ^ 1u)
                | ((flags >> 3) << 29));
            put(node + 0x18, n + 1 < length ? WL_U32(PSX_ADDR(node + 0x40)) : 0);
        }
        g_TimerWorkList = length ? PSX_ADDR(NODE) : NULL;
        const u32 head = WL_U32(g_TimerWorkList);
        put(0x8005958cu, head);
        const u32 null_owner_id = word(0x10);
        memcpy(before, PSX_ADDR(AREA), sizeof(before));
        const u32 expected = retail(0x8001d0a4u, owner, callbacks[cb]);
        const u32 actual = WL_U32(func_8001D0A4((void *)(uintptr_t)owner,
            (void *)(uintptr_t)callbacks[cb]));
        if (actual != expected || memcmp(before, PSX_ADDR(AREA), sizeof(before)) ||
            word(0x10) != null_owner_id ||
            word(0x8005958cu) != head || WL_U32(g_TimerWorkList) != head) {
            fprintf(stderr, "TIMER LOOKUP FAIL case=%u got=%08x expected=%08x\n",
                cases, actual, expected);
            return 0;
        }
    }
    /* Two valid entries must return the first; the matrix above has at most
     * one valid entry per list. Check the tie independently. */
    put(OWNER + 0x10, 7);
    for (unsigned n = 0; n < 2; n++) {
        const u32 node = NODE + n * 0x40;
        put(node, WL_U32(PSX_ADDR(OWNER)));
        put(node + 8, 0x800bcfacu);
        put(node + 0x14, 7 | (n << 29));
        put(node + 0x18, n == 0 ? WL_U32(PSX_ADDR(NODE + 0x40)) : 0);
    }
    g_TimerWorkList = PSX_ADDR(NODE);
    put(0x8005958cu, WL_U32(g_TimerWorkList));
    if (retail(0x8001d0a4u, WL_U32(PSX_ADDR(OWNER)), 0x800bcfacu) != WL_U32(PSX_ADDR(NODE)) ||
        func_8001D0A4(PSX_ADDR(OWNER), (void *)(uintptr_t)0x800bcfacu) != PSX_ADDR(NODE)) {
        fputs("TIMER LOOKUP FAIL duplicate first-match order\n", stderr);
        return 0;
    }
    cases++;
    printf("TIMER LOOKUP RETAIL PASS cases=%u, owner/id/callback/flags/list/guards\n", cases);
    return 1;
}

static int check_callback_lookup(void)
{
    if (!func_8001D164) {
        fputs("TIMER CALLBACK LOOKUP FAIL missing native owner\n", stderr);
        return 0;
    }
    const u32 callbacks[] = {0, 0x800bcbb4u, 0x800bcb54u,
                            0x00412340u, 0x800c11ccu, 0xa00bcbb4u};
    unsigned cases = 0;
    for (unsigned length = 0; length <= 4; length++)
    for (unsigned matches = 0; matches < 16; matches++)
    for (unsigned cb = 0; cb < sizeof(callbacks)/sizeof(*callbacks); cb++) {
        u8 before[0x240];
        memset(PSX_ADDR(AREA), 0xa5, sizeof(before));
        for (unsigned n = 0; n < length; n++) {
            const u32 node = NODE + n * 0x40;
            put(node + 8, matches & (1u << n) ? callbacks[cb] : callbacks[cb] ^ 4u);
            put(node + 0x18, n + 1 < length ? WL_U32(PSX_ADDR(node + 0x40)) : 0);
        }
        g_TimerWorkList = length ? PSX_ADDR(NODE) : NULL;
        const u32 head = WL_U32(g_TimerWorkList);
        put(0x8005958cu, head);
        memcpy(before, PSX_ADDR(AREA), sizeof(before));
        const u32 expected = retail(0x8001d164u, callbacks[cb], 0);
        const u32 actual = WL_U32(func_8001D164((void *)(uintptr_t)callbacks[cb]));
        if (actual != expected || memcmp(before, PSX_ADDR(AREA), sizeof(before)) ||
            WL_U32(g_TimerWorkList) != head || word(0x8005958cu) != head) {
            fprintf(stderr, "TIMER CALLBACK LOOKUP FAIL case=%u got=%08x expected=%08x\n",
                    cases, actual, expected);
            return 0;
        }
        ++cases;
    }
    printf("TIMER CALLBACK LOOKUP RETAIL PASS cases=%u empty/miss/first/multiple/identity/guards\n", cases);
    return 1;
}

int main(void)
{
    if (!TimerWorkListAllocateTask || !TimerWorkListDeleteTask) {
        fprintf(stderr, "TIMER WORK LIST FAIL missing native allocate/delete owner\n");
        return 1;
    }
    assert((uintptr_t)g_PsxRam + sizeof(g_PsxRam) < UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && fseek(image, 0x8001cc18u - 0x8000f800u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x8001cc18u), 1, 0x25c, image) == 0x25c);
    assert(fseek(image, 0x8001d0a4u - 0x8000f800u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x8001d0a4u), 1, 0x68, image) == 0x68);
    assert(fseek(image, 0x8001d164u - 0x8000f800u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x8001d164u), 1, 0x38, image) == 0x38);
    fclose(image);
    unsigned cases = 0;
    for (unsigned flags = 0; flags < 256; flags++, cases++)
        if (!compare_case(flags, flags, flags, flags & 1, 1, flags % 4, 0x78, flags & 1)) return 1;
    const s32 sizes[] = {0, -1, -28, INT32_MIN, INT32_MAX};
    const u32 ids[] = {0, 0x1fffffffu, 0x7fffffffu, 0xffffffffu};
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(*sizes); i++)
        for (unsigned j = 0; j < sizeof(ids) / sizeof(*ids); j++)
            for (unsigned k = 0; k < 16; k++, cases++)
                if (!compare_case(0xff, k & 1 ? 0xff : 0, ids[j], k & 1,
                                  (k >> 1) & 1, k >> 2, sizes[i], k & 1)) return 1;
    printf("TIMER WORK LIST RETAIL PASS cases=%u, allocation/list bytes/counters/delete/guards\n", cases);
    return check_lookup() && check_callback_lookup() ? 0 : 1;
}

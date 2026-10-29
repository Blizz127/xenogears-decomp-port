/* Fixed synthetic work-list unlink cases against retail CPU. One controlled
 * callback frees a synthetic block; arbitrary callbacks remain unproven. */
#include "slus_oracle.h"
typedef struct WorkListEntry WorkListEntry;
extern void func_8001CE74(WorkListEntry *);
extern void WorkListRemoveTask(WorkListEntry *);
WorkListEntry *g_WorkList, *g_TimerWorkList, *D_800594C0, *D_80059590;
int32_t g_NumWorkListEntries, g_NumTimerWorkListEntries, g_WorkListCurTimer, D_80059190;
short D_80059494;
extern unsigned HeapFree(void *p);
extern void HeapInit(void *, void *);
extern void *HeapAlloc(unsigned, unsigned);
extern uint32_t g_HeapNeedsConsolidation, g_HeapIsErrorHandlerOff;
extern uint32_t g_HeapLastAllocSize;
extern uint16_t g_HeapCurContentType, g_HeapCurUser;
extern void *g_Heap;
__attribute__((noreturn)) void MainLoop(int error) { (void)error; abort(); }
void func_8003747C(void *p) { (void)p; }
void func_8001DAE8(void *a, u16 b, u32 c) { (void)a; (void)b; (void)c; abort(); }
void func_800234AC(void *a) { (void)a; abort(); }
void func_8001F8E8(void *a, u16 b, u32 c) { (void)a; (void)b; (void)c; abort(); }
u8 D_8005A474[16], D_8006BE10[16];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
uint8_t g_PsxScratchpad[4096];

static const Glob globals[] = {
    { "work head", &g_WorkList, 0x80059594u, 8, 1 },
    { "timer head", &g_TimerWorkList, 0x8005958cu, 8, 1 },
    { "current cursor", &D_80059590, 0x80059590u, 8, 1 },
    { "work count", &g_NumWorkListEntries, 0x8005918cu, 4, 0 },
    { "heap consolidation", &g_HeapNeedsConsolidation, 0x8005932cu, 4, 0 },
    { "heap error handler", &g_HeapIsErrorHandlerOff, 0x80059330u, 4, 0 },
    { "heap root", &g_Heap, 0x80059320u, 8, 1 },
    { "heap content", &g_HeapCurContentType, 0x80059318u, 2, 0 },
    { "heap user", &g_HeapCurUser, 0x8005931cu, 2, 0 },
    { "last allocation size", &g_HeapLastAllocSize, 0x8005933cu, 4, 0 },
};
static int native_phase, reentrant_case, mutate_next_case, free_case, overwrite_freed_case, realloc_case, default_remove_case;
static void record_callback(uint32_t arg)
{
    uint32_t work = native_phase ? (uint32_t)(uintptr_t)g_WorkList : g32(0x80059594u);
    uint32_t timer = native_phase ? (uint32_t)(uintptr_t)g_TimerWorkList : g32(0x8005958cu);
    uint32_t cursor = native_phase ? (uint32_t)(uintptr_t)D_80059590 : g32(0x80059590u);
    spy(0x8006de00u, arg, work, timer, cursor, 0, 0);
    put32((uint8_t *)(uintptr_t)arg + 4, 0x51425364u);
    if (mutate_next_case) put32((uint8_t *)(uintptr_t)arg + 0x18, 0);
}
static int callback_bridge(PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x8006de00u) return 0;
    record_callback(cpu->gpr[4]);
    if (default_remove_case) {
        PcPortMipsCpu nested;
        PcPortMipsCpuInit(&nested, &cpu->bus);
        nested.gpr[4]=cpu->gpr[4];nested.gpr[28]=cpu->gpr[28];nested.gpr[29]=cpu->gpr[29]-0x200u;nested.gpr[31]=HALT_PC;
        assert(PcPortMipsRun(&nested,0x8001cb48u,HALT_PC,10000)==PC_PORT_MIPS_HALTED);
    }
    if (reentrant_case) {
        PcPortMipsCpu nested;
        PcPortMipsCpuInit(&nested, &cpu->bus);
        nested.gpr[4] = lo_addr(0x180);
        nested.gpr[28] = cpu->gpr[28];
        nested.gpr[29] = cpu->gpr[29] - 0x200u;
        nested.gpr[31] = HALT_PC;
        assert(PcPortMipsRun(&nested, 0x8001ce74u, HALT_PC, 10000) == PC_PORT_MIPS_HALTED);
        spy(0x8006de04u, g32(0x80059594u), g32(0x80059590u), 0, 0, 0, 0);
    }
    if (free_case) {
        PcPortMipsCpu nested;
        PcPortMipsCpuInit(&nested, &cpu->bus);
        nested.gpr[4] = cpu->gpr[4]; nested.gpr[28] = cpu->gpr[28];
        nested.gpr[29] = cpu->gpr[29] - 0x200u; nested.gpr[31] = HALT_PC;
        assert(PcPortMipsRun(&nested, 0x800320e8u, HALT_PC, 10000) == PC_PORT_MIPS_HALTED);
        if (realloc_case) {
            PcPortMipsCpu allocation;
            PcPortMipsCpuInit(&allocation, &cpu->bus);
            allocation.gpr[4] = 0x40; allocation.gpr[5] = 0; allocation.gpr[28] = cpu->gpr[28];
            allocation.gpr[29] = cpu->gpr[29] - 0x200u; allocation.gpr[31] = HALT_PC;
            assert(PcPortMipsRun(&allocation, 0x80031bdcu, HALT_PC, 100000) == PC_PORT_MIPS_HALTED);
            assert(allocation.gpr[2] == cpu->gpr[4]);
            put32((uint8_t *)(uintptr_t)allocation.gpr[2] + 0x18, 0);
        }
        if (overwrite_freed_case) put32((uint8_t *)(uintptr_t)cpu->gpr[4] + 0x18, 0);
    }
    return 1;
}
int PcPort_BattleMipsDispatchCallback(u32 callback, void *argument)
{
    if (callback != 0x8006de00u) return 0;
    record_callback((uint32_t)(uintptr_t)argument);
    if (default_remove_case) WorkListRemoveTask((WorkListEntry *)argument);
    if (reentrant_case) {
        func_8001CE74((WorkListEntry *)(uintptr_t)lo_addr(0x180));
        spy(0x8006de04u, (uint32_t)(uintptr_t)g_WorkList,
            (uint32_t)(uintptr_t)D_80059590, 0, 0, 0, 0);
    }
    if (free_case) {
        assert(HeapFree(argument) == 0);
        if (realloc_case) {
            void *reused = HeapAlloc(0x40, 0);
            assert(reused == argument);
            put32((uint8_t *)reused + 0x18, 0);
        }
        if (overwrite_freed_case) put32((uint8_t *)argument + 0x18, 0);
    }
    return 1;
}
static void entry(unsigned off, uint32_t owner, uint32_t id, uint32_t flags, uint32_t next)
{
    put32(lo(off), owner);
    put32(lo(off + 0x0c), 0); /* no callback */
    put32(lo(off + 0x14), id | flags);
    put32(lo(off + 0x18), next);
}
/* Native debug caller addresses are intentionally zero-based on this port;
 * normalize only the packed source-address field in this synthetic case. */
static void normalize_retail_caller(uint8_t *snapshot)
{
    uint32_t meta; memcpy(&meta, snapshot + 0x404, 4);
    meta = (meta & ~0x1fffffu) | 0x1ffffeu;
    memcpy(snapshot + 0x404, &meta, 4);
}
static void setup(uint32_t which)
{
    uint32_t owner = lo_addr(0x100), a = lo_addr(0x200), b = lo_addr(0x220), c = lo_addr(0x240);
    put32(lo(0x110), 0x12345u);
    entry(0x200, owner, 0x12345u, which == 2 ? 0x40000000u : 0, b);
    entry(0x220, owner, which == 3 ? 0x12346u : 0x12345u, 0, c);
    entry(0x240, lo_addr(0x180), 0x12345u, 0, 0);
    g_WorkList = (WorkListEntry *)(uintptr_t)a;
    g_TimerWorkList = NULL;
    if (which == 4 || which == 6) { g_WorkList = NULL; g_TimerWorkList = (WorkListEntry *)(uintptr_t)a; }
    if (which >= 5) {
        put32(lo(0x20c), 0x8006de00u);
        if (which == 5) put32(lo(0x22c), 0x8006de00u);
    }
    if (which == 7) {
        put32(lo(0x190), 0x54321u);
        entry(0x220, lo_addr(0x180), 0x54321u, 0, c);
        entry(0x240, lo_addr(0x1c0), 0x12345u, 0, 0);
    }
    if (which == 11) {
        HeapInit(lo(0x400), lo(0x800));
        void *first = HeapAlloc(0x40, 0);
        assert(first == lo(0x408));
        a = (uint32_t)(uintptr_t)first;
        g_WorkList = (WorkListEntry *)(uintptr_t)a;
        put32((uint8_t *)first, owner);
        put32((uint8_t *)first + 0x0c, 0x8006de00u);
        put32((uint8_t *)first + 0x14, 0x12345u);
        put32((uint8_t *)first + 0x18, b);
        retail_norm = normalize_retail_caller;
    } else retail_norm = NULL;
    if (which >= 9 && which <= 11) {
        put32(lo(0x1f8), lo_addr(0x280));
        put32(lo(0x1fc), 0x1234u | (3u << 21) | (5u << 26));
    }
    D_80059590 = (WorkListEntry *)(uintptr_t)(which == 3 ? b : a);
    g_NumWorkListEntries = 3;
    if (which == 12) {
        entry(0x220,lo_addr(0x180),0x12345u,0,c);
        put32(lo(0x20c),0x8006de00u);
    }
    native_phase = 0;
    reentrant_case = which == 7;
    mutate_next_case = which == 8;
    free_case = which >= 9 && which <= 11;
    default_remove_case = which == 12;
    overwrite_freed_case = which == 10;
    realloc_case = which == 11;
    g_HeapNeedsConsolidation = 0;
    g_HeapIsErrorHandlerOff = 1;
    A[0] = owner;
}
static uint32_t native_unlink(void) { native_phase = 1; func_8001CE74((WorkListEntry *)(uintptr_t)A[0]); return 0; }
int main(void)
{
    const Fn f = { "func_8001CE74_no_callback", 0x8001ce74u, 0, setup, native_unlink };
    cur_what = "POPUP CLEANUP WORK LIST";
    map_low(); load_slus(cur_what); globs = globals; nglobs = sizeof(globals)/sizeof(globals[0]);
    user_bridge = callback_bridge;
    for (unsigned i = 1; i <= 12; ++i) {
        if (run_case(&f, i, 0)) return 1;
        if (i == 5) {
            assert(ntrace == 2 && trace[0].a[0] == lo_addr(0x200));
            assert(trace[0].a[1] == lo_addr(0x220) && trace[0].a[3] == lo_addr(0x220));
            assert(trace[1].a[0] == lo_addr(0x220) && trace[1].a[1] == lo_addr(0x240));
        }
        if (i == 6) {
            assert(ntrace == 1 && trace[0].a[0] == lo_addr(0x200));
            assert(trace[0].a[2] == lo_addr(0x220) && trace[0].a[3] == lo_addr(0x220));
        }
        if (i == 7) {
            assert(ntrace == 2 && trace[0].a[1] == lo_addr(0x220));
            assert(trace[1].id == 0x8006de04u && trace[1].a[0] == lo_addr(0x240));
            assert(trace[1].a[1] == lo_addr(0x240));
        }
        if (i == 8) {
            assert(ntrace == 1 && trace[0].a[1] == lo_addr(0x220));
            assert((uintptr_t)g_WorkList == lo_addr(0x220));
        }
        if (i == 9) {
            assert(ntrace == 1 && g_HeapNeedsConsolidation == 1);
            assert(*(uint32_t *)lo(0x1fc) == (0x21u << 26));
            assert(*(uint32_t *)lo(0x218) == lo_addr(0x220));
            assert((uintptr_t)g_WorkList == lo_addr(0x240));
        }
        if (i == 10) {
            assert(ntrace == 1 && g_HeapNeedsConsolidation == 1);
            assert(*(uint32_t *)lo(0x1fc) == (0x21u << 26));
            assert(*(uint32_t *)lo(0x218) == 0);
            assert((uintptr_t)g_WorkList == lo_addr(0x220));
        }
        if (i == 11) {
            assert(ntrace == 1 && (uintptr_t)g_WorkList == lo_addr(0x220));
            assert(*(uint32_t *)lo(0x420) == 0);
        }
        if (i == 12) {
            assert(ntrace == 1 && (uintptr_t)g_WorkList == lo_addr(0x220));
            assert((uintptr_t)D_80059590 == lo_addr(0x220) && g_NumWorkListEntries == 3);
        }
    }
    /* Observe actual allocator reuse during a native callback on an isolated
     * heap. The callback deliberately writes the reused link; no live game
     * callback or retail allocator behavior is implied by this case. */
    memset(low, 0, LOW_SIZE);
    HeapInit(lo(0x400), lo(0x800));
    void *first = HeapAlloc(0x40, 0);
    assert(first && (uintptr_t)first < UINT32_MAX);
    put32(lo(0x110), 0x12345u);
    entry(0x220, lo_addr(0x100), 0x12345u, 0, 0);
    put32((uint8_t *)first, lo_addr(0x100));
    put32((uint8_t *)first + 0x0c, 0x8006de00u);
    put32((uint8_t *)first + 0x14, 0x12345u);
    put32((uint8_t *)first + 0x18, lo_addr(0x220));
    g_WorkList = first; g_TimerWorkList = NULL; D_80059590 = first;
    native_phase = free_case = realloc_case = 1;
    reentrant_case = mutate_next_case = overwrite_freed_case = default_remove_case = 0; ntrace = 0;
    func_8001CE74((WorkListEntry *)(uintptr_t)lo_addr(0x100));
    assert(ntrace == 1 && (uintptr_t)g_WorkList == lo_addr(0x220));
    assert(*(uint32_t *)((uint8_t *)first + 0x18) == 0);
    puts("popup cleanup work-list: twelve CPU comparisons (one caller-field normalization) and one native allocator-reuse case PASS");
    return 0;
}

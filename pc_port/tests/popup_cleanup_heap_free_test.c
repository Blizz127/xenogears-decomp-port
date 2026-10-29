/* Fixed synthetic HeapFree comparisons. No real game saves or live heap. */
#include "slus_oracle.h"
extern unsigned int HeapFree(void *);
extern void HeapInit(void *, void *);
extern void *HeapAlloc(unsigned int, unsigned int);
extern uint32_t g_HeapNeedsConsolidation, g_HeapIsErrorHandlerOff;
extern uint32_t g_HeapLastAllocSize, g_HeapLastAllocSrcAddr;
__attribute__((noreturn)) void MainLoop(int error) { (void)error; abort(); }
void func_8003747C(void *p) { (void)p; }
static const Glob globals[] = {
    { "consolidation", &g_HeapNeedsConsolidation, 0x8005932cu, 4, 0 },
    { "error handler", &g_HeapIsErrorHandlerOff, 0x80059330u, 4, 0 },
    { "last size", &g_HeapLastAllocSize, 0x8005933cu, 4, 0 },
    { "last source", &g_HeapLastAllocSrcAddr, 0x80059340u, 4, 0 },
};
static void setup(uint32_t which)
{
    put32(lo(0x100), lo_addr(0x200));
    put32(lo(0x104), 0x1234u | (3u << 21) | ((which == 2u) << 25) | (5u << 26));
    memset(lo(0x108), 0xa5, 0x40);
    g_HeapNeedsConsolidation = 0;
    g_HeapIsErrorHandlerOff = 1;
    g_HeapLastAllocSize = 0x1234;
    g_HeapLastAllocSrcAddr = 0x5678;
    A[0] = which == 3u ? 0 : lo_addr(0x108);
}
static uint32_t native_free(void) { return HeapFree((void *)(uintptr_t)A[0]); }
int main(void)
{
    const Fn f = { "HeapFree", 0x800320e8u, 1, setup, native_free };
    cur_what = "POPUP CLEANUP HEAP FREE";
    map_low(); load_slus(cur_what); globs = globals; nglobs = sizeof(globals)/sizeof(globals[0]);
    for (unsigned i = 1; i <= 3; ++i) {
        if (run_case(&f, i, 0)) return 1;
        assert(g_HeapNeedsConsolidation == (i == 1 ? 1u : 0u));
        if (i == 1) assert(*(uint32_t *)lo(0x104) == (0x21u << 26));
        for (unsigned j = 0; j < 0x40; ++j) assert(lo(0x108)[j] == 0xa5);
    }
    /* Synthetic native heap: a later allocation can reuse the freed payload.
     * This is an observed case, not a guarantee about all allocator states. */
    memset(lo(0x400), 0, 0x400);
    HeapInit(lo(0x400), lo(0x800));
    void *first = HeapAlloc(0x40, 0);
    assert(first);
    put32((uint8_t *)first + 0x18, 0x12345678u);
    assert(HeapFree(first) == 0);
    void *again = HeapAlloc(0x40, 0);
    assert(again == first);
    put32((uint8_t *)again + 0x18, 0);
    assert(*(uint32_t *)((uint8_t *)first + 0x18) == 0);
    puts("popup cleanup heap free: three synthetic CPU comparisons and one native reuse case PASS");
    return 0;
}

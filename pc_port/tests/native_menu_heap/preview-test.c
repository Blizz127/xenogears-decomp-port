#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/battle_menu_heap_preview.h"
extern void *g_Heap;
extern uint16_t g_HeapCurUser, g_HeapCurContentType;
extern uint32_t g_HeapNeedsConsolidation, g_HeapIsErrorHandlerOff;
extern void *HeapAlloc(unsigned int, unsigned int);
void MainLoop(int ignored) { (void)ignored; abort(); }
static uint8_t ram[0x20000];
static void header(uintptr_t payload, uintptr_t next, unsigned tag) {
    uint32_t n=(uint32_t)next, meta=tag<<21;
    memcpy((void*)(payload-8),&n,4); memcpy((void*)(payload-4),&meta,4);
}
int main(void) {
    uintptr_t base=(uintptr_t)ram, a=base+0x108, b=base+0x208,
              end=base+0x7000, picked=0;
    assert(base+sizeof ram<=UINT32_MAX);
    memset(ram,0,sizeof ram);
    header(a,b,2); header(b,end,0); header(end,end,1);
    MenuHeapSnapshot saved;
    assert(menu_heap_snapshot(ram,sizeof ram,a,0,0x5da4,&saved));
    assert(menu_heap_revalidate(ram,sizeof ram,a,0,0x5da4,&saved));
    header(a,b,3); /* callback changed a traversed header, same result */
    assert(!menu_heap_revalidate(ram,sizeof ram,a,0,0x5da4,&saved));
    header(a,b,2);
    assert(!menu_heap_revalidate(ram,sizeof ram,a,1,0x5da4,&saved));
    assert(menu_heap_preview(ram,sizeof ram,a,0,0x5da4,&picked) && picked==b);
    assert(menu_heap_preview(ram,sizeof ram,a,1,0x5da4,&picked) && picked==b);
    assert(!menu_heap_preview(ram,sizeof ram,a,0,0x8000,&picked));
    header(b,a,0); assert(!menu_heap_preview(ram,sizeof ram,a,0,0x5da4,&picked));
    header(b,end,0); header(a,base+sizeof ram+4,2);
    assert(!menu_heap_preview(ram,sizeof ram,a,0,0x5da4,&picked));
    header(a,b,2); header(b,b+0x5dac,0); header(b+0x5dac,b+0x5dac,1);
    assert(menu_heap_preview(ram,sizeof ram,a,0,0x5da4,&picked) && picked==b);
    assert(!menu_heap_preview(ram,sizeof ram,a+1,0,0x5da4,&picked));
    header(a,b,2); header(b,end,0); header(end,end,1);
    g_Heap=(void*)a; g_HeapNeedsConsolidation=0;
    g_HeapIsErrorHandlerOff=1; g_HeapCurUser=2; g_HeapCurContentType=32;
    assert(menu_heap_preview(ram,sizeof ram,a,0,0x5da4,&picked));
    void *allocated=HeapAlloc(0x5da4,0);
    assert((uintptr_t)allocated==picked && picked==b);
    /* First free block is too small; the next block fits exactly. */
    memset(ram,0,sizeof ram); uintptr_t exact=b+0x5dacu;
    header(a,b,0); header(b,exact,0); header(exact,exact,1);
    g_Heap=(void*)a; g_HeapNeedsConsolidation=0;
    g_HeapCurUser=2; g_HeapCurContentType=32;
    assert(menu_heap_preview(ram,sizeof ram,a,0,0x5da4,&picked));
    allocated=HeapAlloc(0x5da4,0);
    assert((uintptr_t)allocated==picked && picked==b);
    uint32_t next; memcpy(&next,(void*)(b-8u),4); assert(next==exact);
    /* First available block wins when its request must be rounded. */
    memset(ram,0,sizeof ram); uintptr_t after=base+0x6800u;
    header(a,after,0); header(after,end,2); header(end,end,1);
    g_Heap=(void*)a; g_HeapNeedsConsolidation=0;
    g_HeapCurUser=2; g_HeapCurContentType=32;
    assert(menu_heap_preview(ram,sizeof ram,a,0,0x5da3,&picked));
    allocated=HeapAlloc(0x5da3,0);
    assert((uintptr_t)allocated==picked && picked==a);
    /* Two adjacent free blocks fit only after real consolidation. */
    memset(ram,0,sizeof ram); uintptr_t middle=base+0x3208u;
    header(a,middle,0); header(middle,end,0); header(end,end,1);
    g_Heap=(void*)a; g_HeapNeedsConsolidation=1;
    g_HeapCurUser=2; g_HeapCurContentType=32;
    assert(menu_heap_snapshot(ram,sizeof ram,a,1,0x5da4,&saved));
    assert(saved.result==a && menu_heap_revalidate(ram,sizeof ram,a,1,0x5da4,&saved));
    allocated=HeapAlloc(0x5da4,0);
    assert((uintptr_t)allocated==saved.result && g_HeapNeedsConsolidation==0);
    assert(!menu_heap_revalidate(ram,sizeof ram,a,0,0x5da4,&saved));
    puts("PASS synthetic heap preview and native HeapAlloc including consolidation");
}

#include "common.h"
#include "system/memory.h"
#include "psyq/pc.h"

extern void func_800379C8(u8*);
extern void FontPrintf(char*, ...);

// .sbss
int g_HeapDebugDumpFileHandle;

// .sdata (linker-placed at the retail word): predefined to the font printer.
FnPrintf_t* g_HeapDebugPrintfFn = FontPrintf;

void HeapWriteToDebugFile(char* pBuffer) {
#ifdef XENO_PC_PORT
    PCwrite(g_HeapDebugDumpFileHandle, pBuffer, strlen(pBuffer));
#else
    /* Retail threads strlen's $v0 result straight into PCwrite's $a2 and
     * copies the buffer pointer to $a1 before loading the handle. */
    register int nLen asm("$2");
    register char* a1v asm("$5");

    nLen = strlen(pBuffer);
    a1v = pBuffer;
    PCwrite(g_HeapDebugDumpFileHandle, a1v, nLen);
#endif
}

void HeapDumpToFile(char *pOutputFilePath) {
#ifdef XENO_PC_PORT
    int h;

    PCinit();
    h = PCcreate(pOutputFilePath, 0);
    g_HeapDebugDumpFileHandle = h;
    g_HeapDebugPrintfFn = HeapWriteToDebugFile;
    HeapDebugDump(1, 0, 0, HEAP_DEBUG_PRINT_ALL);
    g_HeapDebugPrintfFn = func_800379C8;
    PCclose(h);
#else
    /* Retail writes the sink through an absolute address (lui $at per
     * store), not %gp_rel: the pointer lives in .sdata but this TU
     * addresses it absolutely (see also temp3's D_800594D5/D6). A plain C
     * store would CSE the shared address into $s0, so each store carries
     * its own opaque address materialization; the surrounding locals are
     * evaluated in retail order (the PCcreate result stays in $v0 until
     * its store, call args are pinned to $a0-$a2). Explicit %hi(symbol)
     * survives gas -G8 with HI16/LO16 (verified); maspsx only chokes on
     * %hi() around hex literals. */
    register void* sinkFn asm("$2");
    register int a0v asm("$4");
    register int a1v asm("$5");
    register int a2v asm("$6");
    int h;
    int h2;

    PCinit();
    h = PCcreate(pOutputFilePath, 0);
    a0v = 1;
    a1v = 0;
    g_HeapDebugDumpFileHandle = h;
    sinkFn = (void*)HeapWriteToDebugFile;
    a2v = 0;
    __asm__ volatile(
        "lui $at,%%hi(g_HeapDebugPrintfFn)\n\t"
        "sw %0,%%lo(g_HeapDebugPrintfFn)($at)"
        :: "r" (sinkFn) : "memory");
    HeapDebugDump(a0v, a1v, a2v, HEAP_DEBUG_PRINT_ALL);
    h2 = g_HeapDebugDumpFileHandle;
    /* Zero-word fence: the scheduler otherwise sinks this load below the
     * sink-address setup; retail loads the handle first. */
    __asm__ volatile("" ::: "memory");
    sinkFn = (void*)func_800379C8;
    __asm__ volatile(
        "lui $at,%%hi(g_HeapDebugPrintfFn)\n\t"
        "sw %0,%%lo(g_HeapDebugPrintfFn)($at)"
        :: "r" (sinkFn) : "memory");
    PCclose(h2);
#endif
}

void* HeapDerefPtr(u32* pData) {
    return *pData;
}
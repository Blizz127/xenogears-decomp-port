#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
/* The simulated game allocator is distinct from the Win32 heap API. */
#define HeapAlloc fixtureAllocate
#define HeapFree fixtureFree

#ifdef NDEBUG
#error This test requires active assertions
#endif

/* Only a heap link is compact. All callback objects and allocations use
 * their native pointer width. The fixture owns its arena and symbol data. */
typedef uint32_t u32, u_int, mem_addr;
typedef uint16_t u16, u_short;
typedef uint8_t u8, u_char;
typedef struct { u32 link, metadata; } HeapBlock;
typedef struct Delayed {
    struct Delayed *pNext;
    void *pMem;
    u32 delay;
} HeapDelayedFreeBlock;
typedef void FnPrintf_t(char *, ...);
enum { HEAP_CONTENT_SYMBOL_DATA = 46, PAGE_BYTES = 4096 };
static u16 g_HeapCurUser, g_HeapCurContentType;
static u32 g_HeapIsErrorHandlerOff, g_HeapLastAllocSrcAddr, g_HeapLastAllocSize;
static u32 g_HeapNeedsConsolidation;
static char **g_HeapUserContentNames[10];
static void *g_Heap, *g_SymbolDataEndAddress;
static u8 *g_SymbolData;
static HeapDelayedFreeBlock g_HeapDelayedFreeBlocksHead;
static FnPrintf_t *g_HeapDebugPrintfFn;
static u8 *arena;
static void *nextAllocation;
static u32 allocationSize, allocationFlags;
static void *freed[32], *unpinned[32];
static unsigned frees, unpins, resets;
static u8 disk[64], loaded[64];
static int diskSize, shortRead, closes;
static char output[1024];

static void *mapCompactArena(void)
{
    for (uintptr_t hint = 64u << 20; hint < UINT32_MAX - PAGE_BYTES; hint += 64u << 20) {
#ifdef _WIN32
        void *p = VirtualAlloc((void *)hint, PAGE_BYTES, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (p) return p;
#else
        void *p = mmap((void *)hint, PAGE_BYTES, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p != MAP_FAILED) {
            if ((uintptr_t)p <= UINT32_MAX - PAGE_BYTES) return p;
            munmap(p, PAGE_BYTES);
        }
#endif
    }
    return NULL;
}

void func_8003747C(void *resource) { assert(!resource); ++resets; }
static void *HeapAlloc(u_int size, u_int flags)
{
    allocationSize = size;
    allocationFlags = flags;
    return nextAllocation;
}
static void HeapUnpinBlock(void *p)
{
    assert(unpins < 32); unpinned[unpins++] = p;
}
static u_int HeapFree(void *p)
{
    assert(frees < 32); freed[frees++] = p; return 0;
}
static int PCinit(void) { return 0; }
static int PCopen(char *name, int flags, int perms)
{
    assert(!flags && !perms); return *name ? 3 : -1;
}
static int PClseek(int h, int offset, int mode)
{
    assert(h == 3 && !offset); return mode == SEEK_END ? diskSize : 0;
}
static int PCread(int h, char *dst, int size)
{
    assert(h == 3 && size == diskSize);
    if (shortRead) return size - 1;
    memcpy(dst, disk, (size_t)size); return size;
}
static int PCclose(int h) { assert(h == 3); return ++closes; }
static void capture(char *format, ...)
{
    assert(!strcmp(format, "%s"));
    va_list ap;
    va_start(ap, format);
    vsnprintf(output, sizeof output, format, ap);
    va_end(ap);
}
void HeapFreeAllBlocks(void);
void HeapConsolidate(void);

#include "../../src/cleanroom/heap_metadata.inc"
#include "../../src/cleanroom/heap_queries.inc"
#include "../../src/cleanroom/heap_list_sweeps.inc"
#include "../../src/cleanroom/heap_insert_block.inc"
#include "../../src/cleanroom/heap_service_controls.inc"
#include "../../src/cleanroom/heap_debug_services.inc"

static void checkMetadata(void)
{
    HeapBlock *head = (HeapBlock *)arena;
    HeapBlock *payload = head + 1;
    char *names[] = {"test", NULL};
    for (u32 n = 0; n < 256; ++n) {
        u32 word = n * 2654435761u;
        head->metadata = word;
        assert(HeapGetBlockUser(payload) == ((word >> 21) & 15));
        assert(HeapGetBlockSourceAddress(payload) == 0x80000000u + 4u * (word & 0x1fffffu));
        assert(HeapIsBlockPinned(payload) == ((word >> 25) & 1));
        HeapPinBlock(payload);
        assert(head->metadata == (word | (1u << 25)));
        HeapUnpinBlockCopy(payload);
        assert(head->metadata == (word & ~(1u << 25)));
        func_800318F0();
        HeapSetCurrentUser(word);
        assert(HeapGetCurrentUser() == (u16)word);
        HeapSetCurrentContentType((u16)~word);
        assert(g_HeapCurContentType == (u16)~word);
        g_HeapIsErrorHandlerOff = ~word;
        assert(HeapToggleErrorHandler(word) == ~word && g_HeapIsErrorHandlerOff == word);
        g_HeapLastAllocSrcAddr = word;
        g_HeapLastAllocSize = ~word;
        HeapGetAllocInformation(&g_HeapLastAllocSize, &g_HeapLastAllocSrcAddr);
        assert(g_HeapLastAllocSize == word && g_HeapLastAllocSrcAddr == word);
        HeapChangeCurrentUser(n % 10, names);
        assert(g_HeapCurUser == n % 10 && g_HeapUserContentNames[n % 10] == names);
        assert(g_HeapIsErrorHandlerOff == 0);
    }
}

static void block(unsigned payload, unsigned next, unsigned user)
{
    HeapBlock *h = (HeapBlock *)(arena + payload) - 1;
    h->link = (u32)(uintptr_t)(arena + next);
    h->metadata = user << 21;
}

static void checkLists(void)
{
    block(8, 72, 0); block(72, 104, 0); block(104, 136, 3); block(136, 136, 1);
    g_Heap = arena + 8;
    assert(!HeapWalkUntilEnd());
    assert(HeapGetTotalFreeSize() == 80 && HeapGetLargestFreeBlockSize() == 48);
    HeapConsolidate();
    assert(((HeapBlock *)arena)->link == (u32)(uintptr_t)(arena + 104));
    assert(!g_HeapNeedsConsolidation && HeapGetTotalFreeSize() == 88);
    frees = unpins = 0;
    HeapFreeBlocksWithFlag(3);
    assert(frees == 1 && freed[0] == arena + 104);
    HeapFreeAllBlocks();
    assert(frees == 3 && freed[1] == arena + 8 && freed[2] == arena + 104);
    HeapForceFreeAllBlocks();
    assert(frees == 5 && unpins == 2 && unpinned[1] == arena + 104);
    assert(!HeapInsertAlloc(g_Heap, 72));
    assert(HeapInsertAlloc(g_Heap, 12) == arena);
    assert(((HeapBlock *)(arena + 20))->metadata == 0x84000000u && g_HeapNeedsConsolidation);
    assert(((HeapBlock *)arena)->link == (u32)(uintptr_t)(arena + 28));

    HeapDelayedFreeBlock second = {NULL, arena + 128, 1};
    HeapDelayedFreeBlock first = {&second, arena + 112, 0};
    g_HeapDelayedFreeBlocksHead.pNext = &first;
    frees = 0;
    HeapTickDelayedFree();
    assert(frees == 2 && freed[0] == first.pMem && freed[1] == &first);
    assert(g_HeapDelayedFreeBlocksHead.pNext == &second && second.delay == 0);
    HeapFreeAllDelayedBlocks();
    assert(frees == 4 && freed[2] == second.pMem && freed[3] == &second);
    assert(!g_HeapDelayedFreeBlocksHead.pNext);
}

static void checkServices(void)
{
    nextAllocation = arena + 512;
    HeapReset();
    assert(g_HeapCurContentType == 32 && g_HeapCurUser == 10 && resets == 1);
    HeapInit(arena + 1, arena + 256);
    assert(g_Heap == arena + 8 && !g_SymbolData && !g_SymbolDataEndAddress && resets == 2);
    assert(((HeapBlock *)arena)->link == (u32)(uintptr_t)(arena + 256));
    HeapRelocate(arena + 64);
    assert(g_Heap == arena + 72 && ((HeapBlock *)(arena + 64))->link == (u32)(uintptr_t)(arena + 256));
    HeapSetCurrentUser(4);
    assert(HeapAllocSound(71) == nextAllocation && HeapIsBlockPinned(nextAllocation));
    assert(allocationSize == 71 && allocationFlags == 1 && HeapGetCurrentUser() == 4);
    HeapCalloc(0x80000001u, 4);
    assert(allocationSize == 4 && !allocationFlags && HeapGetCurrentUser() == 4);
    HeapForceFree(nextAllocation);
    assert(unpinned[unpins - 1] == nextAllocation && freed[frees - 1] == nextAllocation);
}

static void checkSymbols(void)
{
    u8 name[64];
    memcpy(disk, "SYM1", 4);
    disk[4] = 8; disk[8] = 3; memcpy(disk + 9, "one", 3);
    disk[12] = 20; disk[16] = 3; memcpy(disk + 17, "two", 3);
    diskSize = 20;
    nextAllocation = loaded;
    assert(HeapLoadSymbols("") == -1);
    assert(HeapLoadSymbols("synthetic") == 0 && closes == 1);
    assert(g_SymbolData == loaded && g_SymbolDataEndAddress == loaded + diskSize);
    HeapGetSymbolNameFromAddress(8, name); assert(!name[0]);
    HeapGetSymbolNameFromAddress(20, name); assert(!strcmp((char *)name, "one"));
    HeapGetSymbolNameFromAddress(21, name); assert(!strcmp((char *)name, "two"));
    shortRead = 1;
    assert(HeapLoadSymbols("short") == 0 && closes == 2 && !g_SymbolData && !g_SymbolDataEndAddress);
    g_HeapDebugPrintfFn = capture;
    HeapPrintf("ratio %d%% %s", 17, "ready");
    assert(!strcmp(output, "ratio 17% ready"));
}

int main(void)
{
    arena = mapCompactArena();
    if (!arena) {
        fputs("A writable 4 KiB mapping below 4 GiB is required by the compact heap fixture.\n", stderr);
        return 1;
    }
    checkMetadata(); checkLists(); checkServices(); checkSymbols();
#ifdef _WIN32
    VirtualFree(arena, 0, MEM_RELEASE);
#else
    munmap(arena, PAGE_BYTES);
#endif
    puts("31 heap services passed synthetic metadata, list, allocation and diagnostic checks.");
    return 0;
}

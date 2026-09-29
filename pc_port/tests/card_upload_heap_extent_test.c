/* Native allocation/read-extent diagnostic, not retail pixel acceptance. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/memory.h"

void *g_Heap;
u32 g_HeapNeedsConsolidation, g_HeapIsErrorHandlerOff;
u32 g_HeapLastAllocSize, g_HeapLastAllocSrcAddr;
u16 g_HeapCurUser = HEAP_USER_TEST, g_HeapCurContentType;
void HeapConsolidate(void) { assert(!"unexpected consolidation"); }
void MainLoop(int error) { (void)error; assert(!"unexpected heap error"); }
#include "allocator.inc"

#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512
static unsigned short vram[VRAM_WIDTH * VRAM_HEIGHT];
static int vram_need_update, framebuffer_need_update, g_xeno_vram_fb_synced;
static struct { int x, y, w, h; } g_PreviousFramebuffer;
static void GR_ReadFramebufferDataToVRAM(void) { assert(!"unexpected framebuffer read"); }
#define SDL_memcpy memcpy
#include "copy.inc"

int main(void) {
    u8 *arena = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(arena != MAP_FAILED && (uintptr_t)arena + 0x4000 <= UINT32_MAX);
    assert(sizeof(HeapBlock) == 8);
    memset(arena, 0xa5, 0x4000);
    HeapBlock *first = (HeapBlock *)arena;
    HeapBlock *end = (HeapBlock *)(arena + 0x3ff0);
    memset(first, 0, sizeof *first);
    memset(end, 0, sizeof *end);
    first->pNext = (mem_addr)(uintptr_t)(end + 1);
    end->userTag = HEAP_USER_END;
    g_Heap = first + 1;
    u8 *buffer = HeapAlloc(0x3f6, 0);
    assert(buffer == arena + 8 && g_HeapLastAllocSize == 0x3f6);
    HeapBlock *next = (HeapBlock *)(uintptr_t)first->pNext - 1;
    assert((u8 *)next - buffer == 1016);
    u8 tail[26];
    memcpy(tail, buffer + 1014, sizeof tail);
    memset(buffer, 0, 1014);
    GR_CopyVRAM((unsigned short *)buffer, 0, 0, 40, 13, 384, 0);
    u8 uploaded[1040];
    for (unsigned row = 0; row < 13; ++row)
        memcpy(uploaded + row * 80, vram + row * VRAM_WIDTH + 384, 80);
    for (unsigned i = 0; i < 1014; ++i) assert(uploaded[i] == 0);
    assert(memcmp(uploaded + 1014, tail, sizeof tail) == 0);
    assert(memcmp(uploaded + 1016, next, sizeof *next) == 0);
    assert(vram_need_update == 1 && framebuffer_need_update == 0);
    puts("CONFIRMED split allocation: 1014 requested, 1016 payload bytes");
    puts("CONFIRMED upload: 2 alignment bytes + 8 next-header bytes + 16 free-payload bytes");
    puts("NOT_ACCEPTANCE: controlled native heap; no natural card-menu or retail pixel proof");
    assert(munmap(arena, 0x4000) == 0);
}

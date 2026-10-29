/* Read-only preview of the mode-0 retail heap search. No allocation occurs.
 * Links in the PC heap are 32-bit host pointers to payloads; each header is
 * eight bytes before its payload. This does not authorize a live allocation. */
#ifndef XENO_BATTLE_MENU_HEAP_PREVIEW_H
#define XENO_BATTLE_MENU_HEAP_PREVIEW_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    uintptr_t heap, result;
    uint32_t request;
    unsigned count, needs_consolidation;
    struct { uint32_t address, next, meta; } blocks[128];
} MenuHeapSnapshot;

static int menu_heap_snapshot(const uint8_t *ram, size_t length,
                              uintptr_t heap, unsigned needs_consolidation,
                              uint32_t request, MenuHeapSnapshot *snapshot)
{
    uintptr_t base=(uintptr_t)ram;
    if (!ram || !snapshot || !request || request > 0x7ffffffcu ||
        length < 16 || base > UINT32_MAX || length > UINT32_MAX - base ||
        (heap & 3u) || heap < base + 8u || heap > base + length)
        return 0;
    memset(snapshot,0,sizeof *snapshot);
    snapshot->heap=heap;
    snapshot->request=(request+3u)&~3u;
    snapshot->needs_consolidation=!!needs_consolidation;
    uintptr_t current=heap;
    int reached_end=0;
    for (unsigned i=0;i<128;i++) {
        uintptr_t header=current-8u;
        if (header < base || header > base+length-8u) return 0;
        uint32_t next,meta;
        memcpy(&next,(const void*)header,4);
        memcpy(&meta,(const void*)(header+4u),4);
        snapshot->blocks[i].address=(uint32_t)header;
        snapshot->blocks[i].next=next;
        snapshot->blocks[i].meta=meta;
        snapshot->count=i+1u;
        unsigned tag=(meta>>21)&15u;
        if (tag==1u) { reached_end=1; break; }
        if ((next&3u) || next <= current || next > base+length ||
            (uintptr_t)next-header < 16u) return 0;
        current=next;
    }
    if (!reached_end) return 0;
    for (unsigned i=0;i+1u<snapshot->count;) {
        unsigned tag=(snapshot->blocks[i].meta>>21)&15u;
        unsigned last=i;
        if (tag==0u && needs_consolidation) {
            while (last+1u<snapshot->count &&
                   ((snapshot->blocks[last+1u].meta>>21)&15u)==0u)
                last++;
        }
        if (tag==0u) {
            uint32_t next=snapshot->blocks[last].next;
            uint32_t header=snapshot->blocks[i].address;
            if (next-header-16u >= snapshot->request) {
                snapshot->result=(uintptr_t)header+8u;
                return 1;
            }
        }
        i=last+1u;
    }
    return 0;
}

static int menu_heap_preview(const uint8_t *ram, size_t length,
                             uintptr_t heap, unsigned needs_consolidation,
                             uint32_t request, uintptr_t *result)
{
    MenuHeapSnapshot snapshot;
    if (!result || !menu_heap_snapshot(ram,length,heap,needs_consolidation,
                                       request,&snapshot)) return 0;
    *result=snapshot.result;
    return 1;
}

static int menu_heap_revalidate(const uint8_t *ram, size_t length,
                                uintptr_t heap, unsigned needs_consolidation,
                                uint32_t request, const MenuHeapSnapshot *before)
{
    MenuHeapSnapshot after;
    if (!before || !menu_heap_snapshot(ram,length,heap,needs_consolidation,
                                       request,&after) ||
        before->heap!=after.heap || before->request!=after.request ||
        before->needs_consolidation!=after.needs_consolidation ||
        before->result!=after.result || before->count!=after.count)
        return 0;
    for (unsigned i=0;i<after.count;i++) {
        if (before->blocks[i].address!=after.blocks[i].address ||
            before->blocks[i].next!=after.blocks[i].next ||
            before->blocks[i].meta!=after.blocks[i].meta) return 0;
    }
    return 1;
}
#endif

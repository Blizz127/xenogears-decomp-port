#include <stdint.h>

/* Metadata and the compact next link are full-word storage in the heap header. */
typedef u32 HeapHeaderWord __attribute__((__may_alias__));

static volatile HeapHeaderWord *heapMetadataWord(const void *payload)
{
    return (volatile HeapHeaderWord *)((uintptr_t)payload - sizeof(HeapHeaderWord));
}

static u32 heapWordOwner(u32 metadata)
{
    return (metadata >> 21) & 15u;
}

u_int HeapGetBlockUser(HeapBlock *p)
{
    return heapWordOwner(*heapMetadataWord(p));
}

mem_addr HeapGetBlockSourceAddress(HeapBlock *p)
{
    u32 sourceIndex = *heapMetadataWord(p) & 0x1fffffu;
    return 0x80000000u + sourceIndex * 4u;
}

u_int HeapIsBlockPinned(HeapBlock *p)
{
    return (*heapMetadataWord(p) >> 25) & 1u;
}

void HeapPinBlock(HeapBlock *p)
{
    *heapMetadataWord(p) |= 1u << 25;
}

void HeapUnpinBlockCopy(HeapBlock *p)
{
    *heapMetadataWord(p) &= ~(1u << 25);
}

void func_800318F0(void)
{
}

u_int HeapGetCurrentUser(void)
{
    return *(const volatile u16 *)&g_HeapCurUser;
}

void HeapSetCurrentUser(u_int user)
{
    *(volatile u16 *)&g_HeapCurUser = (u16)user;
}

void HeapSetCurrentContentType(u_short content)
{
    *(volatile u16 *)&g_HeapCurContentType = content;
}

u_int HeapToggleErrorHandler(u_int setting)
{
    volatile u32 *current = &g_HeapIsErrorHandlerOff;
    u32 previous = *current;
    *current = setting;
    return previous;
}

void HeapGetAllocInformation(mem_addr *source, u_int *size)
{
    *(volatile mem_addr *)source = *(const volatile u32 *)&g_HeapLastAllocSrcAddr;
    *(volatile u_int *)size = *(const volatile u32 *)&g_HeapLastAllocSize;
}

void HeapChangeCurrentUser(u_int user, char **names)
{
    *(volatile u16 *)&g_HeapCurUser = (u16)user;
    *(char **volatile *)&g_HeapUserContentNames[user] = names;
    *(volatile u32 *)&g_HeapIsErrorHandlerOff = 0;
}

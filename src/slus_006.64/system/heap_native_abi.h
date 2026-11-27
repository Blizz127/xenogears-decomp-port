/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_NATIVE_HEAP_DECLARATIONS_H
#define XENO_NATIVE_HEAP_DECLARATIONS_H

#include <stddef.h>
#include <stdint.h>
#include <psx/types.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint32_t mem_addr;

typedef struct {
    mem_addr pNext;
    unsigned int sourceAddress : 21;
    unsigned int userTag : 4;
    unsigned int isPinned : 1;
    unsigned int contentTag : 6;
} HeapBlock;

/* The game allocator can return four-byte-aligned storage for these links. */
#pragma pack(push, 4)
typedef struct HeapDelayedFreeBlock {
    struct HeapDelayedFreeBlock *pNext;
    void *pMem;
    u_int delay;
    u_int nativePadding;
} HeapDelayedFreeBlock;
#pragma pack(pop)

_Static_assert(sizeof(HeapBlock) == 8, "compact heap block header");
_Static_assert(sizeof(HeapDelayedFreeBlock) == 24, "64-bit delayed-release record");
_Static_assert(_Alignof(HeapDelayedFreeBlock) == 4, "heap allocation alignment");
_Static_assert(offsetof(HeapDelayedFreeBlock, pMem) == 8, "delayed payload");
_Static_assert(offsetof(HeapDelayedFreeBlock, delay) == 16, "delayed count");

enum {
    HEAP_USER_END = 1,
    HEAP_DEFAULT_CONTENT_TYPES = 0x20,
    HEAP_CONTENT_NONE = HEAP_DEFAULT_CONTENT_TYPES,
    HEAP_CONTENT_SYMBOL_DATA = HEAP_DEFAULT_CONTENT_TYPES | 0x0e,
    ERR_HEAP_OUT_OF_MEMORY = 0x82
};
enum {
    HEAP_DEBUG_PRINT_NO = 1u << 0,
    HEAP_DEBUG_PRINT_MCB = 1u << 1,
    HEAP_DEBUG_PRINT_ADDRESS = 1u << 2,
    HEAP_DEBUG_PRINT_SIZE = 1u << 3,
    HEAP_DEBUG_PRINT_USER = 1u << 4,
    HEAP_DEBUG_PRINT_GETADD = 1u << 5,
    HEAP_DEBUG_PRINT_FUNCTION = 1u << 6,
    HEAP_DEBUG_PRINT_CONTENTS = 1u << 7,
    HEAP_DEBUG_PRINT_TOTAL_FREE_SIZE = 1u << 15
};

typedef void FnPrintf_t(char *format, ...);
extern FnPrintf_t *g_HeapDebugPrintfFn;
extern void *D_80050110[12];
extern void *g_HeapContentTypeNames[20];
extern char **g_HeapUserContentNames[12];

extern u16 g_HeapCurUser, g_HeapCurContentType;
extern void *g_Heap;
extern u32 g_HeapIsErrorHandlerOff, g_HeapNeedsConsolidation;
extern u32 g_HeapLastAllocSize, g_HeapLastAllocSrcAddr, D_80059344;
extern u8 *g_SymbolData;
extern void *g_SymbolDataEndAddress;
extern HeapDelayedFreeBlock g_HeapDelayedFreeBlocksHead;

extern char D_80018A58[10], D_80018A64[9], D_80018A70[10];
extern char D_80059248[6], D_80059250[5], D_80059258[4], D_8005925C[3];
extern char D_80059260[4], D_80059264[2], D_80059268[5], D_80059270[8];
extern char D_80059278[8], D_80059280[8], D_80059288[6], D_80059290[8];
extern char D_80059298[5], D_800592A0[5], D_800592A8[8], D_800592B0[6];

_Noreturn void MainLoop(int reason);
void func_8003747C(void *font);
int PCinit(void);
int PCopen(char *path, int flags, int mode);
int PCclose(int file);
int PClseek(int file, int offset, int origin);
int PCread(int file, char *buffer, int bytes);

/* Public heap entry points used across the native providers. */
void *HeapGetNextBlockHeader(HeapBlock *block);
void HeapPrintf(char *format, ...);
void HeapSetCurrentContentType(u_short content);
void *HeapAlloc(u_int bytes, u_int flags);
void HeapConsolidate(void);
u_int HeapFree(void *payload);
void HeapFreeAllBlocks(void);
void HeapPinBlock(HeapBlock *payload);
void HeapUnpinBlock(HeapBlock *payload);
void HeapGetSymbolNameFromAddress(mem_addr address, u_char *destination);
void HeapDebugDumpBlock(HeapBlock *header, void *payload, u_int size, int flags);
u_int HeapGetTotalFreeSize(void);

#endif

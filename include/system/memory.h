#ifndef _XENO_MEMORY_H
#define _XENO_MEMORY_H

#include "main/main.h"

// Temporary ----------------------------
extern void func_80031FF8();
extern void func_8003747C(void*);
extern void FontPrintf(char*, ...);
#ifndef XENO_PC_PORT
extern u_int strlen(char* pString);
#endif
extern int Sprintf(char* pStr, char* pFormat, ...);
// --------------------------------------

#define VRAM_BASE_ADDRESS 0x80000000

#define HEAP_NUM_USERS 0xA

// 0x80050110 - Block user flags
#define HEAP_USER_NONE 0x0
#define HEAP_USER_END 0x1
#define HEAP_USER_HIG 0x2
#define HEAP_USER_KAZM 0x3
#define HEAP_USER_MASA 0x4
#define HEAP_USER_MIYA 0x5
#define HEAP_USER_SUGI 0x6
#define HEAP_USER_SUZU 0x7
#define HEAP_USER_YOSI 0x8
#define HEAP_USER_SIMA 0x9
#define HEAP_USER_UNKNOWN 0xA
#define HEAP_USER_TEST 0xB

// Block content types 0x1 -> 0x1F can be custom-defined
// on a per-user basis.
#define HEAP_DEFAULT_CONTENT_TYPES 0x20
#define HEAP_CONTENT_NULL 0x0

// 0x80050140 - Default Block content types
#define HEAP_CONTENT_NONE (HEAP_DEFAULT_CONTENT_TYPES | 0x0)
#define HEAP_CONTENT_FREE (HEAP_DEFAULT_CONTENT_TYPES | 0x1)
#define HEAP_CONTENT_FAKE_MALLOC (HEAP_DEFAULT_CONTENT_TYPES | 0x2)
#define HEAP_CONTENT_FAKE_CALLOC (HEAP_DEFAULT_CONTENT_TYPES | 0x3)
#define HEAP_CONTENT_MODEL_DATA (HEAP_DEFAULT_CONTENT_TYPES | 0x4)
#define HEAP_CONTENT_MODEL_PACKET (HEAP_DEFAULT_CONTENT_TYPES | 0x5)
#define HEAP_CONTENT_MODEL_LIGHT (HEAP_DEFAULT_CONTENT_TYPES | 0x6)
#define HEAP_CONTENT_CD_CHACE (HEAP_DEFAULT_CONTENT_TYPES | 0x7)
#define HEAP_CONTENT_MES_IMAGE (HEAP_DEFAULT_CONTENT_TYPES | 0x8)
#define HEAP_CONTENT_MES_WORK (HEAP_DEFAULT_CONTENT_TYPES | 0x9)
#define HEAP_CONTENT_MES_CUE (HEAP_DEFAULT_CONTENT_TYPES | 0xA)
#define HEAP_CONTENT_MIME_WORK (HEAP_DEFAULT_CONTENT_TYPES | 0xB)
#define HEAP_CONTENT_MIME_VERTEX (HEAP_DEFAULT_CONTENT_TYPES | 0xC)
#define HEAP_CONTENT_MIME_NORMAL (HEAP_DEFAULT_CONTENT_TYPES | 0xD)
#define HEAP_CONTENT_SYMBOL_DATA (HEAP_DEFAULT_CONTENT_TYPES | 0xE)
#define HEAP_CONTENT_SOUND (HEAP_DEFAULT_CONTENT_TYPES | 0xF)
#define HEAP_CONTENT_MES_FONT (HEAP_DEFAULT_CONTENT_TYPES | 0x10)
#define HEAP_CONTENT_MES_SYSDATA (HEAP_DEFAULT_CONTENT_TYPES | 0x11)
#define HEAP_CONTENT_LS_FONT (HEAP_DEFAULT_CONTENT_TYPES | 0x12)
#define HEAP_CONTENT_DELAY_FREE (HEAP_DEFAULT_CONTENT_TYPES | 0x13)

// Error handler states
#define HEAP_ERROR_HANDLER_ON 0x0
#define HEAP_ERROR_HANDLER_OFF 0x1

// Error flags
#define ERR_HEAP_OUT_OF_MEMORY 0x82
#define ERR_HEAP_FREE_NULL 0x83

// Debug flags
#define HEAP_DEBUG_PRINT_NO 0x1
#define HEAP_DEBUG_PRINT_MCB 0x2
#define HEAP_DEBUG_PRINT_ADDRESS 0x4
#define HEAP_DEBUG_PRINT_SIZE 0x8
#define HEAP_DEBUG_PRINT_USER 0x10
#define HEAP_DEBUG_PRINT_GETADD 0x20
#define HEAP_DEBUG_PRINT_FUNCTION 0x40
#define HEAP_DEBUG_PRINT_CONTENTS 0x80
#define HEAP_DEBUG_PRINT_TOTAL_FREE_SIZE 0x8000
#define HEAP_DEBUG_PRINT_ALL 0xFFFFFFFF

typedef struct {
#ifdef XENO_PC_PORT
    /* Native port: store the next-block link as a 32-bit PSX address rather than
     * a host pointer. Emulated PSX RAM is linked below 4 GiB (build_port.sh
     * -no-pie), so a host pointer round-trips through 32 bits losslessly. This
     * also keeps sizeof(HeapBlock)==8 exactly as on PSX, which the allocator
     * relies on via hardcoded header arithmetic (e.g. `pMem + 8` for the user
     * pointer and `sizeof(HeapBlock)*2 == 0x10`); a 16-byte host struct would
     * desync those offsets and corrupt the free list. */
    mem_addr pNext;
#else
    void* pNext;
#endif
    u_int sourceAddress: 21;
    u_int userTag: 4;
    u_int isPinned: 1;
    u_int contentTag: 6;
} HeapBlock;

struct HeapDelayedFreeBlock {
    struct HeapDelayedFreeBlock* pNext;
    void* pMem;
    u_int delay;
};

typedef struct HeapDelayedFreeBlock HeapDelayedFreeBlock;

typedef void FnPrintf_t (char*, ...);
extern FnPrintf_t* g_HeapDebugPrintfFn;

extern char** D_80050110;
extern char** g_HeapContentTypeNames[];

// Pointers to Content type name arrays for specific heap users
extern char** g_HeapUserContentNames[HEAP_NUM_USERS];


void* HeapGetNextBlockHeader(HeapBlock* pHeapBlock);
u_int HeapGetBlockUser(HeapBlock* pHeapBlock);
mem_addr HeapGetBlockSourceAddress(HeapBlock* pHeapBlock);
u_int HeapIsBlockPinned(HeapBlock* pHeapBlock);
int HeapLoadSymbols(char* pSymbolFilePath);
void HeapReset(void);
void HeapInit(void* heapStart, void* heapEnd);
void HeapRelocate(void* pNewStartAddress);
u_int HeapGetCurrentUser(void);
void HeapSetCurrentUser(u_int userTag);
u_int HeapToggleErrorHandler(u_int status);
void HeapGetAllocInformation(mem_addr* pAllocSourceAddr, u_int* pAllocSize);
void* HeapAlloc(u_int allocSize, u_int allocFlags);
void* HeapInsertAlloc(HeapBlock* pMem, u_int allocSize);
void HeapConsolidate(void);
void HeapPinBlock(HeapBlock* pBlock);
void HeapUnpinBlock(HeapBlock* pBlock);
void HeapUnpinBlockCopy(HeapBlock* pBlock);
u_int HeapFree(void* pMem);
void HeapFreeBlocksWithFlag(u_char targetFlag);
void HeapFreeAllBlocks(void);
void HeapForceFreeAllBlocks(void);
u_int HeapGetTotalFreeSize(void);
u_int HeapWalkUntilEnd(void);
u_int HeapGetLargestFreeBlockSize(void);
void HeapChangeCurrentUser(u_int userTag, char** pContentTypes);
void HeapSetCurrentContentType(u_short contentTag);
void HeapGetSymbolNameFromAddress(mem_addr address, u_char* pString);
void HeapDebugDumpBlock(HeapBlock* pBlockHeader, void* pBlockMem, u_int blockSize, int debugFlags);
void HeapDebugDump(u_int mode, u_int startBlockIdx, int endBlockIdx, u_int flags);
void* HeapAllocSound(u_int allocSize);
void HeapCalloc(u_int numElements, u_int elementSize);
void HeapForceFree(void* pMem);
//void HeapPrintf(char* pFormat, void* arg);
void HeapDelayedFree(void* pMem, u_int delay);
void HeapTickDelayedFree(void);
void HeapFreeAllDelayedBlocks(void);
void HeapWriteToDebugFile(char* pBuffer);
void HeapDumpToFile(char *pOutputFilePath);
void* HeapDerefPtr(u_int* pData);
#endif

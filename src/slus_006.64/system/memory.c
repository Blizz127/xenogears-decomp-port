/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_PC_PORT
#error "This heap translation unit requires the native PC port."
#endif

#include "heap_native_abi.h"
#include "../../../pc_port/src/retail_data.h"

void *g_Heap;
u16 g_HeapCurUser;
u16 g_HeapCurContentType;
u32 g_HeapIsErrorHandlerOff;
u32 g_HeapNeedsConsolidation;
u32 g_HeapLastAllocSize;
u32 g_HeapLastAllocSrcAddr;
u8 *g_SymbolData;
void *g_SymbolDataEndAddress;
HeapDelayedFreeBlock g_HeapDelayedFreeBlocksHead;
u32 D_80059344;

/* These buffers receive the menu-independent heap report text from the player's executable. */
char D_80059248[6];
char D_80059250[5];
char D_80059258[4];
char D_8005925C[3];
char D_80059260[4];
char D_80059264[2];
char D_80059268[5];
char D_80059270[8];
char D_80059278[8];
char D_80059280[8];
char D_80059288[6];
char D_80059290[8];
char D_80059298[5];
char D_800592A0[5];
char D_800592A8[8];
char D_800592B0[6];
char D_80018A58[10];
char D_80018A64[9];
char D_80018A70[10];

XENO_RETAIL_DATA_BEGIN(heap_dump_strings)
    XENO_RD(D_80018A58, XENO_RD_SLUS, 0x9258, sizeof D_80018A58),
    XENO_RD(D_80018A64, XENO_RD_SLUS, 0x9264, sizeof D_80018A64),
    XENO_RD(D_80018A70, XENO_RD_SLUS, 0x9270, sizeof D_80018A70),
    XENO_RD(D_80059248, XENO_RD_SLUS, 0x49a48, sizeof D_80059248),
    XENO_RD(D_80059250, XENO_RD_SLUS, 0x49a50, sizeof D_80059250),
    XENO_RD(D_80059258, XENO_RD_SLUS, 0x49a58, sizeof D_80059258),
    XENO_RD(D_8005925C, XENO_RD_SLUS, 0x49a5c, sizeof D_8005925C),
    XENO_RD(D_80059260, XENO_RD_SLUS, 0x49a60, sizeof D_80059260),
    XENO_RD(D_80059264, XENO_RD_SLUS, 0x49a64, sizeof D_80059264),
    XENO_RD(D_80059268, XENO_RD_SLUS, 0x49a68, sizeof D_80059268),
    XENO_RD(D_80059270, XENO_RD_SLUS, 0x49a70, sizeof D_80059270),
    XENO_RD(D_80059278, XENO_RD_SLUS, 0x49a78, sizeof D_80059278),
    XENO_RD(D_80059280, XENO_RD_SLUS, 0x49a80, sizeof D_80059280),
    XENO_RD(D_80059288, XENO_RD_SLUS, 0x49a88, sizeof D_80059288),
    XENO_RD(D_80059290, XENO_RD_SLUS, 0x49a90, sizeof D_80059290),
    XENO_RD(D_80059298, XENO_RD_SLUS, 0x49a98, sizeof D_80059298),
    XENO_RD(D_800592A0, XENO_RD_SLUS, 0x49aa0, sizeof D_800592A0),
    XENO_RD(D_800592A8, XENO_RD_SLUS, 0x49aa8, sizeof D_800592A8),
    XENO_RD(D_800592B0, XENO_RD_SLUS, 0x49ab0, sizeof D_800592B0),
XENO_RETAIL_DATA_END(heap_dump_strings)

void *HeapGetNextBlockHeader(HeapBlock *payload)
{
    const HeapBlock *header = payload - 1;
    const mem_addr payload_address = (mem_addr)(uintptr_t)payload;
    /* The public ABI returns the unsigned byte count through a pointer. */
    const mem_addr span = header->pNext - payload_address - (mem_addr)sizeof(*header);
    return (void *)(uintptr_t)span;
}

#include "../../../pc_port/src/cleanroom/heap_metadata.inc"
#include "../../../pc_port/src/cleanroom/heap_debug_services.inc"
#include "../../../pc_port/src/cleanroom/heap_service_controls.inc"
#include "../../../pc_port/src/cleanroom/heap_allocate.inc"
#include "../../../pc_port/src/cleanroom/heap_insert_block.inc"
#include "../../../pc_port/src/cleanroom/heap_list_sweeps.inc"
#include "../../../pc_port/src/cleanroom/heap_unpin_checked.inc"
#include "../../../pc_port/src/cleanroom/heap_release_block.inc"
#include "../../../pc_port/src/cleanroom/heap_queries.inc"
#include "../../../pc_port/src/cleanroom/heap_dump_block.inc"
#include "../../../pc_port/src/cleanroom/heap_dump_report.inc"

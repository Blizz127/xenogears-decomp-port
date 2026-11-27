/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_NATIVE_MAIN_LOOP_DECLARATIONS_H
#define XENO_NATIVE_MAIN_LOOP_DECLARATIONS_H

#include <stdint.h>
#include "../system/heap_native_abi.h"
#include "main/main.h"
#include "psyq/libgpu.h"
#include <psx/libapi.h>
#include <psx/libcd.h>
#include <psx/libspu.h>

typedef int8_t s8;
typedef int32_t s32;

extern s32 D_80010000;
extern char D_800180FC;
extern s8 D_800181B8;
extern char D_800181C8, D_800181DC, D_800181E8, D_800181F4;
extern char D_80018200, D_8001821C, D_80018220, D_80018224;
extern char D_80018238, D_80018258;
extern u32 D_8004F2BC;
extern u32 g_ArchiveDebugTable;
extern u_short g_C1ButtonState;
extern unsigned int g_CurGameState, g_CurGameStateOverlayID;
extern void *g_CurGameStateOverlayBuffer;
extern uint8_t g_PsxRam[];
extern u8 g_PublishedByLogoCompressed[];

/* File, archive and game-state services. */
int PCcreate(char *path, int mode);
int PCwrite(int file, char *data, int size);
int ArchiveCdDataSync(int mode);
int ArchiveDecodeSize(int entry);
int ArchiveGetArchiveOffsetIndices(int *directory, int *entry);
s32 ArchiveReadFileToBuffer(s32 entry, void *buffer, u32 argument, u32 flags);
void ArchiveReset(void);
int ArchiveSetIndex(int directory, int entry);
void *LZSSDecompress(void *source, void *destination);
void *LZSSHeapDecompress(void *source, int flags);
int PcPort_ArchiveReadGameOverlay(unsigned int state, void *destination,
                                  unsigned int capacity, unsigned int *source_size);
int PcPort_BattlingUnavailableGuard(unsigned int state, int entry_bound);
void PcPort_ReportGameError(unsigned int error_code, unsigned int source_address);

/* Reset and diagnostics. */
void ControllerResetState(void);
void FontDrawLetters(void *ordering_table);
void *FontLoadFont(int x, int y, int width, int height, int capacity,
                   unsigned int flags, int page_x, int page_y, int clut_x,
                   int clut_y, void *compressed);
void FontPrintf(char *format, ...);
void HeapChangeCurrentUser(u_int user, char **names);
void HeapDumpToFile(char *path);
void HeapForceFreeAllBlocks(void);
void HeapGetAllocInformation(mem_addr *source, u_int *size);
u_int HeapGetCurrentUser(void);
void HeapRelocate(void *start_address);
void HeapReset(void);
void HeapSetCurrentUser(u_int user);
u_int HeapToggleErrorHandler(u_int setting);
void SoundReset(void);
int Vsync(int intervals);
void func_80019548(void);
void func_800363F0(void (*callback)(void));
void func_800408F4(void);
void func_8004B7D0(void (*callback)(void));
void start(void);

/* Cross-provider calls within this translation unit. */
void ChangeGameState(unsigned int state);
void GameCheckAndHandleSoftReset(void);
void GameShowSplashScreen(void);
void GameSoftReset(void);
void HeapResetUser(void);
void *LoadGameStateOverlay(unsigned int state);
void func_80019C2C(void);

#endif

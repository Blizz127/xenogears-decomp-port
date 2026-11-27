/* SPDX-License-Identifier: MIT */
#ifndef XENO_NATIVE_SOUND_ABI_H
#define XENO_NATIVE_SOUND_ABI_H

#include "types.h"
#include "system/sound.h"
#include "psyq/kernel.h"
#include "psx_memory.h"

typedef struct {
    volatile u16 raw[256];
} SpuUnion;

typedef u8 *(*SoundScriptHandlerFn)(u8 *, AudioManager *, AudioElement *);

extern SpuUnion *g_pSoundSpuRegisters;
extern u8 D_80050824[128], D_800509B0[228], D_80050A94[228], D_80050B78[120];
extern s16 D_80050BF0[3072];
extern SoundPsxAddress D_800508A4[16];
extern SoundScriptHandlerFn g_SoundScriptHandlers[128];
extern u32 g_PcPortDiagPartialWdsLoads;
extern SoundFile D_80050910;
extern u16 D_80050924[];
extern SoundWDSEntry D_80050940;

extern void func_80039E60(s32 id);
extern void func_80039FF8(void);
extern int func_8003BDFC(int flags);
extern SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *entry, s32 mode);
extern void SoundAddSedsEntry(SoundFile *record);
extern void func_8003A89C(AudioManager *manager, s32 level, s32 steps);
extern void func_8003B644(s32 selection, s32 id, s32 volume, s32 pan);
extern void SoundExecuteReverbWorkAreaTransfer(void);
extern void SoundHandleError(s32 error);
extern void SoundHeapFree(void *allocation);
extern void SoundQueueTransferCommand(u32 address, void *data, u_long size,
                                      SoundCommandCallback_t completed, unsigned short operation);
extern int SoundSpuMemoryFreeBlock(int address);
#ifdef _WIN32
extern void PsyX_Sys_SoundGateEnterCritical(void);
extern void PsyX_Sys_SoundGateExitCritical(void);
static void SoundCancelMovieTerminalCdFade(void);
static void SoundSetCdVolumeWithFadeLocked(s32 volume, s32 frames);
void SoundSetMovieTerminalCdFade(unsigned long long token);
#endif

#endif

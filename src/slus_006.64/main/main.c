#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/libapi.h"
#include "psyq/libspu.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"
#include "system/controller.h"
#include "system/kernel.h"
#include "main/main.h"

extern void ControllerInit(void);
extern void func_8004B7D0(void (*fn)(void));
extern void *func_8002DFE0(void);
extern void ArchiveInit(u32 pArchiveTable, u32 pHeaderTable, u32 pDebugTable);
extern void ArchiveCdDataSync(int mode);
extern s32 ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void *LZSSHeapDecompress(void *pData, int flags);
extern void SystemInitializeFont(void *pSystemFont);
extern void SystemInitializeData(void *pSystemData);
extern s16 func_8003BDFC(s32 flags);
extern void func_8001AADC();
extern void func_8001BB50(void);
extern void func_80024F20(void);
extern void func_800379B4(s32 arg);
extern unsigned int ArchiveGetDiscNumber(void);
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void GameShowSplashScreen(void);
extern void func_8001B6BC(void);
extern void ChangeGameState(unsigned int state);
extern void func_8003634C(void);
extern int D_80010004;
extern int D_80018004;
extern SoundWDSEntry *D_80059560;
extern SoundWDSEntry *D_800595AC;
extern unsigned int g_CurGameStateOverlayID;
extern void *g_CurGameStateOverlayBuffer;
extern u8 D_8004FE44;
extern u8 D_8004FE45;
extern u8 D_8004FE46;
extern u8 D_8004FE47;

void func_80019578(void) {
    RECT rect;
    void *pWds2;
    void *pWds3;
    void *pWds4;
    void *pWds5;
    u32 prevHeapUser;
    void *pFileBuf;
    void *pHeapStart;
    u32 nButtons;
    s32 discKind;

    ResetCallback();
    SetGraphDebug(0);
    SetVideoMode(0);
    ResetGraph(0);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x180;
    rect.h = 0x1E0;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    SetDispMask(1);
    InitGeom();
    ControllerInit();
    InitCARD(1);
    StartCARD();
    _bu_init();
    func_8004B7D0(func_8003634C);
    pHeapStart = func_8002DFE0();
    HeapInit(pHeapStart, (void *)0x801FC000);
    SpuInit();
    ArchiveInit((u32)&D_80010004, (u32)&D_80018004, (u32)D_80010000);
    SoundInitialize(0);
    ArchiveSetIndex(0, 1);
    pWds2 = HeapAlloc(ArchiveDecodeSize(2), 0);
    pWds3 = HeapAlloc(ArchiveDecodeSize(3), 0);
    pWds4 = HeapAlloc(ArchiveDecodeSize(4), 0);
    pWds5 = HeapAlloc(ArchiveDecodeSize(5), 0);
    ArchiveReadFileToBuffer(2, pWds2, 0, 0);
    ArchiveReadFileToBuffer(3, pWds3, 0, 0);
    ArchiveReadFileToBuffer(4, pWds4, 0, 0);
    ArchiveReadFileToBuffer(5, pWds5, 0, 0);
    ArchiveCdDataSync(0);
    SoundLoadWdsFile((SoundWDSEntry *)pWds2, 0);
    D_80059560 = SoundLoadWdsFile((SoundWDSEntry *)pWds3, 0);
    SoundLoadWdsFile((SoundWDSEntry *)pWds4, 0);
    D_800595AC = SoundLoadWdsFile((SoundWDSEntry *)pWds5, 0);
    prevHeapUser = HeapGetCurrentUser();
    HeapSetCurrentUser(6);
    pFileBuf = HeapAlloc(ArchiveDecodeSize(6), 0);
    ArchiveReadFileToBuffer(6, pFileBuf, 0, 0);
    ArchiveCdDataSync(0);
    HeapSetCurrentContentType(0x30);
    SystemInitializeFont(LZSSHeapDecompress(pFileBuf, 1));
    HeapFree(pFileBuf);
    pFileBuf = HeapAlloc(ArchiveDecodeSize(7), 0);
    ArchiveReadFileToBuffer(7, pFileBuf, 0, 0);
    ArchiveCdDataSync(0);
    HeapSetCurrentContentType(0x31);
    SystemInitializeData(LZSSHeapDecompress(pFileBuf, 1));
    HeapFree(pFileBuf);
    HeapSetCurrentUser(prevHeapUser);
    func_8003BDFC(0x10);
    HeapFree(pWds2);
    HeapFree(pWds3);
    HeapFree(pWds4);
    HeapFree(pWds5);
    func_8001AADC();
    func_8001BB50();
    func_80024F20();
    func_800379B4(0);
    g_CurGameStateOverlayID = -1;
    g_CurGameStateOverlayBuffer = NULL;
    D_8004FE44 = 1;
    D_8004FE46 = 1;
    D_8004FE47 = 0;
    discKind = ArchiveGetDiscNumber();
    if (discKind == 1) {
        discKind = 0x10;
    } else {
        discKind = 7;
    }
    D_8004FE45 = discKind;
    if ((ControllerGetType(0) != 0) && (g_C1ButtonState == 0x90C)) {
        nButtons = 0x90C;
        do {
            ControllerPopState();
        } while (g_C1ButtonState == nButtons);
    }
    GameShowSplashScreen();
    func_8001B6BC();
    ChangeGameState(6);
    MainLoop(0);
}
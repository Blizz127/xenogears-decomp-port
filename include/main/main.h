#ifndef _XENO_MAIN_H
#define _XENO_MAIN_H

#define NUM_GAME_STATE_OVERLAYS 0x7

#define ERR_KERNEL 0x80

typedef struct {
    void (*pFnMain)(void);
    void* pMemStart;
    void* pHeapStart;
    int hasOverlay;
} MainGameState;

extern void* g_MainGameStateOverlayBuffer;
extern MainGameState g_MainGameStates[];

extern int g_GameStateOverlayArchiveOffsets[NUM_GAME_STATE_OVERLAYS];

extern void ClearMemory(u32* pStart, u32* pEnd);
extern void MainLoop(int) __attribute__((noreturn));
extern void GameHandleError(unsigned int errorCode, unsigned int sourceAddress) __attribute__((noreturn));


#endif
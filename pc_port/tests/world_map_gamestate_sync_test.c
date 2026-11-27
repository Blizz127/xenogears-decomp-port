/* Exercise the production state table and arrival handler with display spies. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;

enum { NUM_GAME_STATE_OVERLAYS = 7 };
static struct {
    void (*pFnMain)(void);
    void *pMemStart;
    void *pHeapStart;
    int hasOverlay;
} g_MainGameStates[NUM_GAME_STATE_OVERLAYS];
static u32 s_GameStateSource[NUM_GAME_STATE_OVERLAYS][4];
#define PSX_ADDR(value) ((void *)(uintptr_t)(value))

static struct RenderEnv { int ot, drawEnv, dispEnv; }
    g_KernelMenuRenderEnvironments[2], *g_KernelMenuCurRenderEnvironment;
static int g_KernelMenuIsRunning, D_800592C4, D_800592C8;
static u16 game_state[0x1200];
static void *g_pGameState = game_state;
static jmp_buf finish;
static unsigned step, frames;
static struct RenderEnv *env(void) { return &g_KernelMenuRenderEnvironments[(frames + 1) % 2]; }
static void event(unsigned expected) { assert(step++ == expected); }

static void KernelMenuMain(void) { abort(); }
static void FieldMain(void) { abort(); }
static void func_8001B6C4(void) { abort(); }
static void MenuMain(void) { abort(); }
void MovieMain(void) { abort(); }
void PcPort_SkipMovieState(void) { abort(); }
static void KernelMenuInitialize(void) { event(0); }
static void SetDispMask(int on) { assert(on == 1); event(1); }
static void TermPrim(void *ot) { assert(ot == &env()->ot); event(2); }
static void FontDrawLetters(void *ot) { assert(ot == &env()->ot); event(3); }
static void FontPrintf(const char *text)
{
    assert(strstr(text,"WORLD MAP NOT YET PORTED") && strstr(text,"FIELD ARRIVAL CAPTURED"));
    event(4);
}
static void DrawSync(int arg) { assert(arg == 0); event(5); }
static void Vsync(int arg) { assert(arg == 0); event(6); }
static void PutDrawEnv(void *p) { assert(p == &env()->drawEnv); event(7); }
static void PutDispEnv(void *p) { assert(p == &env()->dispEnv); event(8); }
static void DrawOTag(void *ot)
{
    assert(ot == &env()->ot && env() == g_KernelMenuCurRenderEnvironment);
    event(9);
    if (++frames == 2) longjmp(finish,1);
    step = 2;
}

#include "arrival.inc"

static void check_tuple(u16 selector, u16 heading, u16 arg2, u16 entrance)
{
    u16 before[sizeof(game_state) / sizeof(game_state[0])];
    game_state[0x231a / 2] = selector;
    game_state[0x231c / 2] = heading;
    game_state[0x231e / 2] = arg2;
    game_state[0x2320 / 2] = entrance;
    memcpy(before, game_state, sizeof before);
    step = 0;
    frames = 0;
    D_800592C4 = 0;
    if (setjmp(finish) == 0) {
        g_MainGameStates[3].pFnMain();
        abort();
    }
    assert(frames == 2 && step == 10);
    assert(!memcmp(game_state, before, sizeof before));
}

int main(void)
{
    s_GameStateSource[3][3] = 1;
    PcPort_InitGameStates();
    assert(g_MainGameStates[3].pFnMain == PcPort_WorldMapPlaceholderMain);
    assert(g_MainGameStates[3].hasOverlay == 1);
    check_tuple(0x0403, 9, 0x002a, 7);
    check_tuple(0xc420, 0x2345, 0xbeef, 0x8123);
    return 0;
}

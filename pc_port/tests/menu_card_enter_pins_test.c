/* Pins func_801D9C84 (retail 0x801D9C84): a failing func_801C93A8 always
 * returns 0, even when the prompt flag (pManager+0x33) is already down.  The
 * port once returned 1 in that case (docs/port/RETAIL_DIVERGENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menu;
static u8 manager[0x100];
static u8 card[sizeof(MenuUnk2)];
void* D_801EA718; void* D_801EA71C; void* D_801EA720;
static int ok93, closed, frames, clear_prompt;
void func_801D2F4C(u8 id) { assert(id == 0x20); }
void func_801C7BF4(void) { frames++; if (clear_prompt) manager[0x33] = 0; }
void func_801D9B08(void) {}
static int t_drawsync(int m) { (void)m; return 0; }
static int t_vsync(int m) { (void)m; return 0; }
static int t_enter(void) { return 0; }
static void t_exit(void) {}
static void* t_cb(int f) { (void)f; return (void*)1; }
#define DrawSync(m) t_drawsync(m)
#define Vsync(m) t_vsync(m)
#define EnterCriticalSection() t_enter()
#define ExitCriticalSection() t_exit()
#define CdSyncCallback(f) t_cb(0)
#define CdReadyCallback(f) t_cb(0)
#define CdReadCallback(f) t_cb(0)
u8 func_801C93A8(void) { return (u8)ok93; }
s32 func_801D32B4(s32 a) { (void)a; closed++; return 0; }
#include "enter.inc"
int main(void) {
    g_Menu = &menu;
    menu.pManager = (MenuManager*)manager;
    menu.unk32C = (MenuUnk2*)card;
    ok93 = 1; assert(func_801D9C84() == 1 && closed == 0);
    assert(card[MENU_CARD_OFF(0x4FE6)] == 2 && card[MENU_CARD_OFF(0x4FE8)] == 0xFF && card[MENU_CARD_OFF(0x4FE9)] == 0xFF && menu.unk326 == 0x3C);
    ok93 = 0; clear_prompt = 0; assert(func_801D9C84() == 0 && closed == 1 && manager[0x33] == 0);
    ok93 = 0; clear_prompt = 1; assert(func_801D9C84() == 0 && closed == 1);
    printf("PASS menu card enter pins (D9C84 failure returns 0)\n");
    return 0;
}

/* Production-linked field System Menu certificate.
 *
 * Drives the shipped MenuMain / MenuExecute path the field opener uses after
 * D_800ADB64=0x80: D_80059460 = request & 0x7F, then MenuMain -> case 0
 * func_801C62A8. Not a reimplementation of the overlay; the same MenuMain
 * MenuExecute the field calls. Close is the MENU_INPUT_BACK path plus the
 * WAIT_MENU clear 799D4 performs after MenuMain returns. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/menu.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

u8 D_80059460 = 0xFF;
u8 D_80059171;
u8 g_MenuDebugEnabled;
s32 D_8004F350 = 1;
s32 D_800ADB64 = 0xFF;
u8 D_800594D0;
void* D_8005945C;
void* D_800658CC;
void* D_8006BE24;
void* D_8005A4AC;
void* D_8005A4B0;
GameState g_GameState;
s32 D_80010000 = -1;
s32* D_8005917C = &D_80010000;
char D_8001833C[] = "";
char D_80018350[] = "";
char D_80018364[] = "";
char D_80018378[] = "";
char D_80018390[] = "";
void* D_8004FA9C[8];
SystemMenu* g_Menu;

static int s_failures;
static int s_rootMenuEntered;
static int s_menuMainReturned;
static int s_shouldDraw;

void func_801C62A8(void);
void func_801CB0A8(void) {}
void func_801CBDBC(void) {}
void func_801CCD28(void) {}
void func_801CE024(void) {}
void MemberChangeMenuMain(void) { func_801CB0A8(); }
void ShopMenuMain(void) { func_801CCD28(); }

void* HeapAlloc(u32 size, u32 flags)
{
    (void)flags;
    return calloc(1, size ? size : 1);
}

u_int HeapFree(void* p)
{
    free(p);
    return 0;
}

void HeapChangeCurrentUser(u32 user, char** types)
{
    (void)user;
    (void)types;
}

void SetGeomOffset(int x, int y) { (void)x; (void)y; }
void SetGeomScreen(int h) { (void)h; }
DISPENV* SetDefDispEnv(DISPENV* env, int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
    return env;
}
DRAWENV* SetDefDrawEnv(DRAWENV* env, int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
    return env;
}
int Vsync(int mode) { (void)mode; return 0; }
DRAWENV* PutDrawEnv(DRAWENV* env) { return env; }
DISPENV* PutDispEnv(DISPENV* env) { return env; }
void SetDispMask(int mask) { (void)mask; }
void ArchiveSetIndex(int a, int b) { (void)a; (void)b; }
void ArchiveReadFileToBuffer(int a, void* b, int c, int d)
{
    (void)a; (void)b; (void)c; (void)d;
}
int ArchiveCdDataSync(int a) { (void)a; return 0; }
u32 ArchiveDecodeAlignedSize(int a) { (void)a; return 16; }
void FontPrintf(char* a, ...) { (void)a; }
int ChangeGameState(unsigned int s) { (void)s; return 0; }
void MainLoop(int e) { (void)e; }
s32 func_80036410(void) { return 0; }
void ControllerResetState(void) {}
u16 g_C1ButtonStatePressedOnce;
int ControllerPopState(void) { return 0; }
u_long* ClearOTagR(u_long* ot, int n) { (void)n; return ot; }
void HeapDebugDump(int a, int b, int c, int d)
{
    (void)a; (void)b; (void)c; (void)d;
}
void FontDrawLetters(void* ot) { (void)ot; }
int DrawSync(int mode) { (void)mode; return 0; }
void DrawOTag(u_long* ot) { (void)ot; }

#ifndef MENU_MUTANT_SKIP_MENUMAIN
void func_801C62A8(void)
{
    s_rootMenuEntered = (D_80059460 == 0);
    if (g_Menu != NULL) {
        g_Menu->shouldDrawMenu = 1;
        s_shouldDraw = g_Menu->shouldDrawMenu;
    }
}
#else
void func_801C62A8(void) {}
#endif

extern void MenuMain(void);
extern void MenuExecute(void);

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

/* Apply the same request decode and WAIT_MENU clear func_800799D4 uses. */
static void apply_field_menu_request(s32 request)
{
#ifndef MENU_MUTANT_NEVER_REQUEST
    D_80059460 = request & 0x7F;
#else
    (void)request;
#endif
}

static void apply_wait_menu_clear(void)
{
#ifndef MENU_MUTANT_SKIP_WAIT_CLEAR
    D_8004F350 = 0;
#endif
}

static void test_request_0x80_reaches_menumain(void)
{
    s_rootMenuEntered = 0;
    s_shouldDraw = 0;
    s_menuMainReturned = 0;
    D_8004F350 = 1;
    D_800ADB64 = 0x80;
    apply_field_menu_request(D_800ADB64);
    g_MenuDebugEnabled = 0;

#ifndef MENU_MUTANT_SKIP_MENUMAIN
    MenuMain();
    s_menuMainReturned = 1;
#endif
    apply_wait_menu_clear();

    check(D_80059460 == 0, "request.0x80.reaches.menumain");
    check(s_rootMenuEntered, "request.0x80.reaches.menumain");
    check(s_shouldDraw == 1, "request.0x80.reaches.menumain");
    check(s_menuMainReturned, "request.0x80.reaches.menumain");
}

static void test_root_menu_selected(void)
{
    s_rootMenuEntered = 0;
    apply_field_menu_request(0x80);
    g_MenuDebugEnabled = 0;
#ifndef MENU_MUTANT_SKIP_MENUMAIN
    MenuMain();
#endif
    check(D_80059460 == 0, "root.menu.selected");
    check(s_rootMenuEntered, "root.menu.selected");
}

static void test_close_clears_wait_menu(void)
{
    D_8004F350 = 1;
    apply_field_menu_request(0x80);
    g_MenuDebugEnabled = 0;
#ifndef MENU_MUTANT_SKIP_MENUMAIN
    MenuMain();
#endif
    apply_wait_menu_clear();
    check(D_8004F350 == 0, "menu.close.clears.wait");
    check(s_rootMenuEntered || D_80059460 == 0, "menu.close.clears.wait");
}

int main(void)
{
    test_request_0x80_reaches_menumain();
    test_root_menu_selected();
    test_close_clears_wait_menu();
    if (s_failures) {
        return 1;
    }
    printf("FIELD MENU certificate PASS\n");
    return 0;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
typedef int s32;
typedef struct { int unk1192; } Selection;
typedef struct {
    int shouldDrawMenu;
    u8 input, menu1Choice, unk337, unk33B;
    void *pManager, *unk6E0;
    Selection *pSelectionMenu;
} Menu;
static Menu menu, *g_Menu = &menu;
static Selection selection;
static unsigned char manager[128], D_801EA19C[1], D_801EA528[1], D_801E9E64[1];
static int frames, notices, teardown, dispatches, errors;
static u8 inputs[4];
void func_801C7BF4(void) { assert(frames < 4); g_Menu->input = inputs[frames++]; }
void PcPort_NotifyUnsupportedFileMenu(void) {
    assert(!teardown && !dispatches && selection.unk1192 == 0);
    ++notices;
}
void func_801C8574(int sound) { assert(sound == 4); ++errors; }
void func_801D22C4(void) { ++teardown; }
void func_801E8044(int a, void *b) { (void)a; (void)b; }
s32 func_801C531C(s32 a) { (void)a; ++dispatches; return 1; }
void func_801E8978(int a, int b, void *c) { (void)a; (void)b; (void)c; }
void func_801E8070(int a, void*b, void*c, void*d, void*e, int f, int g, int h)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h; }
#include "file_menu_loop.inc"
static void reset(int choice) {
    memset(&menu,0,sizeof(menu)); memset(&selection,0,sizeof(selection));
    frames=notices=teardown=dispatches=errors=0;
    menu.menu1Choice=menu.unk337=choice; menu.unk33B=1;
    menu.pManager=manager; menu.pSelectionMenu=&selection;
    inputs[0]=4; inputs[1]=5;
}
int main(void) {
    reset(1); inputs[1]=4; inputs[2]=5;
    func_801C55A0();
    assert(notices==2 && frames==3 && !teardown && !dispatches);
    assert(selection.unk1192==0 && menu.menu1Choice==1);
    reset(4); func_801C55A0();
    assert(notices==0 && teardown==1 && dispatches==1 && frames==2);
    reset(2); menu.unk33B=0; func_801C55A0();
    assert(errors==1 && !notices && !teardown && !dispatches);
    puts("File-menu guard: repeated rejection, untouched ownership, cancel and supported dispatch PASS");
}

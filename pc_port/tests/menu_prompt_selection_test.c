#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
u8 D_801EA8FC, D_801E9778, D_801E977A;
static SystemMenu menu;
static MenuUnk2 cards;
static MenuPointerCursors cursors;
static int events[200], count, pumps, mutation, caller_mode;
static int builds, closes, first_id, second_id, first_choice, second_choice;
extern u32 func_801CAA38(u8) __attribute__((weak));
static void func_801C7BF4(void) {
    assert(pumps<count);
    if(pumps==0 && D_801E977A) assert(cards.unk4F80[0x66]==2);
    if(pumps==0 && caller_mode) assert(cursors.shouldRender[3]==1);
    if(pumps>0 && (events[pumps-1]==0 || events[pumps-1]==2)) {
        assert(cursors.shouldRender[2]==(events[pumps-1]==2));
        assert(cursors.shouldRender[3]==(events[pumps-1]==0));
    }
    menu.input=events[pumps++];
    if(mutation && pumps==1) {
        if(mutation&1) cards.unk4F80[0x64]++;
        if(mutation&2) cards.unk4F80[0x65]++;
        if(mutation&4) D_801E977A=0;
        if(mutation&8) D_801E977A=1;
    }
}
static void func_801D2F4C(u8 id) {
    assert(caller_mode && builds<2);
    assert(id==(builds?second_id:first_id));
    menu.input=8; pumps=0; count=2;
    events[0]=(builds?second_choice:first_choice)?2:0;
    events[1]=4; ++builds;
}
static s32 func_801D32B4(s32 value) {
    /* Retail ignores its input and returns zero. The caller must retain
     * the selection across this call, rather than adopt this return. */
    (void)value;
    ++closes; return 0;
}
#include "selection.inc"
static void reset(void) {
    memset(&menu,0,sizeof menu); memset(&cards,0xa5,sizeof cards);
    memset(&cursors,0xa5,sizeof cursors);
    g_Menu=&menu; menu.unk32C=&cards; menu.pCursors=&cursors; menu.input=8;
    D_801EA8FC=0x77; D_801E9778=0x33; D_801E977A=0;
    pumps=count=mutation=caller_mode=builds=closes=0;
}
static void tail(void) {
    assert(cursors.shouldRender[2]==0 && cursors.shouldRender[3]==0);
    assert(cursors.shouldRender[0]==0xa5 && cursors.shouldRender[1]==0xa5);
    assert(cards.unk4F80[0x66]==0);
}
int main(void) {
    assert((uintptr_t)&menu>UINT32_MAX && (uintptr_t)&cursors>UINT32_MAX);
    assert(func_801CAA38 && "selection loop remains unresolved");
    for(int mode=1;mode<256;++mode) for(int bits=0;bits<256;++bits)
    for(int cancel=0;cancel<2;++cancel) {
        reset(); count=9;
        for(int i=0;i<8;++i) events[i]=(bits&(1<<i))?2:0;
        events[8]=cancel?5:4;
        assert(func_801CAA38(mode)==(cancel?0:(bits>>7)));
        assert(pumps==9 && D_801EA8FC==cancel && D_801E9778==0x33); tail();
    }
    /* Automatic notices inspect the prior input before pumping. 180
     * countdown decrements therefore produce only 179 frame calls. */
    reset(); count=179; for(int i=0;i<count;++i) events[i]=8;
    assert(func_801CAA38(0)==0 && pumps==179 && D_801EA8FC==0); tail();
    for(int input=0;input<256;++input) if(input!=8) {
        reset(); menu.input=input;
        assert(func_801CAA38(0)==0 && pumps==0 && D_801EA8FC==0); tail();
    }
    for(int input=0;input<256;++input) if(input!=8) {
        reset(); count=1; events[0]=input;
        assert(func_801CAA38(0)==(input==2) && pumps==1);
        assert(D_801EA8FC==(input==5)); tail();
    }
    for(int watch=0;watch<2;++watch) for(int change=1;change<16;++change) {
        reset(); D_801E977A=watch; mutation=change;
        count=2; events[0]=2; events[1]=4;
        int active=(change&8)?1:(change&4)?0:watch;
        int cancelled=active && (change&3);
        assert(func_801CAA38(1)==(cancelled?0:1));
        assert(pumps==(cancelled?1:2) && D_801EA8FC==!!cancelled);
        assert(D_801E9778==(cancelled?1:0x33));
        assert(cards.unk4F80[8]==((active&&(change&1))?0:0xa5));
        assert(cards.unk4F80[9]==((active&&(change&2))?0:0xa5)); tail();
    }
    for(int input=0;input<256;++input)
    if(input!=0 && input!=2 && input!=4 && input!=5) {
        reset(); count=3; events[0]=2; events[1]=input; events[2]=4;
        assert(func_801CAA38(1)==1 && pumps==3 && D_801EA8FC==0); tail();
    }
    for(first_id=0;first_id<256;++first_id)
    for(second_id=0;second_id<256;++second_id)
    for(first_choice=0;first_choice<2;++first_choice)
    for(second_choice=0;second_choice<2;++second_choice) {
        reset(); caller_mode=1;
        int twice=first_choice && second_id!=255;
        assert(func_801CACF8(first_id,second_id,1)==(twice?second_choice:first_choice));
        assert(builds==1+twice && closes==builds); tail();
    }
    puts("PASS prompt input sequences, timeout, card changes and 262144 caller cases");
}

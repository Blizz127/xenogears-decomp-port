#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
static SystemMenu menus[24];
static MenuUnk2 cards[24], expected[24];
static int step, replace, pattern;
static int pending, delay, nonready, undelivered;
static const int poll_order[]={3,1,0,2};
static u32 MenuCardEvent(s32 slot);
static const char sequence[]="PEccccXVIsBDVEOOOOeeeeX";
static int owner(void) { return replace?step:0; }
static void advance(char tag) {
    assert(step<(int)sizeof(sequence)-1 && sequence[step]==tag);
    ++step; if(replace) g_Menu=&menus[step];
}
static u32 handle(int row,int index) {
    u32 v; memcpy(&v,&expected[row].unk4F80[0x6c+index*4],4); return v;
}
static void func_801C7BF4(void) { advance('P'); }
int EnterCriticalSection(void) { advance('E'); return 1; }
void ExitCriticalSection(void) { advance('X'); }
int CloseEvent(unsigned int event) {
    assert(event==handle(owner(),step-2)); advance('c'); return 1;
}
int Vsync(int mode) { assert(mode==0); advance('V'); return 0; }
void InitCARD(int mode) { assert(mode==1); advance('I'); }
void StartCARD(void) { advance('s'); }
void _bu_init(void) { advance('B'); }
int DrawSync(int mode) { assert(mode==0); advance('D'); return 0; }
int OpenEvent(unsigned int desc,int spec,int mode,long (*callback)()) {
    const int specs[]={4,0x8000,0x100,0x2000};
    int index=step-14;
    assert(index>=0 && index<4 && desc==0xf4000001u);
    assert(spec==specs[index] && mode==0x2000 && callback==NULL);
    u32 result=0x80000000u | ((u32)pattern<<16) | (0x1234u+index);
    advance('O');
    memcpy(&expected[owner()].unk4F80[0x6c+index*4],&result,4);
    return (int)result;
}
int EnableEvent(unsigned int event) {
    assert(event==handle(owner(),step-18)); advance('e'); return 1;
}
int TestEvent(unsigned int event) {
    assert(step<20);
    int slot=poll_order[step%4];
    assert(event==handle(owner(),slot));
    int result=(step/4>=delay && (pending&(1<<slot)))?1:nonready;
    ++step; if(replace) g_Menu=&menus[step];
    return result;
}
void UnDeliverEvent(unsigned int desc,int spec) {
    const int specs[]={4,0x8000,0x100,0x2000};
    assert(undelivered<4 && desc==0xf4000001u && spec==specs[undelivered++]);
}
#include "events.inc"
int main(void) {
    assert((uintptr_t)cards>UINT32_MAX);
    for(pattern=0;pattern<256;++pattern) for(replace=0;replace<2;++replace) {
        memset(menus,0,sizeof menus); memset(cards,pattern,sizeof cards);
        for(int i=0;i<24;++i) {
            menus[i].unk32C=&cards[i];
            for(int j=0;j<4;++j) {
                u32 value=0xf1234567u + i*256+j;
                memcpy(&cards[i].unk4F80[0x6c+j*4],&value,4);
            }
        }
        memcpy(expected,cards,sizeof cards); step=0;g_Menu=&menus[0];
        func_801D9B08();
        assert(step==23 && !memcmp(cards,expected,sizeof cards));
        for(pending=1;pending<16;++pending) for(delay=0;delay<4;++delay)
        for(nonready=-1;nonready<3;++nonready) if(nonready!=1) {
            step=undelivered=0;g_Menu=&menus[0];
            int i=0;while(!(pending&(1<<poll_order[i]))) ++i;
            assert(func_801C881C(0x1234)==poll_order[i]);
            assert(step==delay*4+i+1 && undelivered==4);
            assert(!memcmp(cards,expected,sizeof cards));
        }
    }
    puts("PASS 512 card event reset sequences: exact four-byte slots and current-menu reloads");
}

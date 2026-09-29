/* Retail 801D3B00: native parameter-layout and geometry-call contract.
 * Does not emulate the GPU or certify the geometry helper's internals. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
uint8_t g_PsxRam[0x200000];
static SystemMenu menu;
static MenuManager manager;
static MenuWindowParameters params[7], expected[7];
static int calls, wanted, indices[7];
static void func_801D4D1C(u8 index, s32 x, s32 y, s32 w, s32 h,
                         u8 flags, s32 z, u8 scroll) {
    assert(calls<wanted);
    MenuWindowParameters *p=&expected[indices[calls++]];
    assert(index==p->index);
    assert(x==(u16)(p->x+p->width/2-p->unk8/2));
    assert(y==(u16)(p->y+p->height/2-p->unkA/2));
    assert(w==p->unk8 && h==p->unkA);
    assert(flags==p->unk12 && z==p->zIndex && scroll==p->hasScrollBar);
}
#include "window.inc"
static void check(void) {
    memcpy(expected,params,sizeof params); wanted=calls=0;
    for(int i=0;i<7;++i) {
        MenuWindowParameters *p=&expected[i];
        if(!manager.unk27[i] || p->unk11) continue;
        indices[wanted++]=i;
        unsigned x=(unsigned)p->unk8+32, y=(unsigned)p->unkA+32;
        int done_x=x>=p->width, done_y=y>=p->height;
        p->unk8=done_x?p->width:x; p->unkA=done_y?p->height:y;
        if(done_x && done_y) p->unk11=1;
    }
    func_801D3B00();
    assert(calls==wanted && !memcmp(params,expected,sizeof params));
}
int main(void) {
    g_Menu=&menu; menu.pManager=&manager;
    for(int i=0;i<7;++i) menu.windowParameters[i]=&params[i];
    /* Isolate the non-terminating 188x64 prompt before the larger sweep. */
    manager.unk27[2]=1;
    params[2]=(MenuWindowParameters){.x=122,.y=150,.width=188,.height=64,.index=2};
    for(int i=0;i<6;++i) check();
    assert(params[2].unk11==1);
    check(); assert(calls==0);
    for(unsigned n=0;n<65536;++n) for(int variant=0;variant<4;++variant) {
        for(int i=0;i<7;++i) {
            memset(&params[i],0xa5,sizeof params[i]);
            MenuWindowParameters *p=&params[i];
            p->x=(u16)(n*13+i); p->y=(u16)(n*17-i);
            p->width=(u16)(n+i); p->height=(u16)(65535-n+i);
            p->unk8=(u16)(variant==0?0:variant==1?n-32:variant==2?n-31:n+1);
            p->unkA=(u16)(variant==0?0:variant==1?65535-n-32:variant==2?65535-n-31:65535-n+1);
            p->index=(u8)(6-i); p->unk12=(u8)n; p->hasScrollBar=(u8)(n>>8);
            p->zIndex=(s32)(n*65537u); p->unk11=(i==5?2:0);
            manager.unk27[i]=(i==6?0:1);
        }
        check();
    }
    puts("PASS window opening: six-frame prompt and 262144 seven-slot boundary cases");
}

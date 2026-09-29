#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "psyq/libgpu.h"
extern void *func_800AC0F0(void*,s32,s32);
s32 D_800AF780;
static uint8_t font[30];
static unsigned moves,loads,ink;
int MoveImage(RECT *r,int x,int y) {
    assert(moves++==0 && r->x==0x389 && r->y==0x110 && r->w==9 && r->h==16);
    assert(x==0x300 && y==16);return 0;
}
int LoadImage(RECT *r,u_long *p) {
    assert(r->w==9 && r->h==16 && r->y==16 && r->x==0x309+(int)loads*9);
    const uint8_t *b=(const uint8_t*)p;
    for(unsigned row=0;row<16;++row)for(unsigned col=0;col<18;++col) {
        uint8_t expected=0;
        if(loads==0 && row<15 && col<16)
            expected=(font[row*2+col/8]>>(7-col%8))&1?255:0;
        assert(b[row*18+col]==expected);ink+=expected!=0;
    }
    ++loads;return 0;
}
int DrawSync(int mode){assert(mode==0);return 0;}
int main(void) {
    FILE *f=fopen("disc/scph5500.bin","rb");assert(f);
    /* B0:51 code 889F maps to BFC69D68 in the pinned BIOS instruction oracle. */
    assert(fseek(f,0x69d68,SEEK_SET)==0 && fread(font,1,sizeof font,f)==sizeof font);fclose(f);
    u8 stream[64]={0x85,0x48,0x88,0x9f,0x0d};D_800AF780=8;
    assert(func_800AC0F0(stream,0x300,1)==stream+5);
    assert(D_800AF780==3 && moves==1 && loads==27 && ink>0);
    printf("PASS field BIOS font and atlas path: %u nonzero pixels; GPU intercepted\n",ink);
}

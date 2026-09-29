/* Controlled six-call boundary oracle from retail 801D3C4C. All heights,
 * signed/high-bit coordinates, real layouts, and helper-replaced globals.
 * This does not execute the atlas or vertex helper implementations. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
static SystemMenu menus[3];
static MenuWindow window, decoy;
SystemMenu *g_Menu;
static int call, swap;
static uint16_t xx, yy, hh;
static void atlas(void *t,int index,void *p,int rc,int x,int y,int scale){
 assert(t==g_Menu->unk2DC && rc==g_Menu->renderContext && x==xx && scale==4096);
 if(call==0){assert(index==261 && p==&window.polysScrollBarEnds[0] && y==yy);}
 else if(call==1){assert(index==261 && p==&window.polysScrollBarEnds[2] && y==(int)yy+hh-8);}
 else {assert(call==2 && index==262 && p==window.polysScrollBarEmpty && y==(int)yy+8);}
 ++call;if(swap && call<3)g_Menu=&menus[call];
}
static s32 func_8002675C(u8*t,s32 i,void*p,s32 r,s32 x,s32 y,s32 s){atlas(t,i,p,r,x,y,s);return 0;}
static s32 func_800263E4(u8*t,s32 i,void*p,s32 r,s32 x,s32 y,s32 s,s32 a,s32 b){assert(call==1&&a==0&&b==1);atlas(t,i,p,r,x,y,s);return 0;}
static void func_801C851C(SVECTOR*p,s32 x,s32 y,s32 w,s32 h){
 assert(x==xx && w==8);
 if(call==3)assert(p==&window.vertsScrollBarEnds[0] && y==yy && h==8);
 else if(call==4)assert(p==&window.vertsScrollBarEnds[4] && y==(uint16_t)(yy+hh) && h==65528);
 else {assert(call==5 && p==window.vertsScrollBarEmpty && y==(uint16_t)(yy+8) && h==(uint16_t)(hh-8));}
 ++call;
}
#include "production.inc"
int main(void){
 for(int i=0;i<3;++i){menus[i].unk2DC=(void*)(uintptr_t)(0x100000+i*16);menus[i].renderContext=i;for(int j=0;j<7;++j)menus[i].windows[j]=i?&decoy:&window;}
 for(swap=0;swap<2;++swap)for(unsigned n=0;n<65536;++n){
  xx=n*40503;yy=n*317+65530;hh=n;call=0;g_Menu=&menus[0];
  func_801D3C4C(n%7,(int32_t)(0x80000000u|xx),(int32_t)(0xffff0000u|yy),123456,(int32_t)(0xabcd0000u|hh));
  assert(call==6);
 }
 puts("PASS 131072 scrollbar six-call layout/coordinate cases");
}

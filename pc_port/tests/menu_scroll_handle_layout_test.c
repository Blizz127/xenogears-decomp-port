/* Actual structures; controlled draw boundary from retail 801D1464. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
static SystemMenu menu, saved;
static MenuManager manager;
static MenuScrollBarHandle handle;
SystemMenu *g_Menu=&menu;
static int calls;
static void func_801CE198(s32 n, void*v, void*p, s32 rc){
 assert(n==1 && v==handle.vertices && p==handle.polys && rc==handle.renderContext);++calls;
}
#include "production.inc"
int main(void){
 menu.pManager=&manager;
 for(int flag=0;flag<256;++flag)for(int rc=0;rc<256;++rc){
  manager.scrollHandleActive=flag;handle.renderContext=rc;
  menu.pScrollHandle=flag?&handle:NULL;calls=0;saved=menu;
  func_801D1464();assert(calls==(flag!=0));assert(!memcmp(&menu,&saved,sizeof menu));
 }
 puts("PASS 65536 scrollbar handle flag/context cases");
}

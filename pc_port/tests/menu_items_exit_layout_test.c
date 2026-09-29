/* Retail teardown call sequence using real native structures. Resource and
 * heap helpers are observation boundaries, not allocator/renderer emulation. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
static SystemMenu menus[7], expected[7];
static MenuManager managers[7], expected_managers[7];
static MenuUnk6 resources[7];
static ItemMenuWork work[7];
SystemMenu *g_Menu;
static int event, replace_menu;
static void next(void) { ++event; if(replace_menu && event<7)g_Menu=&menus[event]; }
static void func_801D3444(void) { assert(event==0); next(); }
static void func_801D4EA0(int window) { assert((event==1 && window==3)||(event==2 && window==4)); next(); }
static void func_801C72BC(int mode) {
    assert(event==3 && mode==16 && g_Menu->pManager->unk48==0);next();
}
u_int HeapFree(void *p) {
    int i=replace_menu?event:0;
    if(event==4) assert((uintptr_t)p==work[i].descriptionBundle);
    else if(event==5) assert(p==&work[i]);
    else { assert(event==6); assert(p==resources[i].pItemsData); }
    next(); return 0;
}
#include "production.inc"
int main(void) {
    assert((uintptr_t)work<=UINT32_MAX);
    for(replace_menu=0;replace_menu<2;++replace_menu)for(int pattern=0;pattern<256;++pattern){
        memset(menus,pattern,sizeof menus);memset(managers,pattern,sizeof managers);
        memset(work,pattern,sizeof work);
        for(int i=0;i<7;++i){
            menus[i].pManager=&managers[i];menus[i].unk330=&resources[i];
            menus[i].unk42C[0]=(u32)(uintptr_t)&work[i];
            work[i].descriptionBundle=0x1000100+i*256;
            resources[i].pItemsData=(void*)(uintptr_t)(0x2000100+i*256);
        }
        memcpy(expected,menus,sizeof menus);memcpy(expected_managers,managers,sizeof managers);
        expected_managers[replace_menu?3:0].unk48=0;
        event=0;g_Menu=&menus[0];func_801DA518();
        assert(event==7);assert(!memcmp(menus,expected,sizeof menus));
        assert(!memcmp(managers,expected_managers,sizeof managers));
    }
    puts("PASS 512 Items teardown layout and retail call-sequence cases");
}

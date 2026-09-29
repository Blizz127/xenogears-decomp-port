/* Native-layout lifecycle assertions derived from the three pinned retail
 * instruction spans. GPU, window and font calls are observation boundaries;
 * this is not a rendered or emulator-differential acceptance test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
extern void bzero(void*, size_t);

SystemMenu *g_Menu;
uint8_t g_PsxRam[0x200000];
static SystemMenu menu;
static MenuManager manager;
static MenuWindowParameters window;
static GfxEnvironment gfx;
static int pumps, wait_frames, allocations, frees, strings, uploads, syncs;
static int draws, direct_draws, closed, base_id, context;
static void *allocated[6];
static unsigned char resource[260];
extern void func_801D2F4C(u8) __attribute__((weak));
#ifdef PROMPT_REAL_ANIMATION
void func_801D3B00(void);
static void func_801D4D1C(u8 index, s32 x, s32 y, s32 w, s32 h,
                         u8 flags, s32 z, u8 scroll) {
    int width=pumps*32<188?pumps*32:188;
    int height=pumps*32<64?pumps*32:64;
    assert(pumps>=1 && pumps<=6 && index==2);
    assert(w==width && h==height && x==122+94-width/2 && y==150+32-height/2);
    assert(flags==1 && z==4 && scroll==0);
}
#include "window.inc"
#endif

static void func_801D397C(u8 i, int x, int y, int w, int h,
                         u8 direct, u8 flag, int z, u8 scroll) {
    assert(i==2 && x==122 && y==150 && w==188 && h==64);
    assert(direct==1 && flag==1 && z==4 && scroll==0);
    manager.shouldRenderWindow[2]=1;
#ifdef PROMPT_REAL_ANIMATION
    window=(MenuWindowParameters){.x=x,.y=y,.width=w,.height=h,
        .index=i,.unk12=flag,.zIndex=z,.hasScrollBar=scroll};
    manager.unk27[2]=1;
#endif
}
static void func_801C7BF4(void) {
    ++pumps;
#ifdef PROMPT_REAL_ANIMATION
    func_801D3B00();
#else
    if (pumps >= wait_frames) window.unk11=1;
#endif
}
void *HeapAlloc(u_int size, u_int flags) {
    static const int order[]={0,1,0,0,1,0};
    assert(allocations<6 && flags==0 && window.unk11);
    assert(size==(order[allocations] ? 0x5ca : sizeof(MenuString)));
    void *p=malloc(size); assert(p && (uintptr_t)p>UINT32_MAX);
    memset(p,0xa5,size); allocated[allocations++]=p; return p;
}
u_int HeapFree(void *p) {
    static const int order[]={1,4,0,2,3,5};
    assert(frees<6 && p==allocated[order[frees]]);
    if(frees<2) assert(syncs==1 && manager.unk2E==1);
    else assert(closed==1 && manager.unk2E==0);
    ++frees; free(p); return 0;
}
static void *GetStringEntry(void *r, int id) {
    assert(r==resource && id==base_id+strings); return resource+id;
}
static int SystemRenderStringEntry(void *s, void *dst, int width, int parity) {
    assert(s==resource+base_id+strings && width==54 && parity==(strings&1));
    assert(dst==allocated[strings<2 ? 1 : 4]);
    return 17 + strings++;
}
static void func_801E7C50(MenuString *s, int i, int offset, int style) {
    assert(i==strings-1 && s==menu.unk1DE0[i] && offset==0 && style==0);
    assert(s->width==17+i); s->unk7F=0;
}
static void func_801E920C(POLY_FT4 *p, int x, int y, int u, int v, int w, int h) {
    int i=strings-1;
    assert(p==&menu.unk1DE0[i]->polys[context] && x==132 && y==160+16*i);
    assert(u==0 && v==78+13*(i/2) && w==17+i && h==13);
}
static void func_801C851C(SVECTOR *p, int x, int y, int w, int h) {
    int i=strings-1;
    assert(p==menu.unk1DE0[i]->vertices && x==132 && y==160+16*i);
    assert(w==17+i && h==13);
}
int LoadImage(RECT *r, u_long *p) {
    int i=uploads++*2;
    assert(i<4 && r==&menu.unk1DE0[i]->vramDest);
    assert((void*)p==allocated[i ? 4 : 1]);
    assert(r->x==320 && r->y==78+13*(i/2) && r->w==58 && r->h==13);
    return 0;
}
int DrawSync(int mode) { assert(mode==0 && uploads==2); ++syncs; return 0; }
static void func_801D4EA0(int i) { assert(i==2); ++closed; }
static void func_801CE198(int flag, SVECTOR *v, POLY_FT4 *p, u8 ctx) {
    int i=draws+direct_draws;
    assert(flag==1 && v==menu.unk1DE0[i]->vertices);
    assert(p==menu.unk1DE0[i]->polys && ctx==context); ++draws;
}
void AddPrim(void *ot, void *p) {
    int i=draws+direct_draws;
    assert(ot==&gfx.ot[4] && p==&menu.unk1DE0[i]->polys[context]); ++direct_draws;
}
#include "production.inc"
int main(void) {
    assert(func_801D2F4C && "prompt builder is still unresolved");
    for(base_id=0;base_id<256;++base_id) for(context=0;context<2;++context)
#ifdef PROMPT_REAL_ANIMATION
    for(wait_frames=6;wait_frames<7;++wait_frames) {
#else
    for(wait_frames=0;wait_frames<4;++wait_frames) {
#endif
        memset(&menu,0,sizeof menu); memset(&manager,0,sizeof manager);
        memset(&window,0,sizeof window);
        pumps=allocations=frees=strings=uploads=syncs=draws=direct_draws=closed=0;
        g_Menu=&menu; menu.pManager=&manager; menu.windowParameters[2]=&window;
        menu.pGfxEnv=&gfx; menu.unk2E0=resource; menu.renderContext=context;
        window.unk11=(wait_frames==0);
        func_801D2F4C((u8)base_id);
        assert(allocations==6 && frees==2 && strings==3 && uploads==2 && syncs==1);
        assert(pumps==wait_frames+2 && manager.unk2E==1);
        for(int i=0;i<3;++i) assert(menu.unk1DE0[i]->unk7F==1 && menu.unk1DE0[i]->renderContext==context);
        MenuString zero={0}; zero.pVramBuffer=menu.unk1DE0[2]->pVramBuffer;
        assert(!memcmp(&zero,menu.unk1DE0[3],sizeof zero));
        menu.unk1DE0[1]->unk7F=0;
        func_801D1030(); assert(draws==2 && direct_draws==1);
        manager.unk2E=0; func_801D1030(); assert(draws==2 && direct_draws==1);
        manager.unk2E=1;
        assert(func_801D32B4(123)==0 && frees==6 && closed==1 && manager.unk2E==0);
        manager.shouldRenderWindow[2]=0;
        assert(func_801D32B4(-1)==0 && frees==6 && closed==1);
    }
#ifdef PROMPT_REAL_ANIMATION
    puts("PASS 512 native prompt cases with production six-frame opening animator");
#else
    puts("PASS 2048 native prompt build/draw/cleanup cases");
#endif
}

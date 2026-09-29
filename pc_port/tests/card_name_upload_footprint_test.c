/* Address-footprint audit, not GPU sampling/visual acceptance. Real native
 * initializer + geometry helper; font emission and primitive init mocked. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "system/menu.h"
SystemMenu *g_Menu;
u32 D_801EA494[9], D_801E9F98[9], D_801E9FBC[9];
u16 D_801EA590[6], D_801EA5DC[6], g_SystemPalette1;
static u8 *panel;
#include "pointer.inc"
static s32 func_8002675C(void *res,s32 glyph,void *p,s32 ctx,s32 x,s32 y,s32 scale) {
    (void)res;(void)glyph;(void)p;(void)ctx;(void)x;(void)y;(void)scale;return 0;
}
static void func_801E927C(POLY_FT4 *p) { memset(p,0,sizeof *p); }
u_short GetTPage(int tp,int abr,int x,int y) {
    assert(tp==0 && abr==0 && x==384 && y==0);return 6;
}
#include "geometry.inc"
#include "init.inc"
u32 D_801E981C[32];
static void *submitted[7];
static unsigned submissions;
static void func_801CE2B4(s32 count,void *p,s32 ctx) { (void)count;(void)p;(void)ctx; }
u_short GetClut(int x,int y) { (void)x;(void)y;assert(!"unexpected icon branch");return 0; }
void AddPrim(void *ot,void *p) {
    assert(ot==&g_Menu->pGfxEnv->ot[4] && submissions<7);
    submitted[submissions++]=p;
}
#include "draw.inc"
static void read_table(FILE *f, unsigned address, void *out, size_t n) {
    assert(fseek(f,address-0x801c5000,SEEK_SET)==0);
    assert(fread(out,1,n,f)==n);
}
int main(void) {
    FILE *f=fopen("disc/menu.bin","rb");assert(f);
    read_table(f,0x801ea494,D_801EA494,sizeof D_801EA494);
    read_table(f,0x801e9f98,D_801E9F98,sizeof D_801E9F98);
    read_table(f,0x801e9fbc,D_801E9FBC,sizeof D_801E9FBC);
    read_table(f,0x801ea590,D_801EA590,sizeof D_801EA590);
    read_table(f,0x801ea5dc,D_801EA5DC,sizeof D_801EA5DC);fclose(f);
    SystemMenu menu;memset(&menu,0,sizeof menu);g_Menu=&menu;
    MenuManager manager;GfxEnvironment gfx;
    memset(&manager,0,sizeof manager);memset(&gfx,0,sizeof gfx);
    menu.pManager=&manager;menu.pGfxEnv=&gfx;manager.unkB=1;
    panel=mmap(NULL,0x3000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(panel!=MAP_FAILED && (uintptr_t)panel+0x3000<=UINT32_MAX);
    u32 address=(uintptr_t)panel;memcpy(menu.unk34C,&address,4);
    unsigned checked=0, masks=0;
    for(unsigned ctx=0;ctx<2;++ctx) {
        menu.renderContext=ctx;memset(panel,0,0x3000);func_801E61B0();
        for(unsigned markers=0;markers<8;++markers)for(unsigned mask=0;mask<8;++mask) {
            panel[0x2dbc]=1;
            for(unsigned slot=0;slot<3;++slot) {
                panel[slot*0x87c+0x1310]=(mask>>slot)&1;
                panel[slot*0x87c+0x130f]=(markers>>slot)&1;
            }
            submissions=0;func_801D02D8();
            unsigned index=0;
            for(unsigned slot=0;slot<3;++slot)if(mask&(1u<<slot)) {
                u8 *base=panel+slot*0x87c;
                /* Retail 801D083C loads from the base panel before adding
                 * the character's primitive offset at 801D0840. */
                assert(submitted[index++]==base+0xa98+(markers&1)*sizeof(POLY_FT4));
                assert(submitted[index++]==base+0x12b8+ctx*sizeof(POLY_FT4));
            }
            assert(submitted[index++]==panel+0xa50+ctx*36);
            assert(submissions==index);
            /* Each VRAM word carries its last upload owner and byte offset.
             * No fabricated padding: offsets >=1014 remain classified tail. */
            int owner[128][64], offset[128][64];
            memset(owner,-1,sizeof owner);memset(offset,-1,sizeof offset);
            for(unsigned slot=0;slot<3;++slot)if(mask&(1u<<slot)) {
                unsigned x=D_801EA590[slot*2],y=D_801EA5DC[slot*2];
                assert(x+40<=64 && y+13<=128);
                for(unsigned row=0;row<13;++row)for(unsigned col=0;col<40;++col) {
                    owner[y+row][x+col]=slot;
                    offset[y+row][x+col]=(row*40+col)*2;
                }
            }
            for(unsigned slot=0;slot<3;++slot)if(mask&(1u<<slot)) {
                POLY_FT4 *p=(POLY_FT4 *)(panel+slot*0x87c+0x12b8)+ctx;
                assert(p->tpage==6 && p->u1-p->u0==72 && p->v2-p->v0==13);
                assert(p->x1-p->x0==72 && p->y2-p->y0==13);
                assert(panel[slot*0x87c+0x1311]==ctx);
                for(unsigned v=p->v0;v<p->v2;++v)for(unsigned u=p->u0;u<p->u1;++u) {
                    assert(owner[v][u/4]==(int)slot);
                    int byte=offset[v][u/4]+(u%4)/2;
                    assert(byte>=0 && byte<1014);++checked;
                }
            }
            ++masks;
        }
    }
    assert(munmap(panel,0x3000)==0);
    printf("PASS %u context/presence combinations, %u name-rectangle texel addresses below allocation clear extent\n",masks,checked);
    puts("PASS detail packet selection; NOT_GPU_ACCEPTANCE: filtering and natural output unverified");
}

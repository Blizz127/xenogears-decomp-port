#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "world_map_resume_initializers.h"

extern s32 test_world_resume_dispatch(u32, s32);
static unsigned char guest[0x200000], scratch[1024], object[256], original[0x200000];
static uint32_t trace[256][7], expected[256][7];
static unsigned nt;
static uint32_t channel_result;
#define GOBJ 0x80150000u
static uint32_t active_slot;
#define SLOT active_slot
static void event(uint32_t id,uint32_t a,uint32_t b,uint32_t c,uint32_t d,uint32_t e,uint32_t f) {
    assert(nt<256);uint32_t row[]={id,a,b,c,d,e,f};memcpy(trace[nt++],row,sizeof(row));
}
static uint32_t address(void *p) {return 0x80000000u+(uint32_t)((unsigned char*)p-g_PsxRam);}
static unsigned char *ptr(uint32_t a) {
    if(a>=0x1f800000u&&a<0x1f800400u)return scratch+a-0x1f800000u;
    a&=0x1fffffffu;assert(a<sizeof(guest));return guest+a;
}
static uint32_t load(uint32_t a,unsigned w) {uint32_t v=0;memcpy(&v,ptr(a),w);return v;}
static void put(uint32_t a,unsigned w,uint32_t v) {memcpy(ptr(a),&v,w);}
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;*v=load(a,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;put(a,w,v);return 0;}
void *func_80024524(void *p,s16 x,s16 y,s16 cx,s16 cy,s16 sz) {
    event(1,p?address(p):0,x,y,cx,cy,sz);return object;
}
void func_800245D8(void *p,s16 a){assert(p==object);event(2,GOBJ,a,0,0,0,0);}
void SpriteSetScale(void *p,short a){assert(p==object);event(3,GOBJ,a,0,0,0,0);}
s32 wm_8008C364(u32 a,s32 b){event(4,a,b,0,0,0,0);*(u32*)PSX_ADDR(a+0x50)=0x1234u+b;return (s32)channel_result;}
s32 wm_80093978(s32 a,s32 b){event(5,a,b,0,0,0,0);return -1234567;}
void wm_800848B4(s32 a,s32 b){event(6,a,b,0,0,0,0);}
MATRIX *RotMatrixYXZ(SVECTOR *v,MATRIX *m) {
    event(7,0x1f8000a0,address(m),(u16)v->vx,(u16)v->vy,(u16)v->vz,0);
    memset(m,0x5a,32);return m;
}
u_short GetTPage(int tp,int abr,int x,int y){event(8,tp,abr,x,y,0,0);return 0x176;}
static int bridge(void*p,PcPortMipsCpu*c,uint32_t target) {
    (void)p;uint32_t a=c->gpr[4],b=c->gpr[5],d=c->gpr[6],e=c->gpr[7];
    switch(target) {
    case 0x80024524:event(1,a,b,d,e,load(c->gpr[29]+16,4),load(c->gpr[29]+20,4));c->gpr[2]=GOBJ;break;
    case 0x800245d8:event(2,a,b,0,0,0,0);break;
    /* SpriteSetScale's actual target is installed below from its symbol. */
    case 0x80022000:event(3,a,b,0,0,0,0);break;
    case 0x8008c364:event(4,a,b,0,0,0,0);put(a+0x50,4,0x1234+b);c->gpr[2]=channel_result;break;
    case 0x80093978:event(5,a,b,0,0,0,0);c->gpr[2]=(uint32_t)-1234567;break;
    case 0x800848b4:event(6,a,b,0,0,0,0);break;
    case 0x8004a92c:event(7,a,b,load(a,2),load(a+2,2),load(a+4,2),0);memset(ptr(b),0x5a,32);c->gpr[2]=b;break;
    case 0x80043a1c:event(8,a,b,d,e,0,0);c->gpr[2]=0x176;break;
    case 0x8003f968:memcpy(ptr(a),ptr(b),d);c->gpr[2]=a;break;
    default:return 0;
    }return 1;
}
struct Test {uint32_t entry;s32(*fn)(s32);};
static const struct Test tests[]={
#define T(a) {0x##a,wm_##a}
T(8008A52C),T(8008B498),T(8008BD1C),T(8008C6EC),T(8008D520),T(8008DE9C),
T(8008E4F4),T(800907C4),T(80087F60),T(8008868C),T(800879E0),T(80088C90)
};
int main(void) {
    FILE*f=fopen("disc/world_map.bin","rb");assert(f);
    size_t n=fread(original+0x6faf0,1,0x80000,f);assert(n&&feof(f));fclose(f);
    PsxMemory_Init();unsigned cases=0;
    const uint32_t modes[]={0,1,2,3,4,5,6,7,8,UINT32_MAX,0x80000000,0x7fffffff};
    for(unsigned t=0;t<sizeof(tests)/sizeof(tests[0]);t++)for(unsigned k=0;k<96;k++) {
        unsigned slot_index = k % 64;
        active_slot = 0x80100000u + slot_index * 0x80u;
        memcpy(guest,original,sizeof(guest));memset(scratch,0xa7,sizeof(scratch));
        for(unsigned i=0;i<0x60000;i++)guest[0x100000+i]=(unsigned char)(i*37+k*13);
        put(0x8009be24,4,0x80100000);put(0x8009be10,4,modes[k%12]);
        put(0x8009c620,4,0x80110000);put(0x8009c610,4,k%9);
        put(0x8009cd34,4,k&1?0x80160000:0);put(0x8009cd38,4,0x80161000);put(0x8009cd3c,4,0x80162000);
        put(0x8006f369,1,k&1?0xff:2);put(0x8006f36a,1,k&1?3:0xff);
        put(0x8006ee68,2,(k/12)&1?0xe000:0xe001);put(0x8009bd3c,2,0x1234+k);
        put(SLOT+0x70,4,k&1?0x80000fff:0x7fffffff);
        channel_result=(k/12)%3==0?3:(k/12)%3==1?1:UINT32_MAX;
        for(unsigned i=0;i<9;i++){
            put(0x8009b674+2*i,2,i*8);put(0x8009b688+2*i,2,i*7);
            put(0x8009b64c+4*i,2,2);put(0x8009b64eu + 4*i,2,5);
        }
        put(0x8009afa0,2,k&1?0xffff:0xfffe);put(0x8009afa2,2,3);
        put(0x8009afa4,2,4);put(0x8009afa6,2,0x8000);put(0x8009afa8,2,0xffff);
        put(0x8009afdc,2,k&1?0xffff:0xfffe);put(0x8009afde,2,6);put(0x8009afe0,2,0xffff);
        for(unsigned i=0;i<2;i++){
            uint32_t rec=0x80110000+(i?5:2)*0x54;
            put(rec+0x40,4,0x80120000+i*0x100);put(0x80120004+i*0x100,2,k%4);
            put(rec+0x48,4,0x80130000+i*0x1000);put(rec+0x4c,4,0x80140000+i*0x1000);
        }
        memcpy(g_PsxRam,guest,sizeof(guest));memcpy(g_PsxScratchpad,scratch,sizeof(scratch));
        memcpy(object,ptr(GOBJ),sizeof(object));nt=0;
        uint32_t result=(uint32_t)test_world_resume_dispatch(tests[t].entry, (s32)slot_index);unsigned expected_n=nt;memcpy(expected,trace,sizeof(trace));
        if(t<=2&&expected_n){*(uint32_t*)PSX_ADDR(SLOT+0x4c)=GOBJ;memcpy(PSX_ADDR(GOBJ),object,sizeof(object));}
        nt=0;PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;bus.bridge=bridge;
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=slot_index;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0x80001000;
        int status=PcPortMipsRun(&cpu,tests[t].entry,0x80001000,20000);
        if(status){fprintf(stderr,"entry %x case %u: %s pc=%x\n",tests[t].entry,k,cpu.error,cpu.pc);return 1;}
        assert(result==cpu.gpr[2]);assert(nt==expected_n);assert(!memcmp(trace,expected,nt*sizeof(trace[0])));
        for(unsigned i=0;i<0x1f0000;i++)if(g_PsxRam[i]!=guest[i]){
            fprintf(stderr,"entry %x case %u mismatch %x: native=%x guest=%x\n",tests[t].entry,k,0x80000000+i,g_PsxRam[i],guest[i]);return 1;
        }
        assert(!memcmp(g_PsxScratchpad,scratch,sizeof(scratch)));cases++;
    }
    printf("World resume initializers: %u retail MIPS differential cases PASS\n",cases);
}

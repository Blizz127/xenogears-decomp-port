#include "common.h"
#include "psyq/libgpu.h"
#include "field/graphics.h"
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern void func_800A8BA4(void) __attribute__((weak));
extern u16 g_FieldOverlayLayoutData[226][4];
extern u32 D_800AFC60,D_800AFC64;
extern s32 D_800AF278,D_800AEB64;
int g_FieldCurRenderContextIndex;
extern void func_800A8408(s32,s32,s32);
static u8 ram[0x200000], buffers[2][0x1108], expected[2][0x1108];
static unsigned allocations,loads,users;
void func_800A8314(void) { ++loads; }
void HeapChangeCurrentUser(u_int tag,char** types) { assert(tag==8 && !types);++users; }
void* HeapAlloc(u_int size,u_int flags) {
    assert(size==0x1108 && flags==0 && allocations<2);return buffers[allocations++];
}
void SetPolyFT4(POLY_FT4* p) { ((u8*)p)[3]=9;((u8*)p)[7]=0x2c; }
void SetSemiTrans(void* p,int on) { ((u8*)p)[7]=(((u8*)p)[7]&~2)|(on?2:0); }
u_short GetClut(int x,int y) { return (y<<6)|((x>>4)&63); }
u_short GetTPage(int tp,int abr,int x,int y) {
    return ((tp&3)<<7)|((abr&3)<<5)|((y&0x100)>>4)|((x&0x3ff)>>6)|((y&0x200)<<2);
}
static u8* address(u32 a,unsigned w) {
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)buffers;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(buffers))return (u8*)(uintptr_t)a;
    return NULL;
}
static int read_bus(void* u,u32 a,unsigned w,u32* v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;*v=0;
    for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);
    return 0;
}
static int write_bus(void* u,u32 a,unsigned w,u32 v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));
    return 0;
}
static int bridge(void* u,PcPortMipsCpu* c,u32 t) {
    (void)u;u32* a=c->gpr+4;
    switch(t) {
    case 0x800a8314: func_800A8314();break;
    case 0x80032498: HeapChangeCurrentUser(a[0],(char**)(uintptr_t)a[1]);break;
    case 0x80031bdc: c->gpr[2]=(u32)(uintptr_t)HeapAlloc(a[0],a[1]);break;
    case 0x80043cb0: SetPolyFT4((POLY_FT4*)(uintptr_t)a[0]);break;
    case 0x80043bfc: SetSemiTrans((void*)(uintptr_t)a[0],a[1]);break;
    case 0x80043a58: c->gpr[2]=GetClut(a[0],a[1]);break;
    case 0x80043a1c: c->gpr[2]=GetTPage(a[0],a[1],a[2],a[3]);break;
    default:return 0;
    }
    return 1;
}
static void reset(void) {
    memset(buffers,0xa5,sizeof(buffers));allocations=loads=users=0;
    D_800AF278=0;D_800AEB64=0x1234;D_800AFC60=D_800AFC64=0;
    write_bus(NULL,0x800af278,4,0);write_bus(NULL,0x800aeb64,4,0x1234);
}
int main(void) {
    if(!func_800A8BA4) {fputs("OVERLAY INIT FAIL missing owner\n",stderr);return 1;}
    assert(sizeof(POLY_FT4)==40);
    FILE* f=fopen("disc/field.bin","rb");assert(f);
    assert(fread(ram+0x6faf0,1,260862,f)==260862);assert(!fclose(f));
#ifdef TEST_OVERLAY_UV
    for(unsigned page=0;page<2;++page) for(unsigned i=0;i<109;++i) {
        static const s32 offsets[3][2]={{0,0},{-17,-9},{255,255}};
        for(unsigned k=0;k<3;++k) {
            reset();
            write_bus(NULL,0x800afc60,4,(u32)(uintptr_t)buffers[0]);
            write_bus(NULL,0x800afc64,4,(u32)(uintptr_t)buffers[1]);
            write_bus(NULL,0x800adb08,4,page);
            PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
            PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
            c.gpr[4]=i;c.gpr[5]=offsets[k][0];c.gpr[6]=offsets[k][1];
            c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffcu;
            if(PcPortMipsRun(&c,0x800a8408,0xfffffffcu,5000)!=PC_PORT_MIPS_HALTED) {
                fprintf(stderr,"OVERLAY UV FAIL oracle: %s\n",c.error);return 1;
            }
            memcpy(expected,buffers,sizeof(expected));reset();
            D_800AFC60=(u32)(uintptr_t)buffers[0];D_800AFC64=(u32)(uintptr_t)buffers[1];
            g_FieldCurRenderContextIndex=page;
            func_800A8408(i,offsets[k][0],offsets[k][1]);
            if(memcmp(buffers,expected,sizeof(buffers))) {
                fprintf(stderr,"OVERLAY UV FAIL page=%u index=%u offset=%u\n",page,i,k);return 1;
            }
        }
    }
    puts("OVERLAY UV PASS 654 cases: 109 quads, both buffers, zero/negative/large offsets");
#else
    for(unsigned variant=0;variant<2;++variant) {
        if(variant) {
            /* Exercise all UV selectors and blend retention beyond source data. */
            static const unsigned blend_nibbles[4]={2,0,3,1};
            for(unsigned i=0;i<109;++i)
                g_FieldOverlayLayoutData[117+i][3]=(i%5)|(blend_nibbles[i%4]<<4);
        }
        memcpy(ram+0xaeb68,g_FieldOverlayLayoutData,0x710);
        reset();
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
        c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&c,0x800a8ba4,0xfffffffcu,200000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"OVERLAY INIT FAIL oracle: %s\n",c.error);return 1;
        }
        assert(allocations==2 && loads==1 && users==1);
        memcpy(expected,buffers,sizeof(expected));
        reset();func_800A8BA4();
        if(memcmp(buffers,expected,sizeof(buffers)) || allocations!=2 || loads!=1 || users!=1 ||
           D_800AF278!=1 || D_800AEB64 || D_800AFC60!=(u32)(uintptr_t)buffers[0] ||
           D_800AFC64!=(u32)(uintptr_t)buffers[1]) {
            fprintf(stderr,"OVERLAY INIT FAIL variant=%u\n",variant);return 1;
        }
    }
    puts("OVERLAY INIT PASS retail/synthetic flags, both complete 109-quad buffers");
#endif
    return 0;
}

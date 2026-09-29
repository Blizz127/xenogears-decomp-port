/* Reuse the auxiliary-pool SDK/heap boundaries and full retail image bus. */
#define XENO_TEST_SPLIT_POOLS 1
#define main AuxPoolTestMain
#include "aux_pool_retail_test.c"
#undef main
extern u32 D_801E85F4[8][2] __attribute__((weak));
extern u32 D_801E8640 __attribute__((weak));
extern u16 D_801E8648[20] __attribute__((weak));
extern u16 D_801E869C __attribute__((weak));
static struct Owner {u32 address;void*host;unsigned size;} owners[8];
static u8 globalInitial[256],globalExpected[256],globalActual[256];
static void snapshot(u8*out){unsigned n=0;for(unsigned i=0;i<8;++i){memcpy(out+n,owners[i].host,owners[i].size);n+=owners[i].size;}assert(n<=256);}
static void restore(const u8*in){unsigned n=0;for(unsigned i=0;i<8;++i){memcpy(owners[i].host,in+n,owners[i].size);n+=owners[i].size;}}
static u8* ownerAddress(u32 a,unsigned w){for(unsigned i=0;i<8;++i)if(a>=owners[i].address&&(uint64_t)a+w<=owners[i].address+owners[i].size)return(u8*)owners[i].host+a-owners[i].address;return NULL;}
static int initRead(void*u,u32 a,unsigned w,u32*v){u8*p=ownerAddress(a,w);if(!p)return rd(u,a,w,v);*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int initWrite(void*u,u32 a,unsigned w,u32 v){u8*p=ownerAddress(a,w);if(!p)return wr(u,a,w,v);for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int initCompare(s32 count,unsigned mask){
    memset(&fixture,0xa5,sizeof(fixture));initial=fixture;
    for(unsigned i=0;i<8;++i)memset(owners[i].host,0xa5,owners[i].size);
    snapshot(globalInitial);nCalls=0;allocationCalls=0;allocationFailureMask=mask;memset(calls,0,sizeof(calls));
    PcPortMipsBus bus={.read=initRead,.write=initWrite,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(u32)count;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,0x801e738c,0xfffffffcu,1000000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"OBJECT INIT FAIL oracle %s\n",cpu.error);return 1;}
    expected=fixture;snapshot(globalExpected);memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;
    fixture=initial;restore(globalInitial);nCalls=0;allocationCalls=0;memset(calls,0,sizeof(calls));
    func_801E738C(count);snapshot(globalActual);
    if(memcmp(&fixture,&expected,sizeof(fixture))||memcmp(globalExpected,globalActual,sizeof(globalActual))||nCalls!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"OBJECT INIT FAIL count=%d mask=%u\n",count,mask);return 1;}
    return 0;
}
int main(void){
    if(!D_801E85F4||!&D_801E8640||!D_801E8648||!&D_801E869C){fputs("OBJECT INIT FAIL missing native data owners\n",stderr);return 1;}
    owners[0]=(struct Owner){0x801e85f4,D_801E85F4,64};
    owners[1]=(struct Owner){0x801e8640,&D_801E8640,4};
    owners[2]=(struct Owner){0x801e8648,D_801E8648,40};
    owners[3]=(struct Owner){0x801e8670,D_801E8670,40};
    owners[4]=(struct Owner){0x801e869c,&D_801E869C,2};
    owners[5]=(struct Owner){0x801e86a0,D_801E86A0,8};
    owners[6]=(struct Owner){0x801e86a8,D_801E86A8,8};
    owners[7]=(struct Owner){0x801e86b0,&D_801E86B0,2};
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    splitPools=1;unsigned cases=0;
    for(s32 count=-1;count<=512;++count)for(unsigned mask=0;mask<4;++mask){if(initCompare(count,mask))return 1;++cases;}
    printf("OBJECT INIT PASS %u cases: complete retail body, both pools, reset and preserved bytes\n",cases);
}

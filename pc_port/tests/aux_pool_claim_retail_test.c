#define main AuxPoolLifecycleMain
#include "aux_pool_retail_test.c"
#undef main
extern u8* func_801E0248(u8*,s32) __attribute__((weak));
extern s32 func_801E0354(u8*,u8*) __attribute__((weak));
enum { COMPARE_BYTES=8192 };
static int claimCompare(int release,s32 value){
    memcpy(&initial,&fixture,COMPARE_BYTES);nCalls=0;memset(calls,0,sizeof(calls));
    u8*entry=release?fixture.entries+16*124+value*124:NULL;
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=(u32)(uintptr_t)fixture.header;cpu.gpr[5]=release?(u32)(uintptr_t)entry:(u32)value;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    if(PcPortMipsRun(&cpu,release?0x801e0354:0x801e0248,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"AUX CLAIM FAIL oracle %s\n",cpu.error);return 1;}
    memcpy(&expected,&fixture,COMPARE_BYTES);memcpy(expectedCalls,calls,4*sizeof(calls[0]));unsigned expectedCount=nCalls;
    memcpy(&fixture,&initial,COMPARE_BYTES);nCalls=0;memset(calls,0,4*sizeof(calls[0]));u32 result;
    if(release)result=(u32)func_801E0354((u8*)fixture.header,entry);
    else result=(u32)(uintptr_t)func_801E0248((u8*)fixture.header,value);
    if(result!=cpu.gpr[2]||memcmp(&fixture,&expected,COMPARE_BYTES)||nCalls!=expectedCount||memcmp(calls,expectedCalls,4*sizeof(calls[0]))){fprintf(stderr,"AUX CLAIM FAIL release=%d value=%d header=%x result=%x/%x\n",release,value,initial.header[1],result,cpu.gpr[2]);return 1;}
    return 0;
}
int main(void){
    if(!func_801E0248||!func_801E0354){fputs("AUX CLAIM FAIL missing native owners\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    const s32 modes[]={0,1,0x8000,0xffff};unsigned cases=0;
    for(s32 count=-8;count<=8;++count)for(s32 next=-8;next<=8;++next)for(unsigned bits=0;bits<256;bits+=17)for(unsigned mode=0;mode<4;++mode){
        memset(&fixture,0xa5,COMPARE_BYTES);fixture.header[0]=(u32)(uintptr_t)(fixture.entries+16*124);fixture.header[1]=(u16)count|((u32)(u16)next<<16);
        for(s32 i=-8;i<=8;++i)*(s16*)(fixture.entries+(16+i)*124+0x16)=((bits>>((i+8)%8))&1)?2:-1;
        if(claimCompare(0,modes[mode]))return 1;++cases;
    }
    for(s32 next=-8;next<=8;++next)for(s32 index=-8;index<=8;++index){
        memset(&fixture,0xa5,COMPARE_BYTES);fixture.header[0]=(u32)(uintptr_t)(fixture.entries+16*124);fixture.header[1]=0x7531|((u32)(u16)next<<16);
        if(claimCompare(1,index))return 1;++cases;
    }
    printf("AUX CLAIM PASS %u cases: signed pool cursors, sentinel return, packet flags and release\n",cases);
}

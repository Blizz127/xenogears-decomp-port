#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"
#include "psx/gtereg.h"
#include "battle_mips_adapter.h"
extern void func_801E1880(u32*) __attribute__((weak));
extern u16 D_801E8648[20];
u8 g_PsxScratchpad[4096];
static u8 ram[0x200000],guestScratch[4096];
static struct {u32 table[16],object[16][2],nodes[16][8][31];} fixture,initial;
static struct Call {u32 kind;u8 a[32],b[32];} calls[8],expectedCalls[8];
static unsigned nCalls;
static struct Call* record(u32 kind){assert(nCalls<8);struct Call*r=&calls[nCalls++];r->kind=kind;return r;}
/* Controlled composition boundary: compare exact input bytes and use real
 * GTE register transfers/commands after composition. Not SDK matrix proof. */
MATRIX* CompMatrix(MATRIX*a,MATRIX*b,MATRIX*out){
    struct Call*r=record(1);memcpy(r->a,a,32);memcpy(r->b,b,32);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out->m[i][j]=(s16)((u16)a->m[i][j]+(u16)b->m[j][i]);
    for(unsigned i=0;i<3;++i)out->t[i]=(u32)a->t[i]+(u32)b->t[i];
    return out;
}
void SetRotMatrix(MATRIX*m){memcpy(record(2)->a,m,32);for(unsigned i=0;i<5;++i){u32 v;memcpy(&v,(u8*)m+i*4,4);CTC2(v,i);}}
void SetTransMatrix(MATRIX*m){memcpy(record(3)->a,m,32);for(unsigned i=0;i<3;++i)CTC2(m->t[i],5+i);}
static u8* address(u32 a,unsigned w){
    if(a>=0x801e8648u&&(uint64_t)a+w<=0x801e8670u)return (u8*)D_801E8648+a-0x801e8648u;
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    if(a>=0x1f800000u&&(uint64_t)a+w<=0x1f801000u)return guestScratch+a-0x1f800000u;
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
    if(t==0x8004931c){CompMatrix((MATRIX*)address(c->gpr[4],32),(MATRIX*)address(c->gpr[5],32),(MATRIX*)address(c->gpr[6],32));c->gpr[2]=c->gpr[6];}
    else if(t==0x80049efc)SetRotMatrix((MATRIX*)address(c->gpr[4],32));
    else if(t==0x80049f8c)SetTransMatrix((MATRIX*)address(c->gpr[4],32));
    else return 0;
    return 1;
}
static u32 copRead(void*u,int control,unsigned reg){(void)u;return control?CFC2(reg):MFC2(reg);}
static void copWrite(void*u,int control,unsigned reg,u32 value){(void)u;if(control)CTC2(value,reg);else MTC2(value,reg);}
static int copCommand(void*u,u32 ins){(void)u;doCOP2(ins&0x1ffffffu);return 0;}
int main(void){
    if(!func_801E1880){fputs("SHARED TRANSFORM FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;
    for(unsigned active=0;active<4;++active)for(int slot=-2;slot<16;++slot)for(int child=-1;child<6;++child)
    for(unsigned present=0;present<2;++present)for(unsigned seed=0;seed<16;++seed){
        for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0x12481248u*(i+seed);
        for(unsigned i=0;i<16;++i){fixture.table[i]=present?(u32)(uintptr_t)fixture.object[i]:0;fixture.object[i][1]=(u32)(uintptr_t)fixture.nodes[i];}
        for(unsigned i=0;i<20;++i)D_801E8648[i]=(u16)(seed*7313+i*1597);
        for(unsigned i=0;i<2;++i){u16*r=D_801E8648+i*10;r[3]=(active>>i)&1?0xffff:0;r[8]=slot;r[9]=child;}
        u16 before[20],expected[20];memcpy(before,D_801E8648,40);initial=fixture;
        memset(g_PsxScratchpad,0x36,4096);memcpy(guestScratch,g_PsxScratchpad,4096);
        memset(&gteRegs,0,sizeof(gteRegs));MTC2(0xfefefefe,0);CTC2(0x9753,31);GTERegisters beforeGte=gteRegs;
        nCalls=0;memset(calls,0,sizeof(calls));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge,.cop2_read=copRead,.cop2_write=copWrite,.cop2_command=copCommand};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)fixture.table;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801e1880,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SHARED TRANSFORM FAIL oracle %s\n",cpu.error);return 1;}
        memcpy(expected,D_801E8648,40);memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;GTERegisters expectedGte=gteRegs;
        memcpy(D_801E8648,before,40);gteRegs=beforeGte;nCalls=0;memset(calls,0,sizeof(calls));func_801E1880(fixture.table);
        if(memcmp(expected,D_801E8648,40)||memcmp(&fixture,&initial,sizeof(fixture))||memcmp(calls,expectedCalls,sizeof(calls))||nCalls!=expectedCount||memcmp(guestScratch,g_PsxScratchpad,4096)||memcmp(&gteRegs,&expectedGte,sizeof(gteRegs))){
            fprintf(stderr,"SHARED TRANSFORM FAIL active=%u slot=%d child=%d present=%u seed=%u\n",active,slot,child,present,seed);return 1;
        }++cases;
    }
    printf("SHARED TRANSFORM PASS %u cases: complete retail body, records, scratch and GTE; controlled matrix boundary, shared PsyCross backend\n",cases);
}

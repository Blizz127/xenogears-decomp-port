#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
#include "psx/inline_c.h"
#include "psx/gtereg.h"
extern void func_801E37D0(u8*) __attribute__((weak));
u8 g_PsxScratchpad[4096];
static u8 ram[0x200000],guestScratch[4096];
static struct {u32 object[0x134/4],parent[0x134/4],nodes[2][5*0x7c/4];} fixture,initial,expected;
static struct Record {u32 kind;u8 a[32],b[32];} records[8],expectedRecords[8];
static unsigned nRecords;
static void record(u32 kind,MATRIX* a,MATRIX* b) {
    assert(nRecords<8);struct Record* r=&records[nRecords++];
    r->kind=kind;memcpy(r->a,a,32);if(b)memcpy(r->b,b,32);else memset(r->b,0,32);
}
static MATRIX* matrixOutput(u32 kind,MATRIX* a,MATRIX* b,MATRIX* out) {
    MATRIX x=*a,y=*b;record(kind,a,b);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
        out->m[i][j]=(u16)x.m[i][j]+(u16)y.m[j][i]+i*7+j;
    if(kind==0x8004931c)for(unsigned i=0;i<3;++i)out->t[i]=(u32)x.t[i]+(u32)y.t[i]+i;
    return out;
}
MATRIX* MulMatrix0(MATRIX* a,MATRIX* b,MATRIX* out) {return matrixOutput(0x8004920c,a,b,out);}
MATRIX* MulMatrix2(MATRIX* a,MATRIX* b) {return matrixOutput(0x80049bdc,a,b,b);}
MATRIX* CompMatrix(MATRIX* a,MATRIX* b,MATRIX* out) {return matrixOutput(0x8004931c,a,b,out);}
void SetRotMatrix(MATRIX* m) {record(0x80049efc,m,NULL);for(unsigned i=0;i<5;++i)CTC2(((u32*)m)[i],i);}
void SetTransMatrix(MATRIX* m) {record(0x80049f8c,m,NULL);for(unsigned i=0;i<3;++i)CTC2(m->t[i],5+i);}
static u8* address(u32 a,unsigned w) {
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    if(a>=0x1f800000u && (uint64_t)a+w<=0x1f801000u)return guestScratch+(a-0x1f800000u);
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(fixture))return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u,u32 a,unsigned w,u32* v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;*v=0;
    for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;
}
static int wr(void* u,u32 a,unsigned w,u32 v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;
}
static u32 c2read(void* u,int control,unsigned r) {(void)u;return control?CFC2(r):MFC2(r);}
static void c2write(void* u,int control,unsigned r,u32 v) {(void)u;if(control)CTC2(v,r);else MTC2(v,r);}
static int c2command(void* u,u32 op) {(void)u;doCOP2(op);return 0;}
static int bridge(void* u,PcPortMipsCpu* c,u32 t) {
    (void)u;u32* a=c->gpr+4;MATRIX* ma=(MATRIX*)address(a[0],32);
    MATRIX* mb=(MATRIX*)address(a[1],32);MATRIX* out=(MATRIX*)address(a[2],32);
    switch(t) {
    case 0x8004920c:MulMatrix0(ma,mb,out);c->gpr[2]=a[2];break;
    case 0x8004931c:CompMatrix(ma,mb,out);c->gpr[2]=a[2];break;
    case 0x80049bdc:MulMatrix2(ma,mb);c->gpr[2]=a[1];break;
    case 0x80049efc:SetRotMatrix(ma);break;
    case 0x80049f8c:SetTransMatrix(ma);break;
    default:return 0;
    }
    return 1;
}
int main(void) {
    if(!func_801E37D0){fputs("OBJECT LINK FAIL missing native owner\n",stderr);return 1;}
    FILE* f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i) {
        assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));
        assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);
    }
    assert(!fclose(f));
    static const s16 joints[]={-1,0,1,2},offsets[]={-32768,-1,0,32767};
    unsigned cases=0;
    for(unsigned slot=0;slot<10;++slot)for(unsigned bits=0;bits<32;++bits)
    for(unsigned ji=0;ji<4;++ji)for(unsigned off=0;off<4;++off) {
        memset(&fixture,0xa5,sizeof(fixture));memset(D_801E8670,0,sizeof(D_801E8670));
        u8* object=(u8*)fixture.object;u8* parent=(u8*)fixture.parent;
        fixture.object[1]=(u32)(uintptr_t)((u8*)fixture.nodes[0]+2*0x7c);
        fixture.parent[1]=(u32)(uintptr_t)((u8*)fixture.nodes[1]+2*0x7c);
        for(unsigned i=0;i<sizeof(fixture.nodes)/4;++i)((u32*)fixture.nodes)[i]=0x1001001u*(i+1);
        object[0x5c]=slot;object[0x5d]=(bits>>3)&1;
        *(u16*)(object+0x4a)=(bits&4)?0x10:0;
        *(s16*)(object+0x5e)=joints[ji];
        object[0x34]=bits&1;parent[0x34]=(bits>>1)&1;
        *(s16*)(object+0x6a)=offsets[off];*(s16*)(object+0x6c)=offsets[(off+1)%4];
        *(s16*)(object+0x6e)=offsets[(off+2)%4];
        D_801E8670[slot]=(bits&16)?0:(u32)(uintptr_t)parent;
        memcpy(ram+0x1e8670,D_801E8670,40);initial=fixture;
        memset(g_PsxScratchpad,0x36,sizeof(g_PsxScratchpad));memcpy(guestScratch,g_PsxScratchpad,sizeof(guestScratch));
        memset(&gteRegs,0,sizeof(gteRegs));nRecords=0;memset(records,0,sizeof(records));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge,.cop2_read=c2read,.cop2_write=c2write,.cop2_command=c2command};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=(u32)(uintptr_t)object;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,0x801e37d0,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"OBJECT LINK FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;memcpy(expectedRecords,records,sizeof(records));unsigned expectedCount=nRecords;
        GTERegisters expectedGte=gteRegs;fixture=initial;
        nRecords=0;memset(records,0,sizeof(records));memset(&gteRegs,0,sizeof(gteRegs));
        func_801E37D0((u8*)fixture.object);
        if(memcmp(&fixture,&expected,sizeof(fixture)) || nRecords!=expectedCount ||
           memcmp(records,expectedRecords,sizeof(records)) || memcmp(guestScratch,g_PsxScratchpad,sizeof(guestScratch)) ||
           memcmp(&gteRegs,&expectedGte,sizeof(gteRegs))) {
            fprintf(stderr,"OBJECT LINK FAIL slot=%u flags=%u joint=%d offset=%u\n",slot,bits,joints[ji],off);return 1;
        }
        ++cases;
    }
    printf("OBJECT LINK PASS %u cases: visibility/matrices/scratch/duplicate stores/GTE\n",cases);
}

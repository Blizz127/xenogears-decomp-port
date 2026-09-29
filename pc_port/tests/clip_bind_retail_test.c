#include <stdio.h>
#include <string.h>
#include "common.h"
#ifdef XENO_TEST_OVERLAY_UNIT
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
/* Test-local link seam: binders stay unchanged in this translation unit.
 * Its replaceable VM owner cannot inline into callers; the fixture's strong
 * recording definition wins at link time. Production is compiled normally. */
void func_801E39F0(u8*,void*,s32,s32,s32) __attribute__((weak,noinline));
#include OBJECT_OVERLAY_SOURCE
#else
#include "battle_mips_adapter.h"
#include "field_clip_control.h"
extern void func_801E3534(u8*,void*,u8*,u32) __attribute__((weak));
extern void func_801E35D0(u8*,u8*,void*,s32) __attribute__((weak));
extern s16 D_801E863C;
static u8 ram[0x200000];
static struct {u32 objects[2][128],table[512],aux[4];} fixture,initial,expected;
static struct {u32 count,args[5],snapshot[128];} call,expectedCall;
#ifndef XENO_TEST_UNSUPPORTED_VM
void func_801E39F0(u8*obj,void*context,s32 limit,s32 ticks,s32 mode){++call.count;call.args[0]=(u32)(uintptr_t)obj;call.args[1]=(u32)(uintptr_t)context;call.args[2]=limit;call.args[3]=ticks;call.args[4]=mode;memcpy(call.snapshot,obj,sizeof(call.snapshot));}
#else
/* The production E39F0 now exists, so the old missing-owner assertion is
 * obsolete. Exercise its unsupported-dispatch fallback with explicit rejected
 * handler boundaries. Entry/exit ownership is covered by clip_vm_owner_test. */
extern void func_801E39F0(u8*,void*,s32,s32,s32);
int PcPort_FieldClipPrelude(u8*obj,int32_t ticks,uint32_t*stream)
{ assert(obj==(u8*)fixture.objects[0]&&ticks==1);memcpy(stream,obj+0x10,4);return 1; }
int PcPort_FieldClipControlStep(PcPortFieldClipControl*state)
{ (void)state;return 0; }
int PcPort_FieldClipDataStep(PcPortFieldClipControl*state)
{ (void)state;return 0; }
#endif
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;if(t!=0x801e39f0)return 0;u32 mode;assert(!rd(NULL,c->gpr[29]+16,4,&mode));func_801E39F0((u8*)(uintptr_t)c->gpr[4],(void*)(uintptr_t)c->gpr[5],c->gpr[6],c->gpr[7],mode);return 1;}
static int oracle(u32 pc,u32 a,u32 b,u32 c,u32 d){PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=a;cpu.gpr[5]=b;cpu.gpr[6]=c;cpu.gpr[7]=d;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;if(PcPortMipsRun(&cpu,pc,0xfffffffcu,2000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CLIP BIND FAIL oracle %s\n",cpu.error);return 0;}return 1;}
static void prepare(unsigned seed){for(unsigned i=0;i<sizeof(fixture);++i)((u8*)&fixture)[i]=(u8)(i*37+seed*63);fixture.objects[0][2]=fixture.objects[1][2]=(u32)(uintptr_t)(fixture.table+128);fixture.objects[0][3]=fixture.objects[1][3]=(u32)(uintptr_t)fixture.aux;fixture.aux[1]=(u32)(uintptr_t)(fixture.table+128);for(unsigned i=0;i<512;++i)fixture.table[i]=(i*199+seed*313)&0xffff;}
int main(void){
#ifdef XENO_TEST_UNSUPPORTED_VM
 prepare(0);fixture.aux[0]=0x0070;
 fixture.objects[0][4]=(u32)(uintptr_t)fixture.aux;
 printf("expected-object=%p expected-ip=%08x\n",(void*)fixture.objects[0],fixture.objects[0][4]);
 assert(!fflush(stdout));
 func_801E39F0((u8*)fixture.objects[0],NULL,85,1,0);
 fputs("CLIP BIND FAIL unsupported interpreter returned\n",stderr);return 1;
#endif
 if(!func_801E3534||!func_801E35D0){fputs("CLIP BIND FAIL missing native entry\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned resets=0,binds=0;const s32 indices[]={-128,-1,0,1,79,80,81,255};
 for(unsigned seed=0;seed<16;++seed){prepare(seed);initial=fixture;u8*obj=(u8*)fixture.objects[0];u8*table=(u8*)(fixture.table+seed);u32 aux=0x80000000u+seed*817;assert(oracle(0x801e3534,(u32)(uintptr_t)obj,0xabcdef00,(u32)(uintptr_t)table,aux));expected=fixture;fixture=initial;func_801E3534(obj,(void*)0xabcdef00,table,aux);if(memcmp(&fixture,&expected,sizeof(fixture))){fputs("CLIP BIND FAIL reset\n",stderr);return 1;}++resets;}
 for(unsigned queue=0;queue<256;++queue)for(unsigned alias=0;alias<2;++alias)for(unsigned k=0;k<8;++k)for(unsigned nulls=0;nulls<4;++nulls){
  prepare(k);u8*obj=(u8*)fixture.objects[0];u8*src=(u8*)fixture.objects[alias?0:1];obj[0x2b]=queue;if(nulls&1)obj=NULL;if(nulls&2)src=NULL;D_801E863C=(s16)(queue*251+k*8101);assert(!wr(NULL,0x801e863c,2,(u16)D_801E863C));
  void*context=(void*)(uintptr_t)(0x45670000+queue*4+k);initial=fixture;memset(&call,0,sizeof(call));assert(oracle(0x801e35d0,(u32)(uintptr_t)obj,(u32)(uintptr_t)src,(u32)(uintptr_t)context,indices[k]));expected=fixture;expectedCall=call;
  fixture=initial;memset(&call,0,sizeof(call));func_801E35D0(obj,src,context,indices[k]);if(memcmp(&fixture,&expected,sizeof(fixture))||memcmp(&call,&expectedCall,sizeof(call))){fprintf(stderr,"CLIP BIND FAIL queue=%u alias=%u index=%d nulls=%u\n",queue,alias,indices[k],nulls);return 1;}++binds;
 }
 printf("CLIP BIND PASS %u resets and %u binds; full memory and VM call snapshot; controlled VM boundary\n",resets,binds);
}
#endif
